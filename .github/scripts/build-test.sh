#! /usr/bin/env bash
#
# Wrapper script to build prmon and run all tests
#
# Note that CXX and CMAKE can all be customised so that
# they can be matched to the development environment in each
# container; CMAKE_EXTRA can be used to pass special options
# to cmake for particular platforms
cd /tmp
echo "Starting build and test for platform $PLATFORM, compiler suite $COMPILER"

# In vanilla containers we need to install the necessary development packages
if [[ "$PLATFORM" == almalinux* ]]; then
    echo "Installing additional RPM development packages"
    dnf install -y gcc-c++ cmake boost boost-devel make clang
fi

if [[ $PLATFORM == ubuntu* ]]; then
    echo "Installing additional DEB development packages"
    apt update
    apt -y install g++ clang libboost-dev cmake make
fi

# If environment variables CXX or CMAKE are defined then use these
# to set the compiler and cmake binary (sometimes needed when there
# is some non-standard binary names)
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

if [ -z "$CMAKE" ]; then
    CMAKE=$(type -p cmake)
fi

# For debugging, handy to print what we're doing
cmd="$CMAKE -S /mnt -B . -DCMAKE_CXX_COMPILER=$CXX -DBUILD_GTESTS=ON $CMAKE_EXTRA"
echo $cmd
$cmd
cmake --build .
cmake --build . --target test
