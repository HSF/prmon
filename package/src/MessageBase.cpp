#include "MessageBase.h"

#include "spdlog/spdlog.h"

void MessageBase::log_init(const std::string& classname,
                           const spdlog::level::level_enum& level) {
  // Initialise sink list
  spdlog::sinks_init_list s_list = {c_sink, f_sink};

  // Use the sink list to create multi sink logger
  logger =
      std::make_shared<spdlog::logger>(classname, s_list.begin(), s_list.end());
  log_level = level;
  set_log_level(level);
  logger->flush_on(level);
  spdlog::register_logger(logger);
  info(classname + " logger initialised!");
}

void MessageBase::set_log_level(const spdlog::level::level_enum& level) {
  log_level = level;
  logger->set_level(level);
  logger->flush_on(level);
}
void MessageBase::debug(const std::string& message) { logger->debug(message); }
void MessageBase::info(const std::string& message) { logger->info(message); }
void MessageBase::warning(const std::string& message) { logger->warn(message); }
void MessageBase::error(const std::string& message) { logger->error(message); }
