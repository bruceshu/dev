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
mkdir Android
cd Android


if [[ "$Mini" = "yes" ]]; then
cmake -D__ANDROIDOS__=1 -D__ANDROID__=1 -DYOUME_MINI=1 -DANDROID_NDK=$NDK_ROOT -DCMAKE_BUILD_TYPE=Release  -DANDROID_ABI="armeabi" ..
else
cmake -D__ANDROIDOS__=1 -D__ANDROID__=1 -DYOUME_MINI=0 -DANDROID_NDK=$NDK_ROOT -DCMAKE_BUILD_TYPE=Release  -DANDROID_ABI="armeabi" ..
fi

