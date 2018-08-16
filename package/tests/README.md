# Tests README

## Test model

- Standalone binaries provide executables that `prmon` can test
	- e.g., `io-burner` and `mem-burner` 
- Python scripts invoke these binaries and get `prmon` to monitor them, checking that the output is as expected
	- Model is to use xUnit style of testing, with the normal python `unittest` module 
