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

/** \brief print true as an empty element and false as nothing
 */
struct Flag
{
  const char* elementName;
  bool flag;
};

std::ostream&
operator<<(std::ostream& os, Flag v);

/** \return duration in XML duration format
 *
 *  Definition of this format: https://www.w3.org/TR/xmlschema11-2/#duration
 */
std::string
formatDuration(time::nanoseconds d);

/** \return timestamp in XML dateTime format
 *
 *  Definition of this format: https://www.w3.org/TR/xmlschema11-2/#dateTime
 */
std::string
formatTimestamp(time::system_clock::TimePoint t);

} // namespace xml

namespace text {

/** \brief print a number of whitespaces
 */
struct Spaces
{
  int nSpaces; ///< number of spaces; print nothing if negative
};

std::ostream&
operator<<(std::ostream& os, const Spaces& spaces);

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
class Separator : noncopyable
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

/** \brief print attributes of an item
 *
 *  \code
 *  ItemAttributes ia(wantMultiLine, 3);
 *  os << ia("id") << 500
 *     << ia("uri") << "udp4://192.0.2.1:6363"
 *     << ia.end();
 *
 *  // prints in single-line style (wantMultiLine==false):
 *  // id=500 uri=udp4://192.0.2.1:6363 [no-newline]
 *
 *  // prints in multi-line style (wantMultiLine==true):
 *  //  id=500
 *  // uri=udp4://192.0.2.1:6363 [newline]
 *  \endcode
 */
class ItemAttributes : noncopyable
{
public:
  /** \brief constructor
   *  \param wantMultiLine true to select multi-line style, false to use single-line style
   *  \param maxAttributeWidth maximum width of attribute names, for alignment in multi-line style
   */
  explicit
  ItemAttributes(bool wantMultiLine = false, int maxAttributeWidth = 0);

  struct Attribute
  {
    ItemAttributes& ia;
    std::string attribute;
  };

  /** \note Caller must ensure ItemAttributes object is alive until after all Attribute objects are
   *        destructed.
   */
  Attribute
  operator()(const std::string& attribute);

  std::string
  end() const;

private:
  bool m_wantMultiLine;
  int m_maxAttributeWidth;
  int m_count;

  friend std::ostream& operator<<(std::ostream& os, const ItemAttributes::Attribute& attr);
};

std::ostream&
operator<<(std::ostream& os, const ItemAttributes::Attribute& attr);

/** \brief print boolean as 'on' or 'off'
 */
struct OnOff
{
  bool flag;
};

std::ostream&
operator<<(std::ostream& os, OnOff v);

namespace detail {

template<typename DurationT>
std::string
getTimeUnit(bool isLong);

template<>
inline std::string
getTimeUnit<time::nanoseconds>(bool isLong)
{
  return isLong ? "nanoseconds" : "ns";
}

template<>
inline std::string
getTimeUnit<time::microseconds>(bool isLong)
{
  return isLong ? "microseconds" : "us";
}

template<>
inline std::string
getTimeUnit<time::milliseconds>(bool isLong)
{
  return isLong ? "milliseconds" : "ms";
}

template<>
inline std::string
getTimeUnit<time::seconds>(bool isLong)
{
  return isLong ? "seconds" : "s";
}

template<>
inline std::string
getTimeUnit<time::minutes>(bool isLong)
{
  return isLong ? "minutes" : "m";
}

template<>
inline std::string
getTimeUnit<time::hours>(bool isLong)
{
  return isLong ? "hours" : "h";
}

template<>
inline std::string
getTimeUnit<time::days>(bool isLong)
{
  return isLong ? "days" : "d";
}

} // namespace detail

template<typename OutputPrecision>
std::string
formatDuration(time::nanoseconds d, bool isLong = false)
{
  return to_string(time::duration_cast<OutputPrecision>(d).count()) +
         (isLong ? " " : "") + detail::getTimeUnit<OutputPrecision>(isLong);
}

std::string
formatTimestamp(time::system_clock::TimePoint t);

} // namespace text

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_FORMAT_HELPERS_HPP
