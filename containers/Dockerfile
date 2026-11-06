# Copyright (C) 2018-2026 CERN
# License Apache2 - see LICENCE file
#
# Development container for prmon, providing everything needed to build
# the project and run all of its tests (including the gtest unit tests).
#
# Build the image (from the top level of the prmon repository):
#   podman build -t prmon-dev -f containers/Dockerfile .
#
# Then mount a prmon checkout (cloned with --recurse-submodules) and build:
#   podman run -it --rm -v $PWD:/home/hsf/prmon:Z prmon-dev
#   cmake -S prmon -B build -DBUILD_GTESTS=ON
#   cmake --build build
#   cd build && ctest --output-on-failure
#
# Note that the gtest sources are downloaded at configure time, so the
# container needs network access when cmake is first run.

FROM almalinux:latest

LABEL maintainer="Graeme Stewart <graeme.andrew.stewart@desy.de>"

# Build toolchain and dependencies; python3 is used by the integration
# tests, git to manage submodules and sudo to give the hsf user root rights
RUN dnf install -y \
        gcc-c++ cmake boost boost-devel make clang curl-devel \
        clang-tools-extra util-linux \
        python3 git sudo && \
    dnf clean all && \
    rm -rf /var/cache/dnf

# Unprivileged user 'hsf' (password 'hsf') who can use sudo
RUN useradd -m -s /bin/bash hsf && \
    echo "hsf:hsf" | chpasswd && \
    usermod -aG wheel hsf

USER hsf
WORKDIR /home/hsf

CMD ["/bin/bash"]
