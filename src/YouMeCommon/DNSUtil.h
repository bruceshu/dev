#ifndef DNS_UTIL_H
#define DNS_UTIL_H


#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include "YouMeCommon/XCondWait.h"

namespace youmecommon
{
	enum DNSParseStatus
	{
		DNSPARSE_DOING,
		DNSPARSE_TIMEOUT,
		DNSPARSE_CANCEL,
		DNSPARSE_SUCCESS,
		DNSPARSE_FAIL
	};

	struct DNSInfo
	{
		std::thread::id threadID;
		std::string hostName;
		DNSParseStatus status;
		std::vector<std::string> result;
	};

	class DNSUtil
	{
	public:
		~DNSUtil();

		static DNSUtil* Instance();
		bool GetHostByName(const std::string& hostName, std::vector<std::string>& ipList);
		bool GetHostByNameAsync(const std::string& hostName, std::vector<std::string>& ips, int timeout = 10000);
		void Cancel(const std::string& hostName);
		void Cancel();

	private:
		DNSUtil(){};
		DNSUtil(const DNSUtil& dns){};

		void GetIP(const std::string& hostName);

		std::mutex m_mutex;
		std::vector<DNSInfo> m_dnsinfoList;
		std::map<std::thread::id, CXCondWait*> m_waitMap;
		static DNSUtil* m_pInstace;
	};
}

#endif
