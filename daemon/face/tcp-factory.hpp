/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_TCP_FACTORY_HPP
#define NFD_DAEMON_FACE_TCP_FACTORY_HPP

#include "protocol-factory.hpp"
#include "tcp-channel.hpp"

namespace nfd {

class TcpFactory : public ProtocolFactory
{
public:
  /**
   * \brief Create TCP-based channel using tcp::Endpoint
   *
   * tcp::Endpoint is really an alias for boost::asio::ip::tcp::endpoint.
   *
   * If this method called twice with the same endpoint, only one channel
   * will be created.  The second call will just retrieve the existing
   * channel.
   *
   * \returns always a valid pointer to a TcpChannel object, an exception
   *          is thrown if it cannot be created.
   *
   * \see http://www.boost.org/doc/libs/1_42_0/doc/html/boost_asio/reference/ip__tcp/endpoint.html
   *      for details on ways to create tcp::Endpoint
   */
  shared_ptr<TcpChannel>
  createChannel(const tcp::Endpoint& localEndpoint);

  /**
   * \brief Create TCP-based channel using specified IP address and port number
   *
   * This method is just a helper that converts a string representation of localIp and port to
   * tcp::Endpoint and calls the other createChannel overload.
   *
   * \throws std::runtime_error
   */
  shared_ptr<TcpChannel>
  createChannel(const std::string& localIp, const std::string& localPort);

public: // from ProtocolFactory
  virtual void
  createFace(const FaceUri& uri,
             ndn::nfd::FacePersistency persistency,
             bool wantLocalFieldsEnabled,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) override;

  virtual std::vector<shared_ptr<const Channel>>
  getChannels() const override;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  prohibitEndpoint(const tcp::Endpoint& endpoint);

  void
  prohibitAllIpv4Endpoints(uint16_t port);

  void
  prohibitAllIpv6Endpoints(uint16_t port);

private:
  /**
   * \brief Look up TcpChannel using specified local endpoint
   *
   * \returns shared pointer to the existing TcpChannel object
   *          or empty shared pointer when such channel does not exist
   */
  shared_ptr<TcpChannel>
  findChannel(const tcp::Endpoint& localEndpoint) const;

private:
  std::map<tcp::Endpoint, shared_ptr<TcpChannel>> m_channels;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  std::set<tcp::Endpoint> m_prohibitedEndpoints;
};

} // namespace nfd

#endif // NFD_DAEMON_FACE_TCP_FACTORY_HPP
