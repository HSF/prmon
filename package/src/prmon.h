/*
  Copyright (C) 2018, CERN
*/

#include <sys/types.h>
#include <vector>
#include <string>

int MemoryMonitor(pid_t mpid, char* filename, char* jsonSummary,
                  unsigned int interval,
                  const std::vector<std::string> netdevs);
