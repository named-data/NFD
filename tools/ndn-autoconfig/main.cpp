/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "procedure.hpp"
#include "core/extended-error-message.hpp"
#include "core/scheduler.hpp"
#include "core/version.hpp"

#include <signal.h>
#include <string.h>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <ndn-cxx/net/network-monitor.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/scheduler-scoped-event-id.hpp>
#include <ndn-cxx/util/time.hpp>

// suppress warning caused by boost::program_options::parse_config_file
#ifdef __clang__
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif

// ndn-autoconfig is an NDN tool not an NFD tool, so it uses ndn::tools::autoconfig namespace.
// It lives in NFD repository because nfd-start can automatically start ndn-autoconfig in daemon mode.
namespace ndn {
namespace tools {
namespace autoconfig {

static const time::nanoseconds DAEMON_INITIAL_DELAY = time::milliseconds(100);
static const time::nanoseconds DAEMON_UNCONDITIONAL_INTERVAL = time::hours(1);
static const time::nanoseconds NETMON_DAMPEN_PERIOD = time::seconds(5);

namespace po = boost::program_options;

static void
usage(std::ostream& os,
      const po::options_description& opts,
      const char* programName)
{
  os << "Usage: " << programName << " [options]\n"
     << "\n"
     << opts;
}

static void
runDaemon(Procedure& proc)
{
  boost::asio::signal_set terminateSignals(proc.getIoService());
  terminateSignals.add(SIGINT);
  terminateSignals.add(SIGTERM);
  terminateSignals.async_wait([&] (const boost::system::error_code& error, int signalNo) {
    if (error) {
      return;
    }
    const char* signalName = ::strsignal(signalNo);
    std::cerr << "Exiting on signal ";
    if (signalName == nullptr) {
      std::cerr << signalNo;
    }
    else {
      std::cerr << signalName;
    }
    std::cerr << std::endl;
    proc.getIoService().stop();
  });

  util::Scheduler sched(proc.getIoService());
  util::scheduler::ScopedEventId runEvt(sched);
  auto scheduleRerun = [&] (time::nanoseconds delay) {
    runEvt = sched.scheduleEvent(delay, [&] { proc.runOnce(); });
  };

  proc.onComplete.connect([&] (bool isSuccess) {
    scheduleRerun(DAEMON_UNCONDITIONAL_INTERVAL);
  });

  net::NetworkMonitor netmon(proc.getIoService());
  netmon.onNetworkStateChanged.connect([&] { scheduleRerun(NETMON_DAMPEN_PERIOD); });

  scheduleRerun(DAEMON_INITIAL_DELAY);
  proc.getIoService().run();
}

static int
main(int argc, char** argv)
{
  Options options;
  bool isDaemon = false;
  std::string configFile;

  po::options_description optionsDescription("Options");
  optionsDescription.add_options()
    ("help,h", "print this message and exit")
    ("version,V", "show version information and exit")
    ("daemon,d", po::bool_switch(&isDaemon)->default_value(isDaemon),
     "Run in daemon mode, detecting network change events and re-running the auto-discovery procedure. "
     "In addition, the auto-discovery procedure is unconditionally re-run every hour.\n"
     "NOTE: if the connection to NFD fails, the daemon will exit.")
    ("ndn-fch-url", po::value<std::string>(&options.ndnFchUrl)->default_value(options.ndnFchUrl),
     "URL for NDN-FCH (Find Closest Hub) service")
    ("config,c", po::value<std::string>(&configFile),
     "Configuration file. Exit immediately unless 'enabled = true' is specified in the config file.")
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, optionsDescription), vm);
    po::notify(vm);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n\n";
    usage(std::cerr, optionsDescription, argv[0]);
    return 2;
  }

  if (vm.count("help")) {
    usage(std::cout, optionsDescription, argv[0]);
    return 0;
  }

  if (vm.count("version")) {
    std::cout << NFD_VERSION_BUILD_STRING << std::endl;
    return 0;
  }

  if (vm.count("config")) {
    po::options_description configFileOptions;
    configFileOptions.add_options()
      ("enabled", po::value<bool>()->default_value(false))
      ;
    try {
      po::store(po::parse_config_file<char>(configFile.data(), configFileOptions), vm);
      po::notify(vm);
    }
    catch (const std::exception& e) {
      std::cerr << "ERROR in config: " << e.what() << "\n\n";
      return 2;
    }
    if (!vm["enabled"].as<bool>()) {
      // not enabled in config
      return 0;
    }
  }

  int exitCode = 0;
  try {
    Face face;
    KeyChain keyChain;
    Procedure proc(face, keyChain);
    proc.initialize(options);

    if (isDaemon) {
      runDaemon(proc);
    }
    else {
      proc.onComplete.connect([&exitCode] (bool isSuccess) { exitCode = isSuccess ? 0 : 1; });
      proc.runOnce();
      face.processEvents();
    }
  }
  catch (const std::exception& e) {
    std::cerr << ::nfd::getExtendedErrorMessage(e) << std::endl;
    return 1;
  }

  return exitCode;
}

} // namespace autoconfig
} // namespace tools
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::tools::autoconfig::main(argc, argv);
}
