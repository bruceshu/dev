#pragma once
#include "YouMeCommon/XSemaphore.h"
#include "YouMeCommon/XCondWait.h"
#include <thread>
#include <memory>
#include <list>
#include <map>

namespace youmecommon
{

	class ITimeOutCallback
	{
	public:
		virtual void OnTimeOut(const void* pParam, int iTimerID) = 0;
	};
	
	struct TimerParam
	{
		uint64_t ulTimeout;
		youmecommon::CXCondWait conWait;
		std::thread waitThrad;
		ITimeOutCallback* pCallback;
		void* pParam;
		//从汇编来看加上volatile，编译器不会再使用寄存器来保存这个变量，每次需要存取的时候都会读一次内存，极大的减少重入的概率
		volatile bool bCancle;
		bool bOnce;
		TimerParam()
		{
			pCallback = NULL;
			pParam = NULL;
			bCancle = false;
			ulTimeout = 0;
			bOnce = false;
		}
	};


	class CXTimer
	{
	public:
		static CXTimer* GetInstance();
		CXTimer();
		~CXTimer();

		//启动一个主线成，用来回调
		bool Init();
		void UnInit();

		//返回一个定时器ID
		int PostTimer(uint64_t ulTimeout, ITimeOutCallback* callback, void* pParam, bool bOnceTimer = false);
		bool CancleTimer(long iTimerID);
	private:

		static void WaitThread(void * pParam);
		std::mutex m_mapMutex;
		bool m_bExit;
		long m_iTimerID;
		std::map<int, std::shared_ptr<TimerParam> > m_timerMap;
	};


}