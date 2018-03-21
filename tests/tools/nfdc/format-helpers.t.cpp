/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

BOOST_AUTO_TEST_CASE(Flag)
{
  output_test_stream os;
  os << "<A>" << xml::Flag{"B", true} << "</A><C>" << xml::Flag{"D", false} << "</C>";

  BOOST_CHECK(os.is_equal("<A><B/></A><C></C>"));
}

BOOST_AUTO_TEST_CASE(DurationFormatting)
{
  time::nanoseconds d1 = 53000_s + 87_ms + 3_us;
  BOOST_CHECK_EQUAL(xml::formatDuration(d1), "PT53000.087S");

  time::nanoseconds d2 = 56_s;
  BOOST_CHECK_EQUAL(xml::formatDuration(d2), "PT56S");

  time::nanoseconds d3 = 3_min;
  BOOST_CHECK_EQUAL(xml::formatDuration(d3), "PT180S");

  time::nanoseconds d4 = 0_ns;
  BOOST_CHECK_EQUAL(xml::formatDuration(d4), "PT0S");

  time::nanoseconds d5 = 1_ms;
  BOOST_CHECK_EQUAL(xml::formatDuration(d5), "PT0.001S");

  time::nanoseconds d6 = -53_s - 250_ms;
  BOOST_CHECK_EQUAL(xml::formatDuration(d6), "-PT53.250S");

  time::nanoseconds d7 = 1_us;
  BOOST_CHECK_EQUAL(xml::formatDuration(d7), "PT0S");
}

BOOST_AUTO_TEST_SUITE_END() // Xml

BOOST_AUTO_TEST_SUITE(Text)

BOOST_AUTO_TEST_CASE(Space)
{
  output_test_stream os;
  os << 'A' << text::Spaces{-1} << 'B' << text::Spaces{0} << 'C' << text::Spaces{5} << 'D';

  BOOST_CHECK(os.is_equal("ABC     D"));
}

BOOST_AUTO_TEST_CASE(Sep)
{
  output_test_stream os;
  text::Separator sep(",");
  for (int i = 1; i <= 3; ++i) {
    os << sep << i;
  }

  BOOST_CHECK(os.is_equal("1,2,3"));
}

static void
printItemAttributes(std::ostream& os, bool wantMultiLine)
{
  text::ItemAttributes ia(wantMultiLine, 3);
  os << ia("id") << 500
     << ia("uri") << "udp4://192.0.2.1:6363"
     << ia.end();
}

BOOST_AUTO_TEST_CASE(ItemAttributesSingleLine)
{
  output_test_stream os;
  printItemAttributes(os, false);
  BOOST_CHECK(os.is_equal("id=500 uri=udp4://192.0.2.1:6363"));
}

BOOST_AUTO_TEST_CASE(ItemAttributesMultiLine)
{
  output_test_stream os;
  printItemAttributes(os, true);
  BOOST_CHECK(os.is_equal(" id=500\nuri=udp4://192.0.2.1:6363\n"));
}

BOOST_AUTO_TEST_CASE(OnOff)
{
  output_test_stream os;
  os << 'A' << text::OnOff{true} << 'B' << text::OnOff{false} << 'C';

  BOOST_CHECK(os.is_equal("AonBoffC"));
}

BOOST_AUTO_TEST_SUITE_END() // Text

BOOST_AUTO_TEST_SUITE_END() // TestFormatHelpers
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
