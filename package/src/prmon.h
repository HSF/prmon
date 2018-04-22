/*
  Copyright (C) 2018, CERN
*/

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>

int ReadProcs(const std::vector<pid_t>& cpids, unsigned long values[4],
              const bool verbose = false);
int MemoryMonitor(pid_t mpid, char* filename, char* jsonSummary,
                  unsigned int interval,
                  const std::vector<std::string> netdevs);
