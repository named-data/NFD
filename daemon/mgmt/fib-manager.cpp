/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fib-manager.hpp"

#include "table/fib.hpp"
#include "fw/forwarder.hpp"
#include "mgmt/internal-face.hpp"
#include "mgmt/app-face.hpp"

#include <ndn-cpp-dev/management/fib-management-options.hpp>

namespace nfd {

NFD_LOG_INIT("FibManager");

const Name FibManager::FIB_MANAGER_REQUEST_PREFIX = "/localhost/nfd/fib";

// In the future this should be 3 (Signature TLV, Signature Info TLV, Timestamp)
const size_t FibManager::FIB_MANAGER_REQUEST_SIGNED_INTEREST_NCOMPS = 0;

const size_t FibManager::FIB_MANAGER_REQUEST_COMMAND_MIN_NCOMPS =
  FibManager::FIB_MANAGER_REQUEST_PREFIX.size() +
  1 + // verb
  1 + // verb options
  FibManager::FIB_MANAGER_REQUEST_SIGNED_INTEREST_NCOMPS;

const Name::Component FibManager::FIB_MANAGER_REQUEST_VERB_INSERT = "insert";
const Name::Component FibManager::FIB_MANAGER_REQUEST_VERB_DELETE = "delete";
const Name::Component FibManager::FIB_MANAGER_REQUEST_VERB_ADD_NEXTHOP = "add-nexthop";
const Name::Component FibManager::FIB_MANAGER_REQUEST_VERB_REMOVE_NEXTHOP = "remove-nexthop";
const Name::Component FibManager::FIB_MANAGER_REQUEST_VERB_STRATEGY = "strategy";


const FibManager::VerbAndProcessor FibManager::FIB_MANAGER_REQUEST_VERBS[] =
  {
    // Unsupported

    // VerbAndProcessor(
    //                  FibManager::FIB_MANAGER_REQUEST_VERB_INSERT,
    //                  &FibManager::fibInsert
    //                  ),

    // VerbAndProcessor(
    //                  FibManager::FIB_MANAGER_REQUEST_VERB_DELETE,
    //                  &FibManager::fibDelete
    //                  ),

    VerbAndProcessor(
                     FibManager::FIB_MANAGER_REQUEST_VERB_ADD_NEXTHOP,
                     &FibManager::fibAddNextHop
                     ),

    // Unsupported

    // VerbAndProcessor(
    //                  FibManager::FIB_MANAGER_REQUEST_VERB_REMOVE_NEXTHOP,
    //                  &FibManager::fibRemoveNextHop
    //                  ),

    // VerbAndProcessor(
    //                  FibManager::FIB_MANAGER_REQUEST_VERB_STRATEGY,
    //                  &FibManager::fibStrategy
    //                  )

  };

FibManager::FibManager(Fib& fib,
                       function<shared_ptr<Face>(FaceId)> getFace,
                       shared_ptr<AppFace> face)
  : ManagerBase(face),
    m_managedFib(fib),
    m_getFace(getFace),
    m_verbDispatch(FIB_MANAGER_REQUEST_VERBS,
                   FIB_MANAGER_REQUEST_VERBS +
                   (sizeof(FIB_MANAGER_REQUEST_VERBS) / sizeof(VerbAndProcessor)))
{

}

void
FibManager::onFibRequest(const Interest& request)
{
  const Name& requestCommand = request.getName();

  /// \todo Separate out response status codes 400 and 401

  if (requestCommand.size() < FIB_MANAGER_REQUEST_COMMAND_MIN_NCOMPS ||
      !FIB_MANAGER_REQUEST_PREFIX.isPrefixOf(requestCommand))
    {
      NFD_LOG_INFO("Malformed command: " << requestCommand);
      sendResponse(request.getName(), 404, "MALFORMED");
      return;
    }

  const Name::Component& requestVerb = requestCommand.get(FIB_MANAGER_REQUEST_PREFIX.size());

  VerbDispatchTable::const_iterator verbProcessor = m_verbDispatch.find (requestVerb);
  if (verbProcessor == m_verbDispatch.end())
    {
      NFD_LOG_INFO("Unsupported command verb: " << requestVerb);
      sendResponse(request.getName(), 404, "UNSUPPORTED");

    }
  else
    {
      NFD_LOG_INFO("Processing command verb: " << requestVerb);
      (verbProcessor->second)(this, request);
    }

}

void
FibManager::fibInsert(const Interest& request)
{

}

void
FibManager::fibDelete(const Interest& request)
{

}

void
FibManager::fibAddNextHop(const Interest& request)
{
  ndn::FibManagementOptions options;
  const size_t optionCompIndex =
    FIB_MANAGER_REQUEST_PREFIX.size() + 1;

  const ndn::Buffer& optionBuffer =
    request.getName()[optionCompIndex].getValue();
  shared_ptr<const ndn::Buffer> tmpOptionBuffer(new ndn::Buffer(optionBuffer));
  Block rawOptions(tmpOptionBuffer);

  options.wireDecode(rawOptions);

  /// \todo authorize command
  if (false)
    {
      NFD_LOG_INFO("Unauthorized command attempt");

      sendResponse(request.getName(), 403, "");
      return;
    }

  NFD_LOG_INFO("add-nexthop Name: " << options.getName()
               << " FaceId: " << options.getFaceId()
               << " Cost: " << options.getCost());

  shared_ptr<Face> nextHopFace = m_getFace(options.getFaceId());
  if (nextHopFace)
    {
      std::pair<shared_ptr<fib::Entry>, bool> insertResult = m_managedFib.insert(options.getName());
      insertResult.first->addNextHop(nextHopFace, options.getCost());
      sendResponse(request.getName(), 200, "OK");
    }
  else
    {
      sendResponse(request.getName(), 400, "INCORRECT");
    }
}

void
FibManager::fibRemoveNextHop(const Interest& request)
{

}

void
FibManager::fibStrategy(const Interest& request)
{

}

// void
// FibManager::onConfig(ConfigFile::Node section, bool isDryRun)
// {

// }

} // namespace nfd
