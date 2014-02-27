/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_FIB_MANAGER_HPP
#define NFD_MGMT_FIB_MANAGER_HPP

#include "common.hpp"
#include "face/face.hpp"
#include "mgmt/app-face.hpp"
#include "fw/strategy.hpp"
#include "mgmt/manager-base.hpp"

#include <ndn-cpp-dev/management/nfd-fib-management-options.hpp>

namespace nfd {

using ndn::nfd::FibManagementOptions;

class Forwarder;
class Fib;

const std::string FIB_PRIVILEGE = "fib"; // config file privilege name

class FibManager : public ManagerBase
{
public:

  FibManager(Fib& fib,
             function<shared_ptr<Face>(FaceId)> getFace,
             shared_ptr<InternalFace> face);

  void
  onFibRequest(const Interest& request);

private:

  void
  onValidatedFibRequest(const shared_ptr<const Interest>& request);

  void
  insertEntry(const FibManagementOptions& options,
              ControlResponse& response);


  void
  deleteEntry(const FibManagementOptions& options,
              ControlResponse& response);

  void
  addNextHop(const FibManagementOptions& options,
             ControlResponse& response);

  void
  removeNextHop(const FibManagementOptions& options,
                ControlResponse& response);

  bool
  extractOptions(const Interest& request,
                 FibManagementOptions& extractedOptions);

private:

  Fib& m_managedFib;
  function<shared_ptr<Face>(FaceId)> m_getFace;
  std::map<Name, shared_ptr<fw::Strategy> > m_namespaceToStrategyMap;

  typedef function<void(FibManager*,
                        const FibManagementOptions&,
                        ControlResponse&)> VerbProcessor;

  typedef std::map<Name::Component, VerbProcessor> VerbDispatchTable;

  typedef std::pair<Name::Component, VerbProcessor> VerbAndProcessor;


  const VerbDispatchTable m_verbDispatch;

  static const Name COMMAND_PREFIX; // /localhost/nfd/fib

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nfd/fib + verb + options) = 5
  static const size_t COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // UNSIGNED_NCOMPS + 4 command Interest components = 9
  static const size_t COMMAND_SIGNED_NCOMPS;

  static const VerbAndProcessor COMMAND_VERBS[];

};

} // namespace nfd

#endif // NFD_MGMT_FIB_MANAGER_HPP
