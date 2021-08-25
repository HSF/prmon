#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

int main() {
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
  auto fsink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("sample.log", true);
  spdlog::sinks_init_list sink_list = {sink, fsink};
  auto logger = std::make_shared<spdlog::logger>("benchmark", sink_list.begin(),
                                                 sink_list.end());
  logger->set_level(spdlog::level::warn);
  const int maxN = 1e6;
  for (int i = 0; i < maxN; ++i) {
    logger->warn("Hello, benchmarking currently");
  }
  return 0;
}
