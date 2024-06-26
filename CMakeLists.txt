# - Define the minimum CMake version
# HSF recommends 3.3 to support C/C++ compile features for C/C++11 across all
# platforms
cmake_minimum_required(VERSION 3.3)
# - Call project() to setup system
# From CMake 3, we can set the project version easily in one go
project(prmon VERSION 3.1.0)

# For newer CMakes we ensure correct handling of extracting GTest from tarball
if (POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif (POLICY CMP0135)

#--- Define basic build settings -----------------------------------------------
# - Use GNU-style hierarchy for installing build products
include(GNUInstallDirs)

# Add some build option variables, for static builds and profiling
if(NOT BUILD_STATIC)
  set(BUILD_STATIC OFF)
endif()
set(BUILD_STATIC "${BUILD_STATIC}"
  CACHE BOOL "Build a static version of the prmon binary" FORCE)

if(NOT PROFILE_GPROF)
  set(PROFILE_GPROF OFF)
endif()
set(PROFILE_GPROF "${PROFILE_GPROF}"
  CACHE BOOL "Build with the GNU profile option set" FORCE)

if(NOT PROFILE_GPERFTOOLS)
  set(PROFILE_GPERFTOOLS OFF)
endif()
set(PROFILE_GPERFTOOLS "${PROFILE_GPERFTOOLS}"
  CACHE BOOL "Build with the GPerfTools profiler library" FORCE)

if(NOT BUILD_BENCHMARK_LOG)
  set(BUILD_BENCHMARK_LOG OFF)
endif()
set(BUILD_BENCHMARK_LOG "${BUILD_BENCHMARK_LOG}"
  CACHE BOOL "Build binary that benchmarks spdlog speed" FORCE)

if(NOT BUILD_GTESTS)
  set(BUILD_GTESTS OFF)
endif()
set(BUILD_GTESTS "${BUILD_GTESTS}"
  CACHE BOOL "Add the value tests (dependent on gtest)" FORCE)

# Define a default build type when using a single-mode tool like make/ninja
# We make this default to Release, unless profiling is enabled, in which
# case do RelWithDebInfo (bcause you need the debug symbols)
if(NOT CMAKE_BUILD_TYPE)
  if(PROFILE_GPROF OR PROFILE_GPERFTOOLS)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  else()
    set(CMAKE_BUILD_TYPE Release)
  endif()
endif()
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
  CACHE STRING "Choose the type of build, options are: None Release MinSizeRel Debug RelWithDebInfo" FORCE)


# - Define the C++ Standard to use (Simplest Possible)
# This will add any compiler flags needed to compile against the required standard
# without any vendor extensions
# NOTE: It *does not* guarantee that the compiler in use supports the complete
# standard. Nor does it inform clients of the project what standard was used.
# Both of these issues can be resolved using CMake's compile features, see
#
# - https://cmake.org/cmake/help/v3.3/manual/cmake-compile-features.7.html
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the compiler to be more picky
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -Werror")
string(REGEX MATCH "^arm" ARM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})
if("${ARM_PROCESSOR}" STREQUAL "arm")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
  message(STATUS "Disabled ABI warnings on ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()


# Add package utilities to CMake path
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

#--- enable unit testing capabilities ------------------------------------------
include(CTest)

#--- enable CPack --------------------------------------------------------------
include(cmake/prmonCPack.cmake)

#--- add version files ---------------------------------------------------------
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/prmonVersion.h
               ${CMAKE_CURRENT_BINARY_DIR}/prmonVersion.h )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/prmonVersion.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/prmon )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

#--- add CMake infrastructure --------------------------------------------------
include(cmake/prmonCreateConfig.cmake)

#--- add license files ---------------------------------------------------------
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
              ${CMAKE_CURRENT_SOURCE_DIR}/NOTICE
              DESTINATION ${CMAKE_INSTALL_DOCDIR})

#--- project dependencies ------------------------------------------------------
include(cmake/prmonDependencies.cmake)

#--- project specific subdirectories -------------------------------------------

add_subdirectory(package)

#--- include directories -------------------------------------------
target_include_directories(prmon PRIVATE ${PROJECT_SOURCE_DIR}/package/include)
if (BUILD_BENCHMARK_LOG)
  target_include_directories(benchmark-log PRIVATE ${PROJECT_SOURCE_DIR}/package/include)
endif(BUILD_BENCHMARK_LOG)
#--- create uninstall target ---------------------------------------------------
include(cmake/prmonUninstall.cmake)

#--- code format targets -------------------------------------------------------
include(cmake/clang-format.cmake)
include(cmake/python-format.cmake)
