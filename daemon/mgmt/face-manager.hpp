/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_FACE_MANAGER_HPP
#define NFD_MGMT_FACE_MANAGER_HPP

#include "common.hpp"
#include "face/face.hpp"
#include "mgmt/app-face.hpp"
#include "mgmt/manager-base.hpp"
#include "mgmt/config-file.hpp"

#include <ndn-cpp-dev/management/nfd-face-management-options.hpp>
#include <ndn-cpp-dev/management/nfd-control-response.hpp>

namespace nfd {

const std::string FACE_MANAGER_PRIVILEGE = "faces";
class FaceTable;
class ProtocolFactory;

class FaceManager : public ManagerBase
{
public:
  struct Error : public ManagerBase::Error
  {
    Error(const std::string& what) : ManagerBase::Error(what) {}
  };

  /**
   * \throws FaceManager::Error if localPort is an invalid port number
   */

  FaceManager(FaceTable& faceTable,
              shared_ptr<AppFace> face);

  /** \brief Subscribe to a face management section(s) for the config file
   */
  void
  setConfigFile(ConfigFile& configFile);

  void
  onFaceRequest(const Interest& request);

  VIRTUAL_WITH_TESTS
  ~FaceManager();

PROTECTED_WITH_TESTS_ELSE_PRIVATE:

  void
  onValidatedFaceRequest(const shared_ptr<const Interest>& request);

  VIRTUAL_WITH_TESTS void
  createFace(const Name& requestName,
             ndn::nfd::FaceManagementOptions& options);

  VIRTUAL_WITH_TESTS void
  destroyFace(const Name& requestName,
              ndn::nfd::FaceManagementOptions& options);

  bool
  extractOptions(const Interest& request,
                 ndn::nfd::FaceManagementOptions& extractedOptions);

  void
  onCreated(const Name& requestName,
            ndn::nfd::FaceManagementOptions& options,
            const shared_ptr<Face>& newFace);

  void
  onConnectFailed(const Name& requestName, const std::string& reason);

private:
  void
  onConfig(const ConfigSection& configSection, bool isDryRun);

  void
  processSectionUnix(const ConfigSection& configSection, bool isDryRun);

  void
  processSectionTcp(const ConfigSection& configSection, bool isDryRun);

  void
  processSectionUdp(const ConfigSection& configSection, bool isDryRun);

  void
  processSectionEther(const ConfigSection& configSection, bool isDryRun);

  /** \brief parse a config option that can be either "yes" or "no"
   *  \throw ConfigFile::Error value is neither "yes" nor "no"
   *  \return true if "yes", false if "no"
   */
  bool
  parseYesNo(const ConfigSection::const_iterator& i,
             const std::string& optionName,
             const std::string& sectionName);

private:
  typedef std::map< std::string/*protocol*/, shared_ptr<ProtocolFactory> > FactoryMap;
  FactoryMap m_factories;
  FaceTable& m_faceTable;
  //

  typedef function<void(FaceManager*,
                        const Name&,
                        ndn::nfd::FaceManagementOptions&)> VerbProcessor;

  typedef std::map<Name::Component, VerbProcessor> VerbDispatchTable;

  typedef std::pair<Name::Component, VerbProcessor> VerbAndProcessor;

  const VerbDispatchTable m_verbDispatch;

  static const Name COMMAND_PREFIX; // /localhost/nfd/fib

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nfd/fib + verb + options) = 5
  static const size_t COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // (see UNSIGNED_NCOMPS), 9 with signed Interest support.
  static const size_t COMMAND_SIGNED_NCOMPS;

  static const VerbAndProcessor COMMAND_VERBS[];
};

inline bool
FaceManager::parseYesNo(const ConfigSection::const_iterator& i,
                        const std::string& optionName,
                        const std::string& sectionName)
{
  const std::string value = i->second.get_value<std::string>();
  if (value == "yes")
    {
      return true;
    }
  else if (value == "no")
    {
      return false;
    }

  throw ConfigFile::Error("Invalid value for option \"" +
                          optionName + "\" in \"" +
                          sectionName + "\" section");

}

} // namespace nfd

#endif // NFD_MGMT_FACE_MANAGER_HPP
