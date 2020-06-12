# Additional target to perform clang-format/clang-tidy run
# Requires clang-format and clang-tidy

# Get all project files
file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h)

add_custom_target(
        clang-format
        COMMAND clang-format
        --style=file
        -i
        ${ALL_SOURCE_FILES}
)
