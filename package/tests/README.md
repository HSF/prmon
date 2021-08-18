# Tests README

## Integration Tests

- Standalone binaries provide executables that `prmon` can test
	- e.g., `io-burner` and `mem-burner` 
- Python scripts invoke these binaries and get `prmon` to monitor them, checking that the output is as expected
	- Model is to use xUnit style of testing, with the normal python `unittest` module 

## Unit Tests

- Standalong binaries that utilise the Google Test infrastruture to test specific aspects of the code. These are only built if the CMake option `BUILD_GTESTS` is set to `ON`
	- e.g. `test_parameter_classes`
