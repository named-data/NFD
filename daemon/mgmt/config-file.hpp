/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_CONFIG_FILE_HPP
#define NFD_MGMT_CONFIG_FILE_HPP

#include "common.hpp"

#include <boost/property_tree/ptree.hpp>

namespace nfd {

typedef boost::property_tree::ptree ConfigSection;

/// \brief callback for config file sections
typedef function<void(const ConfigSection&, bool, const std::string&)> ConfigSectionHandler;

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

  ConfigFile();

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
   * \param filename optional convenience argument to provide more detailed error messages
   * \throws ConfigFile::Error if file not found
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const std::string& input, bool isDryRun, const std::string& filename);

  /**
   * \param input stream to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename optional convenience argument to provide more detailed error messages
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(std::istream& input, bool isDryRun, const std::string& filename);

private:

  void
  process(bool isDryRun, const std::string& filename);

private:

  typedef std::map<std::string, ConfigSectionHandler> SubscriptionTable;

  SubscriptionTable m_subscriptions;

  ConfigSection m_global;
};

} // namespace nfd


#endif // NFD_MGMT_CONFIG_FILE_HPP
