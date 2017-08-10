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

#include "command-definition.hpp"
#include <ndn-cxx/util/logger.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

NDN_LOG_INIT(nfdc.CommandDefinition);

std::ostream&
operator<<(std::ostream& os, ArgValueType vt)
{
  switch (vt) {
    case ArgValueType::NONE:
      return os << "none";
    case ArgValueType::ANY:
      return os << "any";
    case ArgValueType::BOOLEAN:
      return os << "boolean";
    case ArgValueType::UNSIGNED:
      return os << "non-negative integer";
    case ArgValueType::STRING:
      return os << "string";
    case ArgValueType::REPORT_FORMAT:
      return os << "ReportFormat";
    case ArgValueType::NAME:
      return os << "Name";
    case ArgValueType::FACE_URI:
      return os << "FaceUri";
    case ArgValueType::FACE_ID_OR_URI:
      return os << "FaceId or FaceUri";
    case ArgValueType::FACE_PERSISTENCY:
      return os << "FacePersistency";
    case ArgValueType::ROUTE_ORIGIN:
      return os << "RouteOrigin";
  }
  return os << static_cast<int>(vt);
}

static std::string
getMetavarFromType(ArgValueType vt)
{
  switch (vt) {
    case ArgValueType::NONE:
      return "";
    case ArgValueType::ANY:
      return "args";
    case ArgValueType::BOOLEAN:
      return "bool";
    case ArgValueType::UNSIGNED:
      return "uint";
    case ArgValueType::STRING:
      return "str";
    case ArgValueType::REPORT_FORMAT:
      return "fmt";
    case ArgValueType::NAME:
      return "name";
    case ArgValueType::FACE_URI:
      return "uri";
    case ArgValueType::FACE_ID_OR_URI:
      return "face";
    case ArgValueType::FACE_PERSISTENCY:
      return "persistency";
    case ArgValueType::ROUTE_ORIGIN:
      return "origin";
  }
  BOOST_ASSERT(false);
  return "";
}

CommandDefinition::CommandDefinition(const std::string& noun, const std::string& verb)
  : m_noun(noun)
  , m_verb(verb)
{
}

CommandDefinition::~CommandDefinition() = default;

CommandDefinition&
CommandDefinition::addArg(const std::string& name, ArgValueType valueType,
                        Required isRequired, Positional allowPositional,
                        const std::string& metavar)
{
  bool isNew = m_args.emplace(name,
    Arg{name, valueType, static_cast<bool>(isRequired),
        metavar.empty() ? getMetavarFromType(valueType) : metavar}).second;
  BOOST_VERIFY(isNew);

  if (static_cast<bool>(isRequired)) {
    m_requiredArgs.insert(name);
  }

  if (static_cast<bool>(allowPositional)) {
    BOOST_ASSERT(valueType != ArgValueType::NONE);
    m_positionalArgs.push_back(name);
  }
  else {
    BOOST_ASSERT(valueType != ArgValueType::ANY);
  }

  return *this;
}

CommandArguments
CommandDefinition::parse(const std::vector<std::string>& tokens, size_t start) const
{
  CommandArguments ca;

  size_t positionalArgIndex = 0;
  for (size_t i = start; i < tokens.size(); ++i) {
    const std::string& token = tokens[i];

    // try to parse as named argument
    auto namedArg = m_args.find(token);
    if (namedArg != m_args.end() && namedArg->second.valueType != ArgValueType::ANY) {
      NDN_LOG_TRACE(token << " is a named argument");
      const Arg& arg = namedArg->second;
      if (arg.valueType == ArgValueType::NONE) {
        ca[arg.name] = true;
        NDN_LOG_TRACE(token << " is a no-param argument");
      }
      else if (i + 1 >= tokens.size()) {
        BOOST_THROW_EXCEPTION(Error(arg.name + ": " + arg.metavar + " is missing"));
      }
      else {
        const std::string& valueToken = tokens[++i];
        NDN_LOG_TRACE(arg.name << " has value " << valueToken);
        try {
          ca[arg.name] = this->parseValue(arg.valueType, valueToken);
        }
        catch (const std::exception& e) {
          NDN_LOG_TRACE(valueToken << " cannot be parsed as " << arg.valueType);
          BOOST_THROW_EXCEPTION(Error(arg.name + ": cannot parse '" + valueToken + "' as " +
                                      arg.metavar + " (" + e.what() + ")"));
        }
        NDN_LOG_TRACE(valueToken << " is parsed as " << arg.valueType);
      }

      // disallow positional arguments after named argument
      positionalArgIndex = m_positionalArgs.size();
      continue;
    }

    // try to parse as positional argument
    for (; positionalArgIndex < m_positionalArgs.size(); ++positionalArgIndex) {
      const Arg& arg = m_args.at(m_positionalArgs[positionalArgIndex]);

      if (arg.valueType == ArgValueType::ANY) {
        std::vector<std::string> values;
        std::copy(tokens.begin() + i, tokens.end(), std::back_inserter(values));
        ca[arg.name] = values;
        NDN_LOG_TRACE((tokens.size() - i) << " tokens are consumed for " << arg.name);
        i = tokens.size();
        break;
      }

      try {
        ca[arg.name] = this->parseValue(arg.valueType, token);
        NDN_LOG_TRACE(token << " is parsed as value for " << arg.name);
        break;
      }
      catch (const std::exception& e) {
        if (arg.isRequired) { // the current token must be parsed as the value for arg
          NDN_LOG_TRACE(token << " cannot be parsed as value for " << arg.name);
          BOOST_THROW_EXCEPTION(Error("cannot parse '" + token + "' as an argument name or as " +
                                      arg.metavar + " for " + arg.name + " (" + e.what() + ")"));
        }
        else {
          // the current token may be a value for next positional argument
          NDN_LOG_TRACE(token << " cannot be parsed as value for " << arg.name);
        }
      }
    }

    if (positionalArgIndex >= m_positionalArgs.size()) {
      // for loop has reached the end without finding a match,
      // which means token is not accepted as a value for positional argument
      BOOST_THROW_EXCEPTION(Error("cannot parse '" + token + "' as an argument name"));
    }

    // token is accepted; don't parse as the same positional argument again
    ++positionalArgIndex;
  }

  for (const std::string& argName : m_requiredArgs) {
    if (ca.count(argName) == 0) {
      BOOST_THROW_EXCEPTION(Error(argName + ": required argument is missing"));
    }
  }

  return ca;
}

static bool
parseBoolean(const std::string& s)
{
  if (s == "on" || s == "true" || s == "enabled" || s == "yes" || s == "1") {
    return true;
  }
  if (s == "off" || s == "false" || s == "disabled" || s == "no" || s == "0") {
    return false;
  }
  BOOST_THROW_EXCEPTION(std::invalid_argument("unrecognized boolean value '" + s + "'"));
}

static FacePersistency
parseFacePersistency(const std::string& s)
{
  if (s == "persistent") {
    return FacePersistency::FACE_PERSISTENCY_PERSISTENT;
  }
  if (s == "permanent") {
    return FacePersistency::FACE_PERSISTENCY_PERMANENT;
  }
  BOOST_THROW_EXCEPTION(std::invalid_argument("unrecognized FacePersistency '" + s + "'"));
}

boost::any
CommandDefinition::parseValue(ArgValueType valueType, const std::string& token) const
{
  switch (valueType) {
    case ArgValueType::NONE:
    case ArgValueType::ANY:
      BOOST_ASSERT(false);
      return boost::any();

    case ArgValueType::BOOLEAN: {
      return parseBoolean(token);
    }

    case ArgValueType::UNSIGNED: {
      // boost::lexical_cast<uint64_t> will accept negative number
      int64_t v = boost::lexical_cast<int64_t>(token);
      if (v < 0) {
        BOOST_THROW_EXCEPTION(std::out_of_range("value '" + token + "' is negative"));
      }
      return static_cast<uint64_t>(v);
    }

    case ArgValueType::STRING:
      return token;

    case ArgValueType::REPORT_FORMAT:
      return parseReportFormat(token);

    case ArgValueType::NAME:
      return Name(token);

    case ArgValueType::FACE_URI:
      return FaceUri(token);

    case ArgValueType::FACE_ID_OR_URI:
      try {
        return boost::lexical_cast<uint64_t>(token);
      }
      catch (const boost::bad_lexical_cast&) {
        return FaceUri(token);
      }

    case ArgValueType::FACE_PERSISTENCY:
      return parseFacePersistency(token);

    case ArgValueType::ROUTE_ORIGIN:
      return boost::lexical_cast<RouteOrigin>(token);
  }

  BOOST_ASSERT(false);
  return boost::any();
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
