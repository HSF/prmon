#! /usr/bin/env bash
#
# Wrapper script to build prmon and run all tests
#
# Note that CXX, CC and CMAKE can all be customised so that
# they can be matched to the development environment in each
# container; CMAKE_EXTRA can be used to pass special options
# to cmake for particular platforms
cd /tmp
echo "Starting build and test for platform $PLATFORM, compiler suite $COMPILER"
if [ -z "$CXX" ]; then
    if [ "$COMPILER" == "clang" ]; then
        CXX=$(type -p clang++)
    else
        # gcc suite is the fall through
        CXX=$(type -p g++)
    fi
else
    echo "CXX was set externally to $CXX"
fi

if [ -z "$CC" ]; then
    if [ "$COMPILER" == "clang" ]; then
        CC=$(type -p clang)
    else
        # gcc suite is the fall through
        CC=$(type -p gcc)
    fi
else
    echo "CC was set externally to $CC"
fi

if [ -z "$CMAKE" ]; then
    CMAKE=$(type -p cmake)
fi

# For debugging, handy to print what we're doing
cmd="$CMAKE -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -DBUILD_GTESTS=ON $CMAKE_EXTRA /mnt"
echo $cmd
$cmd
make -j 4
make test
