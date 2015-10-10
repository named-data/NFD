/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_CORE_CONFIG_FILE_HPP
#define NFD_CORE_CONFIG_FILE_HPP

#include "common.hpp"

#include <boost/property_tree/ptree.hpp>

namespace nfd {

typedef boost::property_tree::ptree ConfigSection;

/// \brief callback for config file sections
typedef function<void(const ConfigSection& /*section*/,
                      bool /*isDryRun*/,
                      const std::string& /*filename*/)> ConfigSectionHandler;

/// \brief callback for config file sections without a subscribed handler
typedef function<void(const std::string& /*filename*/,
                      const std::string& /*sectionName*/,
                      const ConfigSection& /*section*/,
                      bool /*isDryRun*/)> UnknownConfigSectionHandler;

class ConfigFile : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  explicit
  ConfigFile(UnknownConfigSectionHandler unknownSectionCallback = throwErrorOnUnknownSection);

  static void
  throwErrorOnUnknownSection(const std::string& filename,
                             const std::string& sectionName,
                             const ConfigSection& section,
                             bool isDryRun);

  static void
  ignoreUnknownSection(const std::string& filename,
                       const std::string& sectionName,
                       const ConfigSection& section,
                       bool isDryRun);

  /**
   * \brief parse a config option that can be either "yes" or "no"
   *
   * \return true if "yes", false if "no"
   * \throw Error the value is neither "yes" nor "no"
   */
  static bool
  parseYesNo(const ConfigSection::value_type& option,
             const std::string& sectionName);

  /**
   * \brief parse a numeric (integral or floating point) config option
   *
   * \return the numeric value of the parsed option
   * \throw Error the value cannot be converted to the specified type
   */
  template <typename T>
  static typename std::enable_if<std::is_arithmetic<T>::value, T>::type
  parseNumber(const ConfigSection::value_type& option,
              const std::string& sectionName);

  /// \brief setup notification of configuration file sections
  void
  addSectionHandler(const std::string& sectionName,
                    ConfigSectionHandler subscriber);

  /**
   * \param filename file to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \throws ConfigFile::Error if file not found
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const std::string& filename, bool isDryRun);

  /**
   * \param input configuration (as a string) to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if file not found
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const std::string& input, bool isDryRun, const std::string& filename);

  /**
   * \param input stream to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(std::istream& input, bool isDryRun, const std::string& filename);

  /**
   * \param config ConfigSection that needs to be processed
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const ConfigSection& config, bool isDryRun, const std::string& filename);

private:
  void
  process(bool isDryRun, const std::string& filename) const;

private:
  UnknownConfigSectionHandler m_unknownSectionCallback;
  std::map<std::string, ConfigSectionHandler> m_subscriptions;
  ConfigSection m_global;
};

template <typename T>
inline typename std::enable_if<std::is_arithmetic<T>::value, T>::type
ConfigFile::parseNumber(const ConfigSection::value_type& option,
                        const std::string& sectionName)
{
  auto value = option.second.get_value<std::string>();

  try {
    return boost::lexical_cast<T>(value);
  }
  catch (const boost::bad_lexical_cast&) {
    BOOST_THROW_EXCEPTION(Error("Invalid value \"" + value + "\" for option \"" +
                                option.first + "\" in \"" + sectionName + "\" section"));
  }
}

} // namespace nfd

#endif // NFD_CORE_CONFIG_FILE_HPP
