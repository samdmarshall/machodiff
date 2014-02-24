#!/bin/sh

echo "${ACTION}"

if [ "${ACTION}" = "clean" ]; then
	echo "Cleaning Libraries..."
	rm -fd "${BUILD_DIR}/Universal/*"
	rm -fd ${CONFIGURATION_BUILD_DIR}/libcapstone-64.a
	rm -fd ${CONFIGURATION_BUILD_DIR}/libcapstone-32.a
	cd ./${PROJECT_NAME}
	make clean
fi

if [ "${ACTION}" = "build" ] || [ "${ACTION}" = "" ]; then
	cd ./${PROJECT_NAME}

	mkdir -p ${CONFIGURATION_BUILD_DIR}

	if [ ! -f ${CONFIGURATION_BUILD_DIR}/libcapstone-64.a ]; then
		echo "Building capstone for x86_64"
		./make.sh
		mv libcapstone.a ${CONFIGURATION_BUILD_DIR}/libcapstone-64.a
		make clean
	fi

	if [ ! -f ${CONFIGURATION_BUILD_DIR}/libcapstone-32.a ]; then
		echo "Building capstone for i386"
		./make.sh nix32
		mv libcapstone.a ${CONFIGURATION_BUILD_DIR}/libcapstone-32.a
		make clean
	fi

	echo "Creating fat binary"
	cd ${BUILD_DIR}/Universal
	lipo -create ${CONFIGURATION_BUILD_DIR}/libcapstone-64.a ${CONFIGURATION_BUILD_DIR}/libcapstone-32.a -o libcapstone.a

	echo "Complete"
fi