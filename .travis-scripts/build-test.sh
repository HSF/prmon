#! /usr/bin/env bash
#
# Wrapper script to build prmon on travis and run all tests
#
# Note that CXX, CC and CMAKE can all be customised so that
# they can be matched to the development environment in each
# container
cd /tmp
if [ -z "$CXX" ]; then
    CXX=$(type -p g++)
fi
if [ -z "$CC" ]; then
	CC=$(type -p gcc)
fi
if [ -z "$CMAKE" ]; then
	CMAKE=$(type -p cmake)
fi
echo $CMAKE -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC $CMAKE_EXTRA /mnt
$CMAKE -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC $CMAKE_EXTRA /mnt
make -j
make test
