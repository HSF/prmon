# - Define the minimum CMake version
# HSF recommends 3.3 to support C/C++ compile features for C/C++11 across all
# platforms
cmake_minimum_required(VERSION 3.3)
# - Call project() to setup system
# From CMake 3, we can set the project version easily in one go
project(prmon VERSION 0.1.0)

#--- Define basic build settings -----------------------------------------------
# - Use GNU-style hierarchy for installing build products
include(GNUInstallDirs)

# - Define a default build type when using a single-mode tool like make/ninja
# If you're using a build tool that supports multiple modes (Visual Studio,
# Xcode), this setting has no effect.
# HSF recommend RelWithDebInfo (optimized with debugging symbols) as this is
# generally the mode used by system packaging (rpm, deb, spack, macports).
# However, it can be overriden by passing ``-DCMAKE_BUILD_TYPE=<type>`` when
# invoking CMake
if(NOT CMAKE_CONFIGURATION_TYPES)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo
      CACHE STRING "Choose the type of build, options are: None Release MinSizeRel Debug RelWithDebInfo Static"
      FORCE
      )
  else()
    set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
      CACHE STRING "Choose the type of build, options are: None Release MinSizeRel Debug RelWithDebInfo Static"
      FORCE
      )
  endif()
endif()

# - Define the static build type as this is very useful for a utility like prmon
SET( CMAKE_CXX_FLAGS_STATIC "-O2" CACHE STRING
    "Flags used by the C++ compiler during static builds."
    FORCE )
SET( CMAKE_C_FLAGS_STATIC "-O2" CACHE STRING
    "Flags used by the C compiler during static builds."
    FORCE )
SET( CMAKE_EXE_LINKER_FLAGS_STATIC
    "-Wl,--warn-unresolved-symbols,--warn-once -static" CACHE STRING
    "Flags used for linking binaries during static builds."
    FORCE )
SET( CMAKE_SHARED_LINKER_FLAGS_STATIC
    "-Wl,--warn-unresolved-symbols,--warn-once" CACHE STRING
    "Flags used by the shared libraries linker during static builds."
    FORCE )
MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_STATUC
    CMAKE_C_FLAGS_STATIC
    CMAKE_EXE_LINKER_FLAGS_STATIC
    CMAKE_SHARED_LINKER_FLAGS_STATIC )

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

#--- enable unit testing capabilities ------------------------------------------
include(CTest)

#--- enable CPack --------------------------------------------------------------
include(cmake/prmonCPack.cmake)

#--- add version files ---------------------------------------------------------
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/prmonVersion.h
               ${CMAKE_CURRENT_BINARY_DIR}/prmonVersion.h )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/prmonVersion.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/prmon )

#--- add CMake infrastructure --------------------------------------------------
include(cmake/prmonCreateConfig.cmake)

#--- add license files ---------------------------------------------------------
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
              ${CMAKE_CURRENT_SOURCE_DIR}/NOTICE
              DESTINATION ${CMAKE_INSTALL_DOCDIR})

#--- project specific subdirectories -------------------------------------------
add_subdirectory(package)

#--- create uninstall target ---------------------------------------------------
include(cmake/prmonUninstall.cmake)
