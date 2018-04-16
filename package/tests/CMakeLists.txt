# CPU burner
find_package(Threads REQUIRED)

add_executable(burner burner.cpp)
target_link_libraries(burner PRIVATE Threads::Threads)

add_executable(io-burner io-burner.cpp)
target_link_libraries(io-burner PRIVATE Threads::Threads)

# Custom targets for handling scripted wrappers for tests
function(script_install)
	cmake_parse_arguments(SCRIPT_INSTALL "" "SCRIPT;DESTINATION" "" ${ARGN})
	if(NOT SCRIPT_INSTALL_SCRIPT)
		message(FATAL "Script installer argument SCRIPT is mandatory")
	endif(NOT SCRIPT_INSTALL_SCRIPT)
	if(NOT SCRIPT_INSTALL_DESTINATION)
		set(SCRIPT_INSTALL_DESTINATION ".")
	endif(NOT SCRIPT_INSTALL_DESTINATION)
	add_custom_command(
		OUTPUT "${SCRIPT_INSTALL_DESTINATION}/${SCRIPT_INSTALL_SCRIPT}"
		DEPENDS ${SCRIPT_INSTALL_SCRIPT}
		COMMAND mkdir -p ${SCRIPT_INSTALL_DESTINATION}
		COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT_INSTALL_SCRIPT} ${SCRIPT_INSTALL_DESTINATION}
		VERBATIM
	)
	add_custom_target(
		install_${SCRIPT_INSTALL_SCRIPT} ALL
		DEPENDS ${SCRIPT_INSTALL_SCRIPT}
	)
endfunction(script_install)

script_install(SCRIPT testCPU.py)
script_install(SCRIPT testIO.py)
script_install(SCRIPT testNET.py)
script_install(SCRIPT netBurner.py)
script_install(SCRIPT httpBlock.py DESTINATION cgi-bin)

# CPU Tests
add_test(NAME testCPUsingle COMMAND testCPU.py --threads 1 --procs 1) 
add_test(NAME testCPUmultithread COMMAND testCPU.py --threads 2 --procs 1) 
add_test(NAME testCPUmultiproc COMMAND testCPU.py --threads 1 --procs 2) 
add_test(NAME testCPUinvoke COMMAND testCPU.py --invoke) 

# IO Tests
add_test(NAME basicIOsingle COMMAND testIO.py --usleep 100 --io 10)
add_test(NAME basicIOmultithread COMMAND testIO.py --usleep 100 --io 10 --threads 2)
add_test(NAME basicIOmultiproc COMMAND testIO.py --usleep 100 --io 10  --procs 2)

# Net Tests
add_test(NAME basicNET COMMAND python testNET.py)
