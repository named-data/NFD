/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_FIB_MANAGER_HPP
#define NFD_MGMT_FIB_MANAGER_HPP

#include "common.hpp"
#include "face/face.hpp"
#include "mgmt/manager-base.hpp"

namespace nfd {

class AppFace;
class Face;
class Strategy;
class Forwarder;
class Fib;

class FibManager : public ManagerBase
{
public:

  FibManager(Fib& fib, function<shared_ptr<Face>(FaceId)> getFace);

  void
  onFibRequest(const Interest& request);

  const Name&
  getRequestPrefix() const { return FIB_MANAGER_REQUEST_PREFIX; }

private:

  void
  fibInsert(const Interest& request);

  void
  fibDelete(const Interest& request);

  void
  fibAddNextHop(const Interest& request);

  void
  fibRemoveNextHop(const Interest& request);

  void
  fibStrategy(const Interest& request);

  // void
  // onConfig(ConfigFile::Node section, bool isDryRun);

private:

  Fib& m_managedFib;
  function<shared_ptr<Face>(FaceId)> m_getFace;
  std::map<Name, shared_ptr<Strategy> > m_namespaceToStrategyMap;



  typedef function<void(FibManager*,
                        const Interest&)> VerbProcessor;

  typedef std::map<Name::Component, VerbProcessor> VerbDispatchTable;

  typedef std::pair<Name::Component, VerbProcessor> VerbAndProcessor;


  const VerbDispatchTable m_verbDispatch;

  static const Name FIB_MANAGER_REQUEST_PREFIX;
  static const size_t FIB_MANAGER_REQUEST_COMMAND_MIN_NCOMPS;
  static const size_t FIB_MANAGER_REQUEST_SIGNED_INTEREST_NCOMPS;

  static const Name::Component FIB_MANAGER_REQUEST_VERB_INSERT;
  static const Name::Component FIB_MANAGER_REQUEST_VERB_DELETE;
  static const Name::Component FIB_MANAGER_REQUEST_VERB_ADD_NEXTHOP;
  static const Name::Component FIB_MANAGER_REQUEST_VERB_REMOVE_NEXTHOP;
  static const Name::Component FIB_MANAGER_REQUEST_VERB_STRATEGY;

  static const VerbAndProcessor FIB_MANAGER_REQUEST_VERBS[];

};

} // namespace nfd

#endif // NFD_MGMT_FIB_MANAGER_HPP
