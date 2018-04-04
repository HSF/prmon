# Install the scripts under bin in the install area
set(SCRIPTS_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/bin)
install(PROGRAMS prmon_plot.py DESTINATION ${SCRIPTS_INSTALL_PATH})
