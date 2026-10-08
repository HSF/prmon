# Process Monitor (prmon)
<!-- ALL-CONTRIBUTORS-BADGE:START - Do not remove or modify this section -->
[![All Contributors](https://img.shields.io/badge/all_contributors-14-orange.svg?style=flat-square)](#contributors-)
<!-- ALL-CONTRIBUTORS-BADGE:END -->

This fork of prmon aims to test two additional collectors (curlmon and machmon) that can add information regarding power consumption. The idea stems from a proposal to improve job power consumption monitoring via an estimation of the usage of the machine by a given job.

# machmon
Similarly to cpumon, this collector reads the cpu times of the whole machine (utime, stime and ntime). By comparing pidtimes with machtimes, we can compute a ratio that estimates the fraction of the machine that is being used by the monitored process.
$f_{usage} = \frac{t_{PID}}{t_{total}}$

# curlmon
On machines with prometheus exporters running, this collector is able to read their output via curl and store different power consumption metrics: DCMI, IPMI, RAPL and HWMON. These metrics together with $f_{usage}$ provides a precise estimation of the power consumption attributable to the monitored process.
