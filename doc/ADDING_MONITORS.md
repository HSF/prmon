# Adding a New Monitoring Component

Please take a look at the [contribution guide](CONTRIBUTING.md) first!

## Prmon Monitors

All of the prmon monitors are concrete implementations of the virtual
`Imonitor.h` interface. Inside the `package/src` directory you will find all of
the current examples and these are an excellent guide.

## `util.h`

Every monitor describes its column headers (*what* it will monitor), so add
short, descriptive names for the columns you will output.

## Data Structures

Every monitor accumulates statistics into maps, with keys corresponding to the
column header names in `util.h`. Accumulated statistics are `unsigned long long`
values and average statistics are `double` values.

Most monitors keep statistics counters, for accumulation over time, as well as
maximum values and average values. These are updated each update cycle.

## Initialisation

Use an RAII pattern, so that on initialisation your monitor is valid and ready
to be used.

## Registry

All monitors should use the `REGISTER_MONITOR` marco to register themselves. The
signature is:

```
REGISTER_MONITOR(Imonitor, name, "Description")
```

The first parameter is currently always `Imonitor` (base class), the second is
the class name and the third is a short text string describing what the monitor
does.

## Updates

Each monitoring cycle active monitors are called using their `update_stats()`
method. A const vector of process identifiers will be passed in, allowing
monitors to gather statistics only for processes that are part of the monitored
job.

## Reporting Statistics

There are three statistics reports that your monitor should provide:

### Current Values

Polled from `get_text_stats()`, pass back a map of string keys (column headers)
and integral values (`unsigned long long`).

These statistics will be fed into the *text* output file from prmon.

### Maximum Values

Polled from `get_json_total_stats()`, pass back a map of string keys (column
headers) and integral values (`unsigned long long`). These statistics are *peak
values*, which are the same as the *current values* for statistics that simply
accumulate over time (e.g., user cpu seconds used or bytes read), but are
*maximums* in the case of statistics that rise and fall (e.g., number of
threads).

These statistics will be fed into the `Max` dictionary element of the JSON output from prmon.

### Average Values

Polled from `get_json_average_stats(elapsed_clock_ticks)`, pass back a map of
string keys (column headers) and floating point values (`double`). These
statistics are *average values* for the metic. What than means depends on the
statistic (e.g., the memory consumption over the job lifetime or the number of
bytes written per second). If an average makes no real sense for what you
measure then exclude that metric from the values returned (some monitors report
nothing here).

These statistics will be fed into the `Avg` dictionary element of the JSON
output from prmon.

## Hardware Polling

If your monitor provides interesting data on the hardware on which is it
running, it can contribute to the optional report on hardware. This report is
provided as a JSON structure, that will be passed by refernce into the
`get_hardware_info()` method. This is a shared object so always create a
subsection in the JSON dictionary for data from your monitor.

If your monitor dßoes not provide hardware data, simply return.
