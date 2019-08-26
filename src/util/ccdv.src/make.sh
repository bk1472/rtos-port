#!/bin/bash
PROGRAM_NAME="ccdv"

clean_build ()
{
	rm -rf build
	if [ "$1" != "all" ]; then
		mkdir build
	fi
	rm -rf ${PROGRAM_NAME}.dir
}

build ()
{
	pushd build > /dev/null 2>&1
	if [ "$1" == "cmake" ]; then
		cmake ../
	fi
	make
	popd > /dev/null 2>&1
}

new_build()
{
	clean_build
	build "cmake"
}

just_build()
{
	if [ -f build/Makefile ]; then
		build
	else
		new_build
	fi
}

clean()
{
	pushd build > /dev/null 2>&1
	make clean
	popd > /dev/null 2>&1
}

clobber()
{
	clean
	clean_build "all"
}


if   [ "$1" == "new"   ]; then      # whole build
	new_build
elif [ "$1" == "clean"   ]; then    # clean
	clean
elif [ "$1" == "clobber" ]; then    # clean all
	clobber
else                                # just build
	just_build
fi
