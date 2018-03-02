# - Create interface target for local catch copy
# Single header is placed in dedicated directory for clean separation
# Using an interface target means we simply link to it like any other target
# and CMake with add all needed definitions, include paths etc.
# To update the vendored copy of catch, simply download it via:
#
# $ curl -O https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp
#
# and copy the `catch.hpp` file into the catch subdirectory
add_library(catch INTERFACE)
target_include_directories(catch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/catch)


# - Create and define test executables
# Here a generic example for creating an executable then defining it as a
# test to CTest
# 1. Create executable
add_executable(test_example test_example.cpp)

# 2. Link it to the library being tested, plus the unit test library(ies)
target_link_libraries(test_example examplelibrary catch)

# 3. Add the test program as a command to be run as a test
# Note: NAME is what will be shown in CTest, COMMAND is what will be run
#       CMake knows that text_example is a CMake target, and knows where
#       the resultant executable is. Things like PATH and so on do not need
#       to be set.
# Note: The executable can be passed command line arguments. Here we add
#       `--success` which is provided by Catch's main() function and reports
#       info about successful as well as failing tests.
add_test(NAME testExampleClass COMMAND test_example --success)

