/*
  Copyright (C) 2018, CERN
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

int ReadProcs(const pid_t mother_pid, unsigned long values[4],
              unsigned long long valuesIO[4], unsigned long long valuesCPU[4],
              const bool verbose = false);
int MemoryMonitor(pid_t mpid, char* filename = NULL, char* jsonSummary = NULL,
                  unsigned int interval = 600);
