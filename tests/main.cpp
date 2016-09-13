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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_ALTERNATIVE_INIT_API

#include <boost/version.hpp>

#if BOOST_VERSION >= 106200
// Boost.Test v3.3 (Boost 1.62) natively supports multi-logger output
#include "boost-test.hpp"
#else
#define BOOST_TEST_NO_MAIN
#include "boost-test.hpp"

#include "boost-multi-log-formatter.hpp"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <fstream>
#include <iostream>

static bool
init_tests()
{
  init_unit_test();

  namespace po = boost::program_options;
  namespace ut = boost::unit_test;

  po::options_description extraOptions;
  std::string logger;
  std::string outputFile = "-";
  extraOptions.add_options()
    ("log_format2", po::value<std::string>(&logger), "Type of second log formatter: HRF or XML")
    ("log_sink2", po::value<std::string>(&outputFile)->default_value(outputFile), "Second log sink, - for stdout")
    ;
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(ut::framework::master_test_suite().argc,
                                      ut::framework::master_test_suite().argv)
                .options(extraOptions)
                .run(),
              vm);
    po::notify(vm);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n"
              << extraOptions << std::endl;
    return false;
  }

  if (vm.count("log_format2") == 0) {
    // second logger is not configured
    return true;
  }

  std::shared_ptr<ut::unit_test_log_formatter> formatter;
  if (logger == "XML") {
    formatter = std::make_shared<ut::output::xml_log_formatter>();
  }
  else if (logger == "HRF") {
    formatter = std::make_shared<ut::output::compiler_log_formatter>();
  }
  else {
    std::cerr << "ERROR: only HRF or XML log formatter can be specified" << std::endl;
    return false;
  }

  std::shared_ptr<std::ostream> output;
  if (outputFile == "-") {
    output = std::shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }
  else {
    output = std::make_shared<std::ofstream>(outputFile.c_str());
  }

  auto multiFormatter = new ut::output::multi_log_formatter;
  multiFormatter->add(formatter, output);
  ut::unit_test_log.set_formatter(multiFormatter);

  return true;
}

int
main(int argc, char* argv[])
{
  return ::boost::unit_test::unit_test_main(&init_tests, argc, argv);
}

#endif // BOOST_VERSION >= 106200
