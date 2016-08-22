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

#ifndef NFD_TOOLS_NFDC_FORWARDER_GENERAL_MODULE_HPP
#define NFD_TOOLS_NFDC_FORWARDER_GENERAL_MODULE_HPP

#include "module.hpp"
#include <ndn-cxx/security/validator.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

using ndn::nfd::ForwarderStatus;

class NfdIdCollector;

/** \brief provides access to NFD forwarder general status
 *  \sa https://redmine.named-data.net/projects/nfd/wiki/ForwarderStatus
 */
class ForwarderGeneralModule : public Module, noncopyable
{
public:
  ForwarderGeneralModule();

  virtual void
  fetchStatus(Controller& controller,
              const function<void()>& onSuccess,
              const Controller::DatasetFailCallback& onFailure,
              const CommandOptions& options) override;

  void
  setNfdIdCollector(const NfdIdCollector& nfdIdCollector)
  {
    m_nfdIdCollector = &nfdIdCollector;
  }

  virtual void
  formatStatusXml(std::ostream& os) const override;

  /** \brief format a single status item as XML
   *  \param os output stream
   *  \param item status item
   *  \param nfdId NFD's signing certificate name
   */
  void
  formatItemXml(std::ostream& os, const ForwarderStatus& item, const Name& nfdId) const;

  virtual void
  formatStatusText(std::ostream& os) const override;

  /** \brief format a single status item as text
   *  \param os output stream
   *  \param item status item
   *  \param nfdId NFD's signing certificate name
   */
  void
  formatItemText(std::ostream& os, const ForwarderStatus& item, const Name& nfdId) const;

private:
  const Name&
  getNfdId() const;

private:
  const NfdIdCollector* m_nfdIdCollector;
  ForwarderStatus m_status;
};


/** \brief a validator that can collect NFD's signing certificate name
 *
 *  This validator redirects all validation requests to an inner validator.
 *  For the first Data packet accepted by the inner validator that has a Name in KeyLocator,
 *  this Name is collected as NFD's signing certificate name.
 */
class NfdIdCollector : public ndn::Validator
{
public:
  explicit
  NfdIdCollector(unique_ptr<ndn::Validator> inner);

  bool
  hasNfdId() const
  {
    return m_hasNfdId;
  }

  const Name&
  getNfdId() const;

protected:
  virtual void
  checkPolicy(const Interest& interest, int nSteps,
              const ndn::OnInterestValidated& accept,
              const ndn::OnInterestValidationFailed& reject,
              std::vector<shared_ptr<ndn::ValidationRequest>>& nextSteps) override
  {
    BOOST_ASSERT(nSteps == 0);
    m_inner->validate(interest, accept, reject);
  }

  virtual void
  checkPolicy(const Data& data, int nSteps,
              const ndn::OnDataValidated& accept,
              const ndn::OnDataValidationFailed& reject,
              std::vector<shared_ptr<ndn::ValidationRequest>>& nextSteps) override;

private:
  unique_ptr<ndn::Validator> m_inner;
  bool m_hasNfdId;
  Name m_nfdId;
};

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_FORWARDER_GENERAL_MODULE_HPP
