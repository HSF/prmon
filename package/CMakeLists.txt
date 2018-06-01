find_package(RapidJSON REQUIRED)
find_package(Threads REQUIRED)

## Profiling
# If you wish to compile prmon with profiling then just 
# uncomment the line below to use gprof:
#
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#
# If you want to use gperftools then add "profiler" to
# the prmon target_link_libraries command instead

add_executable(prmon src/prmon.cpp
					 src/pidutils.cpp
					 src/netmon.cpp 
					 src/iomon.cpp 
					 src/cpumon.cpp 
					 src/wallmon.cpp
					 src/memmon.cpp)
target_include_directories(prmon PRIVATE ${RapidJSON_INCLUDE_DIR})
target_link_libraries(prmon Threads::Threads)

if(BUILD_STATIC)
  set_target_properties(prmon PROPERTIES LINK_FLAGS "-static")
endif(BUILD_STATIC)

# - Install the example library into the install time library directory
# The EXPORT name is used so that we can "export" the target for
# use by client projects
install(TARGETS prmon
  EXPORT prmonTargets
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  )


# - If testing is required, recurse into the directory holding them
# Note: The BUILD_TESTING variable is set by the inclusion of the CTest
#       Module (done in the root CMake script for the project). It's set
#       as a CMake option that's ON by default, but may be toggled by
#       a cmake-time command line option or through the CMake curses/GUI.
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Build useful scripts - currently plotting only
add_subdirectory(scripts)
