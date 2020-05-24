#include "DNSUtil.h"
#include <YouMeCommon/Log.h>
#include <curl/inet_ntop.h>
#ifndef WIN32
#include <netdb.h>
#include <arpa/inet.h>
#endif


namespace youmecommon
{

	DNSUtil* DNSUtil::m_pInstace = NULL;

	DNSUtil::~DNSUtil()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (std::map<std::thread::id, youmecommon::CXCondWait*>::iterator itr = m_waitMap.begin(); itr != m_waitMap.end(); ++itr)
			{
				//itr->second->SetSignal();
				delete itr->second;
				itr->second = NULL;
			}
		}
	}

	DNSUtil* DNSUtil::Instance()
	{
		if (NULL == m_pInstace)
		{
			m_pInstace = new DNSUtil;
		}
		return m_pInstace;
	}

	bool DNSUtil::GetHostByNameAsync(const std::string& hostName, std::vector<std::string>& ips, int timeout)
	{
		if (hostName.empty())
		{
			return false;
		}

		std::thread t(&DNSUtil::GetIP, this, hostName);

		DNSInfo info;
		info.threadID = t.get_id();
		info.hostName = hostName;
		info.status = DNSPARSE_DOING;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_dnsinfoList.push_back(info);
			m_waitMap[info.threadID] = new youmecommon::CXCondWait;
		}

		bool bSuccess = false;
		while (true)
		{
			youmecommon::WaitResult waitRet = m_waitMap[info.threadID]->WaitTime(timeout);

			bool bDelete = false;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				std::vector<DNSInfo>::iterator itr = m_dnsinfoList.begin();
				for (; itr != m_dnsinfoList.end(); ++itr)
				{
					if (info.threadID == itr->threadID)
					{
						break;
					}
				}

				YouMe_LOG_Info(__XT("waitRet:%d status:%d host:%s"), waitRet, itr->status, UTF8TOXString(itr->hostName).c_str());

				if (itr != m_dnsinfoList.end())
				{
					if (youmecommon::WaitResult_Timeout == waitRet)
					{
						itr->status = DNSPARSE_TIMEOUT;
					}

					if (DNSPARSE_DOING == itr->status)
					{
						continue;
					}
					else if (DNSPARSE_SUCCESS == itr->status)
					{
						if (hostName == itr->hostName)
						{
							ips.insert(ips.end(), itr->result.begin(), itr->result.end());
							bDelete = true;
							bSuccess = true;
						}
						else
						{
							break;
						}
					}
					else if (DNSPARSE_SUCCESS == itr->status || DNSPARSE_CANCEL == itr->status || DNSPARSE_FAIL == itr->status)
					{
						bDelete = true;
						break;
					}
					bDelete = true;
				}

				if (bDelete)
				{
					std::map<std::thread::id, youmecommon::CXCondWait*>::iterator waitItr = m_waitMap.find(itr->threadID);
					if (waitItr != m_waitMap.end() && waitItr->second != NULL)
					{
						delete waitItr->second;
						m_waitMap.erase(waitItr);
					}

					m_dnsinfoList.erase(itr);
				}
			}

			break;
		}

		t.detach();

		return bSuccess;
	}

	bool DNSUtil::GetHostByName(const std::string& hostName, std::vector<std::string>& ipList)
	{
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		struct addrinfo *result = NULL;
		if (getaddrinfo(hostName.c_str(), NULL, &hints, &result) != 0)
		{
			return false;
		}

		struct addrinfo* pAddrInfo = NULL;
		for (pAddrInfo = result; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
		{
			switch (pAddrInfo->ai_family)
			{
			case AF_INET:
			{
				struct sockaddr_in* pSockaddrIpv4 = (struct sockaddr_in*) pAddrInfo->ai_addr;
				char szAddr[16];
				memset(szAddr, 0, 16);
				youmecommon::Curl_inet_ntop(AF_INET, &pSockaddrIpv4->sin_addr, szAddr, 16);
				if (strlen(szAddr) > 0)
				{
					ipList.push_back(std::string(szAddr));
				}
			}
			break;
			case AF_INET6:
			{
				char szAddr[64];
				memset(szAddr, 0, 64);
				struct sockaddr_in6* pSockaddrIpv6 = (struct sockaddr_in6*) pAddrInfo->ai_addr;
				youmecommon::Curl_inet_ntop(AF_INET6, &pSockaddrIpv6->sin6_addr, szAddr, 64);
				if (strlen(szAddr) > 0)
				{
					ipList.push_back(std::string(szAddr));
				}
			}
			break;
			default:
				break;
			}
		}

		freeaddrinfo(result);
		return true;
	}

	void DNSUtil::Cancel(const std::string& hostName)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (std::vector<DNSInfo>::iterator itr = m_dnsinfoList.begin(); itr != m_dnsinfoList.end(); ++itr)
		{
			if (hostName == itr->hostName)
			{
				itr->status = DNSPARSE_CANCEL;
				std::map<std::thread::id, youmecommon::CXCondWait*>::iterator waitItr = m_waitMap.find(itr->threadID);
				if (waitItr != m_waitMap.end() && waitItr->second != NULL)
				{
					waitItr->second->SetSignal();
				}
				break;
			}
		}
	}

	void DNSUtil::Cancel()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (std::vector<DNSInfo>::iterator itr = m_dnsinfoList.begin(); itr != m_dnsinfoList.end(); ++itr)
		{
			itr->status = DNSPARSE_CANCEL;
			std::map<std::thread::id, youmecommon::CXCondWait*>::iterator waitItr = m_waitMap.find(itr->threadID);
			if (waitItr != m_waitMap.end() && waitItr->second != NULL)
			{
				waitItr->second->SetSignal();
			}
		}
	}

	void DNSUtil::GetIP(const std::string& hostName)
	{
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		struct addrinfo *result = NULL;
		if (getaddrinfo(hostName.c_str(), NULL, &hints, &result) != 0)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (std::vector<DNSInfo>::iterator itr = m_dnsinfoList.begin(); itr != m_dnsinfoList.end(); ++itr)
			{
				if (hostName == itr->hostName)
				{
					itr->status = DNSPARSE_FAIL;
					std::map<std::thread::id, youmecommon::CXCondWait*>::iterator waitItr = m_waitMap.find(itr->threadID);
					if (waitItr != m_waitMap.end() && waitItr->second != NULL)
					{
						waitItr->second->SetSignal();
					}
					break;
				}
			}

			return;
		}

		std::vector<std::string> ipList;
		struct addrinfo* pAddrInfo = NULL;
		for (pAddrInfo = result; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
		{
			switch (pAddrInfo->ai_family)
			{
			case AF_INET:
			{
				struct sockaddr_in* pSockaddrIpv4 = (struct sockaddr_in*) pAddrInfo->ai_addr;
				char szAddr[16];
				memset(szAddr, 0, 16);
				youmecommon::Curl_inet_ntop(AF_INET, &pSockaddrIpv4->sin_addr, szAddr, 16);
				if (strlen(szAddr) > 0)
				{
					ipList.push_back(std::string(szAddr));
				}
			}
			break;
			case AF_INET6:
			{
				char szAddr[64];
				memset(szAddr, 0, 64);
				struct sockaddr_in6* pSockaddrIpv6 = (struct sockaddr_in6*) pAddrInfo->ai_addr;
				youmecommon::Curl_inet_ntop(AF_INET6, &pSockaddrIpv6->sin6_addr, szAddr, 64);
				if (strlen(szAddr) > 0)
				{
					ipList.push_back(std::string(szAddr));
				}
			}
			break;
			default:
				break;
			}
		}

		freeaddrinfo(result);

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (std::vector<DNSInfo>::iterator itr = m_dnsinfoList.begin(); itr != m_dnsinfoList.end(); ++itr)
			{
				if (hostName == itr->hostName)
				{
					itr->result = ipList;
					itr->status = ipList.size() > 0 ? DNSPARSE_SUCCESS : DNSPARSE_FAIL;

					std::map<std::thread::id, youmecommon::CXCondWait*>::iterator waitItr = m_waitMap.find(itr->threadID);
					if (waitItr != m_waitMap.end() && waitItr->second != NULL)
					{
						waitItr->second->SetSignal();
					}

					break;
				}
			}
		}
	}

}