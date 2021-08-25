# CPU burner
find_package(Threads REQUIRED)

add_executable(burner burner.cpp)
target_link_libraries(burner PRIVATE Threads::Threads)

add_executable(io-burner io-burner.cpp)
target_link_libraries(io-burner PRIVATE Threads::Threads)

add_executable(mem-burner mem-burner.cpp)

if (${CMAKE_VERSION} VERSION_GREATER "3.14.0" AND BUILD_GTESTS)

    include(FetchContent)
    FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    include(GoogleTest)

    add_executable(test_parameter_classes test_parameter_classes.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/parameter.cpp
    )
    target_include_directories(test_parameter_classes PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    )
    target_link_libraries(test_parameter_classes
                            PRIVATE
                            Threads::Threads
                            gtest_main
    )

    gtest_discover_tests(test_parameter_classes)

    add_executable(test_values test_values.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/countmon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/cpumon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/memmon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/netmon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/nvidiamon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/iomon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/MessageBase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/parameter.cpp
    )
    target_compile_definitions(test_values PRIVATE TESTS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/../scripts/precooked_tests/)

    target_include_directories(test_values PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../include 
        ${CMAKE_CURRENT_SOURCE_DIR}/../src 
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    )
    if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(test_values 
                            PRIVATE 
                            nlohmann_json::nlohmann_json
    )
    endif(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(test_values
                            PRIVATE
                            Threads::Threads
                            gtest_main
    )
    gtest_discover_tests(test_values)

endif()

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

script_install(SCRIPT test_cpu.py)
script_install(SCRIPT test_io.py)
script_install(SCRIPT test_net.py)
script_install(SCRIPT test_mem.py)
script_install(SCRIPT net_burner.py)
script_install(SCRIPT http_block.py DESTINATION cgi-bin)
script_install(SCRIPT test_count.py)
script_install(SCRIPT test_exit.py)
script_install(SCRIPT test_disable.py)

# Setup the target version of Python we will use for testing
set(PYTHON_TEST "python3" CACHE STRING "Python binary to use for tests")
message(STATUS "Setting Python test binary to '${PYTHON_TEST}' (use -DPYTHON_TEST to change)")

# Python2 has a different version of the network test
# as the modules are significantly different from Python3
if(${PYTHON_TEST} MATCHES "python2")
    script_install(SCRIPT test_net2.py)
    script_install(SCRIPT net_burner2.py)
    script_install(SCRIPT http_block2.py DESTINATION cgi-bin)
endif()

# CPU Tests
add_test(NAME testCPUsingle COMMAND ${PYTHON_TEST} test_cpu.py --threads 1 --procs 1) 
add_test(NAME testCPUmultithread COMMAND ${PYTHON_TEST} test_cpu.py --threads 2 --procs 1) 
add_test(NAME testCPUmultiproc COMMAND ${PYTHON_TEST} test_cpu.py --threads 1 --procs 2 --child-fraction 0.5 --time 12) 
add_test(NAME testCPUinvoke COMMAND ${PYTHON_TEST} test_cpu.py --invoke) 

# IO Tests
add_test(NAME basicIOsingle COMMAND ${PYTHON_TEST} test_io.py --usleep 100 --io 10)
add_test(NAME basicIOmultithread COMMAND ${PYTHON_TEST} test_io.py --usleep 100 --io 10 --threads 2)
add_test(NAME basicIOmultiproc COMMAND ${PYTHON_TEST} test_io.py --usleep 100 --io 10  --procs 2)

# Net Tests
# These have to be different for python2 and python3
if(${PYTHON_TEST} MATCHES "python3")
    add_test(NAME basicNET COMMAND ${PYTHON_TEST} test_net.py)
elseif(${PYTHON_TEST} MATCHES "python2")
    add_test(NAME basicNET COMMAND ${PYTHON_TEST} test_net2.py)
endif()

# Memory Tests
add_test(NAME singleMem COMMAND ${PYTHON_TEST} test_mem.py --procs 1)
add_test(NAME childMem COMMAND ${PYTHON_TEST} test_mem.py --procs 4)

# Process and thread counting Tests
add_test(NAME basicCOUNT COMMAND ${PYTHON_TEST} test_count.py --procs 2 --threads 2)

# Units check test
add_test(NAME testUnits COMMAND ${PYTHON_TEST} test_cpu.py --units --time 3 --slack 0 --interval 1) 

# Test passing the child exit code works
add_test(NAME testExitCode COMMAND ${PYTHON_TEST} test_exit.py --exit-code 43)

# Test for disabling monitor components
add_test(NAME testDisable COMMAND ${PYTHON_TEST} test_disable.py --disable iomon countmon)
