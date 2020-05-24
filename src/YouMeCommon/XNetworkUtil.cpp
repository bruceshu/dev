#include "XNetworkUtil.h"

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
#ifdef ANDROID
#include <network/android/ifaddrs.h>
#else
#include <ifaddrs.h>
#endif
#else
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

namespace youmecommon {

#ifndef WIN32
int get_ip_address_ios_android()
{
    struct ifaddrs *interfaces;
    char addrBuf4[INET_ADDRSTRLEN];
    char addrBuf6[INET6_ADDRSTRLEN];
    
    int type = 4;
    const char *addr_head = "fe80";
    
    if(!getifaddrs(&interfaces)) {
        // Loop through linked list of interfaces
        struct ifaddrs *interface;
        for(interface=interfaces; interface; interface=interface->ifa_next) {
            if(!(interface->ifa_flags & IFF_UP) || (interface->ifa_flags & IFF_LOOPBACK) ) {
                continue; // deeply nested code harder to read
            }
            
            const struct sockaddr_in *addr = (const struct sockaddr_in*)interface->ifa_addr;
            
            if(addr && (addr->sin_family==AF_INET || addr->sin_family==AF_INET6)) {
                //NSString *name = [NSString stringWithUTF8String:interface->ifa_name];
                //NSString *type;
                if(addr->sin_family == AF_INET) {
                    if(inet_ntop(AF_INET, &addr->sin_addr, addrBuf4, INET_ADDRSTRLEN)) {
                        //YouMe_LOG_Warning("get_ip_address_ios ipv444 [%s]", addrBuf4);
                        type = 4;
                        continue;
                    }
                } else {
                    const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6*)interface->ifa_addr;
                    if(inet_ntop(AF_INET6, &addr6->sin6_addr, addrBuf6, INET6_ADDRSTRLEN)) {
                        if (!strncmp(addrBuf6, addr_head, strlen(addr_head)))
                        {
                            //ms_report("get_ip_address_ios test [%s]", addrBuf6);
                            continue;
                        }
                        
                        YouMe_LOG_Info("get_ip_address_ios ipv6 [%s]", addrBuf6);
                        type = 6;
                        break;
                    }
                }
            }
        }
        // Free memory
        freeifaddrs(interfaces);
    }
    
    return type;
}
#endif
    
bool XNetworkUtil::DetectIsIPv6()
{
#ifdef WIN32
    FIXED_INFO* pFixedInfo = (FIXED_INFO*)malloc(sizeof(FIXED_INFO));
    if (pFixedInfo == NULL)
    {
        return false;
    }
    ULONG ulOutBufLen = sizeof(FIXED_INFO);
    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        free(pFixedInfo);
        pFixedInfo = (FIXED_INFO *)malloc(ulOutBufLen);
        if (pFixedInfo == NULL)
        {
            return false;
        }
    }
    std::string strDNSAddr;
    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR)
    {
        strDNSAddr = pFixedInfo->DnsServerList.IpAddress.String;
    }
    if (pFixedInfo)
    {
        free(pFixedInfo);
    }
    
    struct hostent* pHostent = gethostbyname(strDNSAddr.c_str());
    if (pHostent != NULL)
    {
        return pHostent->h_addrtype == AF_INET6;
    }
    return false;
#else
    //return get_ip_address_ios_android() == 6 ;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo *result = NULL;
    
    int iRet = getaddrinfo("localhost", NULL, &hints, &result);
    if (iRet != 0)
    {
        if (nullptr != result) {
            freeaddrinfo(result);
        }
        return false;
    }
    
    int iFamily = result->ai_family;
    freeaddrinfo(result);
    return iFamily == AF_INET6;
#endif // WIN32
}

}
