/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "../test-common.hpp"
#include "core/logger.hpp"
#include "mgmt/config-file.hpp"

#include <boost/filesystem.hpp>

namespace nfd {
namespace tests {

class GlobalConfigurationFixture
{
public:
  GlobalConfigurationFixture()
  {
    const std::string filename = "unit-tests.conf";
    if (boost::filesystem::exists(filename))
      {
        ConfigFile config;
        LoggerFactory::getInstance().setConfigFile(config);

        config.parse(filename, false);
      }
  }
};

BOOST_GLOBAL_FIXTURE(GlobalConfigurationFixture)

} // namespace tests
} // namespace nfd
