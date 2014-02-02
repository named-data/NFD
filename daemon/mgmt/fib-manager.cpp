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
#include <ndn-cpp-dev/encoding/tlv.hpp>

#include <ndn-cpp-dev/management/fib-management-options.hpp>

namespace nfd {

NFD_LOG_INIT("FibManager");

const Name FibManager::FIB_MANAGER_COMMAND_PREFIX = "/localhost/nfd/fib";

const size_t FibManager::FIB_MANAGER_COMMAND_UNSIGNED_NCOMPS =
  FibManager::FIB_MANAGER_COMMAND_PREFIX.size() +
  1 + // verb
  1;  // verb options

const size_t FibManager::FIB_MANAGER_COMMAND_SIGNED_NCOMPS =
  FibManager::FIB_MANAGER_COMMAND_UNSIGNED_NCOMPS +
  0; // No signed Interest support in mock, otherwise 3 (timestamp, signed info tlv, signature tlv)

const Name::Component FibManager::FIB_MANAGER_COMMAND_VERB_INSERT = "insert";
const Name::Component FibManager::FIB_MANAGER_COMMAND_VERB_DELETE = "delete";
const Name::Component FibManager::FIB_MANAGER_COMMAND_VERB_ADD_NEXTHOP = "add-nexthop";
const Name::Component FibManager::FIB_MANAGER_COMMAND_VERB_REMOVE_NEXTHOP = "remove-nexthop";
const Name::Component FibManager::FIB_MANAGER_COMMAND_VERB_STRATEGY = "strategy";


const FibManager::VerbAndProcessor FibManager::FIB_MANAGER_COMMAND_VERBS[] =
  {
    // Unsupported

    // VerbAndProcessor(
    //                  FibManager::FIB_MANAGER_COMMAND_VERB_INSERT,
    //                  &FibManager::fibInsert
    //                  ),

    // VerbAndProcessor(
    //                  FibManager::FIB_MANAGER_COMMAND_VERB_DELETE,
    //                  &FibManager::fibDelete
    //                  ),

    VerbAndProcessor(
                     FibManager::FIB_MANAGER_COMMAND_VERB_ADD_NEXTHOP,
                     &FibManager::fibAddNextHop
                     ),

    // Unsupported

    // VerbAndProcessor(
    //                  FibManager::FIB_MANAGER_COMMAND_VERB_REMOVE_NEXTHOP,
    //                  &FibManager::fibRemoveNextHop
    //                  ),

    // VerbAndProcessor(
    //                  FibManager::FIB_MANAGER_COMMAND_VERB_STRATEGY,
    //                  &FibManager::fibStrategy
    //                  )

  };

FibManager::FibManager(Fib& fib,
                       function<shared_ptr<Face>(FaceId)> getFace,
                       shared_ptr<AppFace> face)
  : ManagerBase(face),
    m_managedFib(fib),
    m_getFace(getFace),
    m_verbDispatch(FIB_MANAGER_COMMAND_VERBS,
                   FIB_MANAGER_COMMAND_VERBS +
                   (sizeof(FIB_MANAGER_COMMAND_VERBS) / sizeof(VerbAndProcessor)))
{
  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&FibManager::onFibRequest, this, _2));
}

void
FibManager::onFibRequest(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  /// \todo Separate out response status codes 400 and 401

  if (FIB_MANAGER_COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
      commandNComps < FIB_MANAGER_COMMAND_SIGNED_NCOMPS)
    {
      NFD_LOG_INFO("Unsigned command: " << command);
      sendResponse(command, 401, "SIGREQ");

      return;
    }
  else if (commandNComps < FIB_MANAGER_COMMAND_SIGNED_NCOMPS ||
      !FIB_MANAGER_COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_INFO("Malformed command: " << command);
      sendResponse(command, 400, "MALFORMED");
      return;
    }


  const Name::Component& verb = command.get(FIB_MANAGER_COMMAND_PREFIX.size());

  VerbDispatchTable::const_iterator verbProcessor = m_verbDispatch.find (verb);
  if (verbProcessor != m_verbDispatch.end())
    {
      NFD_LOG_INFO("Processing command verb: " << verb);
      (verbProcessor->second)(this, request);
    }
  else
    {
      NFD_LOG_INFO("Unsupported command verb: " << verb);
      sendResponse(request.getName(), 404, "UNSUPPORTED");
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
  const Name& command = request.getName();
  ndn::FibManagementOptions options;
  const size_t optionCompIndex =
    FIB_MANAGER_COMMAND_PREFIX.size() + 1;

  const ndn::Buffer& optionBuffer =
    request.getName()[optionCompIndex].getValue();
  shared_ptr<const ndn::Buffer> tmpOptionBuffer(make_shared<ndn::Buffer>(optionBuffer));

  try
    {
      Block rawOptions(tmpOptionBuffer);
      options.wireDecode(rawOptions);
    }
  catch (const ndn::Tlv::Error& e)
    {
      NFD_LOG_INFO("Bad command option parse: " << command);
      sendResponse(request.getName(), 400, "MALFORMED");
      return;
    }

  /// \todo authorize command
  if (false)
    {
      NFD_LOG_INFO("Unauthorized command attempt: " << command);
      sendResponse(request.getName(), 403, "UNAUTHORIZED");
      return;
    }

  NFD_LOG_INFO("add-nexthop Name: " << options.getName()
               << " FaceId: " << options.getFaceId()
               << " Cost: " << options.getCost());

  shared_ptr<Face> nextHopFace = m_getFace(options.getFaceId());
  if (nextHopFace != 0)
    {
      std::pair<shared_ptr<fib::Entry>, bool> insertResult = m_managedFib.insert(options.getName());
      insertResult.first->addNextHop(nextHopFace, options.getCost());
      sendResponse(request.getName(), 200, "OK");
    }
  else
    {
      NFD_LOG_INFO("Unknown FaceId: " << command);
      sendResponse(request.getName(), 400, "MALFORMED");
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
