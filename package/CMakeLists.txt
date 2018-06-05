find_package(RapidJSON REQUIRED)

add_executable(prmon src/prmon.cpp
					 src/pidutils.cpp
					 src/netmon.cpp 
					 src/iomon.cpp 
					 src/cpumon.cpp 
					 src/wallmon.cpp
					 src/memmon.cpp)
target_include_directories(prmon PRIVATE ${RapidJSON_INCLUDE_DIR})

# Set flags based on more unusual build flags
if(BUILD_STATIC)
  set_target_properties(prmon PROPERTIES LINK_FLAGS "-static")
endif(BUILD_STATIC)

if(PROFILE_GPROF)
  set_target_properties(prmon PROPERTIES COMPILE_FLAGS "-pg")
  set_target_properties(prmon PROPERTIES LINK_FLAGS "-pg")
endif(PROFILE_GPROF)

if(PROFILE_GPERFTOOLS)
  set_target_properties(prmon PROPERTIES LINK_LIBRARIES "profiler")
endif(PROFILE_GPERFTOOLS)

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
