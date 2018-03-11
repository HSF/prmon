#! /bin/bash
#
# Execute the cpu burner and check we can monitor it
./burner --time 5 &
../prmon --pid $! --filename test.txt --json-summary test.json --interval 1

# TODO - add some check that results are what we expected
