#--- nlohmann-json -------------------------------------------------------------
find_package(nlohmann_json REQUIRED)

#--- spdlog --------------------------------------------------------------------
option(USE_SPDLOG_SUBMODULE "Always use the internal copy of spdlog" FALSE)
mark_as_advanced(USE_SPDLOG_SUBMODULE)
if(NOT USE_SPDLOG_SUBMODULE)
  find_package(spdlog)
endif()
if (spdlog_FOUND)
  message(STATUS
          "Found spdlog: ${spdlog_CONFIG} (found version \"${spdlog_VERSION}\")")
else()
  set_property(CACHE USE_SPDLOG_SUBMODULE PROPERTY VALUE TRUE)
  message(STATUS "spdlog not found, using included submodule.")
  if (NOT EXISTS ${PROJECT_SOURCE_DIR}/submodules/spdlog/CMakeLists.txt)
    message(FATAL_ERROR "spdlog submodule is not available.")
  endif()
  add_subdirectory(submodules/spdlog EXCLUDE_FROM_ALL)
endif()
