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

const Name FibManager::COMMAND_PREFIX = "/localhost/nfd/fib";

const size_t FibManager::COMMAND_UNSIGNED_NCOMPS =
  FibManager::COMMAND_PREFIX.size() +
  1 + // verb
  1;  // verb options

const size_t FibManager::COMMAND_SIGNED_NCOMPS =
  FibManager::COMMAND_UNSIGNED_NCOMPS +
  4; // (timestamp, nonce, signed info tlv, signature tlv)

const FibManager::VerbAndProcessor FibManager::COMMAND_VERBS[] =
  {

    VerbAndProcessor(
                     Name::Component("add-nexthop"),
                     &FibManager::addNextHop
                     ),

    VerbAndProcessor(
                     Name::Component("remove-nexthop"),
                     &FibManager::removeNextHop
                     ),

  };

FibManager::FibManager(Fib& fib,
                       function<shared_ptr<Face>(FaceId)> getFace,
                       shared_ptr<InternalFace> face)
  : ManagerBase(face, FIB_PRIVILEGE),
    m_managedFib(fib),
    m_getFace(getFace),
    m_verbDispatch(COMMAND_VERBS,
                   COMMAND_VERBS +
                   (sizeof(COMMAND_VERBS) / sizeof(VerbAndProcessor)))
{
  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&FibManager::onFibRequest, this, _2));
}

FibManager::~FibManager()
{

}

void
FibManager::onFibRequest(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
      commandNComps < COMMAND_SIGNED_NCOMPS)
    {
      NFD_LOG_INFO("command result: unsigned verb: " << command);
      sendResponse(command, 401, "Signature required");

      return;
    }
  else if (commandNComps < COMMAND_SIGNED_NCOMPS ||
      !COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_INFO("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  validate(request,
           bind(&FibManager::onValidatedFibRequest,
                this, _1),
           bind(&ManagerBase::onCommandValidationFailed,
                this, _1, _2));
}

void
FibManager::onValidatedFibRequest(const shared_ptr<const Interest>& request)
{
  const Name& command = request->getName();
  const Name::Component& verb = command.get(COMMAND_PREFIX.size());

  VerbDispatchTable::const_iterator verbProcessor = m_verbDispatch.find (verb);
  if (verbProcessor != m_verbDispatch.end())
    {
      FibManagementOptions options;
      if (!extractOptions(*request, options))
        {
          NFD_LOG_INFO("command result: malformed verb: " << verb);
          sendResponse(command, 400, "Malformed command");
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
      sendResponse(command, 501, "Unsupported command");
    }
}

bool
FibManager::extractOptions(const Interest& request,
                           FibManagementOptions& extractedOptions)
{
  const Name& command = request.getName();
  const size_t optionCompIndex = COMMAND_PREFIX.size() + 1;

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
FibManager::addNextHop(const FibManagementOptions& options,
                       ControlResponse& response)
{
  NFD_LOG_DEBUG("add-nexthop prefix: " << options.getName()
                << " faceid: " << options.getFaceId()
                << " cost: " << options.getCost());

  shared_ptr<Face> nextHopFace = m_getFace(options.getFaceId());
  if (static_cast<bool>(nextHopFace))
    {
      shared_ptr<fib::Entry> entry = m_managedFib.insert(options.getName()).first;

      entry->addNextHop(nextHopFace, options.getCost());

      NFD_LOG_INFO("add-nexthop result: OK"
                   << " prefix:" << options.getName()
                   << " faceid: " << options.getFaceId()
                   << " cost: " << options.getCost());
      setResponse(response, 200, "Success", options.wireEncode());
    }
  else
    {
      NFD_LOG_INFO("add-nexthop result: FAIL reason: unknown-faceid: " << options.getFaceId());
      setResponse(response, 404, "Face not found");
    }
}

void
FibManager::removeNextHop(const FibManagementOptions& options,
                          ControlResponse& response)
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

          if (!entry->hasNextHops())
            {
              m_managedFib.erase(*entry);
            }

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

} // namespace nfd
