# - Create and define test executables
# Here a generic example for creating an executable then defining it as a
# test to CTest
# 1. Create executable
#add_executable(test_example test_example.cpp)

# 2. Link it to the library being tested, plus the unit test library(ies)
#target_link_libraries(test_example examplelibrary catch)

# 3. Add the test program as a command to be run as a test
# Note: NAME is what will be shown in CTest, COMMAND is what will be run
#       CMake knows that text_example is a CMake target, and knows where
#       the resultant executable is. Things like PATH and so on do not need
#       to be set.
# Note: The executable can be passed command line arguments. Here we add
#       `--success` which is provided by Catch's main() function and reports
#       info about successful as well as failing tests.
#add_test(NAME testExampleClass COMMAND test_example --success)

find_package(Boost COMPONENTS program_options REQUIRED)
find_package(Threads REQUIRED)

add_executable(burner burner.cpp)
target_include_directories(burner PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(burner PRIVATE ${Boost_LIBRARIES} Threads::Threads)
