/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "common.hpp"
#include "nrd.hpp"
#include "nrd-config.hpp"
#include <getopt.h>

struct ProgramOptions
{
  bool showUsage;
  std::string config;
};

static void
printUsage(std::ostream& os, const std::string& programName)
{
  os << "Usage: \n"
    << "  " << programName << " [options]\n"
    << "\n"
    << "Run NRD daemon\n"
    << "\n"
    << "Options:\n"
    << "  [--help]   - print this help message\n"
    << "  [--config /path/to/nrd.conf] - path to configuration file "
    << "(default: " << DEFAULT_CONFIG_FILE << ")\n"
    ;
}

static bool
parseCommandLine(int argc, char** argv, ProgramOptions& options)
{
  options.showUsage = false;
  options.config = DEFAULT_CONFIG_FILE;

  while (true) {
    int optionIndex = 0;
    static ::option longOptions[] = {
      { "help"   , no_argument      , 0, 0 },
      { "config" , required_argument, 0, 0 },
      { 0        , 0                , 0, 0 }
    };
    int c = getopt_long_only(argc, argv, "", longOptions, &optionIndex);
    if (c == -1)
      break;

    switch (c) {
      case 0:
        switch (optionIndex) {
          case 0: // help
            options.showUsage = true;
            break;
          case 1: // config
            options.config = ::optarg;
            break;
          default:
            return false;
        }
        break;
    }
  }
  return true;
}

int
main(int argc, char** argv)
{
  //processing command line arguments, if available
  ProgramOptions options;
  bool isCommandLineValid = parseCommandLine(argc, argv, options);
  if (!isCommandLineValid) {
    printUsage(std::cerr, argv[0]);
    return 1;
  }
  if (options.showUsage) {
    printUsage(std::cout, argv[0]);
    return 0;
  }

  //Now read the config file and pass the security section to validator
  try {
    std::string configFilename(options.config);

    ndn::nrd::NrdConfig config;
    config.load(configFilename);
    ndn::nrd::ConfigSection securitySection = config.getSecuritySection();

    ndn::nrd::Nrd nrd(securitySection, configFilename);
    nrd.enableLocalControlHeader();
    nrd.listen();
  }
  catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }

  return 0;
}
