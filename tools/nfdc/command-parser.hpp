/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NFDC_COMMAND_PARSER_HPP
#define NFD_TOOLS_NFDC_COMMAND_PARSER_HPP

#include "command-definition.hpp"
#include "execute-command.hpp"

#include <type_traits>

namespace nfd::tools::nfdc {

/** \brief Indicates which modes is a command allowed.
 */
enum AvailableIn : uint8_t {
  AVAILABLE_IN_NONE     = 0,
  AVAILABLE_IN_ONE_SHOT = 1 << 0, ///< one-shot mode
  AVAILABLE_IN_BATCH    = 1 << 1, ///< batch mode
  AVAILABLE_IN_HELP     = 1 << 7, ///< visible in help listing
  AVAILABLE_IN_ALL      = 0xff
};

std::ostream&
operator<<(std::ostream& os, AvailableIn modes);

/** \brief Indicates which mode is the parser operated in.
 */
enum class ParseMode : uint8_t {
  ONE_SHOT = AVAILABLE_IN_ONE_SHOT, ///< one-shot mode
  BATCH    = AVAILABLE_IN_BATCH     ///< batch mode
};

std::ostream&
operator<<(std::ostream& os, ParseMode mode);

/** \brief Parses a command.
 */
class CommandParser : noncopyable
{
public:
  class NoSuchCommandError : public std::invalid_argument
  {
  public:
    NoSuchCommandError(const std::string& noun, const std::string& verb)
      : std::invalid_argument("No such command: " + noun + " " + verb)
    {
    }
  };

  /** \brief Add an available command.
   *  \param def command semantics definition
   *  \param execute a function to execute the command
   *  \param modes parse modes this command should be available in, must not be AVAILABLE_IN_NONE
   */
  CommandParser&
  addCommand(const CommandDefinition& def, const ExecuteCommand& execute,
             std::underlying_type_t<AvailableIn> modes = AVAILABLE_IN_ALL);

  /** \brief Add an alias "noun verb2" to existing command "noun verb".
   *  \throw std::out_of_range "noun verb" does not exist
   */
  CommandParser&
  addAlias(const std::string& noun, const std::string& verb, const std::string& verb2);

  /** \brief List known commands for help.
   *  \param noun if not empty, filter results by this noun
   *  \param mode include commands for the specified parse mode
   *  \return commands in insertion order
   */
  std::vector<const CommandDefinition*>
  listCommands(std::string_view noun, ParseMode mode) const;

  /** \brief Parse a command line.
   *  \param tokens command line
   *  \param mode parser mode, must be ParseMode::ONE_SHOT, other modes are not implemented
   *  \throw NoSuchCommandError command not found
   *  \throw CommandDefinition::Error command arguments are invalid
   *  \return noun, verb, arguments, execute function
   */
  std::tuple<std::string, std::string, CommandArguments, ExecuteCommand>
  parse(const std::vector<std::string>& tokens, ParseMode mode) const;

private:
  using CommandName = std::pair<std::string, std::string>;

  struct Command
  {
    CommandDefinition def;
    ExecuteCommand execute;
    AvailableIn modes;
  };

  /** \brief Map from command name or alias to command definition.
   */
  using CommandContainer = std::map<CommandName, shared_ptr<Command>>;
  CommandContainer m_commands;

  /** \brief Commands in insertion order.
   */
  std::vector<CommandContainer::const_iterator> m_commandOrder;
};

void
registerCommands(CommandParser& parser);

} // namespace nfd::tools::nfdc

#endif // NFD_TOOLS_NFDC_COMMAND_PARSER_HPP
