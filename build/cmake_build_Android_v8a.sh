#!/usr/bin/env bash
this_dir=`pwd`

Mini="no"
for var in $*
do
    if [[ $var = "mini" ]]; then
        Mini="yes"
    fi
done


cd ..
mkdir Android_v8a
cd Android_v8a


if [[ "$Mini" = "yes" ]]; then
cmake -D__ANDROIDOS__=1 -D__ANDROID__V8=1 -DYOUME_MINI=1 -DANDROID_NDK=$NDK_ROOT -DCMAKE_BUILD_TYPE=Release  -DANDROID_ABI="arm64-v8a" ..
else
cmake -D__ANDROIDOS__=1 -D__ANDROID__V8=1 -DYOUME_MINI=0 -DANDROID_NDK=$NDK_ROOT -DCMAKE_BUILD_TYPE=Release  -DANDROID_ABI="arm64-v8a" ..
fi

