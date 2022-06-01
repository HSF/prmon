# Additional target to run python linters and formatters on python scripts
#
# Requires black/flake8 to be available in the environment


# Get all our Python files (submodules are excluded!)
file(GLOB_RECURSE ALL_PYTHON_FILES ${PROJECT_SOURCE_DIR}/package/*.py)

# Black is rather simple because there are no options...
add_custom_target(
        black
        COMMAND black
        ${ALL_PYTHON_FILES}
)

add_custom_target(
        flake8
        COMMAND flake8
        --config=${CMAKE_CURRENT_SOURCE_DIR}/.flake8
        ${ALL_PYTHON_FILES}
)
