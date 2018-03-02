# List sources for target
# - Note that we include headers. CMake will resolve dependencies on headers
#   correctly, but they will not show up in IDEs like Xcode if they are not
#   listed in the files for a target
set(examplelibrary_SOURCES
  include/prmon/Example.h
  src/Example.cpp
  )

## create a library target
add_library(examplelibrary SHARED ${examplelibrary_SOURCES})

# - Define its header interface when building and when installed
# (i.e. used by a third party)
target_include_directories(examplelibrary
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

## if needed, link against "someexternal"
## which has been included with find_package(someexternal)
#target_link_libraries(examplelibrary ${someexternal_LIBRARIES})

# - Install the example library into the install time library directory
# The EXPORT name is used so that we can "export" the target for
# use by client projects
install(TARGETS examplelibrary
  EXPORT prmonTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

## install the headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/prmon
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

# - If testing is required, recurse into the directory holding them
# Note: The BUILD_TESTING variable is set by the inclusion of the CTest
#       Module (done in the root CMake script for the project). It's set
#       as a CMake option that's ON by default, but may be toggled by
#       a cmake-time command line option or through the CMake curses/GUI.
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

