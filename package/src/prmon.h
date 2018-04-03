/*
  Copyright (C) 2018, CERN
*/

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>

int ReadProcs(const pid_t mother_pid, unsigned long values[4],
              unsigned long long valuesIO[4], unsigned long long valuesCPU[4],
              const bool verbose = false);
int MemoryMonitor(pid_t mpid, char* filename, char* jsonSummary,
                  unsigned int interval,
                  const std::vector<std::string> netdevs);
