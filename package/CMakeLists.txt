add_executable(prmon src/prmon.cpp
           src/prmonutils.cpp
           src/utils.cpp
           src/MessageBase.cpp
           src/parameter.cpp
           src/netmon.cpp
           src/iomon.cpp
           src/cpumon.cpp
           src/countmon.cpp
           src/wallmon.cpp
           src/memmon.cpp
           src/nvidiamon.cpp
           )

target_link_libraries(prmon PRIVATE spdlog::spdlog_header_only)

if (BUILD_BENCHMARK_LOG)
  add_executable(benchmark-log benchmarks/benchmark-log.cpp)
endif(BUILD_BENCHMARK_LOG)

# Some older versions of the nlohmann_json package don't define
# this target (< 3.0.0 ?). The build on these platforms seems to
# go ok even without this "link library" definition.
if(TARGET nlohmann_json::nlohmann_json)
  target_link_libraries(prmon PRIVATE nlohmann_json::nlohmann_json)
endif(TARGET nlohmann_json::nlohmann_json)

# Set flags based on more unusual build flags
if(BUILD_STATIC)
  set_target_properties(prmon PROPERTIES LINK_FLAGS "-static")
endif(BUILD_STATIC)

if(PROFILE_GPROF)
  set_target_properties(prmon PROPERTIES COMPILE_FLAGS "-pg")
  set_target_properties(prmon PROPERTIES LINK_FLAGS "-pg")
endif(PROFILE_GPROF)

if(PROFILE_GPERFTOOLS)
  find_package(Gperftools REQUIRED)
  set_target_properties(prmon PROPERTIES LINK_LIBRARIES ${GPERFTOOLS_PROFILE_LIBRARY})
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
