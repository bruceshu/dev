#include "YouMeDataChannel.h"
#include <YouMeCommon/XFile.h>
#include <YouMeCommon/Log.h>
#include <YouMeCommon/XDNSParse.h>
#include <YouMeCommon/SyncTCP.h>
#include <YouMeCommon/SyncUDP.h>
#define MAX_RETRY_COUNT 3
CYouMeDataChannel* CYouMeDataChannel::CreateInstance(const XString& strCachePath)
{
	return new CYouMeDataChannel(strCachePath);
}

void CYouMeDataChannel::DestroyInstance(CYouMeDataChannel* pInstance)
{
	delete pInstance;
}

void CYouMeDataChannel::SendData(const XString&strDomain, int iPort, bool bUseTcp, const byte* pBuffer, int iBufferLen)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_bInit)
	{
        // init dns parse instance
        m_dnsHandle = youmecommon::DNSUtil::Instance();
        if (!m_dnsHandle) {
            YouMe_LOG_Error(__XT("Not get dns parse instance"));
        }
        
		m_sqliteDb.Open(m_strCacheDBPath.c_str());
		
		for (int i = 0; i < sizeof(s_szYoumeCreateTableSQL) / sizeof(s_szYoumeCreateTableSQL[0]); i++)
		{
			if (!m_sqliteDb.IsTableExist(s_szYoumeTableName[i].c_str()))
			{
				youmecommon::CSqliteOperator sqliteOperator(m_sqliteDb);
				sqliteOperator.PrepareSQL(s_szYoumeCreateTableSQL[i].c_str());
				sqliteOperator.Execute();
			}
		}
		//插入一列
		{
			youmecommon::CSqliteOperator sqliteOperateor(m_sqliteDb);
			sqliteOperateor.PrepareSQL(__XT("alter table report add column retry int;"));
			sqliteOperateor.Execute();
		}


		//从数据库中读取所有的需要上报的日志
		youmecommon::CSqliteOperator sqliteOperator(m_sqliteDb);
		XString strSql = __XT("select * from report");
		sqliteOperator.PrepareSQL(strSql);
		sqliteOperator.Execute();
		while (sqliteOperator.Next())
		{
			std::shared_ptr<DataChannelServer> pPtr(new DataChannelServer);
			pPtr->bUseTcp = true;
			youmecommon::CXSharedArray<char> buffer;
			sqliteOperator >> pPtr->iSerial >> pPtr->strIPOrDomain >> pPtr->iPort >> pPtr->buffer >> pPtr->iRetry;
			if (pPtr->iSerial > m_iMaxID)
			{
				m_iMaxID = pPtr->iSerial;
			}
            
            pPtr->bInDb = true;
			m_dataCacheQueue.push(pPtr);
			m_wait.Increment();
		}
		//启动一个线程用来数据上报
		m_reportThread = std::thread(&CYouMeDataChannel::ReportProc, this);
		m_bInit = true;
	}
	if (nullptr != pBuffer)
	{
		std::shared_ptr<DataChannelServer> pPtr(new DataChannelServer);
		pPtr->strIPOrDomain = strDomain;
		pPtr->bUseTcp = bUseTcp;
		pPtr->iPort = iPort;
		pPtr->buffer.Allocate(iBufferLen);
		pPtr->bInDb = false;
		memcpy(pPtr->buffer.Get(), pBuffer, iBufferLen);

		if (pPtr->bUseTcp)
		{
			//先入本地db
			m_iMaxID++;
			pPtr->iSerial = m_iMaxID;
		}

		m_dataCacheQueue.push(pPtr);
		m_wait.Increment();
	}
	
}

int CYouMeDataChannel::StartReport()
{
	SendData(__XT(""), 0, false, nullptr, 0);
	return m_dataCacheQueue.size();
}

void CYouMeDataChannel::SetOnlyWriteDB(bool bOnlyWrite /*= false*/)
{
	m_bOnlyWriteDB = bOnlyWrite;
}

int CYouMeDataChannel::GetUnReportCount()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_dataCacheQueue.size();
}

CYouMeDataChannel::CYouMeDataChannel(const XString& strCachePath)
{
	m_bInit = false;
	m_bExitReport = false;
	m_iMaxID = 0;
	m_strCacheDBPath = strCachePath;
}


CYouMeDataChannel::~CYouMeDataChannel()
{
	m_bExitReport = true;
	m_wait.Increment();
	if (m_reportThread.joinable())
	{
		m_reportThread.join();
	}
}

void CYouMeDataChannel::ReportProc()
{
	YouMe_LOG_Info(__XT("Enter"));
	while (m_wait.Decrement())
	{
        if (m_bExitReport)
		{
			YouMe_LOG_Info(__XT("up level uninit ,exit"));
			break;
		}
		//取出一个来上报
		std::shared_ptr<DataChannelServer> pDataPtr;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_dataCacheQueue.empty())
			{
				continue;
			}
			//把取出来的复制到另一个缓存，然后从map 中删除
			pDataPtr = m_dataCacheQueue.front();
			m_dataCacheQueue.pop();
		}
        
        //如果是还没入库的数据，先入库
		if (m_bOnlyWriteDB || (pDataPtr->bUseTcp && !pDataPtr->bInDb))
        {
            //插入到db
            youmecommon::CSqliteOperator    sqliteOperator(m_sqliteDb);
            XString strSql = __XT("insert into report values(?1,?2,?3,?4,?5)");
            sqliteOperator.PrepareSQL(strSql);
            sqliteOperator << pDataPtr->iSerial;
            sqliteOperator << pDataPtr->strIPOrDomain;
            sqliteOperator << pDataPtr->iPort;
            sqliteOperator << pDataPtr->buffer;
            sqliteOperator << pDataPtr->iRetry;
            sqliteOperator.Execute();
        }
		if (m_bOnlyWriteDB)
		{
			continue;
		}
		//先判断是IP 还是域名，如果是域名的话查一下缓存
        if (nullptr == pDataPtr || (__XT("") == pDataPtr->strIPOrDomain))
        {
            YouMe_LOG_Error(__XT("the value in data queue is null"));
            continue;
        }
		XString strReportIP = pDataPtr->strIPOrDomain;
        
		if (!youmecommon::CXDNSParse::IsIPAddress(pDataPtr->strIPOrDomain))
        {
			std::map<XString, XString>::iterator it = m_domainIPMap.find(pDataPtr->strIPOrDomain);
			if (it != m_domainIPMap.end())
			{
				strReportIP = it->second;
			}
			else
			{
                //解析DNS
                if (!m_dnsHandle) {
                    YouMe_LOG_Error(__XT("m_dnsHandle is null, retry get, start"));
                    m_dnsHandle = youmecommon::DNSUtil::Instance();
                    if (!m_dnsHandle) {
                        YouMe_LOG_Error(__XT("m_dnsHandle is null, retry get fail"));
                        continue;
                    }
                }
        
                std::vector<std::string> ipLists;
				//youmecommon::CXDNSParse::ParseDomain2(pDataPtr->strIPOrDomain, ipLists);
                m_dnsHandle->GetHostByNameAsync(XStringToLocal(pDataPtr->strIPOrDomain), ipLists, 1000);    // timeout is 1000ms
                if (ipLists.size() > 0)
				{				
					strReportIP = LocalToXString(ipLists[0]);
					m_domainIPMap[pDataPtr->strIPOrDomain] = strReportIP;
				}
				else
				{
                    YouMe_LOG_Warning(__XT("DNS parse fail, ignore: %s"),pDataPtr->strIPOrDomain.c_str());
					continue;
				}
			}
		}
        
        if (strReportIP.empty() || !youmecommon::CXDNSParse::IsIPAddress(strReportIP)) {
            continue;
        }
        
		//开始上报了
		bool bReportSuccess = false;
		do 
		{
			pDataPtr->iRetry++;
			if (pDataPtr->bUseTcp)
			{
				youmecommon::CSyncTCP syncTcp;
				syncTcp.Init(strReportIP, pDataPtr->iPort);
				if (!syncTcp.Connect(5))
				{
					YouMe_LOG_Warning(__XT("connect fail:%s  %d"), strReportIP.c_str(), pDataPtr->iPort);
					break;
				}
				int iSendLen =syncTcp.SendData((const char*)pDataPtr->buffer.Get(), pDataPtr->buffer.GetBufferLen());
				if (iSendLen != pDataPtr->buffer.GetBufferLen())
				{
					YouMe_LOG_Warning(__XT("send fail:%s"), strReportIP.c_str());
					break;
				}
				//接收服务器返回的数据
				youmecommon::CXSharedArray<char> recvBuf;
				int iRecvLen = syncTcp.RecvDataByLen(1,recvBuf);
				if (iRecvLen <= 0)
				{
					YouMe_LOG_Warning(__XT("recv fail:%s"), strReportIP.c_str());
					break;
				}
				//1 字节的数据。 1 表示成功 0 标识失败
				if (recvBuf.Get()[0] == 1)
				{
					bReportSuccess = true;					
				}
				else
				{
					YouMe_LOG_Warning(__XT("server reply err:%s"), strReportIP.c_str());
				}
			}
			else
			{
				bReportSuccess = true;
				//使用UDP 上报,不需要关心成功失败
				youmecommon::CSyncUDP udp;
				udp.Init(strReportIP, pDataPtr->iPort);
				udp.SendData((const char*)pDataPtr->buffer.Get(), pDataPtr->buffer.GetBufferLen());
			}
		} while (0);
        
		//成功，或者重试3次那就删掉
		if (bReportSuccess || (pDataPtr->iRetry >= MAX_RETRY_COUNT))
		{
			//成功或者超过了次数
			std::lock_guard<std::mutex> lock(m_mutex);
			youmecommon::CSqliteOperator	sqliteOperator(m_sqliteDb);
			XString strSql = __XT("delete from report where id=?1");
			sqliteOperator.PrepareSQL(strSql);
			sqliteOperator << pDataPtr->iSerial;
			sqliteOperator.Execute();
            
		}
		else
		{
			//更新数据库重试次数
			std::lock_guard<std::mutex> lock(m_mutex);
			youmecommon::CSqliteOperator	sqliteOperator(m_sqliteDb);
			XString strSql = __XT("update report set retry=?1 where id=?2");
			sqliteOperator.PrepareSQL(strSql);
			sqliteOperator << pDataPtr->iRetry;
			sqliteOperator << pDataPtr->iSerial;
			sqliteOperator.Execute();
		}
	}
	YouMe_LOG_Info(__XT("Leave"));
}
