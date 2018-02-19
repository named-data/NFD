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

#ifndef NFD_TOOLS_NFDC_HELP_HPP
#define NFD_TOOLS_NFDC_HELP_HPP

#include "command-parser.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

/** \brief writes the list of available commands to a stream
 *  \param os the output stream to write the list to
 *  \param parser instance of CommandParser containing the commands to list
 *  \param mode only the commands available in this mode are listed
 *  \param noun if not empty, only the commands starting with this noun are listed
 */
void
helpList(std::ostream& os, const CommandParser& parser,
         ParseMode mode = ParseMode::ONE_SHOT, const std::string& noun = "");

/** \brief tries to help the user, if requested on the command line
 *
 *  Depending on the provided command line arguments \p args, this function can either
 *  open the man page for a specific command, or list all commands available in \p parser.
 *  In the former case, this function never returns if successful.
 *
 *  \retval 0 a list of available commands was successfully written to \p os
 *  \retval 1 help was requested, but an error was encountered while exec'ing the `man` binary
 *  \retval 2 help was not provided because \p args did not contain any help-related options
 */
int
help(std::ostream& os, const CommandParser& parser,
     std::vector<std::string> args);

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_HELP_HPP
