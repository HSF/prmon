set(CPACK_PACKAGE_DESCRIPTION "prmon Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "prmon Project")
set(CPACK_PACKAGE_VENDOR "HEP Software Foundation")
set(CPACK_PACKAGE_VERSION ${prmon_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${prmon_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${prmon_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${prmon_PATCH_VERSION})

set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

#--- source package settings ---------------------------------------------------
set(CPACK_SOURCE_IGNORE_FILES
    ${PROJECT_BINARY_DIR}
    "~$"
    "/.git/"
    "/\\\\\\\\.git/"
    "/#"
)
set(CPACK_SOURCE_STRIP_FILES "")

#--- translate buildtype -------------------------------------------------------
if(NOT CMAKE_CONFIGURATION_TYPES)
  string(TOLOWER "${CMAKE_BUILD_TYPE}" HSF_DEFAULT_BUILDTYPE)
endif()

set(HSF_BUILDTYPE "unknown")

if(HSF_DEFAULT_BUILDTYPE STREQUAL "release")
  set(HSF_BUILDTYPE "opt")
elseif(HSF_DEFAULT_BUILDTYPE STREQUAL "debug")
  set(HSF_BUILDTYPE "dbg")
elseif(HSF_DEFAULT_BUILDTYPE STREQUAL "relwithdebinfo")
  set(HSF_BUILDTYPE "owd")
endif()

#--- use HSF platform name if possible -----------------------------------------
function(hsf_get_platform _output_var)
  # - Determine arch for target of project build
  set(HSF_ARCH ${CMAKE_SYSTEM_PROCESSOR})

  # - Translate compiler info to HSF format
  string(TOLOWER ${CMAKE_CXX_COMPILER_ID} HSF_COMPILER_ID)
  if(NOT HSF_COMPILER_ID)
    set(HSF_COMPILER_ID "unknown")
  endif()

  set(HSF_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
  if(NOT HSF_COMPILER_VERSION)
    set(HSF_COMPILER_VERSION "0")
  endif()
  # Strip version to MAJORMINOR (?)
  string(REGEX REPLACE "([a-z0-9]+)(\.|-|_)([a-z0-9]+).*" "\\1\\3" HSF_COMPILER_VERSION ${HSF_COMPILER_VERSION})

  # - Determine OS info - only Linux is really supported
  #   as we only function with /proc 
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    if(BUILD_STATIC)
      # For a static build the linux distribution
      # doesn't really matter
      set(HSF_OS_ID "static")
      set(HSF_OS_VERSION "")
    else()
    # Use /etc/os-release if it's present
      if(EXISTS "/etc/os-release")
        # - Parse based on spec from freedesktop
        # http://www.freedesktop.org/software/systemd/man/os-release.html
        # - ID
        file(STRINGS "/etc/os-release" HSF_OS_ID REGEX "^ID=.*$")
        if(HSF_OS_ID)
          string(REGEX REPLACE "ID=|\"" "" HSF_OS_ID ${HSF_OS_ID})
        else()
          set(HSF_OS_ID "unknown")
        endif()
        # - VERSION_ID
        file(STRINGS "/etc/os-release" HSF_OS_VERSION REGEX "^VERSION_ID=.*$")
        if(HSF_OS_VERSION)
          string(REGEX REPLACE "VERSION_ID=|\"" "" HSF_OS_VERSION ${HSF_OS_VERSION})
          string(REGEX REPLACE "([a-z0-9]+)(\.|-|_)([a-z0-9]+).*" "\\1\\3" HSF_OS_VERSION ${HSF_OS_VERSION})
        else()
          set(HSF_OS_VERSION "0")
        endif()
      else()
        # Workaround for older systems
        # 1. Might be lucky and have lsb_release
        find_program(prmon_LSB_RELEASE_EXECUTABLE lsb_release
          DOC "Path to lsb_release program"
          )
        mark_as_advanced(prmon_LSB_RELEASE_EXECUTABLE)
        if(prmon_LSB_RELEASE_EXECUTABLE)
          # - ID
          execute_process(COMMAND ${prmon_LSB_RELEASE_EXECUTABLE} -is
            OUTPUT_VARIABLE HSF_OS_ID
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
          string(TOLOWER ${HSF_OS_ID} HSF_OS_ID)
          # - Version
          execute_process(COMMAND ${prmon_LSB_RELEASE_EXECUTABLE} -ir
            OUTPUT_VARIABLE HSF_OS_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
          string(REGEX REPLACE "([a-z0-9]+)(\.|-|_)([a-z0-9]+).*" "\\1\\3" HSF_OS_VERSION ${HSF_OS_VERSION})
        else()
          # 2. Only mark in general terms, or have to check for possible /etc/VENDOR-release files
          set(HSF_OS_ID "linux")
          string(REGEX REPLACE "([a-z0-9]+)(\.|-|_)([a-z0-9]+).*" "\\1\\3" HSF_OS_VERSION ${CMAKE_SYSTEM_VERSION})
        endif()
      endif()
    endif()
  else()
    set(HSF_OS_ID "unknown")
    set(HSF_OS_VERSION "0")
  endif()

  set(${_output_var} "${HSF_ARCH}-${HSF_OS_ID}${HSF_OS_VERSION}-${HSF_COMPILER_ID}${HSF_COMPILER_VERSION}-${HSF_BUILDTYPE}" PARENT_SCOPE)
endfunction()

execute_process(
  COMMAND hsf_get_platform.py --buildtype ${HSF_BUILDTYPE}
  OUTPUT_VARIABLE HSF_PLATFORM OUTPUT_STRIP_TRAILING_WHITESPACE
  )

# If hsf_get_platform isn't available, use CMake function
if(NOT HSF_PLATFORM)
  hsf_get_platform(HSF_PLATFORM)
endif()

set(CPACK_PACKAGE_RELOCATABLE True)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "prmon_${prmon_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "prmon_${prmon_VERSION}_${HSF_PLATFORM}")

include(CPack)
