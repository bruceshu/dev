#
project(YouMeCommon)
INCLUDE_DIRECTORIES (${COMMON_INCLUDE} . .. ./mars/openssl/include ./opencore/opencore/codecs_v2/audio/gsm_amr/amr_nb/common/include ./opencore/oscl ./opencore/opencore/codecs_v2/audio/gsm_amr/amr_nb/dec/include ./opencore/opencore/codecs_v2/audio/gsm_amr/common/dec/include ./opencore/opencore/codecs_v2/audio/gsm_amr/amr_nb/enc/src ./opencore/opencore/codecs_v2/audio/gsm_amr/amr_nb/enc/include ./opencore/opencore/codecs_v2/audio/gsm_amr/amr_nb/dec/src ./opencore/opencore/codecs_v2/audio/gsm_amr/amr_wb/dec/include ./opencore/opencore/codecs_v2/audio/gsm_amr/amr_wb/dec/src ./opus-1.1/include ./opus-1.1/celt ./opus-1.1/silk/fixed ./opus-1.1/silk ./jpeg_turbo)
INCLUDE_DIRECTORIES(opencc/deps/darts-clone opencc/deps/rapidjson-0.11 opencc/deps/tclap-1.2.1)

IF(WIN32)
ADD_DEFINITIONS(-DCONFIG_WIN32_USE_CRYPTO_LIB -DUSE_ALLOCA -DFIXED_POINT -DCONFIG_PLATFORM_WIN32 -DCONFIG_OPENSSL_COMPATIBLE -DCONFIG_WIN32_USE_CRYPTO_LIB -DCURL_STATICLIB -DCURL_STATICLIB -D_XKEYCHECK_H -DUSE_AXTLS -DNOCRYPT -DOPENSSL_SYSNAME_WIN32 -DLPDIR_H -DCURL_STATICLIB -DHTTP_ONLY -DENABLE_IPV6 -DOPUS_BUILD -DHAVE_JPEG)

IF(WIN64)
ADD_DEFINITIONS(-DCRYPTOPP_DISABLE_ASM -DHAVE_JPEG)

ENDIF(WIN64)


ELSE(WIN32)
    IF(__ANDROIDOS__)
        ADD_DEFINITIONS(-DVAR_ARRAYS=1 -DHAVE_LRINTF=1 -DFIXED_POINT -DUSE_AXTLS -DCURL_STATICLIB -DCURL_STATICLIB -DCURL_STATICLIB -DCURL_HAVE_CONFIG_ -DHTTP_ONLY -DENABLE_IPV6 -DOPUS_BUILD)
        ADD_DEFINITIONS(-DCURL_DISABLE_CRYPTO_AUTH -DCURL_DISABLE_COOKIES -DCURL_DISABLE_PROXY -DCURL_DISABLE_TELNET -DCURL_DISABLE_LDAP)
    ELSE(__ANDROIDOS__)
        ADD_DEFINITIONS(-DUSE_ALLOCA -DFIXED_POINT -DUSE_AXTLS -DCURL_STATICLIB -DCURL_STATICLIB -DCURL_STATICLIB -DCURL_HAVE_CONFIG_ -DHTTP_ONLY -DENABLE_IPV6 -DUSE_ALLOCA -DOPUS_BUILD)
    ENDIF(__ANDROIDOS__)
ADD_DEFINITIONS(-DOPENSSL_SMALL_FOOTPRINT -DCRYPTOPP_DISABLE_ASM)
ENDIF(WIN32)

ADD_DEFINITIONS(-DSOUNDTOUCH_INTEGER_SAMPLES)

ADD_DEFINITIONS(-DSQLITE_ENABLE_COLUMN_METADATA -DSQLITE_TEMP_STORE=2 -DSQLITE_ENABLE_RTREE)

IF(__ANDROID_LLVM__)
ADD_DEFINITIONS(-DANDROID_LLVM)
ENDIF(__ANDROID_LLVM__)

FILE (GLOB header_files *.h *.hpp)
FILE (GLOB source_files *.c *.cc *.cpp)

LINK_DIRECTORIES(${COMMON_LIB})


FILE (GLOB CrossPlatformDefine CrossPlatformDefine/*.h CrossPlatformDefine/*.cpp CrossPlatformDefine/*.c CrossPlatformDefine/*.cc)
FILE (GLOB_RECURSE crypto crypto/*.h crypto/*.cpp crypto/*.c)
# IF(__ANDROIDOS__)

# ELSE(__ANDROIDOS__)
FILE (GLOB_RECURSE axTLS axTLS/*.h axTLS/*.cpp axTLS/*.c)
# ENDIF(__ANDROIDOS__)

FILE (GLOB_RECURSE google google/*.h google/*.cpp google/*.cc)
FILE (GLOB_RECURSE opencore opencore/*.h opencore/*.cpp opencore/*.cc)
FILE (GLOB_RECURSE wavresample wavresample/*.h wavresample/*.cpp wavresample/*.cc)
FILE (GLOB rscode rscode-1.3/*.h rscode-1.3/*.cpp rscode-1.3/*.c)
FILE (GLOB minizip minizip/*.h minizip/*.cpp minizip/*.c)
FILE (GLOB Sqlite3 Sqlite3/*.h Sqlite3/*.cpp Sqlite3/*.c)
FILE (GLOB tinyxml tinyxml/*.h tinyxml/*.cpp tinyxml/*.c)
FILE (GLOB zlib zlib/*.h zlib/*.cpp zlib/*.c)
FILE (GLOB yuvlib yuvlib/*.h yuvlib/*.cpp yuvlib/*.cc yuvlib/*.c)
FILE (GLOB pb pb/*.h pb/*.cpp pb/*.cc)
FILE (GLOB json json/*.h json/*.cpp json/*.cc)
FILE (GLOB_RECURSE curl curl/*.h curl/*.cpp curl/*.c)
FILE (GLOB trie trie/*.h trie/*.cpp trie/*.c)
FILE (GLOB soundTouch soundTouch/*.h soundTouch/*.cpp soundTouch/*.c)
FILE (GLOB network network/*.h network/*.cpp network/*.cc network/*.c)
FILE (GLOB networkUnix network/unix/*.h network/unix/*.cc network/unix/*.c)
FILE (GLOB networkAndroid network/android/*.h network/android/*.cc network/android/*.c)
FILE (GLOB networkWindow network/windows/*.h network/windows/*.cc network/windows/*.c)
FILE (GLOB opencc opencc/*.h opencc/*.hpp opencc/*.cpp)
FILE (GLOB kcp kcp/*.h kcp/*.hpp kcp/*.cpp)
FILE (GLOB jitterbuffer jitterbuffer/*.h jitterbuffer/*.hpp jitterbuffer/*.cpp)
FILE (GLOB rnnoise rnnoise/*.h rnnoise/*.cpp rnnoise/*.cc rnnoise/*.c rnnoise/models/*.c)

#FILE (GLOB_RECURSE lua lua-5.3.2/*.h lua-5.3.2/*.c lua-5.3.2/*.cpp)
#SOURCE_GROUP("lua" FILES ${lua})

#opus
FILE (GLOB opus_src opus-1.1/src/*.h opus-1.1/src/*.c opus-1.1/src/*.cpp)
SOURCE_GROUP("opus-1.1\\src" FILES ${opus_src})

FILE (GLOB opus_celt_arm opus-1.1/celt/arm/*.h opus-1.1/celt/arm/*.c opus-1.1/celt/arm/*.cpp)
SOURCE_GROUP("opus-1.1\\celt\\arm" FILES ${opus_celt_arm})

FILE (GLOB opus_celt opus-1.1/celt/*.h opus-1.1/celt/*.c opus-1.1/celt/*.cpp)
SOURCE_GROUP("opus-1.1\\celt" FILES ${opus_celt})

FILE (GLOB opus_silk_arm opus-1.1/silk/arm/*.h opus-1.1/silk/arm/*.c opus-1.1/silk/arm/*.cpp)
SOURCE_GROUP("opus-1.1\\silk\\arm" FILES ${opus_silk_arm})

FILE (GLOB opus_silk_fixed opus-1.1/silk/fixed/*.h opus-1.1/silk/fixed/*.c opus-1.1/silk/fixed/*.cpp)
SOURCE_GROUP("opus-1.1\\silk\\fixed" FILES ${opus_silk_fixed})

FILE (GLOB opus_silk opus-1.1/silk/*.h opus-1.1/silk/*.c opus-1.1/silk/*.cpp)
SOURCE_GROUP("opus-1.1\\silk" FILES ${opus_silk})

#下面是跨平台的部分

FILE (GLOB IOS CrossPlatformDefine/IOS/*.h CrossPlatformDefine/IOS/*.cpp CrossPlatformDefine/IOS/*.m CrossPlatformDefine/IOS/*.mm )
FILE (GLOB OSX CrossPlatformDefine/OSX/*.h CrossPlatformDefine/OSX/*.cpp CrossPlatformDefine/OSX/*.m CrossPlatformDefine/OSX/*.mm )
FILE (GLOB Windows CrossPlatformDefine/Windows/*.h CrossPlatformDefine/Windows/*.cpp CrossPlatformDefine/Windows/*.cc)


SOURCE_GROUP("CrossPlatformDefine" FILES ${CrossPlatformDefine})
SOURCE_GROUP("crypto" FILES ${crypto})

# IF(__ANDROIDOS__)

# ELSE(__ANDROIDOS__)
SOURCE_GROUP("axTLS" FILES ${axTLS})
# ENDIF(__ANDROIDOS__)

SOURCE_GROUP("google" FILES ${google})
SOURCE_GROUP("opencore" FILES ${opencore})
SOURCE_GROUP("wavresample" FILES ${wavresample})

SOURCE_GROUP("minizip" FILES ${minizip})
SOURCE_GROUP("rscode" FILES ${rscode})

SOURCE_GROUP("zlib" FILES ${zlib})
SOURCE_GROUP("yuvlib" FILES ${yuvlib})
SOURCE_GROUP("pb" FILES ${pb})
SOURCE_GROUP("json" FILES ${json})
SOURCE_GROUP("curl" FILES ${curl})
SOURCE_GROUP("tinyxml" FILES ${tinyxml})
SOURCE_GROUP("trie" FILES ${trie})
SOURCE_GROUP("soundTouch" FILES ${soundTouch})
SOURCE_GROUP("network" FILES ${network})
SOURCE_GROUP("network\\unix" FILES ${networkUnix})
SOURCE_GROUP("network\\android" FILES ${networkAndroid})
SOURCE_GROUP("network\\windwos" FILES ${networkWindow})
SOURCE_GROUP("opencc" FILES ${opencc})
#SOURCE_GROUP("kcp" FILES ${kcp})
SOURCE_GROUP("jitterbuffer" FILES ${jitterbuffer})
SOURCE_GROUP("rnnoise" FILES ${rnnoise})


#跨平台的子目录

SOURCE_GROUP("CrossPlatformDefine\\IOS" FILES ${IOS})
SOURCE_GROUP("CrossPlatformDefine\\OSX" FILES ${OSX})
SOURCE_GROUP("CrossPlatformDefine\\Windows" FILES ${Windows})

IF(YOUME_MINI)
	ADD_DEFINITIONS(-DYOUME_MINI=1)
ELSE(YOUME_MINI)
	ADD_DEFINITIONS(-DYOUME_MINI=0)
	FILE (GLOB cryptopp_files cryptopp/*.h cryptopp/*.cpp cryptopp/*c)
	SOURCE_GROUP("cryptopp" FILES ${cryptopp_files})
ENDIF(YOUME_MINI)
 
IF(__ANDROIDOS__)
	SOURCE_GROUP("Sqlite3" FILES ${Sqlite3})
	ADD_LIBRARY (YouMeCommon ${header_files}  ${soundTouch} ${cryptopp_files} ${rscode} ${wavresample} ${source_files} ${trie} ${opencore} ${json} ${CrossPlatformDefine} ${Android} ${crypto} ${google} ${minizip} ${yuvlib} ${Sqlite3} ${pb} ${curl} ${tinyxml} ${axTLS} ${network} ${networkUnix} ${networkAndroid} ${opus_celt_arm} ${opus_celt} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${opencc} ${kcp} ${jitterbuffer} ${rnnoise})
ELSEIF (__Linux__)
	ADD_LIBRARY (YouMeCommon ${header_files}  ${soundTouch} ${cryptopp_files} ${rscode} ${wavresample} ${source_files} ${trie} ${opencore} ${json} ${CrossPlatformDefine} ${Android}  ${crypto} ${google} ${minizip} ${yuvlib} ${Sqlite3} ${pb} ${curl} ${tinyxml} ${axTLS} ${network} ${networkUnix} ${opus_celt_arm} ${opus_celt} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${opencc} ${kcp} ${jitterbuffer} ${rnnoise})
ELSEIF (WIN32) 
	SOURCE_GROUP("Sqlite3" FILES ${Sqlite3})
	ADD_LIBRARY (YouMeCommon ${header_files} ${soundTouch} ${cryptopp_files} ${rscode} ${wavresample} ${source_files} ${trie} ${opencore} ${json} ${CrossPlatformDefine} ${Windows} ${zlib} ${yuvlib}    ${minizip} ${google} ${Sqlite3} ${crypto} ${pb} ${curl} ${tinyxml} ${axTLS} ${network} ${networkWindow} ${opus_celt_arm} ${opus_celt} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${opencc} ${kcp} ${jitterbuffer} ${rnnoise})
	
	#add_custom_target(YouMeCommon ALL
	#	COMMAND ${CMAKE_AR} rc libYouMeCommon.a $<TARGET_FILE:YouMeCommonTemp> libjpeg.a)

ELSEIF (__OSX__)	
	ADD_LIBRARY (YouMeCommon ${header_files} ${soundTouch} ${cryptopp_files} ${rscode} ${wavresample} ${source_files} ${trie} ${opencore} ${json} ${CrossPlatformDefine} ${OSX}  ${google} ${minizip} ${crypto} ${pb} ${curl} ${yuvlib} ${tinyxml} ${axTLS} ${network} ${networkUnix} ${opus_celt_arm} ${opus_celt} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${opencc} ${kcp} ${jitterbuffer} ${rnnoise})
	
ELSEIF(__IOS__)
	ADD_DEFINITIONS(-DLIBYUV_NEON)
	ADD_LIBRARY (YouMeCommon ${header_files} ${soundTouch} ${cryptopp_files} ${rscode} ${wavresample} ${source_files} ${trie} ${opencore} ${json} ${CrossPlatformDefine} ${IOS}  ${google} ${minizip} ${crypto} ${pb} ${curl} ${tinyxml} ${yuvlib} ${axTLS} ${network} ${networkUnix} ${opus_celt_arm} ${opus_celt} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${opencc} ${kcp} ${jitterbuffer} ${rnnoise})
	set_xcode_property(YouMeCommon CLANG_ENABLE_OBJC_ARC "YES")
	set_xcode_property(YouMeCommon ENABLE_BITCODE "YES")
	set_xcode_property(YouMeCommon IPHONEOS_DEPLOYMENT_TARGET "7.0")
ELSEIF(__IOSSIMULATOR__)
	ADD_LIBRARY (YouMeCommon ${header_files} ${soundTouch} ${rscode} ${cryptopp_files} ${wavresample} ${source_files} ${trie} ${opencore} ${json} ${CrossPlatformDefine} ${IOS}  ${google} ${minizip} ${crypto} ${pb} ${curl} ${tinyxml} ${yuvlib} ${axTLS} ${network} ${networkUnix} ${opus_celt_arm} ${opus_celt} ${opus_silk_arm} ${opus_silk_fixed} ${opus_silk} ${opus_src} ${opencc} ${kcp} ${jitterbuffer} ${rnnoise})
	set_xcode_property(YouMeCommon CLANG_ENABLE_OBJC_ARC "YES")
	set_xcode_property(YouMeCommon IPHONEOS_DEPLOYMENT_TARGET "7.0")
ENDIF(__ANDROIDOS__)





