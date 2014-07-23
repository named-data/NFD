/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#ifndef NFD_DAEMON_MGMT_FIB_MANAGER_HPP
#define NFD_DAEMON_MGMT_FIB_MANAGER_HPP

#include "common.hpp"
#include "mgmt/manager-base.hpp"
#include "mgmt/fib-enumeration-publisher.hpp"

namespace nfd {

class Face;
class Forwarder;
class Fib;

const std::string FIB_PRIVILEGE = "fib"; // config file privilege name

class FibManager : public ManagerBase
{
public:

  FibManager(Fib& fib,
             function<shared_ptr<Face>(FaceId)> getFace,
             shared_ptr<InternalFace> face,
             ndn::KeyChain& keyChain);

  virtual
  ~FibManager();

  void
  onFibRequest(const Interest& request);

private:

  void
  onValidatedFibRequest(const shared_ptr<const Interest>& request);

  void
  addNextHop(ControlParameters& parameters,
             ControlResponse& response);

  void
  removeNextHop(ControlParameters& parameters,
                ControlResponse& response);

  void
  listEntries(const Interest& request);

private:

  Fib& m_managedFib;
  function<shared_ptr<Face>(FaceId)> m_getFace;
  FibEnumerationPublisher m_fibEnumerationPublisher;

  typedef function<void(FibManager*,
                        ControlParameters&,
                        ControlResponse&)> SignedVerbProcessor;

  typedef std::map<Name::Component, SignedVerbProcessor> SignedVerbDispatchTable;

  typedef std::pair<Name::Component, SignedVerbProcessor> SignedVerbAndProcessor;

  typedef function<void(FibManager*, const Interest&)> UnsignedVerbProcessor;

  typedef std::map<Name::Component, UnsignedVerbProcessor> UnsignedVerbDispatchTable;
  typedef std::pair<Name::Component, UnsignedVerbProcessor> UnsignedVerbAndProcessor;


  const SignedVerbDispatchTable m_signedVerbDispatch;
  const UnsignedVerbDispatchTable m_unsignedVerbDispatch;

  static const Name COMMAND_PREFIX; // /localhost/nfd/fib

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nfd/fib + verb + parameters) = 5
  static const size_t COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // UNSIGNED_NCOMPS + 4 command Interest components = 9
  static const size_t COMMAND_SIGNED_NCOMPS;

  static const SignedVerbAndProcessor SIGNED_COMMAND_VERBS[];
  static const UnsignedVerbAndProcessor UNSIGNED_COMMAND_VERBS[];

  static const Name LIST_COMMAND_PREFIX;
  static const size_t LIST_COMMAND_NCOMPS;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_FIB_MANAGER_HPP
