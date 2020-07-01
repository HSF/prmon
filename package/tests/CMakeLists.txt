# CPU burner
find_package(Threads REQUIRED)

add_executable(burner burner.cpp)
target_link_libraries(burner PRIVATE Threads::Threads)

add_executable(io-burner io-burner.cpp)
target_link_libraries(io-burner PRIVATE Threads::Threads)

add_executable(mem-burner mem-burner.cpp)

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
script_install(SCRIPT testMEM.py)
script_install(SCRIPT netBurner.py)
script_install(SCRIPT httpBlock.py DESTINATION cgi-bin)
script_install(SCRIPT testCOUNT.py)

# Setup the target version of Python we will use for testing
set(PYTHON_TEST "python3" CACHE STRING "Python binary to use for tests")
message(STATUS "Setting Python test binary to '${PYTHON_TEST}' (use -DPYTHON_TEST to change)")

# Python2 has a different version of the network test
# as the modules are significantly different from Python3
if(${PYTHON_TEST} MATCHES "python2")
    script_install(SCRIPT testNET2.py)
    script_install(SCRIPT netBurner2.py)
    script_install(SCRIPT httpBlock2.py DESTINATION cgi-bin)
endif()

# CPU Tests
add_test(NAME testCPUsingle COMMAND ${PYTHON_TEST} testCPU.py --threads 1 --procs 1) 
add_test(NAME testCPUmultithread COMMAND ${PYTHON_TEST} testCPU.py --threads 2 --procs 1) 
add_test(NAME testCPUmultiproc COMMAND ${PYTHON_TEST} testCPU.py --threads 1 --procs 2 --child-fraction 0.5 --time 12) 
add_test(NAME testCPUinvoke COMMAND ${PYTHON_TEST} testCPU.py --invoke) 

# IO Tests
add_test(NAME basicIOsingle COMMAND ${PYTHON_TEST} testIO.py --usleep 100 --io 10)
add_test(NAME basicIOmultithread COMMAND ${PYTHON_TEST} testIO.py --usleep 100 --io 10 --threads 2)
add_test(NAME basicIOmultiproc COMMAND ${PYTHON_TEST} testIO.py --usleep 100 --io 10  --procs 2)

# Net Tests
# These have to be different for python2 and python3
if(${PYTHON_TEST} MATCHES "python3")
    add_test(NAME basicNET COMMAND ${PYTHON_TEST} testNET.py)
elseif(${PYTHON_TEST} MATCHES "python2")
    add_test(NAME basicNET COMMAND ${PYTHON_TEST} testNET2.py)
endif()

# Memory Tests
add_test(NAME singleMem COMMAND ${PYTHON_TEST} testMEM.py --procs 1)
add_test(NAME childMem COMMAND ${PYTHON_TEST} testMEM.py --procs 4)

# Process and thread counting Tests
add_test(NAME basicCOUNT COMMAND ${PYTHON_TEST} testCOUNT.py --procs 2 --threads 2)
