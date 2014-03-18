/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "nrd.hpp"

namespace ndn {
namespace nrd {

const Name Nrd::COMMAND_PREFIX = "/localhost/nrd";
const Name Nrd::REMOTE_COMMAND_PREFIX = "/localhop/nrd";

const size_t Nrd::COMMAND_UNSIGNED_NCOMPS =
  Nrd::COMMAND_PREFIX.size() +
  1 + // verb
  1;  // verb options

const size_t Nrd::COMMAND_SIGNED_NCOMPS =
  Nrd::COMMAND_UNSIGNED_NCOMPS +
  4; // (timestamp, nonce, signed info tlv, signature tlv)

const Nrd::VerbAndProcessor Nrd::COMMAND_VERBS[] =
  {
    VerbAndProcessor(
                     Name::Component("register"),
                     &Nrd::insertEntry
                     ),

    VerbAndProcessor(
                     Name::Component("unregister"),
                     &Nrd::deleteEntry
                     ),
  };

void
Nrd::setInterestFilterFailed(const Name& name, const std::string& msg) 
{
  std::cerr << "Error in setting interest filter (" << name << "): " << msg << std::endl;
  m_face.shutdown();
}

Nrd::Nrd()
  : m_nfdController(new nfd::Controller(m_face)),
    m_verbDispatch(COMMAND_VERBS,
                   COMMAND_VERBS + (sizeof(COMMAND_VERBS) / sizeof(VerbAndProcessor)))
{
  //check whether the components of localhop and localhost prefixes are same
  BOOST_ASSERT(COMMAND_PREFIX.size() == REMOTE_COMMAND_PREFIX.size ()); 

  std::cerr << "Setting interest filter on: " << COMMAND_PREFIX.toUri() << std::endl;
  m_face.setController(m_nfdController);
  m_face.setInterestFilter(COMMAND_PREFIX.toUri(),
                           bind(&Nrd::onRibRequest, this, _2),
                           bind(&Nrd::setInterestFilterFailed, this, _1, _2));
  
  std::cerr << "Setting interest filter on: " << REMOTE_COMMAND_PREFIX.toUri() << std::endl;
  m_face.setInterestFilter(REMOTE_COMMAND_PREFIX.toUri(),
                           bind(&Nrd::onRibRequest, this, _2),
                           bind(&Nrd::setInterestFilterFailed, this, _1, _2));
}

void
Nrd::sendResponse(const Name& name,
                  const nfd::ControlResponse& response)
{
  const Block& encodedControl = response.wireEncode();

  Data responseData(name);
  responseData.setContent(encodedControl);

  m_keyChain.sign(responseData);
  m_face.put(responseData);
}

void
Nrd::sendResponse(const Name& name,
                  uint32_t code,
                  const std::string& text)
{
  nfd::ControlResponse response(code, text);
  sendResponse(name, response);
}

void
Nrd::onRibRequest(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
      commandNComps < COMMAND_SIGNED_NCOMPS)
    {
      std::cerr << "Error: Signature required" << std::endl;
      sendResponse(command, 401, "Signature required");
      return;
    }
  else if (commandNComps < COMMAND_SIGNED_NCOMPS ||
      !(COMMAND_PREFIX.isPrefixOf(command)  ||
       REMOTE_COMMAND_PREFIX.isPrefixOf(command)))
  {
      std::cerr << "Error: Malformed Command" << std::endl;
      sendResponse(command, 400, "Malformed command");
      return;
    }

  //REMOTE_COMMAND_PREFIX number of componenets are same as 
  // NRD_COMMAND_PREFIX's so no extra checks are required.
  const Name::Component& verb = command.get(COMMAND_PREFIX.size());
  VerbDispatchTable::const_iterator verbProcessor = m_verbDispatch.find(verb);

  if (verbProcessor != m_verbDispatch.end())
    {
      PrefixRegOptions options;
      if (!extractOptions(request, options))
        {
          sendResponse(command, 400, "Malformed command");
          return;
        }

      /// \todo authorize command
      if (false)
        {
          sendResponse(request.getName(), 403, "Unauthorized command");
          return;
        }

      // \todo add proper log support
      std::cout << "Received options (name, faceid, cost): " << options.getName() << 
                   ", " << options.getFaceId() << ", "  << options.getCost() << std::endl;

      nfd::ControlResponse response;
      (verbProcessor->second)(this, request, options);
    }
  else
    {
      sendResponse(request.getName(), 501, "Unsupported command");
    }
}

bool
Nrd::extractOptions(const Interest& request,
                    PrefixRegOptions& extractedOptions)
{
  const Name& command = request.getName();
  //REMOTE_COMMAND_PREFIX is same in size of NRD_COMMAND_PREFIX 
  //so no extra processing is required.
  const size_t optionCompIndex = COMMAND_PREFIX.size() + 1;

  try
    {
      Block rawOptions = request.getName()[optionCompIndex].blockFromValue();
      extractedOptions.wireDecode(rawOptions);
    }
  catch (const ndn::Tlv::Error& e)
    {
      return false;
    }

  if (extractedOptions.getFaceId() == 0)
    {
      std::cout <<"IncomingFaceId: " << request.getIncomingFaceId() << std::endl;
      extractedOptions.setFaceId(request.getIncomingFaceId());
    }
  return true;
}

void
Nrd::onCommandError(const std::string& error, 
                    const Interest& request, 
                    const PrefixRegOptions& options)
{
  nfd::ControlResponse response;
  
  response.setCode(400);
  response.setText(error);
  
  std::cout << "Error: " << error << std::endl;
  sendResponse(request.getName(), response);
  m_managedRib.erase(options);
}

void
Nrd::onUnRegSuccess(const Interest& request, const PrefixRegOptions& options)
{
  nfd::ControlResponse response;
  
  response.setCode(200);
  response.setText("Success");
  response.setBody(options.wireEncode());
 
  std::cout << "Success: Name unregistered (" << 
                options.getName() << ", " << 
                options.getFaceId() << ")" << std::endl;
  sendResponse(request.getName(), response);
  m_managedRib.erase(options);
}

void
Nrd::onRegSuccess(const Interest& request, const PrefixRegOptions& options)
{
  nfd::ControlResponse response;
  
  response.setCode(200);
  response.setText("Success");
  response.setBody(options.wireEncode());
  
  std::cout << "Success: Name registered (" << options.getName() << ", " << 
                                               options.getFaceId() << ")" << std::endl;
  sendResponse(request.getName(), response);
}


void
Nrd::insertEntry(const Interest& request, const PrefixRegOptions& options)
{
  // For right now, just pass the options to fib as it is,
  // without processing flags. Later options will be first added to
  // Rib tree, then nrd will generate fib updates based on flags and then
  // will add next hops one by one..
  m_managedRib.insert(options);
  m_nfdController->fibAddNextHop(options.getName(), options.getFaceId(),
                                 options.getCost(),
                                 bind(&Nrd::onRegSuccess, this, request, options),
                                 bind(&Nrd::onCommandError, this, _1, request, options));
}


void
Nrd::deleteEntry(const Interest& request, const PrefixRegOptions& options)
{
  m_nfdController->fibRemoveNextHop(options.getName(),
                                    options.getFaceId(),
                                    bind(&Nrd::onUnRegSuccess, this, request, options),
                                    bind(&Nrd::onCommandError, this, _1, request, options));
}


void
Nrd::listen()
{
  std::cout << "NRD started: listening for incoming interests" << std::endl;
  m_face.processEvents();
}


void
Nrd::onControlHeaderSuccess()
{
  std::cout << "Local control header enabled" << std::endl;
}


void
Nrd::onControlHeaderError()
{
  std::cout << "Error: couldn't enable local control header" << std::endl;
  m_face.shutdown();
}


void
Nrd::enableLocalControlHeader()
{
  Name enable("/localhost/nfd/control-header/in-faceid/enable");
  Interest enableCommand(enable);
 
  m_keyChain.sign(enableCommand);
  m_face.expressInterest(enableCommand,
                         ndn::bind(&Nrd::onControlHeaderSuccess, this),
                         ndn::bind(&Nrd::onControlHeaderError, this));
}

} // namespace nrd
} // namespace ndn
