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
  m_face->shutdown();
}

Nrd::Nrd(const std::string& validatorConfig)
  : m_face(new Face())
  , m_validator(m_face)
  , m_nfdController(new nfd::Controller(*m_face))
  , m_verbDispatch(COMMAND_VERBS,
                   COMMAND_VERBS + (sizeof(COMMAND_VERBS) / sizeof(VerbAndProcessor)))
{
  //check whether the components of localhop and localhost prefixes are same
  BOOST_ASSERT(COMMAND_PREFIX.size() == REMOTE_COMMAND_PREFIX.size());

  m_validator.load(validatorConfig);

  std::cerr << "Setting interest filter on: " << COMMAND_PREFIX.toUri() << std::endl;
  m_face->setController(m_nfdController);
  m_face->setInterestFilter(COMMAND_PREFIX.toUri(),
                            bind(&Nrd::onRibRequest, this, _2),
                            bind(&Nrd::setInterestFilterFailed, this, _1, _2));

  std::cerr << "Setting interest filter on: " << REMOTE_COMMAND_PREFIX.toUri() << std::endl;
  m_face->setInterestFilter(REMOTE_COMMAND_PREFIX.toUri(),
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
  m_face->put(responseData);
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
  m_validator.validate(request,
                       bind(&Nrd::onRibRequestValidated, this, _1),
                       bind(&Nrd::onRibRequestValidationFailed, this, _1, _2));
}

void
Nrd::onRibRequestValidated(const shared_ptr<const Interest>& request)
{
  const Name& command = request->getName();

  //REMOTE_COMMAND_PREFIX number of componenets are same as
  // NRD_COMMAND_PREFIX's so no extra checks are required.
  const Name::Component& verb = command.get(COMMAND_PREFIX.size());
  VerbDispatchTable::const_iterator verbProcessor = m_verbDispatch.find(verb);

  if (verbProcessor != m_verbDispatch.end())
    {
      PrefixRegOptions options;
      if (!extractOptions(*request, options))
        {
          sendResponse(command, 400, "Malformed command");
          return;
        }

      /// \todo authorize command
      if (false)
        {
          sendResponse(request->getName(), 403, "Unauthorized command");
          return;
        }

      // \todo add proper log support
      std::cout << "Received options (name, faceid, cost): " << options.getName() <<
        ", " << options.getFaceId() << ", "  << options.getCost() << std::endl;

      nfd::ControlResponse response;
      (verbProcessor->second)(this, *request, options);
    }
  else
    {
      sendResponse(request->getName(), 501, "Unsupported command");
    }
}

void
Nrd::onRibRequestValidationFailed(const shared_ptr<const Interest>& request,
                                  const std::string& failureInfo)
{
  sendResponse(request->getName(), 403, failureInfo);
}

bool
Nrd::extractOptions(const Interest& request,
                    PrefixRegOptions& extractedOptions)
{
  // const Name& command = request.getName();
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
Nrd::onCommandError(uint32_t code, const std::string& error,
                    const Interest& request,
                    const PrefixRegOptions& options)
{
  std::cout << "NFD Error: " << error << " (code: " << code << ")" << std::endl;

  nfd::ControlResponse response;

  if (code == 404)
    {
      response.setCode(code);
      response.setText(error);
    }
  else
    {
      response.setCode(533);
      std::ostringstream os;
      os << "Failure to update NFD " << "(NFD Error: " << code << " " << error << ")";
      response.setText(os.str());
    }

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
  m_nfdController->start<nfd::FibAddNextHopCommand>(
    nfd::ControlParameters()
      .setName(options.getName())
      .setFaceId(options.getFaceId())
      .setCost(options.getCost()),
    bind(&Nrd::onRegSuccess, this, request, options),
    bind(&Nrd::onCommandError, this, _1, _2, request, options));
}


void
Nrd::deleteEntry(const Interest& request, const PrefixRegOptions& options)
{
  m_nfdController->start<nfd::FibRemoveNextHopCommand>(
    nfd::ControlParameters()
      .setName(options.getName())
      .setFaceId(options.getFaceId()),
    bind(&Nrd::onUnRegSuccess, this, request, options),
    bind(&Nrd::onCommandError, this, _1, _2, request, options));
}


void
Nrd::listen()
{
  std::cout << "NRD started: listening for incoming interests" << std::endl;
  m_face->processEvents();
}


void
Nrd::onControlHeaderSuccess()
{
  std::cout << "Local control header enabled" << std::endl;
}


void
Nrd::onControlHeaderError(uint32_t code, const std::string& reason)
{
  std::cout << "Error: couldn't enable local control header "
            << "(code: " << code << ", info: " << reason << ")" << std::endl;
  m_face->shutdown();
}


void
Nrd::enableLocalControlHeader()
{
  m_nfdController->start<nfd::FaceEnableLocalControlCommand>(
    nfd::ControlParameters()
      .setLocalControlFeature(nfd::LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID),
    bind(&Nrd::onControlHeaderSuccess, this),
    bind(&Nrd::onControlHeaderError, this, _1, _2));
}

} // namespace nrd
} // namespace ndn
