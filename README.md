# Process Monitor (prmon)

The PRocess MONitor is a small stand alone program that can monitor
the resource consumption of a process and its children. This is 
useful in the context of the WLCG/HSF working group to evaluate
the costs and performance of HEP workflows in WLCG. In a previous
incarnation (MemoryMonitor) it has been used by ATLAS for sometime to
gather data on resource consumption by production jobs. 

prmon currently runs on linux machines as it requires access to the
`/proc` interface to process statistics.

## Build and Deployment

### Building the project

Building prmon requires a modern C++ compiler, CMake version 3.1 or
higher and the RapidJSON libraries.

Building should be as simple as

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=<installdir> [-Dprmon_BUILD_DOCS=ON] <path to sources>
    make -j<number of cores on your machine>
    make install

The `prmon_BUILD_DOCS` variable is optional [placeholder - documentation is currently TODO].

### Creating a package with CPack

A cpack based package can be created by invoking

    make package

### Running the tests

[Placeholder - tests are still TODO]

To run the tests of the project, first build it and then invoke

    make test
    
## Running

The `prmon` binary is invoked with the following arguments, all manadtory:

```sh
prmon --pid PPP --filename mon.txt --json-summary mon.json --interval N
```

* `--pid` the 'mother' PID to monitor (all children in the same process tree are monitored as well)
* `--filename` output file for timestamped monitored values
* `--json-summmary` output file for summary data written in JSON format
* `--interval` time (in seconds) between monitoring snapshots

# Copyright

Copyright (c) 2018, CERN.

 