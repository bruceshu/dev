#include "JitterbufferCommon.h"
#include <YouMeCommon/pb/fec.pb.h>

CJitterbufferCommon::CJitterbufferCommon(int iConv, int iDefautDelay) :
CJitterbufferBase(iConv,iDefautDelay)
{
}


CJitterbufferCommon::~CJitterbufferCommon()
{
}

bool CJitterbufferCommon::Send(const void*data, int dataLen)
{
	//加上一个 jitterbuf 头，编号序号
	youmecommon::jitterbufferpacket jitpacket;
	jitpacket.set_allocated_comhead(GenComHead());
	jitpacket.set_allocated_head(GenJitterHead(GetNextIncrementSerial()));
	jitpacket.set_payload(data, dataLen);

	std::string strData;
	jitpacket.SerializeToString(&strData);
	if (nullptr != m_pListen)
	{
		m_pListen->OnJitterReadySend(strData.c_str(), strData.length(),m_pCallbackParam);
	}

#ifdef _DEBUG

		printf("jit Send:%d  %p\r\n", jitpacket.head().ipacketserial(),this);
	
	
	
#endif // _DEBUG
	return true;
}

bool CJitterbufferCommon::Recv(const void* data, int dataLen)
{
	std::shared_ptr<youmecommon::jitterbufferpacket> jitpacket(new youmecommon::jitterbufferpacket);
	if (!jitpacket->ParseFromArray(data, dataLen))
	{
		return false;
	}
	if (jitpacket->head().iconv() != m_iConv)
	{
		return false;
	}
#ifdef _DEBUG
	printf("jit recv:%d %p :serial:%d size:%d\r\n", jitpacket->head().ipacketserial(), this, jitpacket->head().ipacketserial(), dataLen);
#endif // _DEBUG


	//第一次的包序号可能不是0，所以需要特殊处理
	if (m_iMinPacketSerial == 0)
	{
		m_iMinPacketSerial = jitpacket->head().ipacketserial();
	}

	//如果接受的包的最小序号和当前的最小序号一致，说明包是按顺序正常过来的，则解包之后直接回掉上层，并且增加最小序号
	if (jitpacket->head().ipacketserial() == m_iMinPacketSerial)
	{
		if (nullptr != m_pListen)
		{
			m_pListen->OnJitterReadyRecv(jitpacket->payload().c_str(), jitpacket->payload().length(),m_pCallbackParam);
		}
		m_iMinPacketSerial++;
		CheckNextPacket(true);
		return true;
	}
	//接收到的不是最小序号，说明存在丢包或者延迟,先缓存起来
	m_cachePacket[jitpacket->head().ipacketserial()] = jitpacket;
	//需要判断当前是否已经达到了最大缓存，如果已经到了最大缓存的话，那就还需要通知遍历包通知上层
	if (m_cachePacket.size() >= m_iDelay)
	{
		//由于map 包是排好序列的所以只需要遍历map 把连续的包通知上去就可以了。只能通知连续的包
		CheckNextPacket(false);
	}
	return true;
}

void CJitterbufferCommon::CheckNextPacket(bool bChecnNextPacketSerial)
{
	if (m_cachePacket.empty())
	{
		return;
	}
	std::map<int, std::shared_ptr<youmecommon::jitterbufferpacket> >::iterator begin = m_cachePacket.begin();
	if (bChecnNextPacketSerial)
	{
		if (begin->first != m_iMinPacketSerial)
		{
			return;
		}
	}
	m_iMinPacketSerial = begin->first;
	for (; begin != m_cachePacket.end();)
	{
		if (m_iMinPacketSerial == begin->first)
		{
			if (nullptr != m_pListen)
			{
				m_pListen->OnJitterReadyRecv(begin->second->payload().c_str(), begin->second->payload().length(), m_pCallbackParam);
			}
			m_iMinPacketSerial++;
			begin = m_cachePacket.erase(begin);
		}
		else
		{
			//包序号不同就不需要遍历了
			break;
		}
	}
}
