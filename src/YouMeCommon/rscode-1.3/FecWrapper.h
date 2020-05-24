#pragma once

extern "C"
{
#include <rscode-1.3/fec.h>
#include <rscode-1.3/rs.h>
}
#include <YouMeCommon/pb/fec.pb.h>

namespace youmecommon
{
//和 kcp 协议配合，不需要考虑多线程，所有的操作都会post 到同一个线程执行
class IFecWrapperListen
{
public:
    virtual ~IFecWrapperListen(){};
	//fec 编码之后会通知上层 可以发送数据了
	virtual void OnFecReadySend(const void* data, int dataLen, void* pParam) = 0;

	//fec 解码之后通知上层，接受数据了，可以把接受的数据给 kcp 了
	virtual void OnFecReadyRecv(const void*data, int dataLen, void* pParam) = 0;
};


class CFecWrapper
{
public:
	//对于接收方也就是解码来说 K,N 不需要但是编码是需要的
	CFecWrapper(int iConv, int iK = 0,int iN=0);
	~CFecWrapper();
	//当需要发送数据的时候调用，其实就是编码
	bool Send(const void*data, int dataLen);
	//当接收到数据调用，解码
	bool Recv(const void* data, int dataLen);
    
    void setNK( int iK , int iN );

	void SetFecListen(IFecWrapperListen* pListen, void* pParam)
	{
		m_pListen = pListen;
		m_pCallbackParam = pParam;
	}
private:
	youmecommon::fechead* GenFecHead(int iGroupSerial, int iGroupItemSerial, int iType);
	youmecommon::commonhead* GenComHead();
    
    void resetNK( int iK , int iN );

	int m_iConv = 0;
	void * m_fecCodec=nullptr;
	IFecWrapperListen* m_pListen = nullptr;
	void* m_pCallbackParam = nullptr;
	int m_iN = 0;
	int m_iK = 0;
	int m_iMaxPayloadLen = 0;//编解码都需要，用来标识当前一组fec 包中，最大的payload
	//用于编码的字段
	int m_iGroupSerial = 1;
	int m_iCurrentGroupItem = -1;

	//为了避免fec 的时候频繁的申请释放内存，这里 做一个缓存
	char** m_pCacheFecData = nullptr;
	
	//用来缓存收到的包的长度
	int* m_pCacheFecDataLen = nullptr;
    
    int m_iNNew = 0;
    int m_iKNew = 0;
};
}

