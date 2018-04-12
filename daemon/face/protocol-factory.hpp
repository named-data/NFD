/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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
#include "face-system.hpp"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <ndn-cxx/encoding/nfd-constants.hpp>

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
   *  \tparam S subclass of ProtocolFactory
   *  \param id factory identifier
   */
  template<typename PF>
  static void
  registerType(const std::string& id = PF::getId())
  {
    Registry& registry = getRegistry();
    BOOST_ASSERT(registry.count(id) == 0);
    registry[id] = &make_unique<PF, const CtorParams&>;
  }

  /** \brief Create a protocol factory instance
   *  \retval nullptr if factory with \p id is not registered
   */
  static unique_ptr<ProtocolFactory>
  create(const std::string& id, const CtorParams& params);

  /** \brief Get registered protocol factory ids
   */
  static std::set<std::string>
  listRegistered();

public:
  /** \brief Base class for all exceptions thrown by ProtocolFactory subclasses
   */
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  virtual
  ~ProtocolFactory() = default;

#ifdef DOXYGEN
  /** \brief Get id for this ProtocolFactory
   *
   *  face_system.factory-id config section is processed by the protocol factory.
   */
  static const std::string&
  getId();
#endif

  /** \brief Process face_system subsection that corresponds to this ProtocolFactory type
   *  \param configSection the configuration section or boost::null to indicate it is omitted
   *  \param context provides access to data structures and contextual information
   *  \throw ConfigFile::Error invalid configuration
   *
   *  This function updates \p providedSchemes
   */
  virtual void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) = 0;

  /** \brief Get FaceUri schemes accepted by this ProtocolFactory
   */
  const std::set<std::string>&
  getProvidedSchemes()
  {
    return providedSchemes;
  }

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

  /** \brief Try to create a unicast face using the supplied parameters
   *
   * \param req request object containing the face creation parameters
   * \param onCreated callback if face creation succeeds or face already exists; the settings
   *                  of an existing face are not updated if they differ from the request
   * \param onFailure callback if face creation fails
   */
  virtual void
  createFace(const CreateFaceRequest& req,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) = 0;

  virtual std::vector<shared_ptr<const Channel>>
  getChannels() const = 0;

protected:
  explicit
  ProtocolFactory(const CtorParams& params);

  template<typename ChannelMap>
  static std::vector<shared_ptr<const Channel>>
  getChannelsFromMap(const ChannelMap& channelMap)
  {
    std::vector<shared_ptr<const Channel>> channels;
    boost::copy(channelMap | boost::adaptors::map_values, std::back_inserter(channels));
    return channels;
  }

private: // registry
  using CreateFunc = std::function<unique_ptr<ProtocolFactory>(const CtorParams&)>;
  using Registry = std::map<std::string, CreateFunc>; // indexed by factory id

  static Registry&
  getRegistry();

protected:
  std::set<std::string> providedSchemes; ///< FaceUri schemes provided by this ProtocolFactory
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

/** \brief registers a protocol factory
 *
 *  This macro should appear once in .cpp of each protocol factory.
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
