#include "XFile.h"
#include <assert.h>
#include <algorithm>
#ifdef WIN32
#include <tchar.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#endif // WIN32

#include <iostream>
using namespace std;
#include <string>


namespace youmecommon {

CXFile::CXFile(void):m_pFile(NULL)
{

}

CXFile::~CXFile(void)
{
	Close();	
}

int CXFile::LoadFile( const XString& strPath,Mode mode)
{
	assert(NULL == m_pFile);
	XString strMode;
	if (mode == Mode_CREATE_ALWAYS)
	{
		strMode = __XT("wb+");
	}
	else if (mode == Mode_Open_ALWAYS)
	{
#ifdef WIN32
		int iCode = _waccess(strPath.c_str(), 0);
#else
		int iCode = access(strPath.c_str(), 0);
#endif
		if (0 == iCode)
		{
			strMode = __XT("rb+");
		}
		else
		{
			strMode = __XT("wb+");
		}
	}
	else if(mode == Mode_OpenExist)
	{
		strMode = __XT("rb+");
	}
    else if(mode == Mode_OpenExist_ReadOnly)
    {
        strMode = __XT("rb");
    }
	else
	{
		assert(false);
	}
	#ifdef WIN32
	 m_pFile = _wfopen(strPath.c_str(),strMode.c_str());

	#else	
		m_pFile = fopen(strPath.c_str(),strMode.c_str());
	#endif
	if (NULL == m_pFile)
	{
		return -1;
	}
	if ((mode == Mode_Open_ALWAYS) 
		|| (mode == Mode_OpenExist))
	{
		//移动文件指针到头部
		XFSeek( m_pFile, 0, SEEK_SET);
	}

	return 0;
}

XINT64 CXFile::GetFileSize()
{
	if (NULL == m_pFile)
	{
		return -1;
	}
	//保存当前文件指针
	XINT64 iCurPos = XFTell(m_pFile);
	//移动到文件末尾
	XFSeek(m_pFile,0,SEEK_END);
	XINT64 iFileSize = XFTell(m_pFile);
	XFSeek(m_pFile,iCurPos,SEEK_SET);
	return iFileSize;
}

XINT64 CXFile::GetFilePosition()
{
	return XFTell(m_pFile);
}

XINT64 CXFile::SetFilePositon(XINT64 filePos)
{
	return XFSeek(m_pFile,filePos,SEEK_SET);
}

XINT64 CXFile::Read(byte* buffer,XINT64 wantRead)
{
	return fread(buffer,1,(size_t)wantRead,m_pFile);
}

XINT64 CXFile::Write(const byte* buffer,XINT64 wantWrite)
{
	return fwrite(buffer, 1, (size_t)wantWrite, m_pFile);
}

XINT64 CXFile::SetFileSize(XINT64 fileSize)
{
#ifdef WIN32
	_chsize_s(_fileno(m_pFile),fileSize);
	//返回当前文件的大小
	return GetFileSize();
#else
	ftruncate(fileno(m_pFile),fileSize);
	assert(false);
	return -1;
#endif
	
}


void CXFile::Close()
{
	if (NULL != m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

void CXFile::Skip(XINT64 wantSkipInBytes)
{
	SetFilePositon(GetFilePosition()+wantSkipInBytes);
}

bool CXFile::IsEnd()
{
	return (bool)feof(m_pFile);
}

XString CXFile::GetFileName(const XCHAR* file_path)
{
	if (NULL == file_path || '\0' == file_path[0])
	{
		return XString(__XT(""));
	}

#if defined(WIN32)
	const  XCHAR* pos1 = wcsrchr(file_path, '\\');
	const XCHAR* pos2 = wcsrchr(file_path, '/');
	const    XCHAR* pos = (std::max)(pos1, pos2);
#else
	const XCHAR* pos = strrchr(file_path, '/');
#endif

	if (NULL == pos)
	{
		return (XString)file_path;
	}

	return (XString)(pos + 1);
}

bool CXFile::IsOpen()
{
	return m_pFile != NULL;
}

XINT64 CXFile::Seek(XINT64 ulOffset, int iFlag)
{
	return XFSeek(m_pFile, ulOffset, iFlag);
}

void CXFile::Flush()
{
	fflush(m_pFile);
}

XString CXFile::CombinePath(const XString& strDir, const XString& strFileName)
{
	XString dir = strDir;
	if (dir.size() == 0)
	{
		return strFileName;
	}
	XCHAR last_char = dir[dir.size() - 1];
	if (last_char == '\\' || last_char == '/')
	{
		// 末尾就是 '\\'，处理之前先去掉末尾的 '\\'
		dir.resize(dir.size() - 1);
	}
	dir += XPreferredSeparator;
	dir += strFileName;
	return dir;
}

bool CXFile::IsFileExist(const XString&strFilePath)
{

#ifdef WIN32
	WIN32_FIND_DATAW data;
	HANDLE handle = ::FindFirstFileW(strFilePath.c_str(), &data);
	if (handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	else
	{
		bool ret = (0 == (data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY));
		::FindClose(handle);
		return ret;
	}

#elif (defined OS_OSX)
	struct stat64 buf;
	int res = stat64(strFilePath.c_str(), &buf);
	if (-1 == res)
	{
		return false;
	}
	return S_ISREG(buf.st_mode);
#elif (defined OS_IOS) || (defined OS_IOSSIMULATOR)
	struct stat buf;
	int res = stat(strFilePath.c_str(), &buf);
	if (-1 == res)
	{
		return false;
	}

	return S_ISREG(buf.st_mode);

#elif (defined OS_ANDROID) || (defined OS_LINUX)
	struct stat64 buf;
	int res = stat64(strFilePath.c_str(), &buf);
	if (-1 == res)
	{
		return false;
	}

	return S_ISREG(buf.st_mode);
#else
#error "不支持的平台"
#endif

}

XString CXFile::get_upper_dir(const XCHAR* dir_path)
{
	if (NULL == dir_path || '\0' == dir_path[0])
	{
		return __XT("");
	}

#if defined(WIN32)
	XString dir = dir_path;
	XCHAR last_char = dir[dir.size() - 1];
	if (last_char == '\\' || last_char == '/')
	{
		// 末尾就是 '\\'，处理之前先去掉末尾的 '\\'
		dir.resize(dir.size() - 1);
	}
	XString::size_type pos1 = dir.rfind('\\');
	XString::size_type pos2 = dir.rfind('/');
	XString::size_type pos;
	if (pos1 != XString::npos && pos2 != XString::npos)
	{
		pos = max(pos1, pos2);
	}
	else
	{
		pos = (pos1 != XString::npos ? pos1 : pos2);
	}
#else
	XCHAR sperator = '/';

	XString dir = dir_path;
	if (dir[dir.size() - 1] == sperator)
	{
		// 末尾就是 '\\'，处理之前先去掉末尾的 '\\'
		dir.resize(dir.size() - 1);
	}

	XString::size_type pos = dir.rfind(sperator);
#endif

	if (XString::npos == pos)
	{
		// 字符串中不包含 '\\'
		return __XT("");
	}
	else
	{
		return dir.substr(0, pos + 1);
	}
}

bool CXFile::is_dir(const XCHAR* path)
{
	if (NULL == path || '\0' == path[0])
	{
		return false;
	}
    
#ifdef WIN32
    
    DWORD dwAttr = ::GetFileAttributesW(path);
    return (dwAttr != -1) && ((dwAttr&FILE_ATTRIBUTE_DIRECTORY) != 0);
#elif (defined OS_OSX)
    struct stat64 buf;
    int res = stat64(path, &buf);
    if (-1 == res)
    {
        return false;
    }
    return S_ISDIR(buf.st_mode);
#elif (defined OS_IOS) || (defined OS_IOSSIMULATOR)
    struct stat buf;
    int res = stat(path, &buf);
    if (-1 == res)
    {
        return false;
    }
    
    return S_ISDIR(buf.st_mode);
    
#elif (defined OS_ANDROID) || (defined OS_LINUX)
    struct stat64 buf;
    int res = stat64(path, &buf);
    if (-1 == res)
    {
        return false;
    }
    
    return S_ISDIR(buf.st_mode);
#else
#error "不支持的平台"
#endif
    
    
}

#ifdef WIN32

bool CXFile::make_dir(const XCHAR* path)
{
	if (NULL == path || '\0' == path[0])
	{
		return false;
	}

	return TRUE == CreateDirectoryW(path, NULL);
}



#else
bool   CXFile::make_dir(const XCHAR* path)
{
	if(NULL == path || '\0' == path[0])
	{
		return false;
	}

    if (is_dir(path)) {
        return true;
    }

	return 0 == mkdir(path, 0777);//妈的，8进制的，不是10 进制

}
#endif // WIN32

bool CXFile::make_dir_tree(const XCHAR* path)
{
	if (is_dir(path))
	{
		// 如果已经存在，则返回TRUE
		return true;
	}
	bool ret = make_dir(path);
	if (ret == false)
	{
		XString upper_dir = get_upper_dir(path);

		if (!upper_dir.empty())
		{
			bool bCreateUpperDir = make_dir_tree(upper_dir.c_str());
			if (bCreateUpperDir)
			{
				return make_dir(path);
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

bool CXFile::delete_dir(const XCHAR* dir_path, bool deleteSelf)
{
	if (NULL == dir_path || '\0' == dir_path[0] || !is_dir(dir_path))
	{
		return false;
	}

#if defined(WIN32)
	std::wstring strDirectory(dir_path);
	XCHAR cEnd = strDirectory[strDirectory.length() - 1];
	if (cEnd != L'\\' && cEnd != L'/')
	{
		strDirectory += L"\\";
	}

	XCHAR szFind[MAX_PATH] = { 0 };
	swprintf(szFind, MAX_PATH, L"%s*.*", strDirectory.c_str());

	WIN32_FIND_DATAW fd;
	HANDLE hFile = ::FindFirstFileW(szFind, &fd);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wcscmp(fd.cFileName, __XT(".")) == 0
				|| wcscmp(fd.cFileName, __XT("..")) == 0)
			{
				continue;
			}
			else if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				XCHAR szFoder[MAX_PATH] = { 0 };
				swprintf(szFoder, MAX_PATH, __XT("%s%s"), strDirectory.c_str(), fd.cFileName);
				delete_dir(szFoder, true);
				::RemoveDirectoryW(szFoder);
			}
			else if (wcscmp(fd.cFileName, __XT("Thumbs.db")) == 0)
			{
				XCHAR szFile[MAX_PATH] = { 0 };
				swprintf(szFile, MAX_PATH, __XT("%sThumbs.db"), strDirectory.c_str());
				::DeleteFileW(szFile);
			}
			else
			{
				XCHAR szFileName[MAX_PATH];
				swprintf(szFileName, MAX_PATH, __XT("%s%s"), strDirectory.c_str(), fd.cFileName);
				::SetFileAttributesW(szFileName, FILE_ATTRIBUTE_NORMAL);
				::DeleteFileW(szFileName);
			}
		} while (::FindNextFileW(hFile, &fd));

		::FindClose(hFile);

		if (deleteSelf)
		{
			::RemoveDirectoryW(dir_path);
			return !is_dir(dir_path);
		}
		else
		{
			return true;
		}
	}
	else
	{
		return false;
	}

#else
	DIR* dirp = opendir(dir_path);    
	if(!dirp)
	{
		return false;
	}
	struct dirent* dir;
	struct stat st;
	while ((dir = readdir(dirp)) != NULL)
	{
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
		{
			continue;
		}
        std::string subPath = dir_path;
        subPath.append("/");
        subPath.append(dir->d_name);
		if (lstat(subPath.c_str(), &st) == -1)
		{
			continue;
		}
		if (S_ISDIR(st.st_mode))
		{
			if (!delete_dir(subPath.c_str(), true))
			{
				closedir(dirp);
				return false;
			}
			rmdir(subPath.c_str());
		}
		else if (S_ISREG(st.st_mode))
		{
			unlink(subPath.c_str());
		}
		else
		{
			continue;
		}
	}
	if (deleteSelf)
	{
		if (rmdir(dir_path) == -1)
		{
			closedir(dirp);
			return false;
		}
	}
	closedir(dirp);
	return true;
#endif
}

XString CXFile::GetFileExt(const XString& strFilePath)
{
	XString strExtension;
	XString::size_type stPos = strFilePath.find_last_of(__XT("."));
	if (stPos != XString::npos)
	{
		strExtension = strFilePath.substr(stPos + 1);
	}
	return strExtension;
}

void CXFile::remove_file(const XString& strFilePath)
{
#ifdef WIN32
	::DeleteFileW(strFilePath.c_str());
#else
	remove(strFilePath.c_str());
#endif
}


bool CXFile::rename_file(const XString& strSrcPath, const XString& strDestPath)
{
#ifdef WIN32
	return (bool)::MoveFileW(strSrcPath.c_str(),strDestPath.c_str());
#else
	return rename(strSrcPath.c_str(),strDestPath.c_str())==0;
#endif
}
 
bool CXFile::getPathContents( const XString& strPath , std::vector<XString>&  vecAllFiles,  bool needDir)
{
    const std::string folderPath = XStringToUTF8( strPath );
#ifdef WIN32
    _finddata_t FileInfo;
    string strfind = folderPath + "\\*";
    long Handle = _findfirst(strfind.c_str(), &FileInfo);
    
    if (Handle == -1L)
    {
        return false;
    }
    do{
        string filename = FileInfo.name;
        //判断是否有子目录
        if (FileInfo.attrib & _A_SUBDIR)
        {
            if( needDir )
            {
                 if( (strcmp(FileInfo.name,".") == 0 ) &&(strcmp(FileInfo.name,"..") == 0))
                    continue;
                
                vecAllFiles.push_back( UTF8TOXString(filename) );
            }
        }
        else
        {
            if( !needDir )
            {
                vecAllFiles.push_back( UTF8TOXString(filename) );
            }
        }
    }while (_findnext(Handle, &FileInfo) == 0);
    
    _findclose(Handle);
#else
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    if((dp = opendir(folderPath.c_str())) == NULL) {
        return false ;
    }
    chdir(folderPath.c_str());
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);
        string filename = entry->d_name;
        if(S_ISDIR(statbuf.st_mode)) {
            if( needDir )
            {
                if(strcmp(".", filename.c_str() ) == 0 ||
                   strcmp("..", filename.c_str() ) == 0 )
                    continue;
                
                vecAllFiles.push_back( UTF8TOXString(filename) );
            }
        } else {
            if( !needDir )
            {
                vecAllFiles.push_back( UTF8TOXString(filename) );
            }


        }
    }
    chdir("..");
    closedir(dp);
#endif
    
    return true;
        
}

}
