/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_RIB_RIB_MANAGER_HPP
#define NFD_RIB_RIB_MANAGER_HPP

#include "rib.hpp"
#include "core/config-file.hpp"
#include "rib-status-publisher.hpp"
#include "remote-registrator.hpp"

#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/management/nfd-face-monitor.hpp>
#include <ndn-cxx/management/nfd-controller.hpp>
#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-control-response.hpp>
#include <ndn-cxx/management/nfd-control-parameters.hpp>

namespace nfd {
namespace rib {

using ndn::nfd::ControlCommand;
using ndn::nfd::ControlResponse;
using ndn::nfd::ControlParameters;

using ndn::nfd::FaceEventNotification;

class RibManager : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  explicit
  RibManager(ndn::Face& face);

  ~RibManager();

  void
  registerWithNfd();

  void
  enableLocalControlHeader();

  void
  setConfigFile(ConfigFile& configFile);

private:
  typedef uint32_t TransactionId;

  void
  onConfig(const ConfigSection& configSection,
           bool isDryRun,
           const std::string& filename);

  void
  startListening(const Name& commandPrefix, const ndn::OnInterest& onRequest);

  void
  onLocalhopRequest(const Interest& request);

  void
  onLocalhostRequest(const Interest& request);

  void
  sendResponse(const Name& name,
               const ControlResponse& response);

  void
  sendResponse(const Name& name,
               uint32_t code,
               const std::string& text);

  void
  sendSuccessResponse(const shared_ptr<const Interest>& request,
                      const ControlParameters& parameters);

  void
  sendErrorResponse(uint32_t code, const std::string& error,
                    const shared_ptr<const Interest>& request);

  void
  registerEntry(const shared_ptr<const Interest>& request,
                ControlParameters& parameters);

  void
  unregisterEntry(const shared_ptr<const Interest>& request,
                  ControlParameters& parameters);

  void
  expireEntry(const shared_ptr<const Interest>& request, ControlParameters& params);

  void
  onCommandValidated(const shared_ptr<const Interest>& request);

  void
  onCommandValidationFailed(const shared_ptr<const Interest>& request,
                            const std::string& failureInfo);


  void
  onCommandError(uint32_t code, const std::string& error,
                 const shared_ptr<const Interest>& request,
                 const FaceEntry& faceEntry);

  void
  onRegSuccess(const shared_ptr<const Interest>& request,
               const ControlParameters& parameters,
               const FaceEntry& faceEntry);

  void
  onUnRegSuccess(const shared_ptr<const Interest>& request,
                 const ControlParameters& parameters,
                 const FaceEntry& faceEntry);

  void
  onNrdCommandPrefixAddNextHopSuccess(const Name& prefix);

  void
  onNrdCommandPrefixAddNextHopError(const Name& name, const std::string& msg);

  void
  onAddNextHopSuccess(const shared_ptr<const Interest>& request,
                      const ControlParameters& parameters,
                      const TransactionId transactionId,
                      const bool shouldSendResponse);

  void
  onAddNextHopError(uint32_t code, const std::string& error,
                    const shared_ptr<const Interest>& request,
                    const TransactionId transactionId, const bool shouldSendResponse);

  void
  onRemoveNextHopSuccess(const shared_ptr<const Interest>& request,
                         const ControlParameters& parameters,
                         const TransactionId transactionId,
                         const bool shouldSendResponse);

  void
  onRemoveNextHopError(uint32_t code, const std::string& error,
                       const shared_ptr<const Interest>& request,
                       const TransactionId transactionId, const bool shouldSendResponse);

  void
  onControlHeaderSuccess();

  void
  onControlHeaderError(uint32_t code, const std::string& reason);

  static bool
  extractParameters(const Name::Component& parameterComponent,
                    ControlParameters& extractedParameters);

  bool
  validateParameters(const ControlCommand& command,
                     ControlParameters& parameters);

  void
  onNotification(const FaceEventNotification& notification);

  void
  processErasureAfterNotification(uint64_t faceId);

  void
  sendUpdatesToFib(const shared_ptr<const Interest>& request,
                   const ControlParameters& parameters);

  void
  sendUpdatesToFibAfterFaceDestroyEvent();

  /** \brief Checks if the transaction has received all of the expected responses
   *         from the FIB.
   *  \return{ True if the transaction with the passed transactionId has applied
   *           all of its FIB updates successfully; otherwise, false }
   */
  bool
  isTransactionComplete(const TransactionId transactionId);

  void
  invalidateTransaction(const TransactionId transactionId);

  void
  listEntries(const Interest& request);

  void
  scheduleActiveFaceFetch(const time::seconds& timeToWait);

  void
  fetchActiveFaces();

  void
  fetchSegments(const Data& data, shared_ptr<ndn::OBufferStream> buffer);

  void
  onFetchFaceStatusTimeout();

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /** \param buffer Face dataset contents
  */
  void
  removeInvalidFaces(shared_ptr<ndn::OBufferStream> buffer);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  Rib m_managedRib;

private:
  ndn::Face& m_face;
  ndn::nfd::Controller m_nfdController;
  ndn::KeyChain m_keyChain;
  ndn::ValidatorConfig m_localhostValidator;
  ndn::ValidatorConfig m_localhopValidator;
  ndn::nfd::FaceMonitor m_faceMonitor;
  bool m_isLocalhopEnabled;
  RemoteRegistrator m_remoteRegistrator;

  RibStatusPublisher m_ribStatusPublisher;

  /** \brief The last transaction ID for FIB update response messages.
   *         Each group of FIB updates applied to the FIB is assigned an incrementing
   *         ID that is used to track the number of successfully applied updates.
   */
  TransactionId m_lastTransactionId;

  /// table of FIB update transactions => count of pending FIB updates
  typedef std::map<TransactionId, int> FibTransactionTable;

  /** \brief Table used to track the number of FIB updates that have not yet received
   *         a response from the FIB.
   *         The table maps a transaction ID to the number of updates remaining for that
   *         specific transaction.
   */
  FibTransactionTable m_pendingFibTransactions;

  typedef function<void(RibManager*,
                        const shared_ptr<const Interest>& request,
                        ControlParameters& parameters)> SignedVerbProcessor;

  typedef std::map<name::Component, SignedVerbProcessor> SignedVerbDispatchTable;

  typedef std::pair<name::Component, SignedVerbProcessor> SignedVerbAndProcessor;


  const SignedVerbDispatchTable m_signedVerbDispatch;

  static const Name COMMAND_PREFIX; // /localhost/nrd
  static const Name REMOTE_COMMAND_PREFIX; // /localhop/nrd

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nrd + verb + options) = 4
  static const size_t COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // 8 with signed Interest support.
  static const size_t COMMAND_SIGNED_NCOMPS;

  static const SignedVerbAndProcessor SIGNED_COMMAND_VERBS[];

  typedef function<void(RibManager*, const Interest&)> UnsignedVerbProcessor;
  typedef std::map<Name::Component, UnsignedVerbProcessor> UnsignedVerbDispatchTable;
  typedef std::pair<Name::Component, UnsignedVerbProcessor> UnsignedVerbAndProcessor;

  const UnsignedVerbDispatchTable m_unsignedVerbDispatch;
  static const UnsignedVerbAndProcessor UNSIGNED_COMMAND_VERBS[];

  static const Name LIST_COMMAND_PREFIX;
  static const size_t LIST_COMMAND_NCOMPS;

  static const Name FACES_LIST_DATASET_PREFIX;

  static const time::seconds ACTIVE_FACE_FETCH_INTERVAL;
  EventId m_activeFaceFetchEvent;

  typedef std::set<uint64_t> FaceIdSet;
  /** \brief contains FaceIds with one or more Routes in the RIB
  */
  FaceIdSet m_registeredFaces;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_MANAGER_HPP
