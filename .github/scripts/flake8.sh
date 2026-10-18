#! /bin/sh

# First we need to install Python and Flake 8
dnf -y install python3 python-pip cmake gcc-c++ boost boost-devel make
pip install flake8

# Configure and run flake8 target
cmake -S /mnt -B .
cmake --build . --target flake8
