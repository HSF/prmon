# Adding a New Monitoring Component

Please take a look at the [contribution guide](CONTRIBUTING.md) first!

## Prmon Monitors

All of the prmon monitors are concrete implementations of the virtual
`Imonitor.h` interface. Inside the `package/src` directory you will find all of
the current examples and these are an excellent guide.

## `parameter.h`

All monitors need to include this header, which defines utility classes and
standard types used.

### `parameter` class

This class defines a few properties that are needed for any value that is going
to be measured by prmon. It consists of three strings:
- name of parameter
- units for maximum/continuous value
- units for average value
If the later is an empty string it means that there is no meaningful value
for the average and so nothing will be output for an average of this stat.

Monitors usually define a `const prmon::parameter_list`, which is a vector
describing all of the parameters that they will measure.

### `monitored_value` class

This class represents a monitored value as measured by a concrete monitor. This
is initialised from a `parameter` class, plus a boolean flag that signals if the
monitored quanitity is *monotonic*, i.e., can only increase over time. A third,
optional, parameter gives an *offset* value that will be subtracted from the
"input" values. This is useful when a statistic does not start from zero at the
beginning of the job, e.g., network counter statistics.

The class provides accessors for current, maximum and average values of a
statistic. There is a setter, `set_value` that should be used to update the
value of the statistic on each monitoring cycle. There is also a setter for the
offset, `set_offset` that should only be called before any values have been
monitored.

`set_value()` will protect a monotonic value from being lowered and prevent any
attempt to set a value less than any offset.

Monitors generally hold a `prmon::monitored_list`, which is a map from the
string name of the monitored value to its `monitored_value` instance. In the
constuctor of the monitor the `monitored_list` is initialised from the
corresponding `const parameter_list`.

### Data Structures and Types

Every monitor accumulates statistics into maps, with keys corresponding to the
parameter names (from `parameter` classes). To ensure consistency and readabity
these maps are defined as `prmon::monitored_value_map` for snapshot and
accumulated statistics and `prmon::monitored_average_map` for average statistics
(the snapshots values are `mon_value`, which is itself defined as
`unsigned long long`; the averages are `mon_average`, which are `double`).

Monitors update their parameters in the `monitored_list` and use the getters to
construct and return the appropriate data structure to the main `prmon` loop.

## Initialisation

Use an RAII pattern, so that on initialisation the monitor is valid and ready
to be used.

For most monitors the `monitored_list` is initalised from the corresponding
`const parameter_list`.

## Registry

All monitors should use the `REGISTER_MONITOR` macro to register themselves. The
signature is:

```
REGISTER_MONITOR(Imonitor, classname, "Description")
```

The first parameter is currently always `Imonitor` (base class), the second is
the class name and the third is a short text string describing what the monitor
does.

## Updates

Each monitoring cycle active monitors are called using their `update_stats()`
method. A const vector of process identifiers will be passed in, allowing
monitors to gather statistics only for processes that are part of the monitored
job. For each relevant statistic the `set_value()` setter of the
`monitored_value` class will be called.

## Reporting Statistics

There are three statistics reports that your monitor should provide:

### Current Values

Polled from `get_text_stats()`, pass back a `prmon::monitored_value_map`.

These statistics will be fed into the *text* output file from prmon.

### Maximum Values

Polled from `get_json_total_stats()`, pass back a `prmon::monitored_value_map`.
These statistics are *peak values*, which are the same as the *current values*
for statistics that simply accumulate over time (e.g., user cpu seconds used or
bytes read), but are *maximums* in the case of statistics that rise and fall
(e.g., number of threads).

These statistics will be fed into the `Max` dictionary element of the JSON
output from prmon.

### Average Values

Polled from `get_json_average_stats(elapsed_clock_ticks)`, pass back a map of
`prmon::monitored_average_map`. These statistics are *average values* for the
metric. What than means depends on the statistic (e.g., the memory consumption
over the job lifetime or the number of bytes written per second). If an average
makes no real sense for what you measure then exclude that metric from the
values returned (some monitors report nothing here).

These statistics will be fed into the `Avg` dictionary element of the JSON
output from prmon.

## Hardware Polling

If your monitor provides interesting data on the hardware on which is it
running, it can contribute to the optional report on hardware. This report is
provided as a JSON structure, that will be passed by reference into the
`get_hardware_info()` method. This is a shared object so always create a
subsection in the JSON dictionary for data from your monitor.

If your monitor does not provide hardware data, simply return.
