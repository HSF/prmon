#! /usr/bin/env bash
#
# Wrapper script to run make clang-format 
# The assumption is that we're running in
# hepsoftwarefoundation/u20-dev

# Setup
echo ">> Setting things up..."
CXX=$(type -p clang++-10)
CC=$(type -p clang-10)
CMAKE=$(type -p cmake)

# Configure cmake from /tmp
echo ">> Running the cmake configuration..."
cd /tmp
cmd="$CMAKE -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC /mnt"
echo $cmd
$cmd

# Run clang-format target
echo ">> Running make clang-format..."
make clang-format
