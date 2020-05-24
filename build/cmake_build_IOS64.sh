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
mkdir IOS
cd IOS

if [[ "$Mini" = "yes" ]]; then
cmake -D__IOS__=1 -DYOUME_MINI=1 -GXcode ..
else
cmake -D__IOS__=1 -DYOUME_MINI=0 -GXcode ..
fi