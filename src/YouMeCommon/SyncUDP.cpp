#include "SyncUDP.h"
#include <assert.h>
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>

#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#else
#define  MSG_DONTWAIT 0
#include <iphlpapi.h>
#include <ws2tcpip.h>
#endif
#include <YouMeCommon/Log.h>
#include "XNetworkUtil.h"


namespace youmecommon {
    
//internet MTU
#define INTERNET_MTU 1460
CSyncUDP::CSyncUDP()
{
	m_client = -1;
}
CSyncUDP::~CSyncUDP()
{
	Close();
}

void CSyncUDP::Close()
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

bool CSyncUDP::Init(const XString&strServerIP, int iPort)
{
	if (-1 != m_client)
	{
		return true;
	}
    int ipType = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ipType == AF_INET6? AF_UNSPEC : AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    struct addrinfo * result = NULL;
	std::string strIP = XStringToUTF8(strServerIP);
    int iRet = getaddrinfo(strIP.c_str(), NULL, &hints, &result);
    
    if (iRet != 0)
    {
        return false;
    }
    
    
    struct addrinfo* pAddrInfo = NULL;
    
    
    for (pAddrInfo = result; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
    {
        if (pAddrInfo->ai_family == AF_INET)
        {
            ipType = GetIPType();
            
            if (AF_INET6 == ipType)
            {
                struct addrinfo * addrinfoResult = NULL;
                if (getaddrinfo(strIP.c_str(), "http", &hints, &addrinfoResult) != 0)
                {
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
                        return false;
                    }
                    
                    SetSocketOpt();
                    
                    if (res->ai_family == AF_INET)
                    {
                        m_serverIPType = AF_INET;
                        m_serverAddr = *((struct sockaddr_in *) res->ai_addr);
                        m_serverAddr.sin_port = htons(iPort);
                        break;
                    }
                    
                    else if (res->ai_family == AF_INET6)
                    {
                        m_serverIPType = AF_INET6;
                        m_serverAddr6 = *((struct sockaddr_in6 *) res->ai_addr);
                        m_serverAddr6.sin6_port = htons(iPort);
                        break;
                    }
                }
                freeaddrinfo(addrinfoResult);
            }
            
            else if (ipType == AF_INET)
            {
                m_serverIPType = AF_INET;
                m_client = socket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);
                SetSocketOpt();
                m_serverAddr = *((struct sockaddr_in *) pAddrInfo->ai_addr);
                m_serverAddr.sin_port = htons(iPort);
                break;
            }
            
            else
            {
                // Error
            }
        }
        
        else if ( pAddrInfo->ai_family == AF_INET6)
        {
            m_serverIPType = AF_INET6;
            m_client = socket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);
            SetSocketOpt();
            m_serverAddr6 = *((struct sockaddr_in6 *)pAddrInfo->ai_addr);
            m_serverAddr6.sin6_port = htons(iPort);
            break;
        }
        
        else
        {
            m_serverIPType = pAddrInfo->ai_family;
        }
    }
    
    freeaddrinfo(result);

	return true;
}

int CSyncUDP::SendData(const char* buffer, int iBufferSize)
{
 	 int ret = -1;
    
    if (m_serverIPType == AF_INET) { // ipv4
        ret = sendto(m_client, buffer, iBufferSize, 0, (const sockaddr *)&m_serverAddr, sizeof(struct sockaddr_in));
    }
    
    else if (m_serverIPType == AF_INET6) { // ipv6
        ret = sendto(m_client, buffer, iBufferSize, 0, (const sockaddr *)&m_serverAddr6, sizeof(struct sockaddr_in6));
    }
    
    else { // unknown
        ret = -1;
    }
    
    return ret;
}


void CSyncUDP::UnInit()
{
	Close();
}

//select 500ms 一次
int CSyncUDP::RecvData(CXSharedArray<char>& recvBuffer,int iTimeout,CXCondWait* pCondiWait)
{
	sockaddr_storage addr;
	socklen_t addrSize = sizeof(addr);
	recvBuffer.Allocate(INTERNET_MTU);
    int iExt = iTimeout % 500;
    int iMaxTryCount = iTimeout / 500;
    if (iExt != 0) {
        iMaxTryCount++;
    }
    if (0 == iMaxTryCount)
    {
        iMaxTryCount = 1;
    }
    int iCurTryCount = 0;
    fd_set  read_set;
    while (iCurTryCount < iMaxTryCount)
    {
        if (NULL != pCondiWait)
        {
            if (WaitResult_Timeout != pCondiWait->WaitTime(0))
            {
                //收到通知了，需要退出接受
                break;
            }
        }
        struct timeval  tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500*1000;
        FD_ZERO(&read_set);
        FD_SET(m_client, &read_set);
        //select 参数处理
        
        /*
         http://linux.die.net/man/2/select
         https://msdn.microsoft.com/en-us/library/windows/desktop/ms740141
         Return Value
         On success, select() and pselect() return the number of file descriptors contained in the three returned descriptor sets (that is, the total number of bits that are set in readfds, writefds, exceptfds) which may be zero if the timeout expires before anything interesting happens. On error, -1 is returned, and errno is set appropriately; the sets and timeout become undefined, so do not rely on their contents after an error.
         */
        int ret = select(m_client+1, &read_set, NULL, NULL, &tv);
        if (-1 == ret)
        {
            //socket 错误直接退出，可以通过关闭socket 来达到退出目的
            break;
        }
        if (0 == ret)
        {
            //超时了，continue一下
            iCurTryCount++;
            continue;
        }
        //其他情况表示已经触发了可用的条件
        if (FD_ISSET(m_client, &read_set)){
            return recvfrom(m_client, recvBuffer.Get(), INTERNET_MTU, MSG_DONTWAIT, (struct sockaddr *)&addr, &addrSize);
        }
    }
    return -1;
}

/**
* 判断ipv4还是ipv6
*/
int CSyncUDP::GetIPType()
{
    return XNetworkUtil::DetectIsIPv6() ? AF_INET6 : AF_INET;
}
/***
* 设置套接字选项
*/
void CSyncUDP::SetSocketOpt()
{
	int r = 0;

#if (OS_IOS || OS_IOSSIMULATOR || OS_OSX)
	setsockopt(m_client, SOL_SOCKET, SO_NOSIGPIPE, &r, sizeof(int));
#elif OS_ANDROID
	setsockopt(m_client, SOL_SOCKET, MSG_NOSIGNAL, &r, sizeof(int));
#endif
}
}
