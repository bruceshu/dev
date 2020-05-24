#pragma once

#ifdef WIN32
#include <Windows.h>
typedef HANDLE SEMAPHORE_T;
#elif defined OS_OSX
#include <dispatch/dispatch.h>
typedef dispatch_semaphore_t SEMAPHORE_T;
#else
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include <limits.h>
#include <errno.h>

#	include <fcntl.h> /* O_CREAT */
#	include <sys/stat.h> /* S_IRUSR, S_IWUSR*/
typedef sem_t* SEMAPHORE_T;


#	include <fcntl.h> /* O_CREAT */
#	include <sys/stat.h> /* S_IRUSR, S_IWUSR*/
#endif // WIN32

#ifndef NAME_MAX
#define NAME_MAX 255

#endif

namespace youmecommon {

class CXSemaphore
{
public:
	CXSemaphore(int iInitValue=0);
	~CXSemaphore();

	bool Increment();
	bool Decrement();

	SEMAPHORE_T m_handle;
    //在头文件里，通过宏控制类的结果很不安全啊。生成库和链接库的时候可能使用不同的宏定义
//#if (defined OS_OSX) || (defined OS_IOS) || (defined OS_IOSSIMULATOR)
    char m_semName[NAME_MAX+1];
    
//#endif
};
}

