#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

class Logger {
public:
  enum class Level {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
  };

  static Logger& instance();

  void init(const fs::path& log_file = "", Level level = Level::Info, bool console_output = true);
  
  void set_level(Level level);
  Level get_level() const;

  template<typename... Args>
  void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (logger_) logger_->trace(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (logger_) logger_->debug(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (logger_) logger_->info(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (logger_) logger_->warn(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (logger_) logger_->error(fmt, std::forward<Args>(args)...);
  }

  template<typename... Args>
  void critical(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (logger_) logger_->critical(fmt, std::forward<Args>(args)...);
  }

  void flush();
  void shutdown();

private:
  Logger() = default;
  ~Logger();
  
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  std::shared_ptr<spdlog::logger> logger_;
  Level current_level_ = Level::Info;
};

#define LOG_TRACE(...) Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...) Logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::instance().critical(__VA_ARGS__)
