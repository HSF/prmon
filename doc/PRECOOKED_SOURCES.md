# PRECOOKED SOURCES

## Usage

PRMON monitors read from `/proc`, `/sys` or by piping the output of some 
commands to get the data of the processes. Whenever there is a bug
report, it is important to add tests simulating the condition.
To facilitate such tests, the PRMON monitors have been augmented
to optionally read from sources containing data that simulate the
condition.

## Directory Structure

The precooked sources are located at [precooked_tests](../package/scripts/precooked_tests). 

It contains a directory for
each class of tests. For example, `drop` contains tests where the values of 
some metrics decrease after some iteration. These tests are mainly to test 
the protection of monotonic metrics against drop in values.

Each test directory consists of a directory for each iteration.

Inside each iteration, we have a `proc`, `net`, `nvidia`. These are 
directories emulating the actual directories that the monitors read from.
Some directories can be omitted or others can be added depending on the
nature and requirements of the test.
The structure of these directories should be as close to the actual 
directories as possible.
This is not necessary, but helps reduce the changes required 
in the actual PRMON sources. For example, `cpumon` now reads
from `prefix/proc` where `prefix` can be `precooked_sources/drop`.

## Generation

Precooked sources can be added by manually creating the
directories and files, and writing the data. 
A script can also be used to
generate the directories and files. See 
[precook_tests.py](../package/scripts/precook_test.py) for
an example. You may also modify `precook_tests.py` to
additionally generate your sources.

## Testing

Once the precooked sources are generated, a test harness 
has to be written. This harness should initialise the specific 
monitor(s) and pass the path to the directory
containing the test sources. See 
[test_values.cpp](../package/tests/test_values.cpp) for an example.