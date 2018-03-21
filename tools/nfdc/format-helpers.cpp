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

#include "format-helpers.hpp"

#include <iomanip>
#include <sstream>

namespace nfd {
namespace tools {
namespace nfdc {

namespace xml {

void
printHeader(std::ostream& os)
{
  os << "<?xml version=\"1.0\"?>"
     << "<nfdStatus xmlns=\"ndn:/localhost/nfd/status/1\">";
}

void
printFooter(std::ostream& os)
{
  os << "</nfdStatus>";
}

std::ostream&
operator<<(std::ostream& os, const Text& text)
{
  for (char ch : text.s) {
    switch (ch) {
      case '"':
        os << "&quot;";
        break;
      case '&':
        os << "&amp;";
        break;
      case '\'':
        os << "&apos;";
        break;
      case '<':
        os << "&lt;";
        break;
      case '>':
        os << "&gt;";
        break;
      default:
        os << ch;
        break;
    }
  }
  return os;
}

std::ostream&
operator<<(std::ostream& os, Flag v)
{
  if (!v.flag) {
    return os;
  }
  return os << '<' << v.elementName << "/>";
}

std::string
formatDuration(time::nanoseconds d)
{
  std::ostringstream str;

  if (d < 0_ns) {
    str << "-";
  }

  str << "PT";

  time::seconds seconds(time::duration_cast<time::seconds>(time::abs(d)));
  time::milliseconds ms(time::duration_cast<time::milliseconds>(time::abs(d) - seconds));

  str << seconds.count();

  if (ms >= 1_ms) {
    str << "." << std::setfill('0') << std::setw(3) << ms.count();
  }

  str << "S";

  return str.str();
}

std::string
formatTimestamp(time::system_clock::TimePoint t)
{
  return time::toString(t, "%Y-%m-%dT%H:%M:%S%F");
}

} // namespace xml

namespace text {

std::ostream&
operator<<(std::ostream& os, const Spaces& spaces)
{
  for (int i = 0; i < spaces.nSpaces; ++i) {
    os << ' ';
  }
  return os;
}

Separator::Separator(const std::string& first, const std::string& subsequent)
  : m_first(first)
  , m_subsequent(subsequent)
  , m_count(0)
{
}

Separator::Separator(const std::string& subsequent)
  : Separator("", subsequent)
{
}

std::ostream&
operator<<(std::ostream& os, Separator& sep)
{
  if (++sep.m_count == 1) {
    return os << sep.m_first;
  }
  return os << sep.m_subsequent;
}

ItemAttributes::ItemAttributes(bool wantMultiLine, int maxAttributeWidth)
  : m_wantMultiLine(wantMultiLine)
  , m_maxAttributeWidth(maxAttributeWidth)
  , m_count(0)
{
}

ItemAttributes::Attribute
ItemAttributes::operator()(const std::string& attribute)
{
  return {*this, attribute};
}

std::string
ItemAttributes::end() const
{
  return m_wantMultiLine ? "\n" : "";
}

std::ostream&
operator<<(std::ostream& os, const ItemAttributes::Attribute& attr)
{
  ++attr.ia.m_count;
  if (attr.ia.m_wantMultiLine) {
    if (attr.ia.m_count > 1) {
      os << '\n';
    }
    os << Spaces{attr.ia.m_maxAttributeWidth - static_cast<int>(attr.attribute.size())};
  }
  else {
    if (attr.ia.m_count > 1) {
      os << ' ';
    }
  }
  return os << attr.attribute << '=';
}

std::ostream&
operator<<(std::ostream& os, OnOff v)
{
  return os << (v.flag ? "on" : "off");
}

std::string
formatTimestamp(time::system_clock::TimePoint t)
{
  return time::toIsoString(t);
}

} // namespace text

} // namespace nfdc
} // namespace tools
} // namespace nfd
