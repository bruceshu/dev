#include "JitterbufferBase.h"
#include <YouMeCommon/jitterbuffer/JitterbufferCommon.h>

CJitterbufferBase::CJitterbufferBase(int iConv, int iDefautDelay) :
m_iDelay(iDefautDelay),
m_iConv(iConv)
#ifndef WIN32
, m_iAutoIncrementSerial(0)
#endif
{
}


CJitterbufferBase::~CJitterbufferBase()
{
}

CJitterbufferBase* CJitterbufferBase::CreateJitterInstance(int iJitterType, int iConv, int iDefautDelay)
{
	return new CJitterbufferCommon(iConv, iDefautDelay);
}

void CJitterbufferBase::DestroyJitterInstance(CJitterbufferBase* pInstance)
{
	delete pInstance;
}
youmecommon::commonhead* CJitterbufferBase::GenComHead()
{
	youmecommon::commonhead* head = new youmecommon::commonhead;
	head->set_iver(1);
	head->set_type(youmecommon::packettype_jit);
	return head;
}

youmecommon::jitterbufferhead* CJitterbufferBase::GenJitterHead(int iPacketSerial)
{
	youmecommon::jitterbufferhead* head = new youmecommon::jitterbufferhead;
	head->set_iconv(m_iConv);
	head->set_iver(1);
	head->set_ipacketserial(iPacketSerial);
	return head;
}
