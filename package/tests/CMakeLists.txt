# CPU burner
find_package(Threads REQUIRED)

add_executable(burner burner.cpp)
target_link_libraries(burner PRIVATE Threads::Threads)

add_executable(io-burner io-burner.cpp)
target_link_libraries(io-burner PRIVATE Threads::Threads)

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
script_install(testIO.py)

# CPU Tests
add_test(NAME testCPUsingle COMMAND testCPU.py --threads 1 --procs 1) 
add_test(NAME testCPUmultithread COMMAND testCPU.py --threads 2 --procs 1) 
add_test(NAME testCPUmultiproc COMMAND testCPU.py --threads 1 --procs 2) 

# IO Tests
add_test(NAME basicIOsingle COMMAND testIO.py --usleep 100 --io 10)
add_test(NAME basicIOmultithread COMMAND testIO.py --usleep 100 --io 10 --threads 2)
add_test(NAME basicIOmultiproc COMMAND testIO.py --usleep 100 --io 10  --procs 2)
