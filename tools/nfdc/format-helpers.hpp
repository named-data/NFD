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

#ifndef NFD_TOOLS_NFDC_FORMAT_HELPERS_HPP
#define NFD_TOOLS_NFDC_FORMAT_HELPERS_HPP

#include "core/common.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

namespace xml {

void
printHeader(std::ostream& os);

void
printFooter(std::ostream& os);

struct Text
{
  const std::string& s;
};

/** \brief print XML text with special character represented as predefined entities
 */
std::ostream&
operator<<(std::ostream& os, const Text& text);

std::string
formatSeconds(time::seconds d);

template<typename DURATION>
std::string
formatDuration(DURATION d)
{
  return formatSeconds(time::duration_cast<time::seconds>(d));
}

std::string
formatTimestamp(time::system_clock::TimePoint t);

} // namespace xml

namespace text {

/** \brief print different string on first and subsequent usage
 *
 *  \code
 *  Separator sep(",");
 *  for (int i = 1; i <= 3; ++i) {
 *    os << sep << i;
 *  }
 *  // prints: 1,2,3
 *  \endcode
 */
class Separator
{
public:
  Separator(const std::string& first, const std::string& subsequent);

  explicit
  Separator(const std::string& subsequent);

  int
  getCount() const
  {
    return m_count;
  }

private:
  std::string m_first;
  std::string m_subsequent;
  int m_count;

  friend std::ostream& operator<<(std::ostream& os, Separator& sep);
};

std::ostream&
operator<<(std::ostream& os, Separator& sep);

std::string
formatSeconds(time::seconds d, bool isLong = false);

template<typename DURATION>
std::string
formatDuration(DURATION d, bool isLong = false)
{
  return formatSeconds(time::duration_cast<time::seconds>(d), isLong);
}

std::string
formatTimestamp(time::system_clock::TimePoint t);

} // namespace text

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_FORMAT_HELPERS_HPP
