#ifndef CPUINFO_H
#define CPUINFO_H

#include <vector>

namespace cpuinfo {
  // Method to get the total number of processors
  int get_number_of_cpus();
  // Method to get the processor clock freqs
  std::vector<float> get_processor_clock_freqs();
  // Method to get the scaling information
  // 0 for false, 1 for true, 2 for error
  unsigned int cpu_scaling_info(int nCPUs);
}

#endif // CPUINFO_H
