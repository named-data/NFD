/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "core/common.hpp"

#include "tests/boost-test.hpp"

#include <boost/filesystem.hpp>
#include <fstream>
#include <stdlib.h>

namespace nfd {
namespace tests {

class GlobalConfiguration
{
public:
  GlobalConfiguration()
  {
    const char* envHome = ::getenv("HOME");
    if (envHome)
      m_home = envHome;

    auto testHome = boost::filesystem::path(UNIT_TEST_CONFIG_PATH) / "test-home";
    if (::setenv("HOME", testHome.c_str(), 1) != 0)
      NDN_THROW(std::runtime_error("setenv() failed"));

    boost::filesystem::create_directories(testHome);

    std::ofstream clientConf((testHome / ".ndn" / "client.conf").c_str());
    clientConf << "pib=pib-sqlite3" << std::endl
               << "tpm=tpm-file" << std::endl;
  }

  ~GlobalConfiguration() noexcept
  {
    if (m_home.empty())
      ::unsetenv("HOME");
    else
      ::setenv("HOME", m_home.data(), 1);
  }

private:
  std::string m_home;
};

#if BOOST_VERSION >= 106500
BOOST_TEST_GLOBAL_CONFIGURATION(GlobalConfiguration);
#elif BOOST_VERSION >= 105900
BOOST_GLOBAL_FIXTURE(GlobalConfiguration);
#else
BOOST_GLOBAL_FIXTURE(GlobalConfiguration)
#endif

} // namespace tests
} // namespace nfd
