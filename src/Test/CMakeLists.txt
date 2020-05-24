project (mytest)

ADD_DEFINITIONS(-DCURL_STATICLIB -DCARES_STATICLIB -DWIN32_LEAN_AND_MEAN)
INCLUDE_DIRECTORIES (${COMMON_INCLUDE} .. "..\\common" .)

FILE (GLOB_RECURSE header_files *.h)
FILE (GLOB_RECURSE source_files *.c *.cc *.cpp)

LINK_DIRECTORIES(${COMMON_LIB})

ADD_EXECUTABLE (mytest ${header_files} ${source_files})



IF(WIN32)
	TARGET_LINK_LIBRARIES (mytest  wldap32.lib  Version.lib Setupapi.lib ws2_32 YouMeCommon Winmm.lib)
ELSEIF (__ANDROID__) 
	TARGET_LINK_LIBRARIES (mytest  z)
ELSE(WIN32)

	TARGET_LINK_LIBRARIES (mytest YouMeCommon   z )
ENDIF(WIN32)