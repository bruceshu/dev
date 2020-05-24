#include "XPowerUnion.h"
#include <YouMeCommon/TimeUtil.h>
#include <YouMeCommon/XSharedArray.h>
#define MAX_KCP_BODY 1024
#define MAX_KCP_HEAD 50
#ifndef WIN32
#include <unistd.h>
#endif
namespace youmecommon
{
    
    class QosSendRunable: public youmecommon::IMessageRunable
    {
    public:
        QosSendRunable( CXPowerUnion* pQos, const void* pData, int iDataLen )
        {
            m_sendBuffer.Allocate(iDataLen);
            memcpy(m_sendBuffer.Get(), pData, iDataLen);
            m_pQos = pQos;
        }
        
        void Run()
        {
            if(m_pQos->m_pUnionListen )
            {
                m_pQos->m_pUnionListen->OnPowerUnionSend( m_sendBuffer.Get() ,m_sendBuffer.GetBufferLen(), m_pQos->m_pListenParam );
            }
        }
    private:
        CXPowerUnion* m_pQos;
        youmecommon::CXSharedArray<char> m_sendBuffer;
    };
    
    class QosRecvRunable: public youmecommon::IMessageRunable
    {
    public:
        QosRecvRunable( CXPowerUnion* pQos, const void* pData, int iDataLen )
        {
            m_sendBuffer.Allocate(iDataLen);
            memcpy(m_sendBuffer.Get(), pData, iDataLen);
            m_pQos = pQos;
        }
        
        void Run()
        {
            if(m_pQos->m_pUnionListen )
            {
                m_pQos->m_pUnionListen->OnPowerUnionRecv( m_sendBuffer.Get() ,m_sendBuffer.GetBufferLen(), m_pQos->m_pListenParam );
            }
        }
    private:
        CXPowerUnion* m_pQos;
        youmecommon::CXSharedArray<char> m_sendBuffer;
    };
/*
class KCPSendRunable:public youmecommon::IMessageRunable
{
public:
	KCPSendRunable(ikcpcb* pKCP, const void* pData, int iDataLen)
	{
		m_sendBuffer.Allocate(iDataLen);
		memcpy(m_sendBuffer.Get(), pData, iDataLen);
		m_pKCP = pKCP;
	}
	void Run()
	{
		int iRemainCount = ikcp_waitsnd(m_pKCP);
		int iSendWnd = m_pKCP->snd_wnd;
		//如果待发送的超过了 发送窗口的阈值2倍。 就不要发送了。直到 一半再来发送
		ikcp_send(m_pKCP, m_sendBuffer.Get(), m_sendBuffer.GetBufferLen());	
	}
private:
	ikcpcb* m_pKCP;
	youmecommon::CXSharedArray<char> m_sendBuffer;
};




class KCPUpdateRunable :public youmecommon::IMessageRunable
{
public:
	KCPUpdateRunable(ikcpcb* pKCP, CXPowerUnion* pThis)
	{
		m_pKCP = pKCP;
		m_pThis = pThis;
	}
	void Run()
	{
		ikcp_update(m_pKCP, youmecommon::CTimeUtil::GetTimeOfDay_MS());
		while (true)
		{
			char buffer[1480] = { 0 };
			int hr = ikcp_recv(m_pKCP, buffer, 1480);
			// 没有收到包就退出
			if (hr < 0)
			{
				break;
			}
			if (nullptr != m_pThis->m_pUnionListen)
			{
				m_pThis->m_pUnionListen->OnPowerUnionRecv(buffer, hr, m_pThis->m_pListenParam);
			}
		}
	}
private:
	ikcpcb* m_pKCP;
	CXPowerUnion* m_pThis;
};


class KCPRecvRunable :public youmecommon::IMessageRunable
{
public:
	KCPRecvRunable(ikcpcb* pKCP, const void* pData, int iDataLen, CXPowerUnion* pThis)
	{
		m_sendBuffer.Allocate(iDataLen);
		memcpy(m_sendBuffer.Get(), pData, iDataLen);
		m_pKCP = pKCP;
		m_pThis = pThis;
	}
	void Run()
	{
		ikcp_input(m_pKCP, m_sendBuffer.Get(), m_sendBuffer.GetBufferLen());
		while (true)
		{
			char buffer[1480] = { 0 };
			int hr = ikcp_recv(m_pKCP, buffer, 1480);
			// 没有收到包就退出
			if (hr < 0)
			{
				break;
			}
			if (nullptr != m_pThis->m_pUnionListen)
			{
				m_pThis->m_pUnionListen->OnPowerUnionRecv(buffer, hr, m_pThis->m_pListenParam);
			}
		}
	}
private:
	ikcpcb* m_pKCP;
	youmecommon::CXSharedArray<char> m_sendBuffer;
	CXPowerUnion* m_pThis;
};


class FECSendRunable :public youmecommon::IMessageRunable
{
public:
	FECSendRunable(CFecWrapper* pKCP, const void* pData, int iDataLen)
	{
		m_sendBuffer.Allocate(iDataLen);
		memcpy(m_sendBuffer.Get(), pData, iDataLen);
		m_pKCP = pKCP;
	}
	void Run()
	{
		m_pKCP->Send(m_sendBuffer.Get(), m_sendBuffer.GetBufferLen());
	}
private:
	CFecWrapper* m_pKCP;
	youmecommon::CXSharedArray<char> m_sendBuffer;
};

class FECRecvRunable :public youmecommon::IMessageRunable
{
public:
	FECRecvRunable(CFecWrapper* pKCP, const void* pData, int iDataLen)
	{
		m_sendBuffer.Allocate(iDataLen);
		memcpy(m_sendBuffer.Get(), pData, iDataLen);
		m_pKCP = pKCP;
	}
	void Run()
	{
		m_pKCP->Recv(m_sendBuffer.Get(), m_sendBuffer.GetBufferLen());
	}
private:
	CFecWrapper* m_pKCP;
	youmecommon::CXSharedArray<char> m_sendBuffer;
};


class JitSendRunable :public youmecommon::IMessageRunable
{
public:
	JitSendRunable(CJitterbufferBase* pKCP, const void* pData, int iDataLen)
	{
		m_sendBuffer.Allocate(iDataLen);
		memcpy(m_sendBuffer.Get(), pData, iDataLen);
		m_pKCP = pKCP;
	}
	void Run()
	{
		m_pKCP->Send(m_sendBuffer.Get(), m_sendBuffer.GetBufferLen());
	}
private:
	CJitterbufferBase* m_pKCP;
	youmecommon::CXSharedArray<char> m_sendBuffer;
};


class JitRecvRunable :public youmecommon::IMessageRunable
{
public:
	JitRecvRunable(CJitterbufferBase* pKCP, const void* pData, int iDataLen)
	{
		m_sendBuffer.Allocate(iDataLen);
		memcpy(m_sendBuffer.Get(), pData, iDataLen);
		m_pKCP = pKCP;
	}
	void Run()
	{
		m_pKCP->Recv(m_sendBuffer.Get(), m_sendBuffer.GetBufferLen());
	}
private:
	CJitterbufferBase* m_pKCP;
	youmecommon::CXSharedArray<char> m_sendBuffer;
};
*/


CXPowerUnion::CXPowerUnion(int flags)
{
	m_flags = flags;
    m_iConv = 0;
}


CXPowerUnion::~CXPowerUnion()
{
	Uninit();
}

void  CXPowerUnion::writeKCPlog(const char *log, struct IKCPCB *kcp, void *user)
{
#ifdef _DEBUG
	OutputDebugStringA(log);
	OutputDebugStringA("\r\n");
#endif
}


bool CXPowerUnion::Init( int iConv, int iK, int iN, int iDefaultDelay)
{
	
	m_bIsInit = true;
	
	m_iConv = iConv;
	//kcp 
	m_KCP = ikcp_create(iConv, this);
	m_KCP->output = CXPowerUnion::UdpSendKcpOutput;
	
#ifdef _DEBUG
	m_KCP->logmask = 0xFFFFFFFF;
	m_KCP->writelog = CXPowerUnion::writeKCPlog;
#endif
    
    m_msgHandle.Init();
	
	//快速模式
	ikcp_nodelay(m_KCP, 1, 20, 10, 1);  //
	//普通模式
	//ikcp_nodelay(m_KCP, 0, 40, 0, 0);
	ikcp_wndsize(m_KCP, 1024, 1024);
	m_KCP->rx_minrto = 1500; //最小多少ms 之内检测丢包，网络不好的时候可以适当增加这个值
	m_KCP->mtu = 1024;  //设置小一些可以优先通过路由器


	//FEC
	m_pSendFec = new CFecWrapper(iConv, iK, iN);
	m_pSendFec->SetFecListen(this, nullptr);

	m_pRecvFec = new CFecWrapper(iConv);
	m_pRecvFec->SetFecListen(this, nullptr);
	//jit
	m_pSendJitter = CJitterbufferBase::CreateJitterInstance(1, iConv, 0);
	m_pSendJitter->SetJitterbufferListen(this, nullptr);

	m_pRecvJitter = CJitterbufferBase::CreateJitterInstance(1, iConv, iDefaultDelay);
	m_pRecvJitter->SetJitterbufferListen(this, nullptr);


	//启动一个线程用来kcpupdate 每个20毫秒
	m_updateKcpThread = std::thread([this](){
		while (m_bIsInit)
		{
			int iDelayTime = 20;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				//检查下一次update时间
				IUINT32 iNowTime = youmecommon::CTimeUtil::GetTimeOfDay_MS();
				IUINT32 iUpdateTime = ikcp_check(m_KCP, iNowTime);
				iDelayTime = iUpdateTime - iNowTime;
			}
#ifdef WIN32
			Sleep(iDelayTime);
#else
			usleep(iDelayTime * 1000);
#endif // WIN32

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				ikcp_update(m_KCP, youmecommon::CTimeUtil::GetTimeOfDay_MS());
			}
		}
	});

	return true;
}
    
void CXPowerUnion::setSendFecNK( int iK, int iN)
{
    m_pSendFec->setNK( iK, iN );
}

void CXPowerUnion::Uninit()
{
	if (!m_bIsInit)
	{
		return;
	}
	m_bIsInit = false;
	if (m_updateKcpThread.joinable())
	{
		m_updateKcpThread.join();
	}
    
    m_msgHandle.UnInit();

	//销毁 kcp
	ikcp_release(m_KCP);
	delete m_pSendFec;
	delete m_pRecvFec;


	CJitterbufferBase::DestroyJitterInstance(m_pSendJitter);
	CJitterbufferBase::DestroyJitterInstance(m_pRecvJitter);
}



int CXPowerUnion::InterRecvData(const void* pData, int iDataLen)
{
	//接收应该必须根据实际的类型来怕暖到底给哪个
	youmecommon::commonheadpacket comhead;
	if (!comhead.ParseFromArray(pData, iDataLen))
	{
		//解析失败，不是自定义协议
        InnerOnPowerUnionRecv( pData, iDataLen );

		return iDataLen;
	}

	//交给jit	
	switch (comhead.head().type())
	{
	case youmecommon::packettype_fec:
	{
		m_pRecvFec->Recv(pData, iDataLen);
	}
	break;
	case youmecommon::packettype_kcp:
	{
		//需要解包
		youmecommon::kcppacket kcpPacket;
		kcpPacket.ParseFromArray(pData, iDataLen);

		ikcp_input(m_KCP, (const char*)kcpPacket.payload().c_str(), kcpPacket.payload().length());
		while (true)
		{
			char buffer[1480] = { 0 };
			int hr = ikcp_recv(m_KCP, buffer, 1480);
			// 没有收到包就退出
			if (hr < 0)
			{
				break;
			}
            
            InnerOnPowerUnionRecv( buffer, hr );
		}
	}
	break;
	case youmecommon::packettype_jit:
	{
		m_pRecvJitter->Recv(pData, iDataLen);
	}
	break;
	default:
	{
        InnerOnPowerUnionRecv( pData, iDataLen );
	}
	break;
	}

	return iDataLen;
}

youmecommon::commonhead* CXPowerUnion::GenComHead()
{
	youmecommon::commonhead* head = new youmecommon::commonhead;
	head->set_iver(1);
	head->set_type(youmecommon::packettype_kcp);
	return head;
}


youmecommon::kcphead* CXPowerUnion::GenKCPHead()
{
	youmecommon::kcphead* head = new youmecommon::kcphead;
	head->set_iconv(m_iConv);
	head->set_iver(1);
	return head;
}

int CXPowerUnion::SendData(const void* pData, int iDataLen)
{
	if (!m_bIsInit)
	{
		return -1;
	}
	if (iDataLen > MAX_KCP_BODY - MAX_KCP_HEAD)
	{

		return -1;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_flags & XPowerFlag_KCP)
	{
		//开启KCP
		int iRemainCount = ikcp_waitsnd(m_KCP);
		int iSendWnd = m_KCP->snd_wnd;
		//如果待发送的超过了 发送窗口的阈值2倍。 就不要发送了。直到 一半再来发送
		/*if (iRemainCount >= 2*iSendWnd)
		{
			printf("send data: %d  %d\r\n",iRemainCount,iSendWnd);
			return -1;
		}*/

		//加上一个KCP 头
		ikcp_send(m_KCP,(const char*) pData, iDataLen);
	}
	else if (m_flags & XPowerFlag_FEC)
	{
		m_pSendFec->Send(pData, iDataLen);
	}
	else if (m_flags & XPowerFlag_JIT)
	{
		m_pSendJitter->Send(pData, iDataLen);
	}
	else
	{
        InnerOnPowerUnionSend(pData, iDataLen);
	}
	return iDataLen;
}






//KCP 发送处理完，继续交给上层
int CXPowerUnion::UdpSendKcpOutput(const char *buf, int len, ikcpcb *kcp, void *user)
{
	CXPowerUnion* pThis = (CXPowerUnion*)user;
	//KCP 的回调，加上一个KCP头
	youmecommon::kcppacket packet;
	packet.set_allocated_comhead(pThis->GenComHead());
	packet.set_allocated_head(pThis->GenKCPHead());
	packet.set_payload(buf, len);

	//序列化kcp 包
	std::string strData;
	packet.SerializeToString(&strData);


	if (pThis->m_flags & XPowerFlag_FEC)
	{
		//在一个线程里面，所以直接调用
		pThis->m_pSendFec->Send(strData.c_str(), strData.length());
	}
	else if (pThis->m_flags & XPowerFlag_JIT)
	{
		pThis->m_pSendJitter->Send(strData.c_str(), strData.length());
	}
	else
	{
        pThis->InnerOnPowerUnionSend(strData.c_str(), strData.length());

	}
	//pThis->m_pUnionListen->OnPowerUnionSend(buf, len, pThis->m_pListenParam);
	return 0;
}

void CXPowerUnion::OnFecReadySend(const void* data, int dataLen, void* pParam)
{
	//发送给jit
	if (m_flags & XPowerFlag_JIT)
	{
		m_pSendJitter->Send(data, dataLen);
	}
	else
	{
		InnerOnPowerUnionSend( data, dataLen );
	}
}


void CXPowerUnion::OnJitterReadySend(const void* data, int dataLen, void* pParam)
{
	//通过UDP 发送出去
	InnerOnPowerUnionSend( data, dataLen );
}

//接收到上层的数据
int CXPowerUnion::RecvData(const void* pData, int iDataLen)
{
	if (!m_bIsInit)
	{
		return -1;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	return InterRecvData(pData, iDataLen);
}


void CXPowerUnion::OnJitterReadyRecv(const void*data, int dataLen, void* pParam)
{
	InterRecvData(data, dataLen);
}

void CXPowerUnion::OnFecReadyRecv(const void*data, int dataLen, void* pParam)
{
	InterRecvData(data, dataLen);
}
    
void CXPowerUnion::InnerOnPowerUnionSend( const void* pBuffer, int iBufferLen )
{
    if( m_pUnionListen == nullptr )
    {
        return ;
    }
//    m_pUnionListen->OnPowerUnionSend( pBuffer, iBufferLen,  m_pListenParam );
    std::shared_ptr<IMessageRunable> runable( new QosSendRunable( this, pBuffer, iBufferLen ) );
    m_msgHandle.Post( runable );
}
void CXPowerUnion::InnerOnPowerUnionRecv(const void* pBuffer, int iBufferLen)
{
    if( m_pUnionListen == nullptr )
    {
        return ;
    }
//    m_pUnionListen->OnPowerUnionRecv( pBuffer, iBufferLen,  m_pListenParam );
    std::shared_ptr<IMessageRunable> runable( new QosRecvRunable( this, pBuffer, iBufferLen ) );
    m_msgHandle.Post( runable );
}
    
}



