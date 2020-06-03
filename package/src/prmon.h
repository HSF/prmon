// Copyright (C) 2020, CERN

#include <sys/types.h>
#include <vector>
#include <string>

int MemoryMonitor(pid_t mpid, char* filename, char* jsonSummary,
                  unsigned int interval, const bool store_hw_info,
                  const std::vector<std::string> netdevs);
