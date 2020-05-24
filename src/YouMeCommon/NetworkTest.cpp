#include "NetworkTest.h"
#include <YouMeCommon/SyncTCP.h>
#include <YouMeCommon/XSharedArray.h>
#include <YouMeCommon/pb/youme_service_head.h>
#include <YouMeCommon/pb/youme_comm.pb.h>
#include <YouMeCommon/pb/youme_comm_conf.pb.h>
#include <YouMeCommon/TimeUtil.h>
CNetworkTest::CNetworkTest(XString strIP, int iPort,bool bSyncMode)
{
	m_strIP = strIP;
	m_iPort = iPort;
	m_bSyncMode = bSyncMode;
}

void CNetworkTest::UnInit()
{
	if (m_bExit)
	{
		return;
	}
	m_bExit = true;
	if (NULL != m_sem)
	{
		m_sem->Increment();
	}
	
}

struct TestItem
{
	XString strDes;
	int iSerial = 0;
	youmecommon::CXSharedArray<char> data;

};
CNetworkTest::~CNetworkTest()
{
	delete m_mutex;
	delete m_sem;
}

void CNetworkTest::StartCheck()
{
	m_sem = new youmecommon::CXSemaphore;
	m_mutex = new std::mutex;
	std::vector<TestItem> testLists;
	//连续发送1-10 个0
	for (int i = 1; i <= 10;i++)
	{
		TestItem item;		
		XStringStream strream;
		strream << __XT("发送(") << i << __XT(")个 二进制(0)");
		item.strDes = strream.str();
		item.data.Allocate(i);
		for (int j = 0; j < i;j++)
		{
			*(item.data.Get()+j) = 0;
		}

		
		testLists.push_back(item);
	}
	//发送1-10 个二进制1
	for (int i = 1; i <= 10; i++)
	{
		TestItem item;
		XStringStream strream;
		strream << __XT("发送(") << i << __XT(")个 二进制(1)");
		item.strDes = strream.str();
		item.data.Allocate(i);
		for (int j = 0; j < i; j++)
		{
			*(item.data.Get() + j) = 1;
		}
		testLists.push_back(item);
	}
	
	{
		TestItem item;
		XStringStream strream;
		strream << __XT("发送(1000)个 二进制(0)");
		item.strDes = strream.str();
		item.data.Allocate(1000);
		for (int j = 0; j < 1000; j++)
		{
			*(item.data.Get() + j) = 0;
		}
		testLists.push_back(item);
	}


	{
		TestItem item;
		XStringStream strream;
		strream << __XT("发送(1000)个 二进制(1)");
		item.strDes = strream.str();
		item.data.Allocate(1000);
		for (int j = 0; j < 1000; j++)
		{
			*(item.data.Get() + j) = 1;
		}
		testLists.push_back(item);
	}

	{
		TestItem item;
		XStringStream strream;
		strream << __XT("发送可见字符串 \"ab\"");
		item.strDes = strream.str();
		item.data.Allocate(2);
		char source[] = "ab";
		memcpy(item.data.Get(), source, 2);
		testLists.push_back(item);
	}


	{
		TestItem item;
		XStringStream strream;
		strream << __XT("发送可见字符串 \"601个\"a\"");
		item.strDes = strream.str();
		
		char  strABuffer[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
		item.data.Allocate(sizeof(strABuffer)/sizeof(char));
		memcpy(item.data.Get(), strABuffer, sizeof(strABuffer) / sizeof(char));
		testLists.push_back(item);
	}

	//发送PB包
	{
		TestItem item;
		XStringStream strream;
		strream << __XT("发送PB数据");
		item.strDes = strream.str();
		

		YOUMEServiceProtocol::CommConfReq req;
		req.set_version(1);
		req.set_appkey("YOUMEBC2B3171A7A165DC10918A7B50A4B939F2A187D0");
		req.set_verify("111111111111111");
		req.set_service_type(YOUMEServiceProtocol::SERVICE_IM);
#ifdef WIN32
		req.set_platform(YOUMECommonProtocol::Platform_Windows);
#elif OS_OSX
		req.set_platform(YOUMECommonProtocol::Platform_OSX);
#elif OS_IOS
		req.set_platform(YOUMECommonProtocol::Platform_IOS);
#elif OS_IOSSIMULATOR
		req.set_platform(YOUMECommonProtocol::Platform_IOS);
#elif OS_ANDROID
		req.set_platform(YOUMECommonProtocol::Platform_Android);
#elif OS_LINUX
		req.set_platform(YOUMECommonProtocol::Platform_Linux);
#endif
		req.set_brand("brand");
		req.set_sys_version("1");
		req.set_cpu_arch("cpuarch");
		req.set_pkg_name("pkgname");
		req.set_device_token("token");
		req.set_model("model");
		req.set_cpu_chip("cpuchip");
		req.set_sdk_version(1);
		req.set_sdk_name("name");
		req.set_strzone("china");


		SYoumeConnHead connHead;
		connHead.m_usCmd = 1;
		connHead.m_ullReqSeq = youmecommon::CTimeUtil::GetTimeOfDay_MS();
		connHead.m_uiServiceId = 1;

		unsigned char szReqBuffer[4096] = { 0 };
		unsigned int iDataSize = 0;
		memcpy(szReqBuffer, &connHead, sizeof(connHead));
		iDataSize += sizeof(connHead);


		req.SerializeToArray(szReqBuffer + iDataSize, 4096 - iDataSize);
		iDataSize += req.ByteSize();

		((SYoumeConnHead*)(szReqBuffer))->ToNetOrder();
		((SYoumeConnHead*)(szReqBuffer))->SetTotalLength(szReqBuffer, iDataSize);

		item.data.Allocate(iDataSize);
		memcpy(item.data.Get(), szReqBuffer, iDataSize);

		testLists.push_back(item);
	}

	if (m_bSyncMode)
	{
		InsertMsg(__XT("同步模式"));
	}
	else
	{
		InsertMsg(__XT("异步模式"));
	}
	

	for (int i = 0; i < testLists.size();i++)
	{
		//插入描述
		InsertMsg(testLists[i].strDes);

		youmecommon::CSyncTCP sync;
		sync.Init(m_strIP, m_iPort);
		if (m_bSyncMode)
		{
			if (!sync.ConnectSync())
			{
				InsertMsg(__XT("连接服务器失败"));
				continue;
			}
		}
		else
		{
			if (!sync.Connect(5000))
			{
				InsertMsg(__XT("连接服务器失败"));
				continue;
			}
		}
		int iSend = sync.SendBufferData(testLists[i].data.Get(), testLists[i].data.GetBufferLen());
		if (iSend != testLists[i].data.GetBufferLen())
		{
			InsertMsg(__XT("发送数据失败"));
			continue;
		}
		youmecommon::CXSharedArray<char> recvBuffer;
		int iRecv = sync.RecvDataByLen(1, recvBuffer);
		if (iRecv != 1)
		{
			InsertMsg(__XT("接收数据失败"));
			continue;
		}
		InsertMsg(__XT("测试成功"));
	}
}

XString CNetworkTest::GetOutPut()
{
	if (m_sem == NULL)
	{
		return __XT("");
	}
	m_sem->Decrement();
	if (m_bExit)
	{
		return __XT("");
	}
	std::lock_guard<std::mutex> lock(*m_mutex);
	if (m_strMsgLists.size() == 0)
	{
		return __XT("");
	}
	XString strResult = *m_strMsgLists.begin();
	m_strMsgLists.pop_front();
	return strResult;
}

void CNetworkTest::InsertMsg(const XString& strMsg)
{
	std::lock_guard<std::mutex> lock(*m_mutex);
	m_strMsgLists.push_back(strMsg);
	m_sem->Increment();
}
