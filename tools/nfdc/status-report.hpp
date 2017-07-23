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

#ifndef NFD_TOOLS_NFDC_STATUS_REPORT_HPP
#define NFD_TOOLS_NFDC_STATUS_REPORT_HPP

#include "module.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/v2/validator.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

using ndn::Face;
using ndn::KeyChain;
using ndn::security::v2::Validator;

enum class ReportFormat {
  XML = 1,
  TEXT = 2
};

ReportFormat
parseReportFormat(const std::string& s);

std::ostream&
operator<<(std::ostream& os, ReportFormat fmt);

/** \brief collects and prints NFD status report
 */
class StatusReport : noncopyable
{
public:
#ifdef WITH_TESTS
  virtual
  ~StatusReport() = default;
#endif

  /** \brief collect status via chosen \p sections
   *
   *  This function is blocking. It has exclusive use of \p face.
   *
   *  \return if status has been fetched successfully, 0;
   *          otherwise, error code from any failed section, plus 1000000 * section index
   */
  uint32_t
  collect(Face& face, KeyChain& keyChain, Validator& validator, const CommandOptions& options);

  /** \brief print an XML report
   *  \param os output stream
   */
  void
  formatXml(std::ostream& os) const;

  /** \brief print a text report
   *  \param os output stream
   */
  void
  formatText(std::ostream& os) const;

private:
  VIRTUAL_WITH_TESTS void
  processEvents(Face& face);

public:
  /** \brief modules through which status is collected
   */
  std::vector<unique_ptr<Module>> sections;
};

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_STATUS_REPORT_HPP
