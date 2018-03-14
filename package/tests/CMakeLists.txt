# CPU burner
find_package(Threads REQUIRED)

add_executable(burner burner.cpp)
target_link_libraries(burner PRIVATE Threads::Threads)

# Custom targets for handling scripted wrappers for tests
function(script_install SCRIPT)
	add_custom_command(
		OUTPUT ${SCRIPT}
		DEPENDS ${SCRIPT}
		COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT} .
		VERBATIM
	)
	add_custom_target(
		install_${SCRIPT} ALL
		DEPENDS ${SCRIPT}
	)
endfunction(script_install)

script_install(basic_prmon.sh)

# Test test
add_test(NAME basic_prmon COMMAND basic_prmon.sh) 
