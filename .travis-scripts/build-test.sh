#! /usr/bin/env bash
#
# Wrapper script to build prmon on travis and run all tests
#
cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++-7 -DCMAKE_C_COMPILER=/usr/bin/gcc-7 /mnt
make -j
make test
