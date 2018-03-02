# - Use CMake's module to help generating relocatable config files
include(CMakePackageConfigHelpers)

# - Versioning
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/prmonConfigVersion.cmake
  VERSION ${prmon_VERSION}
  COMPATIBILITY SameMajorVersion)

# - Install time config and target files
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/prmonConfig.cmake.in
  "${PROJECT_BINARY_DIR}/prmonConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/../share/cmake/prmon"
  PATH_VARS
    CMAKE_INSTALL_BINDIR
    CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR
  )

# - install and export
install(FILES
  "${PROJECT_BINARY_DIR}/prmonConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}/prmonConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/../share/cmake/prmon"
  )
install(EXPORT prmonTargets
  NAMESPACE prmon::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/../share/cmake/prmon"
  )

