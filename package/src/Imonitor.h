// Copyright (C) CERN, 2018
//
// Network monitoring interface class
//
// This is the interface class for all concrete monitoring
// classes

#ifndef PRMON_IMONITOR_H
#define PRMON_IMONITOR_H 1

#include <string>
#include <vector>

class Imonitor {
 public:
  virtual ~Imonitor() {};

  virtual std::vector<std::string> const get_text_headers() = 0;
  virtual std::vector<std::string> const get_json_keys() = 0;

  virtual std::unordered_map<std::string, unsigned long long> read_stats() = 0;
  virtual void read_stats(std::unordered_map<std::string, unsigned long long>& stats) = 0;
};

#endif  // PRMON_IMONITOR_H
