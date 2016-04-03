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

#ifndef NFD_CORE_EXTENDED_ERROR_MESSAGE_HPP
#define NFD_CORE_EXTENDED_ERROR_MESSAGE_HPP

#include <boost/exception/get_error_info.hpp>
#include <sstream>

namespace nfd {

template<typename E>
std::string
getExtendedErrorMessage(const E& exception)
{
  std::ostringstream errorMessage;
  errorMessage << exception.what();

  const char* const* file = boost::get_error_info<boost::throw_file>(exception);
  const int* line = boost::get_error_info<boost::throw_line>(exception);
  const char* const* func = boost::get_error_info<boost::throw_function>(exception);
  if (file && line) {
    errorMessage << " [from " << *file << ":" << *line;
    if (func) {
      errorMessage << " in " << *func;
    }
    errorMessage << "]";
  }

  return errorMessage.str();
}

} // namespace nfd

#endif // NFD_CORE_EXTENDED_ERROR_MESSAGE_HPP
