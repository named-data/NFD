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

#include "status-report.hpp"
#include "format-helpers.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

ReportFormat
parseReportFormat(const std::string& s)
{
  if (s == "xml") {
    return ReportFormat::XML;
  }
  if (s == "text") {
    return ReportFormat::TEXT;
  }
  BOOST_THROW_EXCEPTION(std::invalid_argument("unrecognized ReportFormat '" + s + "'"));
}

std::ostream&
operator<<(std::ostream& os, ReportFormat fmt)
{
  switch (fmt) {
    case ReportFormat::XML:
      return os << "xml";
    case ReportFormat::TEXT:
      return os << "text";
  }
  return os << static_cast<int>(fmt);
}

uint32_t
StatusReport::collect(Face& face, KeyChain& keyChain, Validator& validator, const CommandOptions& options)
{
  Controller controller(face, keyChain, validator);
  uint32_t errorCode = 0;

  for (size_t i = 0; i < sections.size(); ++i) {
    Module& module = *sections[i];
    module.fetchStatus(
      controller,
      []{},
      [i, &errorCode] (uint32_t code, const std::string& reason) {
        errorCode = i * 1000000 + code;
      },
      options);
  }

  this->processEvents(face);
  return errorCode;
}

void
StatusReport::processEvents(Face& face)
{
  face.processEvents();
}

void
StatusReport::formatXml(std::ostream& os) const
{
  xml::printHeader(os);
  for (const unique_ptr<Module>& module : sections) {
    module->formatStatusXml(os);
  }
  xml::printFooter(os);
}

void
StatusReport::formatText(std::ostream& os) const
{
  for (const unique_ptr<Module>& module : sections) {
    module->formatStatusText(os);
  }
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
