/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "test-common.hpp"
#include "core/logger.hpp"
#include "core/config-file.hpp"

#include <boost/filesystem.hpp>
#include <fstream>

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

    if (std::getenv("HOME"))
      m_home = std::getenv("HOME");

    boost::filesystem::path dir(UNIT_TEST_CONFIG_PATH "test-home");
    setenv("HOME", dir.generic_string().c_str(), 1);
    boost::filesystem::create_directories(dir);
    std::ofstream clientConf((dir / ".ndn" / "client.conf").generic_string().c_str());
    clientConf << "pib=pib-sqlite3\n"
               << "tpm=tpm-file" << std::endl;
  }

  ~GlobalConfigurationFixture()
  {
    if (!m_home.empty()) {
      setenv("HOME", m_home.c_str(), 1);
    }
  }

private:
  std::string m_home;
};

BOOST_GLOBAL_FIXTURE(GlobalConfigurationFixture)
#if BOOST_VERSION >= 105900
;
#endif // BOOST_VERSION >= 105900

} // namespace tests
} // namespace nfd
