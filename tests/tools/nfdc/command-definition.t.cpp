/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#include "nfdc/command-definition.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestCommandDefinition, BaseFixture)

BOOST_AUTO_TEST_SUITE(Arguments)

BOOST_AUTO_TEST_CASE(NoArg)
{
  CommandDefinition cs("noun", "verb");

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{});
  BOOST_CHECK_EQUAL(ca.size(), 0);

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"x"}), CommandDefinition::Error);
  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"x", "y"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_CASE(NamedArgs)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::UNSIGNED, Required::NO, Positional::NO, "int")
    .addArg("b", ArgValueType::NAME, Required::NO, Positional::NO, "name");

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{});
  BOOST_CHECK_EQUAL(ca.size(), 0);

  ca = cs.parse(std::vector<std::string>{"a", "1"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK_EQUAL(ca.get<uint64_t>("a"), 1);

  ca = cs.parse(std::vector<std::string>{"a", "1", "b", "/n"});
  BOOST_CHECK_EQUAL(ca.size(), 2);
  BOOST_CHECK_EQUAL(ca.get<uint64_t>("a"), 1);
  BOOST_CHECK_EQUAL(ca.get<Name>("b"), "/n");

  ca = cs.parse(std::vector<std::string>{"b", "/n", "a", "1"});
  BOOST_CHECK_EQUAL(ca.size(), 2);
  BOOST_CHECK_EQUAL(ca.get<uint64_t>("a"), 1);
  BOOST_CHECK_EQUAL(ca.get<Name>("b"), "/n");

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"1"}), CommandDefinition::Error);
  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"c", "1"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_CASE(PositionalArgs)
{
  CommandDefinition cs("face", "create");
  cs.addArg("remote", ArgValueType::FACE_URI, Required::YES, Positional::YES)
    .addArg("persistency", ArgValueType::FACE_PERSISTENCY, Required::NO, Positional::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"udp4://router.example.com", "persistent"});
  BOOST_CHECK_EQUAL(ca.size(), 2);
  BOOST_CHECK_EQUAL(ca.get<FaceUri>("remote"),
                    FaceUri("udp4://router.example.com"));
  BOOST_CHECK_EQUAL(ca.get<FacePersistency>("persistency"),
                    FacePersistency::FACE_PERSISTENCY_PERSISTENT);

  ca = cs.parse(std::vector<std::string>{"udp4://router.example.com"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK_EQUAL(ca.get<FaceUri>("remote"),
                    FaceUri("udp4://router.example.com"));

  ca = cs.parse(std::vector<std::string>{"remote", "udp4://router.example.com"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK_EQUAL(ca.get<FaceUri>("remote"),
                    FaceUri("udp4://router.example.com"));

  ca = cs.parse(std::vector<std::string>{
                "udp4://router.example.com", "persistency", "persistent"});
  BOOST_CHECK_EQUAL(ca.size(), 2);
  BOOST_CHECK_EQUAL(ca.get<FaceUri>("remote"),
                    FaceUri("udp4://router.example.com"));
  BOOST_CHECK_EQUAL(ca.get<FacePersistency>("persistency"),
                    FacePersistency::FACE_PERSISTENCY_PERSISTENT);

  ca = cs.parse(std::vector<std::string>{
                "remote", "udp4://router.example.com", "persistency", "persistent"});
  BOOST_CHECK_EQUAL(ca.size(), 2);
  BOOST_CHECK_EQUAL(ca.get<FaceUri>("remote"),
                    FaceUri("udp4://router.example.com"));
  BOOST_CHECK_EQUAL(ca.get<FacePersistency>("persistency"),
                    FacePersistency::FACE_PERSISTENCY_PERSISTENT);

  ca = cs.parse(std::vector<std::string>{
                "persistency", "persistent", "remote", "udp4://router.example.com"});
  BOOST_CHECK_EQUAL(ca.size(), 2);
  BOOST_CHECK_EQUAL(ca.get<FaceUri>("remote"),
                    FaceUri("udp4://router.example.com"));
  BOOST_CHECK_EQUAL(ca.get<FacePersistency>("persistency"),
                    FacePersistency::FACE_PERSISTENCY_PERSISTENT);

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{
    "persistent", "udp4://router.example.com"}), CommandDefinition::Error);
  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{
    "persistency", "persistent", "udp4://router.example.com"}), CommandDefinition::Error);
  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{
    "remote", "udp4://router.example.com", "persistent"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_SUITE_END() // Arguments

BOOST_AUTO_TEST_SUITE(ParseValue)

BOOST_AUTO_TEST_CASE(NoneType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::NONE, Required::YES, Positional::NO);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(bool));
  BOOST_CHECK_EQUAL(ca.get<bool>("a"), true);

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "value"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_CASE(AnyType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::ANY, Required::NO, Positional::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{});
  BOOST_CHECK_EQUAL(ca.size(), 0);

  ca = cs.parse(std::vector<std::string>{"a"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(std::vector<std::string>));
  std::vector<std::string> values = ca.get<std::vector<std::string>>("a");
  BOOST_CHECK_EQUAL(values.size(), 1);
  BOOST_CHECK_EQUAL(values.at(0), "a");

  ca = cs.parse(std::vector<std::string>{"b", "c"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(std::vector<std::string>));
  values = ca.get<std::vector<std::string>>("a");
  BOOST_CHECK_EQUAL(values.size(), 2);
  BOOST_CHECK_EQUAL(values.at(0), "b");
  BOOST_CHECK_EQUAL(values.at(1), "c");
}

BOOST_AUTO_TEST_CASE(UnsignedType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::UNSIGNED, Required::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a", "0"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(uint64_t));
  BOOST_CHECK_EQUAL(ca.get<uint64_t>("a"), 0);

  ca = cs.parse(std::vector<std::string>{"a", "12923"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(uint64_t));
  BOOST_CHECK_EQUAL(ca.get<uint64_t>("a"), 12923);

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "-25705"}), CommandDefinition::Error);
  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "not-uint"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_CASE(StringType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::STRING, Required::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a", "hello"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(std::string));
  BOOST_CHECK_EQUAL(ca.get<std::string>("a"), "hello");
}

BOOST_AUTO_TEST_CASE(ReportFormatType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::REPORT_FORMAT, Required::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a", "xml"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(ReportFormat));
  BOOST_CHECK_EQUAL(ca.get<ReportFormat>("a"), ReportFormat::XML);

  ca = cs.parse(std::vector<std::string>{"a", "text"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(ReportFormat));
  BOOST_CHECK_EQUAL(ca.get<ReportFormat>("a"), ReportFormat::TEXT);

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "not-fmt"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_CASE(NameType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::NAME, Required::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a", "/n"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(Name));
  BOOST_CHECK_EQUAL(ca.get<Name>("a"), "/n");
}

BOOST_AUTO_TEST_CASE(FaceUriType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::FACE_URI, Required::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a", "udp4://192.0.2.1:6363"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(FaceUri));
  BOOST_CHECK_EQUAL(ca.get<FaceUri>("a"), FaceUri("udp4://192.0.2.1:6363"));

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "208"}), CommandDefinition::Error);
  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "not-FaceUri"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_CASE(FaceIdOrUriType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::FACE_ID_OR_URI, Required::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a", "208"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(uint64_t));
  BOOST_CHECK_EQUAL(ca.get<uint64_t>("a"), 208);

  ca = cs.parse(std::vector<std::string>{"a", "udp4://192.0.2.1:6363"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(FaceUri));
  BOOST_CHECK_EQUAL(ca.get<FaceUri>("a"), FaceUri("udp4://192.0.2.1:6363"));

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "not-FaceUri"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_CASE(FacePersistencyType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::FACE_PERSISTENCY, Required::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a", "persistent"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(FacePersistency));
  BOOST_CHECK_EQUAL(ca.get<FacePersistency>("a"),
                    FacePersistency::FACE_PERSISTENCY_PERSISTENT);

  ca = cs.parse(std::vector<std::string>{"a", "permanent"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(FacePersistency));
  BOOST_CHECK_EQUAL(ca.get<FacePersistency>("a"),
                    FacePersistency::FACE_PERSISTENCY_PERMANENT);

  // nfdc does not accept "on-demand"
  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "on-demand"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_CASE(RouteOriginType)
{
  CommandDefinition cs("noun", "verb");
  cs.addArg("a", ArgValueType::ROUTE_ORIGIN, Required::YES);

  CommandArguments ca;

  ca = cs.parse(std::vector<std::string>{"a", "Nlsr"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(RouteOrigin));
  BOOST_CHECK_EQUAL(ca.get<RouteOrigin>("a"),
                    RouteOrigin::ROUTE_ORIGIN_NLSR);

  ca = cs.parse(std::vector<std::string>{"a", "27"});
  BOOST_CHECK_EQUAL(ca.size(), 1);
  BOOST_CHECK(ca.at("a").type() == typeid(RouteOrigin));
  BOOST_CHECK_EQUAL(ca.get<RouteOrigin>("a"),
                    static_cast<RouteOrigin>(27));

  BOOST_CHECK_THROW(cs.parse(std::vector<std::string>{"a", "not-RouteOrigin"}), CommandDefinition::Error);
}

BOOST_AUTO_TEST_SUITE_END() // ParseValue

BOOST_AUTO_TEST_SUITE_END() // TestCommandDefinition
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
