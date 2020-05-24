#include "FecWrapper.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#define MAX_FEC_PAYLOAD 1400
#define MAX_FEC_HEAD 50  //fec 头最大值
#define FEC_MAGIC_NUMBER "FE!"
#include <YouMeCommon/Log.h>
#include <StringUtil.hpp>
namespace youmecommon
{
    
CFecWrapper::CFecWrapper(int iConv, int iK , int iN )
{
	m_iConv = iConv;
	m_iK = iK;
	m_iN = iN;
}

CFecWrapper::~CFecWrapper()
{
	if (nullptr != m_pCacheFecData)
	{
		for (int i = 0; i < m_iN;i++)
		{
			free(m_pCacheFecData[i]);
		}
		free(m_pCacheFecData);
	}
	delete []m_pCacheFecDataLen;	
	fec_free(m_fecCodec);
}

bool CFecWrapper::Send(const void*data, int dataLen)
{
	if (dataLen > MAX_FEC_PAYLOAD - MAX_FEC_HEAD)
	{
		return false;
	}

	//创建一个编码器
	if (nullptr == m_fecCodec)
	{
		m_fecCodec = fec_new(m_iK, m_iN);
		m_pCacheFecData = (char**)malloc(sizeof(char*) * m_iN);
		for (int i = 0; i < m_iN;i++)
		{
			m_pCacheFecData[i] = (char*)malloc(MAX_FEC_PAYLOAD);
		}
	}
	//判断当前包 
	m_iCurrentGroupItem++;
	if (m_iCurrentGroupItem >= m_iK)
	{
		//开始生成冗余包
		rs_encode(m_fecCodec, m_pCacheFecData, m_iMaxPayloadLen);

		//将生成的fec 冗余包也发出去
		 
		for (int i = m_iK; i < m_iN;i++)
		{
			youmecommon::fecpacket packet;
			packet.set_allocated_comhead(GenComHead());
			packet.set_allocated_head(GenFecHead(m_iGroupSerial, m_iCurrentGroupItem++, 2));
			//生成的冗余包不需要加magicnumber，因为冗余包仅仅是用来生成数据包
			packet.set_payload(m_pCacheFecData[i], m_iMaxPayloadLen);

			//序列化fec 包
			std::string strData;
			packet.SerializeToString(&strData);
			//通知上层可以发送了
			if (nullptr != m_pListen)
			{
				m_pListen->OnFecReadySend(strData.c_str(), strData.length(),m_pCallbackParam);
			}

		}

		//完成了一轮，组序号重新开始
		m_iGroupSerial++;
		m_iCurrentGroupItem = 0;
		m_iMaxPayloadLen = 0;

        
        if( (m_iKNew != 0 && m_iNNew != 0)
           && ( m_iKNew != m_iK || m_iNNew != m_iN ))
        {
            resetNK( m_iKNew, m_iNNew );
        }
        else{
            for (int i = 0; i < m_iN; i++)
            {
                memset(m_pCacheFecData[i],0, MAX_FEC_PAYLOAD);
            }
        }
	}

	//实际情况发现 即使传入错误的数据FEC 居然返回成功，这就会导致FEC 之后的数据无法知道是正确还是错误，因此增加magicnumber。
	memcpy(m_pCacheFecData[m_iCurrentGroupItem], FEC_MAGIC_NUMBER, strlen(FEC_MAGIC_NUMBER));

	//把payload 拷贝相应的缓存
	short sPayloadsLen = dataLen;
	sPayloadsLen = htons(sPayloadsLen);
	//增加长度是为了 fec decode出来之后知道原始数据的长度。冗余包是不需要这个的
	memcpy(m_pCacheFecData[m_iCurrentGroupItem]+strlen(FEC_MAGIC_NUMBER), &sPayloadsLen, sizeof(short));
	memcpy(m_pCacheFecData[m_iCurrentGroupItem] + sizeof(short) + strlen(FEC_MAGIC_NUMBER), data, dataLen);
	//在数据尾部再加上Magicnumber
	memcpy(m_pCacheFecData[m_iCurrentGroupItem] + sizeof(short) + strlen(FEC_MAGIC_NUMBER) + dataLen, FEC_MAGIC_NUMBER, strlen(FEC_MAGIC_NUMBER));

	if (dataLen + sizeof(short) + 2*strlen(FEC_MAGIC_NUMBER) > m_iMaxPayloadLen)
	{
		m_iMaxPayloadLen = dataLen + sizeof(short) + 2*strlen(FEC_MAGIC_NUMBER);
	}

	//生成fec包
	youmecommon::fecpacket packet;
	packet.set_allocated_comhead(GenComHead());
	packet.set_allocated_head(GenFecHead(m_iGroupSerial, m_iCurrentGroupItem, 1));
	packet.set_payload(m_pCacheFecData[m_iCurrentGroupItem], dataLen + sizeof(short) + 2*strlen(FEC_MAGIC_NUMBER));

	//序列化fec 包
	std::string strData;
	packet.SerializeToString(&strData);
	//通知上层可以发送了
	if (nullptr != m_pListen)
	{
		m_pListen->OnFecReadySend(strData.c_str(), strData.length(),m_pCallbackParam);
	}
	return true;
}
    
void CFecWrapper::setNK( int iK , int iN )
{
    m_iNNew = iN;
    m_iKNew = iK;
}

    
void CFecWrapper::resetNK( int iK , int iN )
{
    //先清除原来的
    if (nullptr != m_pCacheFecData)
    {
        for (int i = 0; i < m_iN;i++)
        {
            free(m_pCacheFecData[i]);
        }
        free(m_pCacheFecData);
    }
    delete []m_pCacheFecDataLen;
    fec_free(m_fecCodec);
    
    m_iK = iK;
    m_iN = iN;
    
    m_fecCodec = fec_new(m_iK, m_iN);
    m_pCacheFecData = (char**)malloc(sizeof(char*) * m_iN);
     m_pCacheFecDataLen = new int[m_iN];
    for (int i = 0; i < m_iN;i++)
    {
        m_pCacheFecData[i] = (char*)malloc(MAX_FEC_PAYLOAD);
        m_pCacheFecDataLen[i] = 0;
    }
   
}

bool CFecWrapper::Recv(const void* data, int dataLen)
{
	youmecommon::fecpacket packet;
	if (!packet.ParseFromArray(data, dataLen))
	{
		//给过来的包不是fec 的包
		return false;
	}
	if (packet.head().iconv() != m_iConv)
	{
		//会话标识不一样，不能接受
		return false;
	}

	if (packet.head().igroupserial() < m_iGroupSerial)
	{
		//组序号比当前的小，说明是之前丢掉的又回来了，不要触发 解包,直接抛出去
		//只要是数据包都需要通知上层
		if ((nullptr != m_pListen) && (packet.head().itype() == 1))
		{
			//数据包需要减去真实数据前面的2字节 以及2个 magicnumber
			m_pListen->OnFecReadyRecv(packet.payload().c_str() + sizeof(short) + strlen(FEC_MAGIC_NUMBER), packet.payload().length() - sizeof(short) - 2*strlen(FEC_MAGIC_NUMBER), m_pCallbackParam);
		}
		return true;
	}
    
    if( nullptr == m_fecCodec )
    {
        resetNK( packet.head().ik(), packet.head().in() );
        m_iGroupSerial = packet.head().igroupserial();
    }
    
	//先简单处理，如果group 不同的话就认为新的一帧数据来了，判断是否需要解码
	// 最好这里是需要有一个阈值，组序号被跳过了多少次才认为丢了，解码
	if (m_iGroupSerial != packet.head().igroupserial())
	{
		//检测一下丢失了多少个包
		int iLostPackets = 0;//总的丢了多少
		int iLostDataPackets = 0;//数据包丢了多少
		int iMaxDataLen = 0;


		//由于FEC decode 会挪动内存的位置，并且输入给fec 之前需要把 buffer 指针置空，所以一定要先保存一下地址
		char** pCacheFecData  = (char**)malloc(sizeof(char*) * m_iN);
		for (int i = 0; i < m_iN;i++)
		{
			pCacheFecData[i] = m_pCacheFecData[i];
		}
		//开始遍历找到
		for (int i = 0; i < m_iN;i++)
		{
			
			if (m_pCacheFecDataLen[i] == 0)
			{
				iLostPackets++;
				if (i<m_iK)
				{
					iLostDataPackets++;
				}
				m_pCacheFecData[i] = nullptr;
				
			}

			if (m_pCacheFecDataLen[i] > iMaxDataLen)
			{
				iMaxDataLen = m_pCacheFecDataLen[i];
			}
		}
		do 
		{
			if (iLostPackets > (m_iN - m_iK))
			{
				//丢失太多了
#ifdef _DEBUG
				YouMe_LOG_Warning(__XT("lost packets too much:%d  iLostDataPackets:%d\r\n"), iLostPackets, iLostDataPackets);
#endif			
				break;
			}
			if (0 == iLostDataPackets)
			{
				//没有丢数据包
				break;
			}

			//这个decode 不会改变m_pCacheFecData 内存的地址，但是会移动指针的位置，所以解码之后一定要重新把内存地址给m_pCacheFecData 赋值
#ifdef _DEBUG
			YouMe_LOG_Debug(__XT("lost packets:%d  iLostDataPackets:%d\r\n"), iLostPackets, iLostDataPackets);
#endif
			int ret = rs_decode(m_fecCodec, m_pCacheFecData, iMaxDataLen);
			if (0 != ret)
			{			
				break;
			}
			//把fec 解码的数据包 回掉上去
			for (int i = 0; i < m_iK; i++)
			{
				if (m_pCacheFecDataLen[i] == 0)
				{
					//先判断magcinumber
					if (memcmp(m_pCacheFecData[i],FEC_MAGIC_NUMBER,strlen(FEC_MAGIC_NUMBER)) !=0)
					{
						//乱包
						YouMe_LOG_Debug(__XT("FEC decode failed: head is not magicnumber"));
						continue;
					}

					short sDataLen;
					memcpy(&sDataLen, m_pCacheFecData[i]+strlen(FEC_MAGIC_NUMBER), sizeof(short));
                    sDataLen = ntohs(sDataLen);
					if ((sDataLen + sizeof(short) + 2 * strlen(FEC_MAGIC_NUMBER)) > iMaxDataLen)
					{
						//解包的问题
						YouMe_LOG_Warning(__XT("FEC decode failed:%d"), (int)sDataLen);
						continue;
					}
					//判断数据尾部是否是magicnumber
					if (memcmp(m_pCacheFecData[i] + sizeof(short) + strlen(FEC_MAGIC_NUMBER) + sDataLen, FEC_MAGIC_NUMBER, strlen(FEC_MAGIC_NUMBER)) != 0)
					{
						//乱包
						YouMe_LOG_Warning(__XT("FEC decode failed: tail is not magicnumber"));
						continue;
					}

                    m_pListen->OnFecReadyRecv(m_pCacheFecData[i] + sizeof(short) + strlen(FEC_MAGIC_NUMBER), sDataLen, m_pCallbackParam);
#ifdef _DEBUG
					
					//OutputDebugStringA("fec decode success:%d \n",(int)sDataLen);
					YouMe_LOG_Debug(__XT("FEC decode success:%d"),(int)sDataLen);
#endif
					
				}
			}
		} while (false);
		//重置状态
		m_iGroupSerial = packet.head().igroupserial();
        
        if ((m_iK != packet.head().ik()) || (m_iN != packet.head().in()))
        {
            resetNK( packet.head().ik() , packet.head().in() );
        }
        else
        {
            for (int i = 0; i < m_iN; i++)
            {
                m_pCacheFecDataLen[i] = 0;
            }
            //还原回去
            for (int i = 0; i < m_iN; i++)
            {
                m_pCacheFecData[i] = pCacheFecData[i];
                memset(m_pCacheFecData[i], 0, MAX_FEC_PAYLOAD);
            }
        }
    
		free(pCacheFecData);
	}

    //N和K 必须相同，否则出错,组与组之间可以改变，同组内必须相同
    if ((m_iK != packet.head().ik()) || (m_iN != packet.head().in()))
    {
        return false;
    }


	//把包存起来，然后做fec 的解码,这里不区分数据包和冗余包,存在相应的位置就可以了
	memcpy(m_pCacheFecData[packet.head().igroupitemserial()], packet.payload().c_str(), packet.payload().length());
	m_pCacheFecDataLen[packet.head().igroupitemserial()] = packet.payload().length();

	//只要是数据包都需要通知上层
	if ((nullptr != m_pListen) && (packet.head().itype() == 1))
	{
		//数据包需要减去真实数据前面的2字节 以及 magicnumber
		m_pListen->OnFecReadyRecv(packet.payload().c_str() + sizeof(short) + strlen(FEC_MAGIC_NUMBER), packet.payload().length() - sizeof(short) - 2*strlen(FEC_MAGIC_NUMBER), m_pCallbackParam);
	}
	return true;
}

youmecommon::commonhead* CFecWrapper::GenComHead()
{
	youmecommon::commonhead* head = new youmecommon::commonhead;
	head->set_iver(1);
	head->set_type(youmecommon::packettype_fec);
	return head;
}

youmecommon::fechead* CFecWrapper::GenFecHead(int iGroupSerial, int iGroupItemSerial, int iType)
{
	youmecommon::fechead* head = new youmecommon::fechead;
	head->set_iconv(m_iConv);
	head->set_iver(1);
	head->set_igroupserial(iGroupSerial);
	head->set_igroupitemserial(iGroupItemSerial);
	head->set_itype(iType);
	head->set_in(m_iN);
	head->set_ik(m_iK);
	return head;
}
}
