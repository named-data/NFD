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

#include "nfdc/format-helpers.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using boost::test_tools::output_test_stream;

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_AUTO_TEST_SUITE(TestFormatHelpers)

BOOST_AUTO_TEST_SUITE(Xml)

BOOST_AUTO_TEST_CASE(TextEscaping)
{
  output_test_stream os;
  os << xml::Text{"\"less than\" & 'greater than' surround XML <element> tag name"};

  BOOST_CHECK(os.is_equal("&quot;less than&quot; &amp; &apos;greater than&apos;"
                          " surround XML &lt;element&gt; tag name"));
}

BOOST_AUTO_TEST_SUITE_END() // Xml

BOOST_AUTO_TEST_SUITE(Text)

BOOST_AUTO_TEST_CASE(Sep)
{
  output_test_stream os;
  text::Separator sep(",");
  for (int i = 1; i <= 3; ++i) {
    os << sep << i;
  }

  BOOST_CHECK(os.is_equal("1,2,3"));
}

BOOST_AUTO_TEST_SUITE_END() // Text

BOOST_AUTO_TEST_SUITE_END() // TestFormatHelpers
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
