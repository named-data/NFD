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

#include "command-parser.hpp"
#include "format-helpers.hpp"

#include <ndn-cxx/util/logger.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

NDN_LOG_INIT(nfdc.CommandParser);

static_assert(std::is_same<std::underlying_type<AvailableIn>::type,
                           std::underlying_type<ParseMode>::type>::value,
              "AvailableIn and ParseMode must be declared with same underlying type");

std::ostream&
operator<<(std::ostream& os, AvailableIn modes)
{
  text::Separator sep("|");
  if ((modes & AVAILABLE_IN_ONE_SHOT) != 0) {
    os << sep << "one-shot";
  }
  if ((modes & AVAILABLE_IN_BATCH) != 0) {
    os << sep << "batch";
  }
  if ((modes & AVAILABLE_IN_HELP) == 0) {
    os << sep << "hidden";
  }

  if (sep.getCount() == 0) {
    os << "none";
  }
  return os;
}

std::ostream&
operator<<(std::ostream& os, ParseMode mode)
{
  switch (mode) {
    case ParseMode::ONE_SHOT:
      return os << "one-shot";
    case ParseMode::BATCH:
      return os << "batch";
  }
  return os << static_cast<int>(mode);
}

CommandParser&
CommandParser::addCommand(const CommandDefinition& def, const ExecuteCommand& execute,
                          std::underlying_type<AvailableIn>::type modes)
{
  BOOST_ASSERT(modes != AVAILABLE_IN_NONE);

  m_commands[{def.getNoun(), def.getVerb()}].reset(
    new Command{def, execute, static_cast<AvailableIn>(modes)});

  if ((modes & AVAILABLE_IN_HELP) != 0) {
    m_commandOrder.push_back(m_commands.find({def.getNoun(), def.getVerb()}));
  }

  return *this;
}

CommandParser&
CommandParser::addAlias(const std::string& noun, const std::string& verb, const std::string& verb2)
{
  m_commands[{noun, verb2}] = m_commands.at({noun, verb});
  return *this;
}

std::vector<const CommandDefinition*>
CommandParser::listCommands(const std::string& noun, ParseMode mode) const
{
  std::vector<const CommandDefinition*> results;
  for (auto i : m_commandOrder) {
    const Command& command = *i->second;
    if ((command.modes & static_cast<AvailableIn>(mode)) != 0 &&
        (noun.empty() || noun == command.def.getNoun())) {
      results.push_back(&command.def);
    }
  }
  return results;
}

std::tuple<std::string, std::string, CommandArguments, ExecuteCommand>
CommandParser::parse(const std::vector<std::string>& tokens, ParseMode mode) const
{
  BOOST_ASSERT(mode == ParseMode::ONE_SHOT);

  const std::string& noun = tokens.size() > 0 ? tokens[0] : "";
  const std::string& verb = tokens.size() > 1 ? tokens[1] : "";

  NDN_LOG_TRACE("parse mode=" << mode << " noun=" << noun << " verb=" << verb);

  auto i = m_commands.find({noun, verb});
  if (i == m_commands.end() || (i->second->modes & static_cast<AvailableIn>(mode)) == 0) {
    BOOST_THROW_EXCEPTION(NoSuchCommandError(noun, verb));
  }

  const CommandDefinition& def = i->second->def;
  NDN_LOG_TRACE("found command noun=" << def.getNoun() << " verb=" << def.getVerb());

  size_t nConsumed = std::min<size_t>(2, tokens.size());
  return std::make_tuple(def.getNoun(), def.getVerb(), def.parse(tokens, nConsumed), i->second->execute);
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
