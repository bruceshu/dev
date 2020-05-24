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
mkdir OSX
cd OSX

if [[ "$Mini" = "yes" ]]; then
cmake -D__OSX__=1 -DYOUME_MINI=1 .. -G"Xcode"
else
cmake -D__OSX__=1 -DYOUME_MINI=0 .. -G"Xcode"
fi

