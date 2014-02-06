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

#include <ndn-cpp-dev/management/fib-management-options.hpp>
#include <ndn-cpp-dev/management/control-response.hpp>

namespace nfd {

class Forwarder;
class Fib;

class FibManager : public ManagerBase
{
public:

  FibManager(Fib& fib,
             function<shared_ptr<Face>(FaceId)> getFace,
             shared_ptr<AppFace> face);

  void
  onFibRequest(const Interest& request);

private:

  void
  insertEntry(const ndn::FibManagementOptions& options,
                ndn::ControlResponse& response);

  void
  deleteEntry(const ndn::FibManagementOptions& options,
                ndn::ControlResponse& response);

  void
  addNextHop(const ndn::FibManagementOptions& options,
               ndn::ControlResponse& response);

  void
  removeNextHop(const ndn::FibManagementOptions& options,
                  ndn::ControlResponse& response);

  void
  strategy(const ndn::FibManagementOptions& options,
             ndn::ControlResponse& response);

  bool
  extractOptions(const Interest& request,
                   ndn::FibManagementOptions& extractedOptions);

  // void
  // onConfig(ConfigFile::Node section, bool isDryRun);

private:

  Fib& m_managedFib;
  function<shared_ptr<Face>(FaceId)> m_getFace;
  std::map<Name, shared_ptr<fw::Strategy> > m_namespaceToStrategyMap;

  typedef function<void(FibManager*,
                          const ndn::FibManagementOptions&,
                          ndn::ControlResponse&)> VerbProcessor;

  typedef std::map<Name::Component, VerbProcessor> VerbDispatchTable;

  typedef std::pair<Name::Component, VerbProcessor> VerbAndProcessor;


  const VerbDispatchTable m_verbDispatch;

  static const Name FIB_MANAGER_COMMAND_PREFIX; // /localhost/nfd/fib

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nfd/fib + verb + options) = 5
  static const size_t FIB_MANAGER_COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // 5 in mock (see UNSIGNED_NCOMPS), 8 with signed Interest support.
  static const size_t FIB_MANAGER_COMMAND_SIGNED_NCOMPS;

  static const VerbAndProcessor FIB_MANAGER_COMMAND_VERBS[];

};

} // namespace nfd

#endif // NFD_MGMT_FIB_MANAGER_HPP
