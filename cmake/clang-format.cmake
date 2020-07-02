# Additional target to perform clang-format run
#
# Requires clang-format to be available in the
# environment

# Setup the target version of clang-format we will use
set(CLANG_FORMAT "clang-format" CACHE STRING "Clang format binary")
message(STATUS "Setting clang-format test binary to '${CLANG_FORMAT}' (use -DCLANG_FORMAT to change)")

# Get all project files
file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h)

# Using --style=Google uses the built in rules of
# the clang format binary. This is slightly 
# preferred over an explicit file as new options
# appear in later versions that cause earlier
# versions to baulk
add_custom_target(
        clang-format
        COMMAND clang-format
        --style=Google
        -i
        ${ALL_SOURCE_FILES}
)
