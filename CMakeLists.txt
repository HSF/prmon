# - Define the minimum CMake version
# HSF recommends 3.10 to support C/C++ compile features for C/C++11 across all
# platforms
cmake_minimum_required(VERSION 3.10)

# - Call project() to setup system
# From CMake 3, we can set the project version easily in one go
project(prmon VERSION 3.2.0 LANGUAGES CXX)

if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()
# ---------------------------------

# For newer CMakes use normalized install destination paths
if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)
endif()

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
if(NOT CMAKE_BUILD_TYPE)
  if(PROFILE_GPROF OR PROFILE_GPERFTOOLS)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  else()
    set(CMAKE_BUILD_TYPE Release)
  endif()
endif()
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
  CACHE STRING "Choose the type of build, options are: None Release MinSizeRel Debug RelWithDebInfo" FORCE)


# - Define the C++ Standard to use
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

#--- NVIDIA GPU Monitoring (NVML) ----------------------------------------------
option(ENABLE_NVIDIA_GPU "Enable NVIDIA GPU monitoring support" ON)

if(ENABLE_NVIDIA_GPU)
  message(STATUS "Checking for NVIDIA Management Library (NVML)...")

  find_library(NVML_LIBRARY
    NAMES nvidia-ml
    PATHS
      /usr/lib/x86_64-linux-gnu
      /usr/lib64
      /usr/lib
      /usr/local/cuda/lib64
      /usr/local/cuda/lib
      /usr/lib/wsl/lib  
  )

  find_path(NVML_INCLUDE_DIR
    NAMES nvml.h
    PATHS
      /usr/include
      /usr/include/nvidia/gdk
      /usr/local/cuda/include
      /usr/include/hwloc 
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NVML DEFAULT_MSG NVML_LIBRARY NVML_INCLUDE_DIR)

  if(NVML_FOUND)
    message(STATUS "Found NVML: ${NVML_LIBRARY}")
    set(NVIDIA_LIBRARIES ${NVML_LIBRARY})
    set(NVIDIA_INCLUDE_DIRS ${NVML_INCLUDE_DIR})
    add_definitions(-DENABLE_NVIDIA_GPU)
  else()
    message(WARNING "NVML library or header not found. NVIDIA monitoring will be disabled.")
    set(ENABLE_NVIDIA_GPU OFF)
  endif()
endif()
# ------------------------------------------------------------------------------

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

#
if(ENABLE_NVIDIA_GPU AND NVML_FOUND)
  target_include_directories(prmon PRIVATE ${NVIDIA_INCLUDE_DIRS})
  target_link_libraries(prmon PRIVATE ${NVIDIA_LIBRARIES})
endif()

#--- create uninstall target ---------------------------------------------------
include(cmake/prmonUninstall.cmake)

#--- code format targets -------------------------------------------------------
include(cmake/clang-format.cmake)
include(cmake/python-format.cmake)