#include "Log.h"
#include <YouMeCommon/TimeUtil.h>
#ifdef WIN32
#include <tchar.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#endif
#include <time.h>
#define TSK_LOG_BUFSIZE 1024
#include <assert.h>
#include <YouMeCommon/XFile.h>
#include <YouMeCommon/TimeUtil.h>
#include <YouMeCommon/XSharedArray.h>
#if OS_ANDROID
#include <android/log.h>
#elif OS_LINUX
#include <stdarg.h>
#endif

#include <mutex>
youmecommon::CXFile g_YouMeLogFile;
std::mutex* g_YouMeLogFileMutex = new std::mutex;

YouMe_LOG_LEVEL g_YouMeLogLevel= LOG_LEVEL_DEBUG;
YouMe_LOG_LEVEL g_YouMeLogLevelConsle = LOG_LEVEL_DEBUG;

XString g_strLogPathBak;
XString g_strLogPath;
static uint64_t gMaxLogFileSize = 20*1024*1024; // 20MB by default

const XCHAR *LogLevelToString(int dLevel)
{
	switch (dLevel)
	{
	case LOG_LEVEL_ERROR:
		return __XT("ERROR");

	case LOG_LEVEL_WARNING:
		return __XT("WARNING");

	case LOG_LEVEL_FATAL:
		return __XT("FATAL");

	case LOG_LEVEL_DEBUG:
		return __XT("DEBUG");

	case LOG_LEVEL_INFO:
		return __XT("INFO");


	default:
		return __XT("UNDEFINED");
	}
}
bool YouMe_LOG_Init(const XString& strLogPath)
{
    g_strLogPath = strLogPath;
    std::lock_guard<std::mutex> lock(*g_YouMeLogFileMutex);
    if (g_YouMeLogFile.IsOpen()) {
        g_YouMeLogFile.Close();
    }
    g_YouMeLogFile.LoadFile(strLogPath, youmecommon::CXFile::Mode_Open_ALWAYS);
    bool bSuccess = g_YouMeLogFile.IsOpen();
    if ( bSuccess)
    {
        g_YouMeLogFile.Seek(0, SEEK_END);
    }
    return bSuccess;
}

bool YouMe_LOG_Init_new(const XString& strLogPath, const XString& strLogPathBak, int maxLogSize)
{
    g_strLogPath = strLogPath;
    g_strLogPathBak = strLogPathBak;
    gMaxLogFileSize = maxLogSize;
    
    std::lock_guard<std::mutex> lock(*g_YouMeLogFileMutex);
    if (g_YouMeLogFile.IsOpen()) {
        g_YouMeLogFile.Close();
    }
    g_YouMeLogFile.LoadFile(strLogPath, youmecommon::CXFile::Mode_Open_ALWAYS);
    bool bSuccess = g_YouMeLogFile.IsOpen();
    if ( bSuccess)
    {
        g_YouMeLogFile.Seek(0, SEEK_END);
    }
    
    return bSuccess;
}

void YouMe_Log_Uninit()
{
    std::lock_guard<std::mutex> lock(*g_YouMeLogFileMutex);
    g_YouMeLogFile.Close();
}
void YouMe_LOG_SetLevel(YouMe_LOG_LEVEL level, YouMe_LOG_LEVEL consoleLevel)
{
    g_YouMeLogLevel = level;
    g_YouMeLogLevelConsle = consoleLevel;
}

#if OS_ANDROID
int LogLevelToAndroidLevel (int dLevel)
{
    switch (dLevel)
    {
        case LOG_LEVEL_ERROR:
            return ANDROID_LOG_ERROR;
            
        case LOG_LEVEL_WARNING:
            return ANDROID_LOG_WARN;
            
        case LOG_LEVEL_FATAL:
            return ANDROID_LOG_FATAL;
            
        case LOG_LEVEL_DEBUG:
            return ANDROID_LOG_DEBUG;
            
        case LOG_LEVEL_INFO:
            return ANDROID_LOG_INFO;
            
            
        default:
            return ANDROID_LOG_INFO;
    }
}
#endif
void YouMe_LOG_imp(const XCHAR *pszFun, const XCHAR *pszFile, int dLine, int dLevel, const XCHAR *pszFormat, ...)
{
    bool writeToFile = true;
    bool writeToConsole = true;
    
    if (dLevel > g_YouMeLogLevel)
    {
        writeToFile = false;
    }
    
    if (dLevel > g_YouMeLogLevelConsle)
    {
        writeToConsole = false;
    }
    
    if (!writeToConsole && !writeToFile)
    {
        return;
    }
    
	time_t timep = time(NULL);
	struct tm p_tm;
	youmecommon::CTimeUtil::GetLocalTime(&timep, &p_tm);

	XCHAR buffer[TSK_LOG_BUFSIZE + 1] = { 0 };
#ifdef WIN32
	int dSize = _snwprintf(buffer, TSK_LOG_BUFSIZE, __XT("%04d/%02d/%02d %02d:%02d:%02d.%03d threadid:%d %s "), p_tm.tm_year + 1900,
		p_tm.tm_mon+1,
		p_tm.tm_mday,
		p_tm.tm_hour,
		p_tm.tm_min,
		p_tm.tm_sec,
		(int)(youmecommon::CTimeUtil::GetTimeOfDay_MS() % 1000),
		GetCurrentThreadId(),
		LogLevelToString(dLevel));
#else
    int dSize = snprintf(buffer, TSK_LOG_BUFSIZE, __XT("%04d/%02d/%02d %02d:%02d:%02d.%03d threadid:%lu  %s: "),  p_tm.tm_year+1900,
                         p_tm.tm_mon+1,
                         p_tm.tm_mday,
                         p_tm.tm_hour,
                         p_tm.tm_min,
                         p_tm.tm_sec,
						 (int)(youmecommon::CTimeUtil::GetTimeOfDay_MS() % 1000),
                         (unsigned long)pthread_self(),
                         LogLevelToString(dLevel));


    
#endif // WIN32

	if (dSize < TSK_LOG_BUFSIZE)
	{
		va_list args;
		va_start(args, pszFormat);
#ifdef WIN32

		dSize += _vsnwprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, pszFormat, args);
#else
        dSize += vsnprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, pszFormat, args);
#endif // WIN32
		va_end(args);
	}

	
	if (dSize < TSK_LOG_BUFSIZE)
	{
#ifdef WIN32
		_snwprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, __XT("[%s#%s:%d]\n"),youmecommon::CXFile::GetFileName(pszFile).c_str(), pszFun, dLine);
#else
		snprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, __XT("[%s#%s:%d]\n"),  youmecommon::CXFile::GetFileName(pszFile).c_str(), pszFun,dLine);
#endif
		
	}
	
    if (writeToConsole)
    {
    
#ifdef WIN32
	OutputDebugStringW(buffer);
#else
	printf("%s",buffer);
#ifdef OS_ANDROID
    int iAndroidLogLevel = LogLevelToAndroidLevel (dLevel);
    __android_log_write (iAndroidLogLevel, "YOUMEIM", buffer);
#endif
#endif
    }
	

    std::lock_guard<std::mutex> lock(*g_YouMeLogFileMutex);
	if (g_YouMeLogFile.IsOpen() && writeToFile)
	{
        if( g_YouMeLogFile.GetFileSize() >= gMaxLogFileSize )
        {
            g_YouMeLogFile.Close();
            
            if( !g_strLogPathBak.empty() )
            {
                youmecommon::CXFile::remove_file( g_strLogPathBak );
                youmecommon::CXFile::rename_file( g_strLogPath , g_strLogPathBak);
            }
            else{
                youmecommon::CXFile::remove_file( g_strLogPath );
            }
            
            g_YouMeLogFile.LoadFile(g_strLogPath, youmecommon::CXFile::Mode_Open_ALWAYS);
            bool bSuccess = g_YouMeLogFile.IsOpen();
            if ( bSuccess)
            {
                g_YouMeLogFile.Seek(0, SEEK_END);
            }
        }

#ifdef WIN32
		std::string strUTF8Log = Unicode_to_UTF8(buffer, XStrLen(buffer));
#else
		std::string strUTF8Log = buffer;
#endif

		/*
		//2 字节长度(包含密钥长度,不包含本身2字节)， 4字节密钥  数据内容
		int iPasswd = time(NULL) + 10101;
		youmecommon::CXSharedArray<byte> pLogErtBuffer(2 + 4 + strUTF8Log.length());
		//写入数据长度
		unsigned short netorderDataLen = htons(strUTF8Log.length());
		memcpy(pLogErtBuffer.Get(), &netorderDataLen, 2);
		//写入密码
		int netorderPasswdLen = htonl(iPasswd);
		memcpy(pLogErtBuffer.Get() + 2, &netorderPasswdLen, 4);
		byte* pEncrtyPtr = pLogErtBuffer.Get() + 6;//偏移到6 字节处
		//开始循环加密数据，异或
		int j = 0;
		for (int i = 0; i < strUTF8Log.length(); i++)
		{
			pEncrtyPtr[i] = strUTF8Log.at(i) ^ ((byte*)&iPasswd)[j % 4];
			j++;
		}

		g_logFile.Write(pLogErtBuffer.Get(), pLogErtBuffer.GetBufferLen());*/
		g_YouMeLogFile.Write((const byte*)strUTF8Log.c_str(), strUTF8Log.length());
		g_YouMeLogFile.Flush();
	}
}

void YouMe_LOG_imp_utf8(const XCHAR *pszFun, const XCHAR *pszFile, int dLine, int dLevel, const char *pszFormat, ...)
{
	bool writeToFile = true;
	bool writeToConsole = true;

	if (dLevel > g_YouMeLogLevel)
	{
		writeToFile = false;
	}

	if (dLevel > g_YouMeLogLevelConsle)
	{
		writeToConsole = false;
	}

	if (!writeToConsole && !writeToFile)
	{
		return;
	}

	time_t timep = time(NULL);
	struct tm p_tm;
	youmecommon::CTimeUtil::GetLocalTime(&timep, &p_tm);

	XStringStream ss;
	XCHAR buffer[TSK_LOG_BUFSIZE + 1] = { 0 };
#ifdef WIN32
	int dSize = _snwprintf(buffer, TSK_LOG_BUFSIZE, __XT("%04d/%02d/%02d %02d:%02d:%02d.%03d threadid:%d %s "), p_tm.tm_year + 1900,
		p_tm.tm_mon + 1,
		p_tm.tm_mday,
		p_tm.tm_hour,
		p_tm.tm_min,
		p_tm.tm_sec,
		(int)(youmecommon::CTimeUtil::GetTimeOfDay_MS() % 1000),
		GetCurrentThreadId(),
		LogLevelToString(dLevel));
#else
	int dSize = snprintf(buffer, TSK_LOG_BUFSIZE, __XT("%04d/%02d/%02d %02d:%02d:%02d.%03d threadid:%lu  %s: "),  p_tm.tm_year+1900,
		p_tm.tm_mon+1,
		p_tm.tm_mday,
		p_tm.tm_hour,
		p_tm.tm_min,
		p_tm.tm_sec,
		(int)(youmecommon::CTimeUtil::GetTimeOfDay_MS() % 1000),
		(unsigned long)pthread_self(),
		LogLevelToString(dLevel));

#endif // WIN32
	ss << buffer;


	char buffer2[TSK_LOG_BUFSIZE + 1] = { 0 };
	va_list args;
	va_start(args, pszFormat);
	int size = vsnprintf(buffer2, TSK_LOG_BUFSIZE, pszFormat, args);
	va_end(args);

	XString strTmp = UTF8TOXString2(buffer2);

	ss << strTmp;

	if (dSize < TSK_LOG_BUFSIZE)
	{
#ifdef WIN32
		_snwprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, __XT("[%s#%s:%d]\n"), youmecommon::CXFile::GetFileName(pszFile).c_str(), pszFun, dLine);
#else
		snprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, __XT("[%s#%s:%d]\n"), youmecommon::CXFile::GetFileName(pszFile).c_str(), pszFun, dLine);
#endif

		ss << buffer + dSize;
	}

	XString strLog = ss.str();
	if (writeToConsole)
	{

#ifdef WIN32
		OutputDebugStringW(strLog.c_str() );
#else
		printf("%s",strLog.c_str());
#ifdef OS_ANDROID
		int iAndroidLogLevel = LogLevelToAndroidLevel (dLevel);
		__android_log_write (iAndroidLogLevel, "YOUMEIM", strLog.c_str());

#endif
#endif
	}


    std::lock_guard<std::mutex> lock(*g_YouMeLogFileMutex);
	if (g_YouMeLogFile.IsOpen() && writeToFile)
	{
        if( g_YouMeLogFile.GetFileSize() >= gMaxLogFileSize )
        {
            g_YouMeLogFile.Close();
            
            if( !g_strLogPathBak.empty() )
            {
                youmecommon::CXFile::remove_file( g_strLogPathBak );
                youmecommon::CXFile::rename_file( g_strLogPath , g_strLogPathBak);
            }
            else{
                 youmecommon::CXFile::remove_file( g_strLogPath );
            }
            
            g_YouMeLogFile.LoadFile(g_strLogPath, youmecommon::CXFile::Mode_Open_ALWAYS);
            bool bSuccess = g_YouMeLogFile.IsOpen();
            if ( bSuccess)
            {
                g_YouMeLogFile.Seek(0, SEEK_END);
            }
        }


#ifdef WIN32
		std::string strUTF8Log = Unicode_to_UTF8( strLog.c_str(), strLog.length() );
#else
		std::string strUTF8Log = strLog;
#endif

		g_YouMeLogFile.Write((const byte*)strUTF8Log.c_str(), strUTF8Log.length());

		g_YouMeLogFile.Flush();
	}
}
