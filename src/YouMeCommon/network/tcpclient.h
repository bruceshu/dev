#ifndef __TcpClient__
#define __TcpClient__


#include <list>
#include <thread>
#include <mutex>
#ifdef WIN32
#include <winsock2.h>
#include "YouMeCommon/network/windows/socketselect2.h"
#else
#include "YouMeCommon/network/unix/socketselect.h"
#endif

namespace youmecommon
{

	class MTcpEvent {
	public:
		virtual ~MTcpEvent() {}

		virtual void OnConnect() = 0;
		virtual void OnDisConnect(bool _isremote) = 0;
		virtual void OnError(int _status, int _errcode) = 0;

		virtual void OnWrote(int _id, unsigned int _len) = 0;
		virtual void OnAllWrote() = 0;

		virtual void OnRead() = 0;
	};

	class AutoBuffer;

	class TcpClient {
	public:
		enum TTcpStatus {
			kTcpInit = 0,
			kTcpInitErr,
			kSocketThreadStart,
			kSocketThreadStartErr,
			kTcpConnecting,
			kTcpConnectIpErr,
			kTcpConnectingErr,
			kTcpConnectTimeoutErr,
			kTcpConnected,
			kTcpIOErr,
			kTcpDisConnectedbyRemote,
			kTcpDisConnected,
		};

	public:
		TcpClient(const char* _ip, unsigned short _port, MTcpEvent& _event, int _timeout = 6 * 1000);
		~TcpClient();

	public:
		bool Connect();
		void Disconnect();
		void DisconnectAndWait();

		bool HaveDataRead() const;
		int Read(void* _buf, unsigned int _len);

		bool HaveDataWrite() const;
		int Write(const void* _buf, unsigned int _len);
		int WritePostData(void* _buf, unsigned int _len);

		const char* GetIP() const { return ip_; }
		unsigned short GetPort() const { return port_; }

		TTcpStatus GetTcpStatus() const { return status_; }

		int GetIPType();

	private:
		void __Run();
		void __RunThread();
		void __SendBreak();

	private:
		char* ip_;
		unsigned short port_;
		MTcpEvent& event_;

		SOCKET socket_;
		bool have_read_data_;
		bool will_disconnect_;
		int writedbufid_;
		std::list<AutoBuffer*> lst_buffer_;

		std::thread thread_;
		mutable std::mutex write_mutex_;
		mutable std::mutex read_disconnect_mutex_;
		std::mutex connect_mutex_;

		SocketBreaker pipe_;

		int timeout_;
		volatile TTcpStatus status_;
	};

}

#endif
