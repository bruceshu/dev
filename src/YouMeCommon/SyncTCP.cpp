#include "SyncTCP.h"
#include <assert.h>
#include <time.h>
#include <YouMeCommon/Log.h>
#include <curl/inet_ntop.h>
#ifndef WIN32
#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/socket.h>
#else
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")   
#endif
#include "XNetworkUtil.h"
#include <YouMeCommon/XDNSParse.h>

namespace youmecommon {

CSyncTCP::CSyncTCP()
{
	m_client = -1;
}
CSyncTCP::~CSyncTCP()
{
	Close();
}

void CSyncTCP::Close()
{
	if (-1 != m_client)
	{
#ifdef WIN32
		shutdown(m_client, SD_BOTH);
		closesocket(m_client);
#else
		shutdown(m_client, SHUT_RDWR);
		close(m_client);
#endif
		m_client = -1;
	}
	
}

bool CSyncTCP::Init(const XString&strServerIP, int iPort,int iConnectTimeout)
{
	if (-1 != m_client)
	{
		return true;
	}

	m_strServerIP = strServerIP;
	m_iServerPort = iPort;
	m_iSendTimeout = iConnectTimeout;
	return true;
}

bool CSyncTCP::Connect(int timeout)
{
	struct addrinfo hints;
    int ipType;
    ipType = 0;
    
	memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	struct addrinfo *result = NULL;
    std::string strIP;
    if (!m_strServerIP.empty() && youmecommon::CXDNSParse::IsIPAddress(m_strServerIP)) {
         strIP = XStringToUTF8(m_strServerIP);
    } else {
        return false;
    }
	
	int iRet = getaddrinfo(strIP.c_str(), NULL, &hints, &result);
	if (iRet != 0)
	{
		YouMe_LOG_Info(__XT("getaddrinfo failed"));
		return false;
	}

	int errorCode = -1;
	struct addrinfo* pAddrInfo = NULL;
	
	for (pAddrInfo = result; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
	{
		if (AF_INET == pAddrInfo->ai_family)
		{

			if (AF_INET6 == ipType)
			{
				YouMe_LOG_Debug(__XT("server is IPV4 and client is IPV6"));
				struct addrinfo* addrinfoResult;
				if (getaddrinfo(strIP.c_str(), "http", &hints, &addrinfoResult) != 0)
				{
					YouMe_LOG_Debug(__XT("getaddrinfo failed 2"));
					freeaddrinfo(result);
					return false;
				}

				for (struct addrinfo* res = addrinfoResult; res; res = res->ai_next)
				{
					m_client = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
					if (m_client < 0)
					{
						freeaddrinfo(result);
						freeaddrinfo(addrinfoResult);
						YouMe_LOG_Debug(__XT("getaddrinfo failed 3"));
						return false;
					}
					SetSocketOpt();
					SetBlock(false);

					if (res->ai_family == AF_INET)
					{
						struct sockaddr_in addr_in = *((struct sockaddr_in *) res->ai_addr);
						addr_in.sin_port = htons(m_iServerPort);
						errorCode = connect(m_client, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
						YouMe_LOG_Debug(__XT("connected  AF_INET: %d"), errorCode);
						break;
					}

					else if (res->ai_family == AF_INET6)
					{
						struct sockaddr_in6 addr_in6 = *((struct sockaddr_in6 *) res->ai_addr);
						addr_in6.sin6_port = htons(m_iServerPort);
						errorCode = connect(m_client, (struct sockaddr *)&addr_in6, sizeof(struct sockaddr_in6));
						YouMe_LOG_Debug(__XT("connected AF_INET6: %d"), errorCode);
						break;
					}
					else
					{
						// Error
					}
				}
				if (addrinfoResult != NULL)
				{
					freeaddrinfo(addrinfoResult);
				}
			}
			else
			{
				//YouMe_LOG_Info(__XT("IPV4"));
				m_client = socket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);
				SetSocketOpt();
				SetBlock(false);
				struct sockaddr_in addr_in = *((struct sockaddr_in *) pAddrInfo->ai_addr);
				addr_in.sin_port = htons(m_iServerPort);
				errorCode = connect(m_client, (struct sockaddr *)&addr_in, pAddrInfo->ai_addrlen);
				YouMe_LOG_Debug(__XT("connected  IPV4: %d"), errorCode);
			}
		}
		else if (AF_INET6 == pAddrInfo->ai_family)
		{
			YouMe_LOG_Info(__XT("IPV6"));
			m_client = socket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);
			SetSocketOpt();
			SetBlock(false);
			struct sockaddr_in6 addr_in6 = *((struct sockaddr_in6 *)pAddrInfo->ai_addr);
			addr_in6.sin6_port = htons(m_iServerPort);
			errorCode = connect(m_client, (struct sockaddr *)&addr_in6, pAddrInfo->ai_addrlen);
			YouMe_LOG_Debug(__XT("connected  IPV6: %d"), errorCode);
		}

#ifdef WIN32
		if (errorCode == WSAEWOULDBLOCK || errorCode == -1)
#else
		if (errorCode == -1)
#endif
		{
			fd_set set;
			FD_ZERO(&set);
			FD_SET(m_client, &set);
			struct timeval  stimeout;
			stimeout.tv_sec = timeout;
			stimeout.tv_usec = 0;
			int status = select(m_client + 1, NULL, &set, NULL, &stimeout);
			if (status == -1 || status == 0)
			{
				YouMe_LOG_Debug(__XT("connected failed 9"));
				errorCode = -1;
			}
			else
			{
				if (FD_ISSET(this->m_client, &set)) {
					socklen_t peerAdressLen;
					struct  sockaddr peerAdress;
					peerAdressLen = sizeof(peerAdress);

					if (getpeername(this->m_client, &peerAdress, &peerAdressLen) == 0) { // real connect success
						// int flags = fcntl(m_client, F_GETFL, 0);
						// fcntl(m_client, F_SETFL, flags & ~O_NONBLOCK);
						SetBlock(true);
						errorCode = 0;
					}
					else {
						YouMe_LOG_Debug(__XT("connected failed 10"));
						errorCode = -1;
					}
                }else{
                    YouMe_LOG_Debug(__XT("FD_ISSET return 0"));
                }
			}
		}

		break;
	}
	
	if (result != NULL)
	{
		freeaddrinfo(result);
	}

	return errorCode == 0;
}

//测试用的同步连接接口
bool CSyncTCP::ConnectSync()
{
	m_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = inet_addr(XStringToUTF8(m_strServerIP).c_str());
	addr_in.sin_port = htons(m_iServerPort);


	int errorCode = connect(m_client, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
	return errorCode == 0;
}

int CSyncTCP::SendData(const char* buffer, int iBufferSize)
{
	//先发送数据的长度,转换成网络字节序
	unsigned short iNetOrder = htons((unsigned short)iBufferSize);
	if (sizeof(unsigned short) != SendBufferData((const char*)&iNetOrder, sizeof(unsigned short)))
	{
		return -1;
	}
	return SendBufferData(buffer, iBufferSize);
}


void CSyncTCP::UnInit()
{
	Close();
}

int CSyncTCP::SendBufferData(const char*buffer, int iBufferSize)
{  
	int iSend = 0;
	while (iSend != iBufferSize)
	{
#ifdef WIN32
        int iTmpSend = send(m_client, buffer + iSend, iBufferSize - iSend, 0);
#elif (OS_IOS || OS_IOSSIMULATOR || OS_OSX)
         int iTmpSend = send(m_client, buffer + iSend, iBufferSize - iSend, SO_NOSIGPIPE);
#elif OS_ANDROID || OS_LINUX
        int iTmpSend = send(m_client, buffer + iSend, iBufferSize - iSend, MSG_NOSIGNAL);
#else
#error "nosupport"
#endif

		if (iTmpSend <= 0)
		{
			break;
		}
		iSend += iTmpSend;
	}
	if (iSend != iBufferSize)
	{
		return iSend;
	}
	return iBufferSize;
}

int CSyncTCP::GetRecvDataLen()
{
	//4字节长度
	CXSharedArray<char> recvBuffer;
	if (2 != RecvDataByLen(2, recvBuffer))
	{
		return -1;
	}
	unsigned short iDataLen = *(unsigned short*)recvBuffer.Get();
	return ntohs(iDataLen);
}

int CSyncTCP::RecvDataByLen(int iLen, CXSharedArray<char>& recvBuffer)
{
	int iRecv = 0;
	recvBuffer.Allocate(iLen);
	while (true)
	{
        int iCurRecv = (int)recv(m_client, recvBuffer.Get() + iRecv, iLen - iRecv, 0);       
		if (iCurRecv <= 0)
		{
			break;
		}

		iRecv += iCurRecv;
		if (iRecv == iLen)
		{
			break;
		}
	}
	return iRecv;
}
#define MAXRECVLEN 5*1024*1024
int CSyncTCP::RecvData(CXSharedArray<char>& recvBuffer)
{
	//先接收4字节的长度
	int iDataLen = GetRecvDataLen();
	if (-1 == iDataLen || iDataLen > MAXRECVLEN)
	{
		return -1;
	}
	return RecvDataByLen(iDataLen, recvBuffer);
}

int CSyncTCP::RawSocket()
{
	return m_client;
}

int CSyncTCP::GetIPType()
{
    return XNetworkUtil::DetectIsIPv6() ? AF_INET6 : AF_INET;
}
    


void CSyncTCP::SetSocketOpt()
{
	int r = 0;
#ifdef WIN32

#elif (OS_IOS || OS_IOSSIMULATOR || OS_OSX)
	setsockopt(m_client, SOL_SOCKET, SO_NOSIGPIPE, &r, sizeof(int));
#elif OS_ANDROID || OS_LINUX
	//setsockopt(m_client, SOL_SOCKET, MSG_NOSIGNAL, &r, sizeof(int));
//    int sendbuf = 0;
//    int len = sizeof(sendbuf);
//    getsockopt(m_client, SOL_SOCKET, SO_SNDBUF, &sendbuf, (socklen_t*)&len);
//    YouMe_LOG_Warning(__XT("get default tcp send buf size:%d "), sendbuf);
//    sendbuf = 524288;
//    setsockopt(m_client, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
//    sendbuf = 0;
//    getsockopt(m_client, SOL_SOCKET, SO_SNDBUF, &sendbuf, (socklen_t*)&len);
//    YouMe_LOG_Warning(__XT("set tcp send buf size:%d "), sendbuf);
    
//    int recvbuf = 1048576;
//    setsockopt(m_client, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
#else
#error "nosupport"
#endif

	//设置超时
#ifdef WIN32
	DWORD sendTimeOut = m_iSendTimeout * 1000;
	setsockopt(m_client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sendTimeOut, sizeof(sendTimeOut));
#else
	struct timeval t;
	t.tv_sec = m_iSendTimeout;
	t.tv_usec = 0;
	setsockopt(m_client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&t, sizeof(t));
#endif // WIN32    
}

void CSyncTCP::SetBlock(bool block)
{
#ifdef WIN32
	unsigned long nMode = block ? 0 : 1;
	int nRet = ioctlsocket(m_client, FIONBIO, &nMode);
	if (nRet != NO_ERROR)
	{
		YouMe_LOG_Error(__XT("ioctlsocket FIONBIO failed"));
	}
#else
	int flags = fcntl(m_client, F_GETFL, 0);
	if (block)
	{
		if (fcntl(m_client, F_SETFL, flags & ~O_NONBLOCK) == -1)
		{
			int mode = 0;
			if(ioctl(m_client, FIONBIO, &mode) == -1)
			{
				YouMe_LOG_Warning(__XT("ioctl FIONBIO failed"));
				return;
			}
		}
	}
	else
	{
		if (fcntl(m_client, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			int mode = 1;
			if (ioctl(m_client, FIONBIO, &mode) == -1)
			{
				YouMe_LOG_Warning(__XT("ioctl FIONBIO failed"));
				return;
			}
		}
	}
#endif
}

}

