#pragma once

#include <YouMeCommon/jitterbuffer/JitterbufferBase.h>
#include <map>
#include <memory>
//帧数
class CJitterbufferCommon:public CJitterbufferBase
{
public:
	//可以用于普通udp 的jit，以及音频包的jit，视频的这个效果不好，没有区分I ，P 帧
	CJitterbufferCommon(int iConv, int iDefautDelay); //只有 Conv 相同的才能接收
	~CJitterbufferCommon();


	//当需要发送数据的时候调用，其实就是编码
	virtual bool Send(const void*data, int dataLen);
	//当接收到数据调用，解码. 不想引入时钟，所以使用简单的方法来判断，jittbuf 是否满作为一个出发条件
	virtual bool Recv(const void* data, int dataLen) ;



private:
	//最小包序号
	int m_iMinPacketSerial = 0;
	//用来缓存延迟的或者乱序的包
	std::map<int, std::shared_ptr<youmecommon::jitterbufferpacket> >m_cachePacket;
	void CheckNextPacket(bool bChecnNextPacketSerial);
};

