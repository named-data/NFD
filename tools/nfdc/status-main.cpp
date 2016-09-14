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

#include "status-main.hpp"
#include "core/version.hpp"
#include "status-report.hpp"
#include "forwarder-general-module.hpp"
#include "channel-module.hpp"
#include "face-module.hpp"
#include "fib-module.hpp"
#include "rib-module.hpp"
#include "strategy-choice-module.hpp"

#include <ndn-cxx/security/validator-null.hpp>
#include <boost/program_options.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

struct Options
{
  ReportFormat output = ReportFormat::TEXT;
  bool wantForwarderGeneral = false;
  bool wantChannels = false;
  bool wantFaces = false;
  bool wantFib = false;
  bool wantRib = false;
  bool wantStrategyChoice = false;
};

static void
showUsage(std::ostream& os, const boost::program_options::options_description& cmdOptions)
{
  os << "Usage: nfd-status [options]\n\n"
     << "Show NFD version and status information.\n\n"
     << cmdOptions;
}

/** \brief parse command line, and show usage if necessary
 *  \return if first item is -1, caller should retrieve and display StatusReport;
 *          otherwise, caller should immediately exit with the specified exit code
 */
static std::tuple<int, Options>
parseCommandLine(const std::vector<std::string>& args)
{
  Options options;

  namespace po = boost::program_options;
  po::options_description cmdOptions("Options");
  cmdOptions.add_options()
    ("help,h", "print this help message")
    ("version,V", "show program version")
    ("general,v", po::bool_switch(&options.wantForwarderGeneral), "show general status")
    ("channels,c", po::bool_switch(&options.wantChannels), "show channels")
    ("faces,f", po::bool_switch(&options.wantFaces), "show faces")
    ("fib,b", po::bool_switch(&options.wantFib), "show FIB entries")
    ("rib,r", po::bool_switch(&options.wantRib), "show RIB routes")
    ("sc,s", po::bool_switch(&options.wantStrategyChoice), "show strategy choice entries")
    ("xml,x", "output as XML instead of text (implies -vcfbrs)");
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(args).options(cmdOptions).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << e.what() << "\n";
    showUsage(std::cerr, cmdOptions);
    return std::make_tuple(2, options);
  }

  if (vm.count("help") > 0) {
    showUsage(std::cout, cmdOptions);
    return std::make_tuple(0, options);
  }
  if (vm.count("version") > 0) {
    std::cout << "nfd-status " << NFD_VERSION_BUILD_STRING << "\n";
    return std::make_tuple(0, options);
  }

  if (vm.count("xml") > 0) {
    options.output = ReportFormat::XML;
  }
  if (options.output == ReportFormat::XML ||
      (!options.wantForwarderGeneral && !options.wantChannels && !options.wantFaces &&
       !options.wantFib && !options.wantRib && !options.wantStrategyChoice)) {
    options.wantForwarderGeneral = options.wantChannels = options.wantFaces =
      options.wantFib = options.wantRib = options.wantStrategyChoice = true;
  }

  return std::make_tuple(-1, options);
}

int
statusMain(const std::vector<std::string>& args, Face& face, KeyChain& keyChain)
{
  int exitCode = -1;
  Options options;
  std::tie(exitCode, options) = parseCommandLine(args);
  if (exitCode >= 0) {
    return exitCode;
  }

  unique_ptr<Validator> validator = make_unique<ndn::ValidatorNull>();
  CommandOptions ctrlOptions;

  StatusReport report;

  if (options.wantForwarderGeneral) {
    auto nfdIdCollector = make_unique<NfdIdCollector>(std::move(validator));
    auto forwarderGeneralModule = make_unique<ForwarderGeneralModule>();
    forwarderGeneralModule->setNfdIdCollector(*nfdIdCollector);
    report.sections.push_back(std::move(forwarderGeneralModule));
    validator = std::move(nfdIdCollector);
  }

  if (options.wantChannels) {
    report.sections.push_back(make_unique<ChannelModule>());
  }

  if (options.wantFaces) {
    report.sections.push_back(make_unique<FaceModule>());
  }

  if (options.wantFib) {
    report.sections.push_back(make_unique<FibModule>());
  }

  if (options.wantRib) {
    report.sections.push_back(make_unique<RibModule>());
  }

  if (options.wantStrategyChoice) {
    report.sections.push_back(make_unique<StrategyChoiceModule>());
  }

  uint32_t code = report.collect(face, keyChain, *validator, ctrlOptions);
  if (code != 0) {
    // Give a simple error code for end user.
    // Technical support personnel:
    // 1. get the exact command from end user
    // 2. code div 1000000 is zero-based section index
    // 3. code mod 1000000 is a Controller.fetch error code
    std::cerr << "Error while collecting status report (" << code << ").\n";
    return 1;
  }

  switch (options.output) {
    case ReportFormat::XML:
      report.formatXml(std::cout);
      break;
    case ReportFormat::TEXT:
      report.formatText(std::cout);
      break;
  }
  return 0;
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
