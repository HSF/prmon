// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "utils.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>

const std::pair<int, std::vector<std::string>> prmon::cmd_pipe_output(
    const std::vector<std::string> cmdargs) {
  std::vector<std::string> split_output{};
  std::string output;
  std::pair<int, std::vector<std::string>> ret{0, split_output};

  int fd[2];
  if (pipe(fd) == -1) {
    // Pipe failed
    ret.first = 2;
    return ret;
  }

  pid_t p;
  p = fork();
  if (p < 0) {
    // Fork failed
    ret.first = 3;
    return ret;
  }

  if (p > 0) {
    // Parent
    close(fd[1]);
  } else {
    // Child
    close(fd[0]);
    dup2(fd[1], STDOUT_FILENO);
    dup2(fd[1], STDERR_FILENO);

    // Construct the c-ish char*s needed for execvp
    const char* cmd = cmdargs[0].c_str();
    char** argv = (char**)malloc((cmdargs.size() + 1) * sizeof(char*));
    if (!argv) exit(EXIT_FAILURE);
    for (size_t i = 0; i < cmdargs.size(); ++i) {
      argv[i] = (char*)cmdargs[i].c_str();
    }
    argv[cmdargs.size()] = NULL;

    execvp(cmd, argv);
    // If we got here then something went wrong and exec failed
    // need to signal this back to our parent
    std::cerr << "Exec ERROR: " << errno << std::endl;
    free(argv);
    exit(EXIT_FAILURE);
    // Child
  }

  // Read the output from the executed command and collect
  // it into the output vector of strings
  const size_t buf_len = 100;
  char buffer[buf_len];
  FILE* inp = fdopen(fd[0], "r");
  while (fgets(buffer, buf_len, inp) != NULL) {
    // fgets stops on newlines, so use that to help split
    // the output into the vector of strings
    if ((strlen(buffer) > 0) && (buffer[strlen(buffer) - 1] == '\n')) {
      buffer[strlen(buffer) - 1] = '\0';
      output.append(buffer);
      split_output.push_back(output);
      output.clear();
    } else {
      output.append(buffer);
    }
  }
  if (!output.empty()) {
    split_output.push_back(output);
  }
  fclose(inp);
  ret.second = split_output;

  int status = 0;
  waitpid(p, &status, 0);
  if (status) {
    // Something went wrong in childland...
    ret.first = 1;
    return ret;
  }

  return ret;
}

const void prmon::fill_units(nlohmann::json& unit_json,
                             const parameter_list& params) {
  for (const auto& param : params) {
    if (!param.get_max_unit().empty())
      unit_json["Units"]["Max"][param.get_name()] = param.get_max_unit();
    if (!param.get_avg_unit().empty())
      unit_json["Units"]["Avg"][param.get_name()] = param.get_avg_unit();
  }
  return;
}

const bool prmon::smaps_rollup_exists() {
  struct stat buffer;
  return (stat("/proc/self/smaps_rollup", &buffer) == 0);
}
