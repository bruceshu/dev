#include "YouMeCommon/network/tcpclient.h"
#include <cstdlib>
#include <string>
#include "YouMeCommon/network/autobuffer.h"
#include "YouMeCommon/network/socket_address.h"
#include "YouMeCommon/Log.h"
#ifdef WIN32
#include <iphlpapi.h>
#endif

#ifdef OS_LINUX
#include <functional>
#endif


namespace youmecommon
{

#ifdef _WIN32
#define strdup _strdup
#endif

	TcpClient::TcpClient(const char* _ip, unsigned short _port, MTcpEvent& _event, int _timeout) : ip_(strdup(_ip)), port_(_port), event_(_event)
		, socket_(INVALID_SOCKET)
		, have_read_data_(false)
		, will_disconnect_(false)
		, writedbufid_(0)
		, timeout_(_timeout)
		, status_(kTcpInit)
	{
		if (!pipe_.IsCreateSuc()) status_ = kTcpInitErr;
	}

	TcpClient::~TcpClient()
	{
		DisconnectAndWait();

		for (std::list<AutoBuffer* >::iterator it = lst_buffer_.begin(); it != lst_buffer_.end(); ++it)
		{
			if (*it != NULL)
			{
				delete(*it);
			}
		}

		lst_buffer_.clear();
		if (ip_ != NULL)
		{
			free(ip_);
			ip_ = NULL;
		}
	}

	bool TcpClient::Connect()
	{
		std::lock_guard<std::mutex> lock(connect_mutex_);

		if (kTcpInit != status_)
		{
			////xassert2(kTcpInitErr == status_, "%d", status_);
			return false;
		}

		status_ = kSocketThreadStart;
		thread_ = std::thread(std::bind(&TcpClient::__RunThread, this));

		return true;
	}

	void TcpClient::Disconnect()
	{
		if (will_disconnect_)
		{
			return;
		}

		{
			std::lock_guard<std::mutex> lock(read_disconnect_mutex_);
			will_disconnect_ = true;
		}
		__SendBreak();
	}


	void TcpClient::DisconnectAndWait()
	{
		Disconnect();
		if (thread_.joinable())
		{
			thread_.join();
		}
	}

	bool TcpClient::HaveDataRead() const
	{
		if (kTcpConnected != status_)
		{
			return false;
		}

		return have_read_data_;
	}

	int TcpClient::Read(void* _buf, unsigned int _len)
	{
		if (kTcpConnected != status_)
		{
			return -1;
		}

		//xassert2(INVALID_SOCKET != socket_);
		std::lock_guard<std::mutex> lock(read_disconnect_mutex_);
		int ret = recv(socket_, (char*)_buf, _len, 0);

		have_read_data_ = false;
		__SendBreak();

		return ret;
	}

	bool TcpClient::HaveDataWrite() const
	{
		if (kTcpConnected != status_)
		{
			return false;
		}

		std::lock_guard<std::mutex> lock(write_mutex_);
		return !lst_buffer_.empty();
	}

	int TcpClient::Write(const void* _buf, unsigned int _len)
	{
		if (kTcpConnected != status_) return -1;

		AutoBuffer* tmpbuff = new AutoBuffer;
		tmpbuff->Write(0, _buf, _len);

		std::lock_guard<std::mutex> lock(write_mutex_);
		lst_buffer_.push_back(tmpbuff);
		writedbufid_++;
		__SendBreak();

		return writedbufid_;
	}

	int TcpClient::WritePostData(void* _buf, unsigned int _len) {
		if (kTcpConnected != status_) return -1;

		AutoBuffer* tmpbuff = new AutoBuffer;
		tmpbuff->Attach((void*)_buf, _len);

		std::lock_guard<std::mutex> lock(write_mutex_);
		lst_buffer_.push_back(tmpbuff);
		writedbufid_++;
		__SendBreak();
		return writedbufid_;
	}

	void TcpClient::__Run()
	{
		/*status_ = kTcpConnecting;

		struct sockaddr_in _addr;
		in_addr_t ip = ((sockaddr_in*)&socket_address(ip_, 0).address())->sin_addr.s_addr;

		if ((in_addr_t)(-1) == ip) {
			status_ = kTcpConnectIpErr;
			event_.OnError(status_, SOCKET_ERRNO(EADDRNOTAVAIL));
			return;
		}

		memset(&_addr, 0, sizeof(_addr));
		_addr = *(struct sockaddr_in*)(&socket_address(ip_, port_).address());

		socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (socket_ == INVALID_SOCKET) {
			status_ = kTcpConnectingErr;
			YouMe_LOG_Error(__XT("m_socket errno=%d"), socket_errno);
			event_.OnError(status_, socket_errno);
			return;
		}

//		if (::getNetInfo() == kWifi && socket_fix_tcp_mss(socket_) < 0) {
//#ifdef ANDROID
//			xinfo2(TSF"wifi set tcp mss error:%0", strerror(socket_errno));
//#endif
//		}

#ifdef _WIN32
		if (0 != socket_ipv6only(socket_, 0))
		{ 
			YouMe_LOG_Error(__XT("set ipv6only failed. error:%d"), socket_errno);
		}
#endif
		//xerror2_if(0 != socket_set_nobio(socket_), TSF"socket_set_nobio:%_, %_", socket_errno, socket_strerror(socket_errno));
		int ret = socket_set_nobio(socket_);
		if (ret != 0)
		{
			YouMe_LOG_Error(__XT("socket_set_nobio:%d, %d"), socket_errno, socket_strerror(socket_errno));
		}

		ret = ::connect(socket_, (sockaddr*)&_addr, sizeof(_addr));*/

		status_ = kTcpConnecting;

		in_addr_t ip = ((sockaddr_in*)&socket_address(ip_, 0).address())->sin_addr.s_addr;
		if ((in_addr_t)(-1) == ip) {
			status_ = kTcpConnectIpErr;
			event_.OnError(status_, SOCKET_ERRNO(EADDRNOTAVAIL));
			return;
		}

		socket_address _addr = socket_address(ip_, port_);
		int af = ((sockaddr*)&_addr)->sa_family;
		socket_ = socket(af, SOCK_STREAM, IPPROTO_TCP);
		if (socket_ == INVALID_SOCKET) {
			status_ = kTcpConnectingErr;
			YouMe_LOG_Error(__XT("m_socket errno=%d"), socket_errno);
			event_.OnError(status_, socket_errno);
			return;
		}

		//		if (::getNetInfo() == kWifi && socket_fix_tcp_mss(socket_) < 0) {
		//#ifdef ANDROID
		//			xinfo2(TSF"wifi set tcp mss error:%0", strerror(socket_errno));
		//#endif
		//		}

#ifdef _WIN32
		if (0 != socket_ipv6only(socket_, 0))
		{
			YouMe_LOG_Error(__XT("set ipv6only failed. error:%d"), socket_errno);
		}
#endif
		//xerror2_if(0 != socket_set_nobio(socket_), TSF"socket_set_nobio:%_, %_", socket_errno, socket_strerror(socket_errno));
		int ret = socket_set_nobio(socket_);
		if (ret != 0)
		{
			YouMe_LOG_Error(__XT("socket_set_nobio:%d, %d"), socket_errno, socket_strerror(socket_errno));
		}

		if (af == AF_INET)
		{
			YouMe_LOG_Info(__XT("ipv4"));

			int ipType = GetIPType();

			if (AF_INET6 == ipType)
			{
				struct addrinfo* addrinfoResult;
				struct addrinfo hints;
				memset(&hints, 0, sizeof(hints));
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				if (getaddrinfo(ip_, "http", &hints, &addrinfoResult) != 0)
				{
					YouMe_LOG_Debug(__XT("getaddrinfo failed"));
					return;
				}

				for (struct addrinfo* res = addrinfoResult; res; res = res->ai_next)
				{
					socket_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
					if (socket_ < 0)
					{
						freeaddrinfo(addrinfoResult);
						YouMe_LOG_Debug(__XT("getaddrinfo failed"));
						return;
					}
					ret = socket_set_nobio(socket_);
					if (ret != 0)
					{
						YouMe_LOG_Error(__XT("socket_set_nobio:%d, %d"), socket_errno, socket_strerror(socket_errno));
					}
					if (res->ai_family == AF_INET)
					{
						struct sockaddr_in addr_in = *((sockaddr_in*)res->ai_addr);
						addr_in.sin_port = htons(port_);
						ret = connect(socket_, (sockaddr*)&addr_in, sizeof(sockaddr_in));
						YouMe_LOG_Debug(__XT("connected  AF_INET: %d"), ret);
					}
					else if (res->ai_family == AF_INET6)
					{
						struct sockaddr_in6 addr_in6 = *((sockaddr_in6*)res->ai_addr);
						addr_in6.sin6_port = htons(port_);
						ret = connect(socket_, (sockaddr*)&addr_in6, sizeof(sockaddr_in6));
						YouMe_LOG_Debug(__XT("connected AF_INET6: %d"), ret);
					}
					else
					{
						ret = -2;
						YouMe_LOG_Debug(__XT("can not deal protocal"));
					}
					break;
				}
				freeaddrinfo(addrinfoResult);
			}
			else
			{
				sockaddr_in addr = *(struct sockaddr_in*)(&_addr.address());
				ret = ::connect(socket_, (sockaddr*)&addr, sizeof(addr));
			}
		}
		else if (af == AF_INET6)
		{
			YouMe_LOG_Info(__XT("ipv6"));

			sockaddr_in6 addr = *(struct sockaddr_in6*)(&_addr.address());
			ret = ::connect(socket_, (sockaddr*)&addr, sizeof(addr));
		}

		YouMe_LOG_Info(__XT("svr_ip:%s, svr_port:%d"), UTF8TOXString(std::string(ip_)).c_str(), port_);

		if (0 > ret && ret != -1 && !IS_NOBLOCK_CONNECT_ERRNO(socket_errno)) {
			YouMe_LOG_Error(__XT("connect errno=%d ret=%d"), socket_errno, ret);
			status_ = kTcpConnectingErr;
			event_.OnError(status_, socket_errno);
			return;
		}

		SocketSelect select_connect(pipe_, true);
		select_connect.PreSelect();
		select_connect.Exception_FD_SET(socket_);
		select_connect.Write_FD_SET(socket_);

		int selectRet = 0 < timeout_ ? select_connect.Select(timeout_) : select_connect.Select();

		if (0 == selectRet) {
			status_ = kTcpConnectTimeoutErr;
			event_.OnError(status_, SOCKET_ERRNO(ETIMEDOUT));
			return;
		}

		if (selectRet < 0) {
			YouMe_LOG_Error(__XT("select errno=%d"), socket_errno);
			status_ = kTcpConnectingErr;
			event_.OnError(status_, socket_errno);
			return;
		}

		if (select_connect.Exception_FD_ISSET(socket_)) {
			status_ = kTcpConnectingErr;
			event_.OnError(status_, socket_error(socket_));
			return;
		}

		if (select_connect.Write_FD_ISSET(socket_) && 0 != socket_error(socket_)) {
			status_ = kTcpConnectingErr;
			event_.OnError(status_, socket_error(socket_));
			return;
		}

		if (will_disconnect_) {
			status_ = kTcpDisConnected;
			event_.OnDisConnect(false);
			return;
		}

		status_ = kTcpConnected;
		event_.OnConnect();

		std::string local_ip = socket_address::getsockname(socket_).ip();
		unsigned int local_port = socket_address::getsockname(socket_).port();
		YouMe_LOG_Info(__XT("local_ip:%s, local_port:%d"), UTF8TOXString(local_ip).c_str(), local_port);

		local_ip = socket_address::getsockname(socket_).ip();

		int write_id = 0;

		while (true) {
			SocketSelect select_readwrite(pipe_, true);
			select_readwrite.PreSelect();
			select_readwrite.Exception_FD_SET(socket_);

			//tickcount_t round_tick(true);

			if (!have_read_data_) select_readwrite.Read_FD_SET(socket_);

			{
				std::lock_guard<std::mutex> lock(write_mutex_);

				if (!lst_buffer_.empty())select_readwrite.Write_FD_SET(socket_);
			}
			selectRet = select_readwrite.Select();

			if (0 == selectRet) {
				//xassert2(false);
				continue;
			}

			if (0 > selectRet) {
				YouMe_LOG_Error(__XT("select errno = %d"), socket_errno);
				status_ = kTcpIOErr;
				event_.OnError(status_, socket_errno);
				return;
			}

			if (will_disconnect_) {
				status_ = kTcpDisConnected;
				event_.OnDisConnect(false);
				return;
			}

			if (select_readwrite.Exception_FD_ISSET(socket_)) {
				int error_opt = 0;
				socklen_t error_len = sizeof(error_opt);
				if (0 == getsockopt(socket_, SOL_SOCKET, SO_ERROR, (char*)&error_opt, &error_len)){
					//xerror2("error_opt=%d", error_opt);
					YouMe_LOG_Error(__XT("error_opt=%d"), error_opt);
				}
				else{
					//xerror2("getsockopt error=%d", socket_errno);
					YouMe_LOG_Error(__XT("getsockopt error=%d"), socket_errno);
				}
				status_ = kTcpIOErr;
				event_.OnError(status_, error_opt);
				return;
			}

			if (select_readwrite.Read_FD_ISSET(socket_)) {
				//std::lock_guard<std::mutex> lock(read_disconnect_mutex_);
				std::unique_lock<std::mutex> lock(read_disconnect_mutex_);
				char buf_test;
				ret = (int)recv(socket_, &buf_test, 1, MSG_PEEK);

				if (0 < ret) {
					have_read_data_ = true;

					lock.unlock();
					event_.OnRead();
				}
				else if (0 == ret) {
					YouMe_LOG_Error(__XT("read error %d"), socket_errno);
					have_read_data_ = false;
					status_ = kTcpDisConnected;
					lock.unlock();
					event_.OnDisConnect(true);
					return;
				}
				else if (IS_NOBLOCK_RECV_ERRNO(socket_errno)) {
					//xwarn2(TSF"IS_NOBLOCK_RECV_ERRNO err:%_, %_", socket_errno, socket_strerror(socket_errno));
					YouMe_LOG_Warning(__XT("IS_NOBLOCK_RECV_ERRNO err:%d"), socket_errno);
				}
				else {
					YouMe_LOG_Error(__XT("read error %d"), socket_errno);
					status_ = kTcpIOErr;
					lock.unlock();
					event_.OnError(status_, socket_errno);
					return;
				}
			}

			if (select_readwrite.Write_FD_ISSET(socket_)) {
				std::unique_lock<std::mutex> lock(write_mutex_);
				AutoBuffer& buf = *lst_buffer_.front();
				size_t len = buf.Length();

				if (buf.Pos() < (off_t)len) {
					int send_len = (int)send(socket_, (char*)buf.PosPtr(), (size_t)(len - buf.Pos()), 0);

					if ((0 == send_len) || (0 > send_len && !IS_NOBLOCK_SEND_ERRNO(socket_errno))) {
						YouMe_LOG_Error(__XT("write error %d"), socket_errno);
						status_ = kTcpIOErr;
						lock.unlock();
						event_.OnError(status_, errno);
						return;
					}

					if (0 < send_len){
						buf.Seek(send_len, AutoBuffer::ESeekCur);
						double send_len_bytes = (double)send_len;
						//double cost_sec = ((double)round_tick.gettickspan())/1000;
						//xverbose2(TSF"debug:send_len:%_ bytes, cost_sec:%_ seconds", send_len_bytes, cost_sec);
						//if (cost_sec > 0.0 &&send_len_bytes/cost_sec < 20*1024) {
						//xwarn2(TSF"send speed slow:%_ bytes/sec, send_len:%_ bytes, send buf len:%_, cost_sec:%_ seconds", send_len_bytes/cost_sec, send_len_bytes, len-buf.Pos(), cost_sec);
						//}
					}
				}
				else {
					delete lst_buffer_.front();
					lst_buffer_.pop_front();

					lock.unlock();
					event_.OnWrote(++write_id, (unsigned int)len);

					lock.lock();

					if (lst_buffer_.empty()) {
						lock.unlock();
						event_.OnAllWrote();
					}
				}
			}
		}
	}

	void TcpClient::__RunThread()
	{
		YouMe_LOG_Debug(__XT("enter"));

		__Run();

		if (INVALID_SOCKET != socket_)
		{
			socket_close(socket_);
			socket_ = INVALID_SOCKET;
		}
		will_disconnect_ = false;

		YouMe_LOG_Debug(__XT("leave"));
	}

	void TcpClient::__SendBreak()
	{
		pipe_.Break();
	}

	int TcpClient::GetIPType()
	{
#ifdef WIN32
		FIXED_INFO* pFixedInfo = (FIXED_INFO*)malloc(sizeof(FIXED_INFO));
		if (pFixedInfo == NULL)
		{
			return AF_INET;
		}
		ULONG ulOutBufLen = sizeof(FIXED_INFO);
		if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pFixedInfo);
			pFixedInfo = (FIXED_INFO *)malloc(ulOutBufLen);
			if (pFixedInfo == NULL)
			{
				return AF_INET;
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
			return pHostent->h_addrtype;
		}
		return AF_INET;
#else
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
			return AF_INET;
		}
		int iFamily = result->ai_family;
		freeaddrinfo(result);
		return iFamily;
#endif // WIN32
	}
}
