#! /usr/bin/env bash
#
# Wrapper script to run make clang-format 
# The assumption is that we're running in
# hepsoftwarefoundation/u20-dev

# Setup
echo ">> Setting things up..."
if [ -z "$CXX" ]; then
  CXX=$(type -p g++)
fi
if [ -z "$CC" ]; then
  CC=$(type -p gcc)
fi
if [ -z "$CMAKE" ]; then
  CMAKE=$(type -p cmake)
fi

# Configure cmake from /tmp
echo ">> Running the cmake configuration..."
cd /tmp
cmd="$CMAKE -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC /mnt"
echo $cmd
$cmd

# Make to test
echo ">> Running make..."
make -j 4

# Run clang-format target
echo ">> Running make clang-format..."
make clang-format
