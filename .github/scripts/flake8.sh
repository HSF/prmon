#! /bin/sh

# First we need to install Python and Flake 8
# N.B. this is for an Alma Linux container (specifically almalinux 9
# but should be quite generic)
dnf -y install python3 python-pip
pip install flake8

# Configure and run flake8 target
cmake /mnt
cmake --build . --target flake8
