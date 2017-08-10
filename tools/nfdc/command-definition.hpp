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

#ifndef NFD_TOOLS_NFDC_COMMAND_DEFINITION_HPP
#define NFD_TOOLS_NFDC_COMMAND_DEFINITION_HPP

#include "command-arguments.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

/** \brief indicates argument value type
 */
enum class ArgValueType {
  /** \brief boolean argument without value
   *
   *  The argument appears in CommandArguments as bool value 'true'.
   *  It must not be declared as positional.
   */
  NONE,

  /** \brief any arguments
   *
   *  The argument appears in CommandArguments as std::vector<std::string>.
   *  It must be declared as positional, and will consume all subsequent tokens.
   */
  ANY,

  /** \brief boolean
   *
   * The argument appears in CommandArguments as bool.
   */
  BOOLEAN,

  /** \brief non-negative integer
   *
   *  The argument appears in CommandArguments as uint64_t.
   *  Acceptable input range is [0, std::numeric_limits<int64_t>::max()].
   */
  UNSIGNED,

  /** \brief arbitrary string
   *
   *  The argument appears in CommandArguments as std::string.
   */
  STRING,

  /** \brief report format 'xml' or 'text'
   *
   *  The argument appears in CommandArguments as nfd::tools::nfdc::ReportFormat.
   */
  REPORT_FORMAT,

  /** \brief Name prefix
   *
   *  The argument appears in CommandArguments as ndn::Name.
   */
  NAME,

  /** \brief FaceUri
   *
   *  The argument appears in CommandArguments as ndn::FaceUri.
   */
  FACE_URI,

  /** \brief FaceId or FaceUri
   *
   *  The argument appears in CommandArguments as either uint64_t or ndn::FaceUri.
   */
  FACE_ID_OR_URI,

  /** \brief face persistency 'persistent' or 'permanent'
   *
   *  The argument appears in CommandArguments as ndn::nfd::FacePersistency.
   */
  FACE_PERSISTENCY,

  /** \brief route origin
   *
   *  The argument appears in CommandArguments as ndn::nfd::RouteOrigin.
   */
  ROUTE_ORIGIN,
};

std::ostream&
operator<<(std::ostream& os, ArgValueType vt);

/** \brief indicates whether an argument is required
 */
enum class Required {
  NO = false, ///< argument is optional
  YES = true  ///< argument is required
};

/** \brief indicates whether an argument can be specified as positional
 */
enum class Positional {
  NO = false, ///< argument must be named
  YES = true  ///< argument can be specified as positional
};

/** \brief declares semantics of a command
 */
class CommandDefinition
{
public:
  class Error : public std::invalid_argument
  {
  public:
    explicit
    Error(const std::string& what)
      : std::invalid_argument(what)
    {
    }
  };

  CommandDefinition(const std::string& noun, const std::string& verb);

  ~CommandDefinition();

  const std::string
  getNoun() const
  {
    return m_noun;
  }

  const std::string
  getVerb() const
  {
    return m_verb;
  }

public: // help
  /** \return one-line description
   */
  const std::string&
  getTitle() const
  {
    return m_title;
  }

  /** \brief set one-line description
   *  \param title one-line description, written in lower case
   */
  CommandDefinition&
  setTitle(const std::string& title)
  {
    m_title = title;
    return *this;
  }

public: // arguments
  /** \brief declare an argument
   *  \param name argument name, must be unique
   *  \param valueType argument value type
   *  \param isRequired whether the argument is required
   *  \param allowPositional whether the argument value can be specified as positional
   *  \param metavar displayed argument value placeholder
   */
  CommandDefinition&
  addArg(const std::string& name, ArgValueType valueType,
         Required isRequired = Required::NO,
         Positional allowPositional = Positional::NO,
         const std::string& metavar = "");

  /** \brief parse a command line
   *  \param tokens command line tokens
   *  \param start command line start position, after noun and verb
   *  \throw Error command line is invalid
   */
  CommandArguments
  parse(const std::vector<std::string>& tokens, size_t start = 0) const;

private:
  boost::any
  parseValue(ArgValueType valueType, const std::string& token) const;

private:
  std::string m_noun;
  std::string m_verb;

  std::string m_title;

  struct Arg
  {
    std::string name;
    ArgValueType valueType;
    bool isRequired;
    std::string metavar;
  };
  typedef std::map<std::string, Arg> ArgMap;
  ArgMap m_args;
  std::set<std::string> m_requiredArgs;
  std::vector<std::string> m_positionalArgs;
};

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_COMMAND_DEFINITION_HPP
