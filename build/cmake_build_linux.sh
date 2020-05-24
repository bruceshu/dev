#!/bin/bash
this_dir=`pwd`

#export PATH=$this_dir/cmake/linux/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games
#echo $PATH

function usage()
{
   echo "Usage: ./sh_file target_abi"
   echo "target_abi: x86 mips"
}

if [$# < 1]; then
	usage
	exit
fi

TARGET_ABI=$1
echo "-----------------------"
echo "target_abi: "$TARGET_ABI
echo "-----------------------"

if [ $TARGET_ABI = "mips" ]; then
	export LD_LIBRARY_PATH=/home/parallels/Software/mipsel-24kec-linux-uclibc-4.8-2017.03/usr/lib:$LD_LIBRARY_PATH
	export CC=/home/parallels/Software/mipsel-24kec-linux-uclibc-4.8-2017.03/usr/bin/mipsel-24kec-linux-uclibc-gcc
	export CXX=/home/parallels/Software/mipsel-24kec-linux-uclibc-4.8-2017.03/usr/bin/mipsel-24kec-linux-uclibc-g++
fi

cd ..
if [ ! -d "linux" ]; then
    mkdir linux
    echo "create build workspace"
fi

cd linux && rm -rf * && echo "delete old project"

cmake  -D__Linux__=1 -DYOUMECOMMON_ABI="$TARGET_ABI" ../ -G "Unix Makefiles"

make -j4 && echo "videoSDK build success"
