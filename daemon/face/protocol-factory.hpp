/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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

#ifndef NFD_DAEMON_FACE_PROTOCOL_FACTORY_HPP
#define NFD_DAEMON_FACE_PROTOCOL_FACTORY_HPP

#include "channel.hpp"
#include "face.hpp"
#include "face-system.hpp"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace nfd {
namespace face {

/** \brief Parameters to ProtocolFactory constructor
 *
 *  Every ProtocolFactory subclass is expected to have a constructor that accepts CtorParams,
 *  which in turn passes it to ProtocolFactory base class constructor. Parameters are passed as a
 *  struct rather than individually, so that a future change in list of parameters does not
 *  require updates to subclass constructors.
 */
struct ProtocolFactoryCtorParams
{
  FaceCreatedCallback addFace;
  shared_ptr<ndn::net::NetworkMonitor> netmon;
};

/** \brief Provides support for an underlying protocol
 *  \sa FaceSystem
 *
 *  A protocol factory provides support for an underlying protocol and owns Channel objects.
 *  It can process a subsection of face_system config section and create channels and multicast
 *  faces accordingly.
 */
class ProtocolFactory : noncopyable
{
public: // registry
  using CtorParams = ProtocolFactoryCtorParams;

  /** \brief Register a protocol factory type
   *  \tparam PF subclass of ProtocolFactory
   *  \param id factory identifier
   */
  template<typename PF>
  static void
  registerType(const std::string& id = PF::getId())
  {
    Registry& registry = getRegistry();
    BOOST_ASSERT(registry.count(id) == 0);
    registry[id] = [] (const CtorParams& p) { return make_unique<PF>(p); };
  }

  /** \brief Create a protocol factory instance
   *  \retval nullptr if a factory with the given \p id is not registered
   */
  static unique_ptr<ProtocolFactory>
  create(const std::string& id, const CtorParams& params);

  /** \brief Get all registered protocol factory ids
   */
  static std::set<std::string>
  listRegistered();

public:
  /** \brief Base class for all exceptions thrown by ProtocolFactory subclasses
   */
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  explicit
  ProtocolFactory(const CtorParams& params);

  virtual
  ~ProtocolFactory() = 0;

#ifdef DOXYGEN
  /** \brief Get id for this protocol factory
   *
   *  face_system.factory-id config section is processed by the protocol factory.
   */
  static const std::string&
  getId() noexcept;
#endif

  /** \brief Get FaceUri schemes accepted by this protocol factory
   */
  const std::set<std::string>&
  getProvidedSchemes() const
  {
    return providedSchemes;
  }

  /** \brief Process face_system subsection that corresponds to this protocol factory id
   *  \param configSection the configuration section or boost::none to indicate it is omitted
   *  \param context provides access to data structures and contextual information
   *  \throw ConfigFile::Error invalid configuration
   */
  void
  processConfig(OptionalConfigSection configSection, FaceSystem::ConfigContext& context);

  /** \brief Encapsulates a face creation request and all its parameters
   *
   *  Parameters are passed as a struct rather than individually, so that a future change in the list
   *  of parameters does not require an update to the method signature in all subclasses.
   */
  struct CreateFaceRequest
  {
    FaceUri remoteUri;
    optional<FaceUri> localUri;
    FaceParams params;
  };

  /** \brief Create a unicast face
   *  \param req request object containing the face creation parameters
   *  \param onCreated callback if face creation succeeds or face already exists; the settings
   *                   of an existing face are not updated if they differ from the request
   *  \param onFailure callback if face creation fails
   */
  void
  createFace(const CreateFaceRequest& req,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure);

  /** \brief Create a netdev-bound face
   *  \param remote remote FaceUri, must be canonical
   *  \param netdev local network interface
   *  \return new face
   *  \throw Error cannot create a face using specified arguments
   *  \note The caller must ensure there is no existing netdev-bound face with same remote FaceUri
   *        on the same local network interface.
   */
  shared_ptr<Face>
  createNetdevBoundFace(const FaceUri& remote,
                        const shared_ptr<const ndn::net::NetworkInterface>& netdev);

  /** \brief Get list of open channels (listening + non-listening)
   */
  std::vector<shared_ptr<const Channel>>
  getChannels() const;

protected:
  template<typename ChannelMap>
  static std::vector<shared_ptr<const Channel>>
  getChannelsFromMap(const ChannelMap& channelMap)
  {
    std::vector<shared_ptr<const Channel>> channels;
    boost::copy(channelMap | boost::adaptors::map_values, std::back_inserter(channels));
    return channels;
  }

private:
  /** \brief Process face_system subsection that corresponds to this protocol factory id
   *  \sa processConfig
   *
   *  A subclass should override this method if it supports configuration options in the config
   *  file, and do all the required processing here. The subclass should throw ConfigFile::Error
   *  if it encounters unrecognized options or invalid values. It should also update
   *  \p providedSchemes as needed.
   *
   *  The base class implementation does nothing.
   */
  virtual void
  doProcessConfig(OptionalConfigSection configSection, FaceSystem::ConfigContext& context);

  /** \brief Create a unicast face
   *  \sa createFace
   *
   *  The base class implementation always invokes the failure callback with error code 406,
   *  indicating unicast face creation is not supported.
   */
  virtual void
  doCreateFace(const CreateFaceRequest& req,
               const FaceCreatedCallback& onCreated,
               const FaceCreationFailedCallback& onFailure);

  /** \brief Create a netdev-bound face
   *  \sa createNetdevBoundFace
   *
   *  The base class implementation always throws Error, indicating netdev-bound faces are not
   *  supported.
   *
   *  A subclass that offers netdev-bound faces should override this method, and also expose
   *  "scheme+dev" in providedSchemes. For example, UdpFactory should provide "udp4+dev" scheme,
   *  in addition to "udp4" scheme.
   *
   *  The face should be constructed immediately. Face persistency shall be reported as PERMANENT.
   *  Face state shall remain DOWN until underlying transport is connected. The face shall remain
   *  open until after .close() is invoked, and survive all socket errors; in case the network
   *  interface disappears, the face shall remain DOWN until .close() is invoked.
   */
  virtual shared_ptr<Face>
  doCreateNetdevBoundFace(const FaceUri& remote,
                          const shared_ptr<const ndn::net::NetworkInterface>& netif);

  /** \brief Get list of open channels (listening + non-listening)
   *  \sa getChannels
   *
   *  The base class implementation returns an empty list.
   */
  virtual std::vector<shared_ptr<const Channel>>
  doGetChannels() const;

private: // registry
  using CreateFunc = std::function<unique_ptr<ProtocolFactory>(const CtorParams&)>;
  using Registry = std::map<std::string, CreateFunc>; // indexed by factory id

  static Registry&
  getRegistry();

protected:
  std::set<std::string> providedSchemes; ///< FaceUri schemes provided by this protocol factory
  FaceCreatedCallback addFace; ///< callback when a new face is created

  /** \brief NetworkMonitor for listing available network interfaces and monitoring their changes
   *
   *  ProtocolFactory subclass should check the NetworkMonitor has sufficient capabilities prior
   *  to usage.
   */
  shared_ptr<ndn::net::NetworkMonitor> netmon;
};

} // namespace face
} // namespace nfd

/** \brief Registers a protocol factory
 *
 *  This macro should appear once in the .cpp file of each protocol factory.
 */
#define NFD_REGISTER_PROTOCOL_FACTORY(PF)                      \
static class NfdAuto ## PF ## ProtocolFactoryRegistrationClass \
{                                                              \
public:                                                        \
  NfdAuto ## PF ## ProtocolFactoryRegistrationClass()          \
  {                                                            \
    ::nfd::face::ProtocolFactory::registerType<PF>();          \
  }                                                            \
} g_nfdAuto ## PF ## ProtocolFactoryRegistrationVariable

#endif // NFD_DAEMON_FACE_PROTOCOL_FACTORY_HPP
