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

#include <ndn-cpp-dev/encoding/tlv.hpp>

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

const FibManager::VerbAndProcessor FibManager::FIB_MANAGER_COMMAND_VERBS[] =
  {
    VerbAndProcessor(
                     Name::Component("insert"),
                     &FibManager::insertEntry
                     ),

    VerbAndProcessor(
                     Name::Component("delete"),
                     &FibManager::deleteEntry
                     ),

    VerbAndProcessor(
                     Name::Component("add-nexthop"),
                     &FibManager::addNextHop
                     ),



    VerbAndProcessor(
                     Name::Component("remove-nexthop"),
                     &FibManager::removeNextHop
                     ),

    // Unsupported
    // VerbAndProcessor(
    //                  Name::Component("strategy"),
    //                  &FibManager::strategy
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



  if (FIB_MANAGER_COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
      commandNComps < FIB_MANAGER_COMMAND_SIGNED_NCOMPS)
    {
      NFD_LOG_INFO("command result: unsigned verb: " << command);
      sendResponse(command, 401, "Signature required");

      return;
    }
  else if (commandNComps < FIB_MANAGER_COMMAND_SIGNED_NCOMPS ||
      !FIB_MANAGER_COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_INFO("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  const Name::Component& verb = command.get(FIB_MANAGER_COMMAND_PREFIX.size());

  VerbDispatchTable::const_iterator verbProcessor = m_verbDispatch.find (verb);
  if (verbProcessor != m_verbDispatch.end())
    {
      FibManagementOptions options;
      if (!extractOptions(request, options))
        {
          sendResponse(command, 400, "Malformed command");
          return;
        }

      /// \todo authorize command
      if (false)
        {
          NFD_LOG_INFO("command result: unauthorized verb: " << command);
          sendResponse(request.getName(), 403, "Unauthorized command");
          return;
        }

      NFD_LOG_INFO("command result: processing verb: " << verb);
      ControlResponse response;
      (verbProcessor->second)(this, options, response);

      sendResponse(command, response);
    }
  else
    {
      NFD_LOG_INFO("command result: unsupported verb: " << verb);
      sendResponse(request.getName(), 501, "Unsupported command");
    }

}

bool
FibManager::extractOptions(const Interest& request,
                           FibManagementOptions& extractedOptions)
{
  const Name& command = request.getName();
  const size_t optionCompIndex =
    FIB_MANAGER_COMMAND_PREFIX.size() + 1;

  try
    {
      Block rawOptions = request.getName()[optionCompIndex].blockFromValue();
      extractedOptions.wireDecode(rawOptions);
    }
  catch (const ndn::Tlv::Error& e)
    {
      NFD_LOG_INFO("Bad command option parse: " << command);
      return false;
    }

  if (extractedOptions.getFaceId() == 0)
    {
      extractedOptions.setFaceId(request.getIncomingFaceId());
    }

  NFD_LOG_DEBUG("Options parsed OK");
  return true;
}

void
FibManager::insertEntry(const FibManagementOptions& options,
                        ControlResponse& response)
{
  NFD_LOG_DEBUG("insert prefix: " << options.getName());
  NFD_LOG_INFO("insert result: OK"
               << " prefix: " << options.getName());
  std::pair<shared_ptr<fib::Entry>, bool> insertResult = m_managedFib.insert(options.getName());
  setResponse(response, 200, "Success", options.wireEncode());
}

void
FibManager::deleteEntry(const FibManagementOptions& options,
                        ControlResponse& response)
{
  NFD_LOG_DEBUG("delete prefix: " << options.getName());
  NFD_LOG_INFO("delete result: OK"
               << " prefix: " << options.getName());

  m_managedFib.remove(options.getName());
  setResponse(response, 200, "Success", options.wireEncode());
}

static inline bool
nextHopEqPredicate(const fib::NextHop& target, const fib::NextHop& hop)
{
  return target.getFace()->getId() == hop.getFace()->getId();
}

void
FibManager::addNextHop(const FibManagementOptions& options,
                       ControlResponse& response)
{
  NFD_LOG_DEBUG("add-nexthop prefix: " << options.getName()
                << " faceid: " << options.getFaceId()
                << " cost: " << options.getCost());

  shared_ptr<Face> nextHopFace = m_getFace(options.getFaceId());
  if (static_cast<bool>(nextHopFace))
    {
      shared_ptr<fib::Entry> entry = m_managedFib.findExactMatch(options.getName());
      if (static_cast<bool>(entry))
        {
          entry->addNextHop(nextHopFace, options.getCost());

          NFD_LOG_INFO("add-nexthop result: OK"
                       << " prefix:" << options.getName()
                       << " faceid: " << options.getFaceId()
                       << " cost: " << options.getCost());
          setResponse(response, 200, "Success", options.wireEncode());
        }
      else
        {
          NFD_LOG_INFO("add-nexthop result: FAIL reason: unknown-prefix: " << options.getName());
          setResponse(response, 404, "Prefix not found");
        }
    }
  else
    {
      NFD_LOG_INFO("add-nexthop result: FAIL reason: unknown-faceid: " << options.getFaceId());
      setResponse(response, 404, "Face not found");
    }
}

void
FibManager::removeNextHop(const FibManagementOptions& options,
                          ControlResponse &response)
{
  NFD_LOG_DEBUG("remove-nexthop prefix: " << options.getName()
                << " faceid: " << options.getFaceId());

  shared_ptr<Face> faceToRemove = m_getFace(options.getFaceId());
  if (static_cast<bool>(faceToRemove))
    {
      shared_ptr<fib::Entry> entry = m_managedFib.findExactMatch(options.getName());
      if (static_cast<bool>(entry))
        {
          entry->removeNextHop(faceToRemove);
          NFD_LOG_INFO("remove-nexthop result: OK prefix: " << options.getName()
                       << " faceid: " << options.getFaceId());

          setResponse(response, 200, "Success", options.wireEncode());
        }
      else
        {
          NFD_LOG_INFO("remove-nexthop result: FAIL reason: unknown-prefix: "
                       << options.getName());
          setResponse(response, 404, "Prefix not found");
        }
    }
  else
    {
      NFD_LOG_INFO("remove-nexthop result: FAIL reason: unknown-faceid: "
                   << options.getFaceId());
      setResponse(response, 404, "Face not found");
    }
}

void
FibManager::strategy(const FibManagementOptions& options, ControlResponse& response)
{

}

// void
// FibManager::onConfig(ConfigFile::Node section, bool isDryRun)
// {

// }

} // namespace nfd
