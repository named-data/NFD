/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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
#include <ndn-cxx/encoding/nfd-constants.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace nfd {
namespace face {

/** \brief provide support for an underlying protocol
 *  \sa FaceSystem
 *
 *  A protocol factory provides support for an underlying protocol and owns Channel objects.
 *  It can process a subsection of face_system config section and create channels and multicast
 *  faces accordingly.
 */
class ProtocolFactory : noncopyable
{
public:
  /**
   * \brief Base class for all exceptions thrown by protocol factories
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

  /** \brief process face_system subsection that corresponds to this ProtocolFactory type
   *  \param configSection the configuration section or boost::null to indicate it is omitted
   *  \param context provides access to data structures and contextual information
   *  \throw ConfigFile::Error invalid configuration
   *
   *  This function updates \p providedSchemes
   */
  virtual void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context)
  {
    ///\todo implement in every subclass and make this pure-virtual
    BOOST_THROW_EXCEPTION(Error("processConfig is not implemented"));
  }

  /** \return FaceUri schemes accepted by this ProtocolFactory
   */
  const std::set<std::string>&
  getProvidedSchemes()
  {
    return providedSchemes;
  }

  /** \brief Try to create Face using the supplied FaceUri
   *
   * This method should automatically choose channel, based on supplied FaceUri
   * and create face.
   *
   * \param uri remote URI of the new face
   * \param persistency persistency of the new face
   * \param wantLocalFieldsEnabled whether local fields should be enabled on the face
   * \param onCreated callback if face creation succeeds
   *                  If a face with the same remote URI already exists, its persistency and
   *                  LocalFieldsEnabled setting will not be modified.
   * \param onFailure callback if face creation fails
   */
  virtual void
  createFace(const FaceUri& uri,
             ndn::nfd::FacePersistency persistency,
             bool wantLocalFieldsEnabled,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) = 0;

  virtual std::vector<shared_ptr<const Channel>>
  getChannels() const = 0;

protected:
  template<typename ChannelMap>
  static std::vector<shared_ptr<const Channel>>
  getChannelsFromMap(const ChannelMap& channelMap)
  {
    std::vector<shared_ptr<const Channel>> channels;
    boost::copy(channelMap | boost::adaptors::map_values, std::back_inserter(channels));
    return channels;
  }

protected:
  /** \brief FaceUri schemes provided by this ProtocolFactory
   */
  std::set<std::string> providedSchemes;
};

} // namespace face

using face::ProtocolFactory;

} // namespace nfd

#endif // NFD_DAEMON_FACE_PROTOCOL_FACTORY_HPP
