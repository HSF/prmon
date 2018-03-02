# prmon

Please add some lines describing the project!

## Building the project

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=<installdir> [-Dprmon_BUILD_DOCS=ON] <path to sources>
    make -j<number of cores on your machine>
    make install

The `prmon_BUILD_DOCS` variable is optional, and should be passed if you wish to
build the Doxygen based API documentation. Please note that this requires an existing
installation of [Doxygen](http://www.doxygen.org/index.html). If CMake cannot locate
Doxygen, its install location should be added into `CMAKE_PREFIX_PATH`.
For further details please have a look at [the CMake tutorial](http://www.cmake.org/cmake-tutorial/).

## Building the documentation

The documentation of the project is based on doxygen. To build the documentation,
the project must have been configured with `prmon_BUILD_DOCS` enabled, as
described earlier. It can then be built and installed:

    make doc
    make install

By default, this installs the documentation into `<installdir>/share/doc/HSFTEMPLATE/share/doc`.

## Creating a package with CPack

A cpack based package can be created by invoking

    make package

## Running the tests

To run the tests of the project, first build it and then invoke

    make test

## Inclusion into other projects

If you want to build your own project against prmon, CMake may be the best option for you. Just add its location to _CMAKE_PREFIX_PATH_ and call _find_package(prmon)_ within your CMakeLists.txt.

A `pkg-config` `.pc` file is also installed if you do not use CMake.
Simply add the location of the `.pc` file (nominally `<installdir>/lib/pkgconfig`) and run `pkg-config --cflags --libs HSFTEMPLATE` to get the
include paths and libraries needed to compile and link to HSFTEMPLATE.
