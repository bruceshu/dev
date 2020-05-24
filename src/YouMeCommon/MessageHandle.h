#pragma once
#include <memory>
#include <list>
#include <mutex>
#include <YouMeCommon/XSemaphore.h>
#include <thread>
namespace youmecommon
{

	class IMessageRunable
	{
	public:
		//必须提供一个，否则没法正常析构
		virtual ~IMessageRunable()
		{

		}
		//如果run 耗时的话，需要注意会不会影响其他的任务，而且也可能会影响到 CMessageHandle 的反初始化
		virtual void Run() = 0;
	};

	//初衷是因为kcp 不支持跨线程调用，所以将kcp 的所有调用都放到一个线程，试用messagehandle 的方式将所有操作post 到同一个线程调用。
	//这样解决了多线程的问题。 
	//每一个CMessageHandle 会创建一个自己的线程，所有通过同一个CMessageHandle对象 送入的runable 都会在同一个线程调用
	class CMessageHandle
	{
	public:
		CMessageHandle();
		virtual ~CMessageHandle();

		//投送一个runalbe
		void Post(const std::shared_ptr<IMessageRunable>& runable);

		//初始化
		int Init();

		//反初始化，释放
		void UnInit();
	private:
		void HandleProc();

        std::list<std::shared_ptr<IMessageRunable> >m_tasks;
        std::mutex m_mutex;
        CXSemaphore m_semaphore;
        std::thread m_executeThread;
        bool m_bInit = false;
	};

}
