# A helper function to add dependencies supporting both internal and external
# versions.  This adds a dependency with the following arguments:
#   _cmake_name - The name of the dependency's CMake pacakge used in
#                 find_package(${_cmake_name})
#   _opt_name   - The configure option to create as USE_EXTERNAL_${_opt_name}
#   _sub_name   - The name of the submodule the intal copy resides in at
#                 submodules/${_sub_name}
#
# Note: The reason for all the manual logging is only some package configs
#       give useful output so we're just calling find_package(... QUIET)
#       and doing the logging ourselves.
macro(_prmon_add_internal_external_dependency _cmake_name _opt_name _sub_name)
  set(USE_EXTERNAL_${_opt_name} AUTO CACHE STRING "Use an external ${_cmake_name}")
  set_property(CACHE USE_EXTERNAL_${_opt_name} PROPERTY
    STRINGS "ON;TRUE;AUTO;OFF;FALSE")
  mark_as_advanced(USE_EXTERNAL_${_opt_name})
  if(USE_EXTERNAL_${_opt_name} STREQUAL AUTO)
    find_package(${_cmake_name} CONFIG QUIET)
  elseif(USE_EXTERNAL_${_opt_name})
    find_package(${_cmake_name} CONFIG REQUIRED QUIET)
  endif()
  if(${_cmake_name}_FOUND)
    message(STATUS "Found ${_cmake_name}: ${${_cmake_name}_CONFIG} (found version \"${${_cmake_name}_VERSION}\")")
  else() # USE_EXTERNAL_${_opt_name} = OFF or AUTO and failed to find above
    if(USE_EXTERNAL_${_opt_name} STREQUAL AUTO)
      message(STATUS "External ${_cmake_name} not found, using internal submodule")
    else()
      message(STATUS "Forcing internal submodule for ${_cmake_name}")
    endif()
    if (NOT EXISTS ${PROJECT_SOURCE_DIR}/submodules/${_sub_name}/CMakeLists.txt)
      message(FATAL_ERROR "${_cmake_name} submodule is not available")
    endif()
    add_subdirectory(submodules/${_sub_name} EXCLUDE_FROM_ALL)
  endif()
endmacro()


#--- nlohmann-json -------------------------------------------------------------
_prmon_add_internal_external_dependency(nlohmann_json NLOHMANN_JSON nlohmann_json)

# Setup the imported target alias if an older un-aliased version is found
if(TARGET nlohmann_json AND NOT TARGET nlohmann_json::nlohmann_json)
  add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()

#--- spdlog --------------------------------------------------------------------
_prmon_add_internal_external_dependency(spdlog SPDLOG spdlog)
