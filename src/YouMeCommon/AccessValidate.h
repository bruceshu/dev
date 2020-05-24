#ifndef SDK_VALIDATE_H
#define SDK_VALIDATE_H

#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include <YouMeCommon/CrossPlatformDefine/IYouMeSystemProvider.h>
#include <YouMeCommon/profiledb.h>
#include <YouMeCommon/XCondWait.h>
#include <YouMeCommon/XAny.h>
#include <YouMeCommon/network/tcpclient.h>
#include <YouMeCommon/pb/youme_comm_conf.pb.h>
#include <thread>

namespace youmecommon
{

	enum SDKValidateErrorcode
	{
		SDKValidateErrorcode_Success=0,
		SDKValidateErrorcode_NotInit =1,
		SDKValidateErrorcode_ValidateIng =2,
		SDKValidateErrorcode_NetworkInvalid=3,
		SDKValidateErrorcode_ILLEGAL_SDK=4,
		SDKValidateErrorcode_Abort=5,
		SDKValidateErrorcode_ProtoError=6,
		SDKValidateErrorcode_ServerError=7,
		SDKValidateErrorcode_DNSParseError=8,
		SDKValidateErrorcode_InvalidAppkey = 9,		// appkey 无效，检查平台是否配置了相应的 appky
		SDKValidateErrorcode_InvalidFailed = 10,	// SDK 验证失败，检查前后台加解密使用的 key
		SDKValidateErrorcode_Timeout = 11,
		SDKValidateErrorcode_Fail=1000
	};

	enum ValidateReason
	{
		VALIDATEREASON_INIT,
		VALIDATEREASON_RECONNECT,
		VALIDATEREASON_UPDATE_CONFIG
	};

	class SDKValidateCallback
	{
	public:
		virtual void OnSDKValidteComplete(SDKValidateErrorcode errorcode, std::map<std::string, CXAny>& configurations, unsigned int constTime, ValidateReason reason, std::string& ip) = 0;
	};

	struct SDKValidateParam
	{
		int serviceID;
		YOUMEServiceProtocol::SERVICE_TYPE serviceType;
		std::string sdkName;
		int protocolVersion;
		std::string domain;
		std::string serverZone;
        std::vector<unsigned short> port;
		std::vector<std::string> defaultIP;

		SDKValidateParam() : serviceID(0), protocolVersion(0){}
	};

	class CRSAUtil;

	class AccessValidate : public MTcpEvent
	{
	public:
		AccessValidate(IYouMeSystemProvider* pSystemProvier, CProfileDB* pProfileDB, SDKValidateCallback* callback);
		~AccessValidate();
		SDKValidateErrorcode StartValidate(SDKValidateParam& validataParam, ValidateReason reason);
		void Disconnect();

	private:
		enum ValidateStatus
		{
			VSTATUS_SUCCESS,
			VSTATUS_TIMEOUT,
			VSTATUS_NETWORK_ERROR,
			VSTATUS_ERROR
		};

		bool InterParseSecret(const XString& strSecret,CRSAUtil& rsa);
		void EncryDecryptPacketBody(unsigned char* buffer, int bufferLen, unsigned char* key, int keyLen);
		void GenerateRandKey(unsigned char* key, int keyLen);
		SDKValidateErrorcode OnSDKValidateRsp(YOUMEServiceProtocol::CommConfRsp& rsp);
		void ValidateThread(const SDKValidateParam& validateParam, XUINT64 startTime);
		int StartConnect();
		void RequestValidateData();

		virtual void OnConnect() override;
		virtual void OnDisConnect(bool _isremote) override;
		virtual void OnError(int _status, int _errcode) override;
		virtual void OnWrote(int _id, unsigned int _len) override;
		virtual void OnAllWrote() override;
		virtual void OnRead() override;

		static XUINT64 m_nMsgSerial;
		CProfileDB* m_pProfileDB;
		IYouMeSystemProvider* m_pSystemProvider;

		std::string m_strDomain;
		int m_iServiceID;
		std::vector<unsigned short> m_portList;
		std::vector<std::string> m_ipList;
		short m_iCurrentIpIndex;
		short m_iCurrentPortIndex;
		TcpClient* m_pTcpClient;
		SDKValidateCallback* m_pCallback;
		std::thread m_validateThread;
		std::mutex m_validateThreadMutex;
		CXCondWait m_validateWait;
		bool m_bValidateRunning;
		ValidateStatus m_validateStatus;
		YOUMEServiceProtocol::CommConfReq m_requestData;
		std::map<std::string, CXAny> m_configurations;
		ValidateReason m_reason;
	};

}

#endif
