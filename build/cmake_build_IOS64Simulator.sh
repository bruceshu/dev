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
mkdir IOS_Simulator
cd IOS_Simulator

if [[ "$Mini" = "yes" ]]; then
cmake -D__IOSSIMULATOR__=1 -DIOS_PLATFORM=SIMULATOR -DYOUME_MINI=1 -GXcode ..
else
cmake -D__IOSSIMULATOR__=1 -DIOS_PLATFORM=SIMULATOR -DYOUME_MINI=0 -GXcode ..
fi