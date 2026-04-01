#include "logger.hpp"
#include <spdlog/spdlog.h>

Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

void Logger::init(const fs::path& log_file, Level level, bool console_output) {
  std::vector<spdlog::sink_ptr> sinks;

  if (console_output) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    sinks.push_back(console_sink);
  }

  if (!log_file.empty()) {
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file.string(), 1024 * 1024 * 10, 3);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    sinks.push_back(file_sink);
  }

  if (sinks.empty()) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    sinks.push_back(console_sink);
  }

  logger_ = std::make_shared<spdlog::logger>("file_organizer", sinks.begin(), sinks.end());
  set_level(level);
  spdlog::register_logger(logger_);
}

void Logger::set_level(Level level) {
  current_level_ = level;
  if (!logger_) return;

  switch (level) {
    case Level::Trace:
      logger_->set_level(spdlog::level::trace);
      break;
    case Level::Debug:
      logger_->set_level(spdlog::level::debug);
      break;
    case Level::Info:
      logger_->set_level(spdlog::level::info);
      break;
    case Level::Warn:
      logger_->set_level(spdlog::level::warn);
      break;
    case Level::Error:
      logger_->set_level(spdlog::level::err);
      break;
    case Level::Critical:
      logger_->set_level(spdlog::level::critical);
      break;
    case Level::Off:
      logger_->set_level(spdlog::level::off);
      break;
  }
}

Logger::Level Logger::get_level() const {
  return current_level_;
}

void Logger::flush() {
  if (logger_) {
    logger_->flush();
  }
}

void Logger::shutdown() {
  if (logger_) {
    logger_->flush();
    spdlog::drop("file_organizer");
    logger_.reset();
  }
}

Logger::~Logger() {
  shutdown();
}
