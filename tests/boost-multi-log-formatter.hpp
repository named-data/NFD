/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 Regents of the University of California.
 *
 * Based on work by Martin Ba (http://stackoverflow.com/a/26718189)
 *
 * This file is distributed under the Boost Software License, Version 1.0.
 * (See http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef NDN_TESTS_BOOST_MULTI_LOG_FORMATTER_HPP
#define NDN_TESTS_BOOST_MULTI_LOG_FORMATTER_HPP

#include <boost/version.hpp>

#if BOOST_VERSION >= 105900
#include <boost/test/unit_test_parameters.hpp>
#else
#include <boost/test/detail/unit_test_parameters.hpp>
#endif // BOOST_VERSION >= 105900

#include <boost/test/unit_test_log_formatter.hpp>
#include <boost/test/output/compiler_log_formatter.hpp>
#include <boost/test/output/xml_log_formatter.hpp>

namespace boost {
namespace unit_test {
namespace output {

/**
 * @brief Log formatter for Boost.Test that outputs the logging to multiple formatters
 *
 * The log formatter is designed to output to one or multiple formatters at the same time.  For
 * example, one HRF formatter can output to the standard output, while XML formatter output to
 * the file.
 *
 * Usage:
 *
 *     // Call in init_unit_test_suite: (this will override the --log_format parameter)
 *     auto formatter = new boost::unit_test::output::multi_log_formatter; // same as already configured logger
 *
 *     // Prepare and add additional logger(s)
 *     formatter.add(std::make_shared<boost::unit_test::output::xml_log_formatter>(),
 *                   std::make_shared<std::ofstream>("out.xml"));
 *
 *      boost::unit_test::unit_test_log.set_formatter(formatter);
 *
 * @note Calling `boost::unit_test::unit_test_log.set_stream(...)` will change the stream for
 *       the original logger.
 */
class multi_log_formatter : public unit_test_log_formatter
{
public:
  /**
   * @brief Create instance of the logger, based on the configured logger instance
   */
  multi_log_formatter()
  {
    auto format =
#if BOOST_VERSION > 105900
      runtime_config::get<output_format>(runtime_config::LOG_FORMAT);
#else
      runtime_config::log_format();
#endif // BOOST_VERSION > 105900

    switch (format) {
      default:
#if BOOST_VERSION >= 105900
      case OF_CLF:
#else
      case CLF:
#endif // BOOST_VERSION >= 105900
        m_loggers.push_back({std::make_shared<compiler_log_formatter>(), nullptr});
        break;
#if BOOST_VERSION >= 105900
      case OF_XML:
#else
      case XML:
#endif // BOOST_VERSION >= 105900
        m_loggers.push_back({std::make_shared<xml_log_formatter>(), nullptr});
        break;
    }
  }

  void
  add(std::shared_ptr<unit_test_log_formatter> formatter, std::shared_ptr<std::ostream> os)
  {
    m_loggers.push_back({formatter, os});
  }

  // Formatter interface
  void
  log_start(std::ostream& os, counter_t test_cases_amount)
  {
    for (auto& l : m_loggers)
      l.logger->log_start(l.os == nullptr ? os : *l.os, test_cases_amount);
  }

  void
  log_finish(std::ostream& os)
  {
    for (auto& l : m_loggers)
      l.logger->log_finish(l.os == nullptr ? os : *l.os);
  }

  void
  log_build_info(std::ostream& os)
  {
    for (auto& l : m_loggers)
      l.logger->log_build_info(l.os == nullptr ? os : *l.os);
  }

  void
  test_unit_start(std::ostream& os, const test_unit& tu)
  {
    for (auto& l : m_loggers)
      l.logger->test_unit_start(l.os == nullptr ? os : *l.os, tu);
  }

  void
  test_unit_finish(std::ostream& os, const test_unit& tu, unsigned long elapsed)
  {
    for (auto& l : m_loggers)
      l.logger->test_unit_finish(l.os == nullptr ? os : *l.os, tu, elapsed);
  }

  void
  test_unit_skipped(std::ostream& os, const test_unit& tu)
  {
    for (auto& l : m_loggers)
      l.logger->test_unit_skipped(l.os == nullptr ? os : *l.os, tu);
  }

#if BOOST_VERSION >= 105900
  void
  log_exception_start(std::ostream& os, const log_checkpoint_data& lcd, const execution_exception& ex)
  {
    for (auto& l : m_loggers)
      l.logger->log_exception_start(l.os == nullptr ? os : *l.os, lcd, ex);
  }

  void
  log_exception_finish(std::ostream& os)
  {
    for (auto& l : m_loggers)
      l.logger->log_exception_finish(l.os == nullptr ? os : *l.os);
  }
#else
  void
  log_exception(std::ostream& os, const log_checkpoint_data& lcd, const execution_exception& ex)
  {
    for (auto& l : m_loggers)
      l.logger->log_exception(l.os == nullptr ? os : *l.os, lcd, ex);
  }
#endif // BOOST_VERSION >= 105900

  void
  log_entry_start(std::ostream& os, const log_entry_data& entry_data, log_entry_types let)
  {
    for (auto& l : m_loggers)
      l.logger->log_entry_start(l.os == nullptr ? os : *l.os, entry_data, let);
  }

  void
  log_entry_value(std::ostream& os, const_string value)
  {
    for (auto& l : m_loggers)
      l.logger->log_entry_value(l.os == nullptr ? os : *l.os, value);
  }

  void
  log_entry_finish(std::ostream& os)
  {
    for (auto& l : m_loggers)
      l.logger->log_entry_finish(l.os == nullptr ? os : *l.os);
  }

#if BOOST_VERSION >= 105900
  void
  entry_context_start(std::ostream& os, log_level level)
  {
    for (auto& l : m_loggers)
      l.logger->entry_context_start(l.os == nullptr ? os : *l.os, level);
  }

  void
  log_entry_context(std::ostream& os, const_string value)
  {
    for (auto& l : m_loggers)
      l.logger->log_entry_context(l.os == nullptr ? os : *l.os, value);
  }

  void
  entry_context_finish(std::ostream& os)
  {
    for (auto& l : m_loggers)
      l.logger->entry_context_finish(l.os == nullptr ? os : *l.os);
  }
#endif // BOOST_VERSION >= 105900

private:
  struct LoggerInfo
  {
    std::shared_ptr<unit_test_log_formatter> logger;
    std::shared_ptr<std::ostream> os;
  };
  std::vector<LoggerInfo> m_loggers;
};

} // namespace output
} // namespace unit_test
} // namespace boost

#endif // NDN_TESTS_BOOST_MULTI_LOG_FORMATTER_HPP
