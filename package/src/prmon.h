/*
  Copyright (C) 2018, CERN
*/

#include <sys/types.h>
#include <vector>
#include <string>

int ProcessMonitor(pid_t mpid, char* filename, char* jsonSummary,
                  unsigned int interval, bool store_cpu_freq,
                  const std::vector<std::string> netdevs);
