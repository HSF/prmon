# Process Monitor (prmon)

[![Build Status][build-img]][build-link]  [![License][license-img]][license-url] [![Codefactor][codefactor-img]][codefactor-url] [![HSF Project][hsf-project-img]][hsf-project-url]

[build-img]: https://github.com/HSF/prmon/workflows/CI/badge.svg?branch=main
[build-link]: https://github.com/HSF/prmon/actions?query=workflow%3ACI+branch%3Amain
[license-img]: https://img.shields.io/github/license/hsf/prmon.svg
[license-url]: https://github.com/hsf/prmon/blob/main/LICENSE
[codefactor-img]: https://www.codefactor.io/repository/github/HSF/prmon/badge
[codefactor-url]: https://www.codefactor.io/repository/github/HSF/prmon
[hsf-project-img]: https://img.shields.io/badge/HSF-Bronze_Project-CD7F32?logo=data:image/svg%2bxml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+CjwhLS0gQ3JlYXRlZCB3aXRoIElua3NjYXBlIChodHRwOi8vd3d3Lmlua3NjYXBlLm9yZy8pIC0tPgoKPHN2ZwogICB3aWR0aD0iMTIwbW0iCiAgIGhlaWdodD0iODBtbSIKICAgdmlld0JveD0iMCAwIDEyMCA4MCIKICAgdmVyc2lvbj0iMS4xIgogICBpZD0ic3ZnMSIKICAgaW5rc2NhcGU6dmVyc2lvbj0iMS4zICgwZTE1MGVkLCAyMDIzLTA3LTIxKSIKICAgc29kaXBvZGk6ZG9jbmFtZT0iaHNmLWxvZ28tc2hpZWxkLWZ1bGwuc3ZnIgogICB4bWxuczppbmtzY2FwZT0iaHR0cDovL3d3dy5pbmtzY2FwZS5vcmcvbmFtZXNwYWNlcy9pbmtzY2FwZSIKICAgeG1sbnM6c29kaXBvZGk9Imh0dHA6Ly9zb2RpcG9kaS5zb3VyY2Vmb3JnZS5uZXQvRFREL3NvZGlwb2RpLTAuZHRkIgogICB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciCiAgIHhtbG5zOnN2Zz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciCiAgIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyIKICAgeG1sbnM6Y2M9Imh0dHA6Ly9jcmVhdGl2ZWNvbW1vbnMub3JnL25zIyIKICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIj4KICA8dGl0bGUKICAgICBpZD0idGl0bGUxIj5IU0Y6IEdvbGQgUHJvamVjdDwvdGl0bGU+CiAgPHNvZGlwb2RpOm5hbWVkdmlldwogICAgIGlkPSJuYW1lZHZpZXcxIgogICAgIHBhZ2Vjb2xvcj0iI2ZmZmZmZiIKICAgICBib3JkZXJjb2xvcj0iIzAwMDAwMCIKICAgICBib3JkZXJvcGFjaXR5PSIwLjI1IgogICAgIGlua3NjYXBlOnNob3dwYWdlc2hhZG93PSIyIgogICAgIGlua3NjYXBlOnBhZ2VvcGFjaXR5PSIwLjAiCiAgICAgaW5rc2NhcGU6cGFnZWNoZWNrZXJib2FyZD0idHJ1ZSIKICAgICBpbmtzY2FwZTpkZXNrY29sb3I9IiNkMWQxZDEiCiAgICAgaW5rc2NhcGU6ZG9jdW1lbnQtdW5pdHM9Im1tIgogICAgIGlua3NjYXBlOnpvb209IjEuMTg5MzA0NCIKICAgICBpbmtzY2FwZTpjeD0iMTkxLjcwODciCiAgICAgaW5rc2NhcGU6Y3k9IjE5Ny41OTQ0OSIKICAgICBpbmtzY2FwZTp3aW5kb3ctd2lkdGg9IjE0NTYiCiAgICAgaW5rc2NhcGU6d2luZG93LWhlaWdodD0iODE5IgogICAgIGlua3NjYXBlOndpbmRvdy14PSI1NyIKICAgICBpbmtzY2FwZTp3aW5kb3cteT0iNDEiCiAgICAgaW5rc2NhcGU6d2luZG93LW1heGltaXplZD0iMCIKICAgICBpbmtzY2FwZTpjdXJyZW50LWxheWVyPSJsYXllcjEiCiAgICAgc2hvd2d1aWRlcz0idHJ1ZSIgLz4KICA8ZGVmcwogICAgIGlkPSJkZWZzMSI+CiAgICA8cmVjdAogICAgICAgeD0iMjQ1LjUyMTY3IgogICAgICAgeT0iNjM5LjAyODk5IgogICAgICAgd2lkdGg9IjMyNC41NTk0NSIKICAgICAgIGhlaWdodD0iMjUzLjkyOTk1IgogICAgICAgaWQ9InJlY3Q1IiAvPgogICAgPHJlY3QKICAgICAgIHg9IjI0NS41MjE2NyIKICAgICAgIHk9IjYzOS4wMjg5OSIKICAgICAgIHdpZHRoPSIzMjQuNTU5NDUiCiAgICAgICBoZWlnaHQ9IjI1My45Mjk5NSIKICAgICAgIGlkPSJyZWN0NiIgLz4KICA8L2RlZnM+CiAgPGcKICAgICBpbmtzY2FwZTpsYWJlbD0iTGF5ZXIgMSIKICAgICBpbmtzY2FwZTpncm91cG1vZGU9ImxheWVyIgogICAgIGlkPSJsYXllcjEiPgogICAgPHBhdGgKICAgICAgIHN0eWxlPSJmaWxsOm5vbmU7c3Ryb2tlOiNhYTAwMDA7c3Ryb2tlLXdpZHRoOjkuNTQ4NzY7c3Ryb2tlLWxpbmVjYXA6cm91bmQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjE7ZmlsbC1vcGFjaXR5OjEiCiAgICAgICBkPSJtIDU5Ljc4ODYzNSw5Ljc3NzUzMTEgYyAwLDAgLTE4LjIxNTUwMSw1LjU0ODY2NjkgLTE4LjIyMDI0OCwxOS45OTk5OTk5IDAuMTM5MjIsMTIuMTM3MTU3IDguNjQyMzc0LDEwLjM5NzY5NCAxOC40OTgzODMsMTAuMjc4OTY2IDkuOTIwMTYzLC0wLjExOTUwMSAxNy4xODg5NzQsMS4zNzA5MzkgMTcuMjc0NzA2LDEwLjM4ODQ0MSAwLjA1NTc3LDExLjQxNDU4MiAtMTcuNTUyODQxLDE5LjMzMjU5MyAtMTcuNTUyODQxLDE5LjMzMjU5MyIKICAgICAgIGlkPSJwYXRoNiIKICAgICAgIHNvZGlwb2RpOm5vZGV0eXBlcz0iY2NzY2MiIC8+CiAgICA8cGF0aAogICAgICAgc3R5bGU9ImZpbGw6bm9uZTtzdHJva2U6I2ZmZmZmZjtzdHJva2Utd2lkdGg6ODtzdHJva2UtZGFzaGFycmF5Om5vbmU7c3Ryb2tlLW9wYWNpdHk6MSIKICAgICAgIGQ9Ik0gNC40NDU4MzE5LDc1LjY0MTg3MiA1OS43MTczMDUsNzEuNTQwNjMyIDExNi45MDQxLDc0Ljk3NDQ2NSIKICAgICAgIGlkPSJwYXRoNSIKICAgICAgIHNvZGlwb2RpOm5vZGV0eXBlcz0iY2NjIiAvPgogICAgPHBhdGgKICAgICAgIHN0eWxlPSJmaWxsOm5vbmU7c3Ryb2tlOiNmZmZmZmY7c3Ryb2tlLXdpZHRoOjg7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiCiAgICAgICBkPSJNIDIuODg4NTQ5Miw0LjAxODcxMTkgQyAzNy4xMzc2OTEsNS4zNTIwNDUgNDUuMTE2MDAyLDYuOTA3ODQ3MSA1OS43ODUwODUsOC4yNDExODAyIDgzLjgwMzY2Miw2LjkwNzg0NzEgMTEwLjkwNTc4LDUuMzUyMDQ1IDExNi42ODE2Myw0LjAxODcxMTkiCiAgICAgICBpZD0icGF0aDEiCiAgICAgICBzb2RpcG9kaTpub2RldHlwZXM9ImNjYyIgLz4KICAgIDx0ZXh0CiAgICAgICB4bWw6c3BhY2U9InByZXNlcnZlIgogICAgICAgc3R5bGU9ImZvbnQtc3R5bGU6bm9ybWFsO2ZvbnQtdmFyaWFudDpub3JtYWw7Zm9udC13ZWlnaHQ6bm9ybWFsO2ZvbnQtc3RyZXRjaDpub3JtYWw7Zm9udC1zaXplOjU3LjEyMDFweDtmb250LWZhbWlseTonQXJpYWwgTmFycm93JzstaW5rc2NhcGUtZm9udC1zcGVjaWZpY2F0aW9uOidBcmlhbCBOYXJyb3cnO2ZvbnQtdmFyaWFudC1saWdhdHVyZXM6bm9ybWFsO2ZvbnQtdmFyaWFudC1jYXBzOm5vcm1hbDtmb250LXZhcmlhbnQtbnVtZXJpYzpub3JtYWw7Zm9udC12YXJpYW50LWVhc3QtYXNpYW46bm9ybWFsO2xldHRlci1zcGFjaW5nOjkuMzEzNDZweDt3b3JkLXNwYWNpbmc6MC4wNDI4NDAycHg7ZmlsbDpub25lO3N0cm9rZTojYWEwMDAwO3N0cm9rZS13aWR0aDo2LjQ0MTM7c3Ryb2tlLWxpbmVjYXA6cm91bmQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjE7ZmlsbC1vcGFjaXR5OjEiCiAgICAgICB4PSIyLjYyMTQxNTEiCiAgICAgICB5PSI1Ni43OTYzOTQiCiAgICAgICBpZD0idGV4dDEiCiAgICAgICB0cmFuc2Zvcm09InNjYWxlKDAuOTI0NTI0MTYsMS4wODE2Mzc1KSI+PHRzcGFuCiAgICAgICAgIHNvZGlwb2RpOnJvbGU9ImxpbmUiCiAgICAgICAgIGlkPSJ0c3BhbjEiCiAgICAgICAgIHN0eWxlPSJzdHJva2Utd2lkdGg6Ni40NDEzO3N0cm9rZS1kYXNoYXJyYXk6bm9uZTtmaWxsOm5vbmU7c3Ryb2tlOiNhYTAwMDA7c3Ryb2tlLW9wYWNpdHk6MTtmaWxsLW9wYWNpdHk6MSIKICAgICAgICAgeD0iMi42MjE0MTUxIgogICAgICAgICB5PSI1Ni43OTYzOTQiPkg8L3RzcGFuPjwvdGV4dD4KICAgIDx0ZXh0CiAgICAgICB4bWw6c3BhY2U9InByZXNlcnZlIgogICAgICAgc3R5bGU9ImZvbnQtc3R5bGU6bm9ybWFsO2ZvbnQtdmFyaWFudDpub3JtYWw7Zm9udC13ZWlnaHQ6bm9ybWFsO2ZvbnQtc3RyZXRjaDpub3JtYWw7Zm9udC1zaXplOjU3LjEyMDFweDtmb250LWZhbWlseTonQXJpYWwgTmFycm93JzstaW5rc2NhcGUtZm9udC1zcGVjaWZpY2F0aW9uOidBcmlhbCBOYXJyb3cnO2ZvbnQtdmFyaWFudC1saWdhdHVyZXM6bm9ybWFsO2ZvbnQtdmFyaWFudC1jYXBzOm5vcm1hbDtmb250LXZhcmlhbnQtbnVtZXJpYzpub3JtYWw7Zm9udC12YXJpYW50LWVhc3QtYXNpYW46bm9ybWFsO2xldHRlci1zcGFjaW5nOjkuMzEzNDZweDt3b3JkLXNwYWNpbmc6MC4wNDI4NDAycHg7ZmlsbDpub25lO3N0cm9rZTojYWEwMDAwO3N0cm9rZS13aWR0aDo2LjQ0MTtzdHJva2UtbGluZWNhcDpyb3VuZDtzdHJva2UtZGFzaGFycmF5Om5vbmU7c3Ryb2tlLW9wYWNpdHk6MTtmaWxsLW9wYWNpdHk6MSIKICAgICAgIHg9Ijk0LjAzMTc2MSIKICAgICAgIHk9IjU3LjMzNTk4NyIKICAgICAgIGlkPSJ0ZXh0MS03IgogICAgICAgdHJhbnNmb3JtPSJzY2FsZSgwLjkyNDUyNDE2LDEuMDgxNjM3NSkiPjx0c3BhbgogICAgICAgICBzb2RpcG9kaTpyb2xlPSJsaW5lIgogICAgICAgICBpZD0idHNwYW4xLTUiCiAgICAgICAgIHN0eWxlPSJzdHJva2Utd2lkdGg6Ni40NDE7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZTojYWEwMDAwO3N0cm9rZS1vcGFjaXR5OjE7ZmlsbDpub25lO2ZpbGwtb3BhY2l0eToxIgogICAgICAgICB4PSI5NC4wMzE3NjEiCiAgICAgICAgIHk9IjU3LjMzNTk4NyI+RjwvdHNwYW4+PC90ZXh0PgogIDwvZz4KICA8bWV0YWRhdGEKICAgICBpZD0ibWV0YWRhdGExIj4KICAgIDxyZGY6UkRGPgogICAgICA8Y2M6V29yawogICAgICAgICByZGY6YWJvdXQ9IiI+CiAgICAgICAgPGRjOnRpdGxlPkhTRjogR29sZCBQcm9qZWN0PC9kYzp0aXRsZT4KICAgICAgPC9jYzpXb3JrPgogICAgPC9yZGY6UkRGPgogIDwvbWV0YWRhdGE+Cjwvc3ZnPgo=
[hsf-project-url]: https://hepsoftwarefoundation.org/projects/affiliated.html

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.2554202.svg)](https://doi.org/10.5281/zenodo.2554202)

The PRocess MONitor is a small stand alone program that can monitor
the resource consumption of a process and its children. This is
useful in the context of the WLCG/HSF working group to evaluate
the costs and performance of HEP workflows in WLCG. In a previous
incarnation (MemoryMonitor) it has been used by ATLAS for some time to
gather data on resource consumption by production jobs. One of its
most useful features is to use smaps to correctly calculate the
*Proportional Set Size* in the group of processes monitored, which
is a much better indication of the true memory consumption of
a group of processes where children share many pages.

prmon currently runs on Linux machines as it requires access to the
`/proc` interface to process statistics.

## Build and Deployment

### Cloning the project

As prmon has dependencies on submodules, clone the project as
    
    git clone --recurse-submodules https://github.com/HSF/prmon.git
    
### Building the project

Building prmon requires a C++ compiler that fully supports C++11,
and CMake version 3.3 or higher.  It also has dependencies on:

  - [Niels Lohmann JSON libraries](https://github.com/nlohmann/json)
    - `nlohmann-json-dev` in Ubuntu 18, `nlohmann-json3-dev` in Ubuntu 20
  - [spdlog: Fast C++ logging library](https://github.com/gabime/spdlog)
    - `libspdlog-dev` in Ubuntu

and can use either external system-supplied versions or internal copies
provided by submodules.

Building is usually as simple as:

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=<installdir> <path to sources>
    make -j<number of cores on your machine>
    make install

Unless otherwise specified, the default behavior for dependencies is to first
try to find an external version and fall back to the internal submodule copy
if not found.  To explicitly force the use of either add any of the following
configure options:
  - `-DUSE_EXTERNAL_NLOHMANN_JSON={TRUE,FALSE,`**`AUTO`**`}`
    - `ON`, `TRUE`: Force an external version and fail if not found.
    - `OFF`, `FALSE`: Require the internal copy be used.
    - **`AUTO`**: Search for an external version and fall back to the
       internal copy if not found.
  - `-Dnlohmann_json_DIR=/path/to/config`
    - The path to the directory containing `nlohmann_jsonConfig.cmake`.
      Necessary if nlohmann_json is not installed into CMake's search path.
  - `-DUSE_EXTERNAL_SPDLOG={TRUE,FALSE,`**`AUTO`**`}`
    - `ON`, `TRUE`: Force an external version and fail if not found.
    - `OFF`, `FALSE`: Require the internal copy be used.
    - **`AUTO`**: Search for an external version and fall back to the
       internal copy if not found.
  - `-Dspdlog_DIR=/path/to/config`
    - The path to the directory containing `spdlogConfig.cmake`.
      Necessary if spdlog is not installed into CMake's search path.

The option `-DCMAKE_BUILD_TYPE` can switch between all of the standard
build types. The default is `Release`; use `RelWithDebInfo` if you want
debug symbols.

To build a statically linked version of `prmon`, set the `BUILD_STATIC`
CMake variable to `ON` (e.g., adding `-DBUILD_STATIC=ON` to the
command line).

Note that in a build environment with CVMFS available the C++ compiler
and CMake can be taken by setting up a recent LCG release.

To enable pulling and building the gtest framework as well as tests dependent on gtest,
build with `-DBUILD_GTESTS=ON`.

### Creating a package with CPack

A cpack based package can be created by invoking

    make package

### Running the tests

To run the tests of the project, first build it and then invoke

    make test

Running the tests requires Python version 3.6 or higher.

## Running

The `prmon` binary is invoked with the following arguments:

```sh
prmon [--pid PPP] [--filename prmon.txt] [--json-summary prmon.json] \
      [--log-filename prmon.log] [--interval 30] \
      [--suppress-hw-info] [--units] [--netdev DEV] \
      [--disable MON1] [--level LEV] [--level MON:LEV] \
      [--fast-memmon] \
      [-- prog arg arg ...]
```

* `--pid` the 'mother' PID to monitor (all children in the same process tree are monitored as well)
* `--filename` output file for time-stamped monitored values
* `--json-summmary` output file for summary data written in JSON format
* `--log-filename` output file for log messages
* `--interval` time, in seconds, between monitoring snapshots
* `--suppress-hw-info` flag that turns-off hardware information collection
* `--units` add information on units for each metric to JSON file
* `--netdev` restricts network statistics to one (or more) network devices
* `--disable` is used to disable specific monitors (and can be specified multiple times); 
    the default is that `prmon` monitors everything that it can
  * Note that the `wallmon` monitor is the only monitor that cannot be disabled
* `--level` is used to set the logging level for monitors
  * `--level LEV` sets the level for all monitors to LEV
  * `--level MON:LEV` sets the level for monitor MON to LEV
  * The valid levels are `trace`, `debug`, `info`, `warn`, `error`, `critical`
* `--fast-memmon` toggles on fast memory monitoring using `smaps_rollup`
* `--` after this argument the following arguments are treated as a program to invoke
  and remaining arguments are passed to it; `prmon` will then monitor this process
  instead of being given a PID via `--pid`

`prmon` will exit with `1` if there is a problem with inconsistent or 
incomplete arguments. If `prmon` starts a program itself (using `--`) then
`prmon` will exit with the exit code of the child process.

When invoked with `-h` or `--help` usage information is printed, as well as a
list of all available monitoring components.

### Fast Memory Monitoring

When invoked with `--fast-memmon` `prmon` uses the `smaps_rollup` files
that contain pre-summed memory information for each monitored process.
This is a faster approach compared to the default behavior,
where `prmon` aggregates the results itself by going over each of the monitored
processes' mappings one by one.

If the current kernel doesn't support `smaps_rollup` then the default
approach is used. Users should also note that fast memory monitoring
might not contain all metrics that the default approach supports, e.g.,
`vmem`. In that case, the missing metric will be omitted in the output.
If any of these issues are encountered, a relevant message is printed
to notify the user.

### Environment Variables

The `PRMON_DISABLE_MONITOR` environment variable can be used to specify a comma
separated list of monitor names that will be disabled. This is useful when
`prmon` is being invoked by some other part of a job or workflow, so the user
does not have direct access to the command line options used. e.g.

```sh
export PRMON_DISABLE_MONITOR=nvidiamon
other_code_that_invokes_prmon
...
```

Disables the `nvidiamon` monitor.

## Outputs

In the `filename` output file, plain text with statistics written every
`interval` seconds are written. The first line gives the column names.

In the `json-summary` file values for the maximum and average statistics
are given in JSON format. This file is rewritten every `interval` seconds
with the current summary values. Use the `--units` option to see exactly
which units are used for each metric (the value of `1` for a unit means
it is a pure number).

In the `log-filename` output file, log messages (e.g., errors, warnings etc.)
are written.

Monitoring of CPU, I/O and memory is reliably accurate, at least to within
the sampling time. Monitoring of network I/O is **not reliable** unless the
monitored process is isolated from other processes performing network I/O
(it gives an upper bound on the network activity, but the monitoring is
per network device as Linux does not give per-process network data by
default).

### Visualisation

The `prmon_plot.py` script (Python3) can be used to plot the outputs of prmon from the
timestamped output file (usually `prmon.txt`). Some examples include:

* Memory usage as a function of wall-time:
```sh
prmon_plot.py --input prmon.txt --xvar wtime --yvar vmem,pss,rss,swap --yunit GB
```
![](example-plots/PrMon_wtime_vs_vmem_pss_rss_swap.png)

* Rate of change in memory usage as a function of wall-time:
```sh
prmon_plot.py --input prmon.txt --xvar wtime --yvar vmem,pss,rss,swap --diff --yunit MB
```
![](example-plots/PrMon_wtime_vs_diff_vmem_pss_rss_swap.png)

* Rate of change in CPU usage as a function of wall-time with stacked
user and system utilizations:
```sh
prmon_plot.py --input prmon.txt --xvar wtime --yvar utime,stime --yunit SEC --diff --stacked
```
![](example-plots/PrMon_wtime_vs_diff_utime_stime.png)

The plots above, as well as the input `prmon.txt` file that is used
to produce them, can be found under the `example-plots` folder.

The script allows the user to specify variables, their units, plotting
style (stacked vs overlaid), as well as the format of the output image.
Use `-h` for more information.


### Data Compression

The `prmon_compress_output.py` script (Python3) can be used to compress the output file
while keeping the most relevant information.

The compression algorithm works as follows:
* For the number of processes, threads, and GPUs, only the measurements that are different with respect to the previous ones are kept.
* For all other metrics, only the measurements that satisfy an interpolation condition are kept.

This latter condition can be summarized as:
* For any three neighboring (and time-ordered) measurements, A, B, and C, B is deleted if the linear interpolation between A and C is consistent with B ± *threshold*. Otherwise, it's retained. The *threshold* can be configured via the `--precision` parameter (default: 0.05, i.e. 5%)


The time index of the final output will be the union of the algorithm outputs of the single 
time series. Each series will have NA values where a point was deleted at a kept index and, unless otherwise
specified by the `--skip-interpolate` parameter, will be linearly interpolated to maintain a consistent number of data points
and the result will be rounded to the nearest integer for consistency with the original input.

If the `--skip-interpolate` parameter is passed, deleted values will be written as empty strings in the output file, and will be interpreted
as `NA` values when imported into Pandas.

Example:
```sh
prmon_compress_output.py --input prmon.txt --precision 0.3 --skip-interpolate
```


## Feedback and Contributions

We're very happy to get feedback on prmon as well as suggestions for future
development. Please have a look at our [Contribution
Guide](doc/CONTRIBUTING.md).


### Profiling

To build prmon with profiling, set one of the CMake variables
`PROFILE_GPROF` or `PROFILE_GPERFTOOLS` to `ON`. This enables
GNU prof profiling or gperftools profiling, respectively.
If your gperftools are in a non-standard place, pass a hint
to CMake using `Gperftools_ROOT_DIR`.


# Copyright

Copyright (c) 2018-2024 CERN.
