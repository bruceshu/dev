#!/usr/bin/env bash

print_help_and_exit(){
    # print_info_in_red "${1}"
    echo "Usage: ${0} [android|ios]"
    echo "ios           - 编译iOS平台"
    echo "android       - 编译Android平台"
    exit 1
}

print_hello() {
	echo "hello bruce"
}

WORKSPACE=`pwd`/../
android=0
ios=0

#clean
rm -rf $WORKSPACE/Output
mkdir $WORKSPACE/Output

if [ $# -eq 0 ]; then
	echo "请指定编译平台"
	print_help_and_exit
fi

for var in $*
do
	if [ $var = "android" ]; then
		android=1
		echo "android"
	elif [ $var = "ios" ]; then
		ios=1
		echo "ios"
	else		
		print_help_and_exit
	fi
done

if [ $android -eq 1 ]; then
	#prepare
	chmod +x $WORKSPACE/build/cmake_build_Android.sh
	#chmod +x $WORKSPACE/build/cmake_build_OSX64.sh
	chmod +x $WORKSPACE/build/cmake_build_Android_v7.sh
	chmod +x $WORKSPACE/build/cmake_build_Android_v8a.sh
	chmod +x $WORKSPACE/build/cmake_build_Android_X86.sh
	chmod +x $WORKSPACE/build/cmake_build_Android_X86_64.sh

	#generate android project
	rm -rf $WORKSPACE/Android
	cd $WORKSPACE/build
	./cmake_build_Android.sh
	cd $WORKSPACE/Android
	#build android
	make -j8

	#generate android v7 project
	rm -rf $WORKSPACE/Android_V7
	cd $WORKSPACE/build
	./cmake_build_Android_v7.sh
	cd $WORKSPACE/Android_V7
	#build android v7
	make -j8

	#generate android v8 project
	rm -rf $WORKSPACE/Android_v8a
	cd $WORKSPACE/build
	./cmake_build_Android_v8a.sh
	cd $WORKSPACE/Android_v8a
	#build android v8
	make -j8

	#gernerate macOS project
	#rm -rf $WORKSPACE/OSX
	#cd $WORKSPACE/build
	#./cmake_build_OSX64.sh
	#cd $WORKSPACE/OSX

	
	rm -rf $WORKSPACE/Android_X86
	#build android x86
	cd $WORKSPACE/build/
	./cmake_build_Android_X86.sh
	cd $WORKSPACE/Android_X86
	make -j8
	
	rm -rf $WORKSPACE/Android_X86_64
	#build android x86_64
	cd $WORKSPACE/build/
	./cmake_build_Android_X86_64.sh
	cd $WORKSPACE/Android_X86_64
	make -j8

	cd $WORKSPACE/Output
	mkdir -p youme_common/lib/android/armeabi
	mkdir -p youme_common/lib/android/armeabi-v7a
	mkdir -p youme_common/lib/android/arm64-v8a
	mkdir -p youme_common/lib/android/x86
	mkdir -p youme_common/lib/android/x86_64

	cp $WORKSPACE/lib/Android/Release/libYouMeCommon.a youme_common/lib/android/armeabi
	cp $WORKSPACE/lib/Android_V7/Release/libYouMeCommon.a youme_common/lib/android/armeabi-v7a
	cp $WORKSPACE/lib/Android_V8/Release/libYouMeCommon.a youme_common/lib/android/arm64-v8a
	cp $WORKSPACE/lib/Android_X86/Release/libYouMeCommon.a youme_common/lib/android/x86
	cp $WORKSPACE/lib/Android_X86_64/Release/libYouMeCommon.a youme_common/lib/android/x86_64


#strip symboles
# $NDK_ROOT/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-strip \
# $WORKSPACE/lib/Android/Release/libYouMeCommon.a
	
fi

if [ $ios -eq 1 ]; then
	chmod +x $WORKSPACE/build/cmake_build_IOS64.sh
	chmod +x $WORKSPACE/build/cmake_build_IOS64Simulator.sh

	#gernerate iOS project
	rm -rf $WORKSPACE/IOS
	cd $WORKSPACE/build
	./cmake_build_IOS64.sh

	#rm -rf $WORKSPACE/IOS/IOS_Simulator
	rm -rf $WORKSPACE/IOS_Simulator
	cd $WORKSPACE/build
	./cmake_build_IOS64Simulator.sh

	#build ios
	cd $WORKSPACE/IOS
	xcodebuild clean -project $WORKSPACE/IOS/YouMeCommon.xcodeproj || echo "clean fail,maybe ok."
	xcodebuild -project YouMeCommon.xcodeproj -target YouMeCommon -configuration Release -sdk iphoneos \
	-arch armv7 -arch arm64

	#build ios simulator
	cd $WORKSPACE/IOS_Simulator
	xcodebuild clean -project YouMeCommon.xcodeproj || echo "clean fail,maybe ok."
	xcodebuild -project YouMeCommon.xcodeproj -target YouMeCommon -configuration Release \
	-sdk iphonesimulator -arch i386 -arch x86_64

	#build macosx
	#cd $WORKSPACE/OSX
	#xcodebuild clean -project YouMeCommon.xcodeproj || echo "clean fail,maybe ok."
	#xcodebuild -project YouMeCommon.xcodeproj -target YouMeCommon -configuration Release \
	#-sdk macosx -arch x86_64

	cd $WORKSPACE/Output
	mkdir -p youme_common/lib/ios

	## merge ios device and simulator lib
	lipo -create $WORKSPACE/lib/IOS/Release/libYouMeCommon.a $WORKSPACE/lib/IOSSIMULATOR/Release/libYouMeCommon.a \
	-output $WORKSPACE/Output/youme_common/lib/ios/libYouMeCommon.a
fi

	#macosx
	#cp $WORKSPACE/lib/MAC/Release/libYouMeCommon.a youme_common/lib/macos

	cd $WORKSPACE/Output
	mkdir -p youme_common/include/
	cp -r $WORKSPACE/src/YouMeCommon youme_common/include/
	zip -i '*.h' -i'*.hpp' -i '*.a' -i '*.so' -i '*.lib' -r youme_common_sdk.zip youme_common
