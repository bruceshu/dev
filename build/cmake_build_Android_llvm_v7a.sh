#!/usr/bin/env bash
this_dir=`pwd`

Mini="no"
for var in $*
do
    if [[ $var = "mini" ]]; then
        Mini="yes"
    fi
done

export PATH=$this_dir/cmake/mac/bin/CMake.app/Contents/bin:$PATH
echo $PATH
cd ..
mkdir Android
cd Android


if [[ "$Mini" = "yes" ]]; then
cmake -D__ANDROID__V7=1 -D__ANDROIDOS__=1 -D__ANDROID_LLVM__=1 -DYOUME_MINI=1 -DCMAKE_TOOLCHAIN_FILE=./build/toolchain/android.toolchain.cmake -DANDROID_NDK=$NDK_ROOT -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI="armeabi-v7a" -DANDROID_STL=c++_static ..
else
cmake -D__ANDROID__V7=1 -D__ANDROIDOS__=1 -D__ANDROID_LLVM__=1 -DYOUME_MINI=0 -DCMAKE_TOOLCHAIN_FILE=./build/toolchain/android.toolchain.cmake -DANDROID_NDK=$NDK_ROOT -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI="armeabi-v7a" -DANDROID_STL=c++_static ..
fi

