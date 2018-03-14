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

script_install(testCPU.py)

# Test test
add_test(NAME testCPUsingle COMMAND testCPU.py 1 1) 
add_test(NAME testCPUmultithread COMMAND testCPU.py 2 1) 
add_test(NAME testCPUmultiproc COMMAND testCPU.py 1 2) 
