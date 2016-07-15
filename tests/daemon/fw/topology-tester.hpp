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

/** \file
 *  \brief allows testing forwarding in a network topology
 */

#ifndef NFD_TESTS_NFD_FW_TOPOLOGY_TESTER_HPP
#define NFD_TESTS_NFD_FW_TOPOLOGY_TESTER_HPP

#include <ndn-cxx/face.hpp>
#include "face/internal-transport.hpp"
#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "install-strategy.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

/** \brief identifies a node (forwarder) in the topology
 */
typedef size_t TopologyNode;

/** \brief represents a network link in the topology which connects two or more nodes
 */
class TopologyLink : noncopyable
{
public:
  explicit
  TopologyLink(const time::nanoseconds& delay);

  /** \brief fail the link, cause packets to be dropped silently
   */
  void
  fail()
  {
    m_isUp = false;
  }

  /** \brief recover the link from a failure
   */
  void
  recover()
  {
    m_isUp = true;
  }

  /** \brief change the link delay
   *  \param delay link delay, must be positive
   */
  void
  setDelay(const time::nanoseconds& delay);

  /** \brief attach a face to the link
   *  \param i forwarder index
   *  \param face a Face with InternalForwarderTransport
   */
  void
  addFace(TopologyNode i, shared_ptr<Face> face);

  /** \return a face of forwarder \p i which is attached to this link
   */
  Face&
  getFace(TopologyNode i)
  {
    return *m_faces.at(i);
  }

protected:
  /** \brief attach a Transport onto this link
   */
  void
  attachTransport(TopologyNode i, face::InternalTransportBase* transport);

private:
  void
  transmit(TopologyNode i, const Block& packet);

  void
  scheduleReceive(face::InternalTransportBase* recipient, const Block& packet);

private:
  bool m_isUp;
  time::nanoseconds m_delay;
  std::unordered_map<TopologyNode, face::InternalTransportBase*> m_transports;
  std::unordered_map<TopologyNode, shared_ptr<Face>> m_faces;
};

/** \brief represents a link to a local application
 */
class TopologyAppLink : noncopyable
{
public:
  /** \brief constructor
   *  \param forwarderFace a Face with InternalForwarderTransport
   */
  explicit
  TopologyAppLink(shared_ptr<Face> forwarderFace);

  /** \brief fail the link, cause packets to be dropped silently
   */
  void
  fail();

  /** \brief recover the link from a failure
   */
  void
  recover();

  /** \return face on forwarder side
   */
  Face&
  getForwarderFace()
  {
    return *m_face;
  }

  /** \return face on application side
   */
  ndn::Face&
  getClientFace()
  {
    return *m_client;
  }

private:
  shared_ptr<Face> m_face;
  face::InternalForwarderTransport* m_forwarderTransport;
  shared_ptr<face::InternalClientTransport> m_clientTransport;
  shared_ptr<ndn::Face> m_client;
};

/** \brief builds a topology for forwarding tests
 */
class TopologyTester : noncopyable
{
public:
  /** \brief creates a forwarder
   *  \return index of new forwarder
   */
  TopologyNode
  addForwarder(const std::string& label);

  /** \return forwarder instance \p i
   */
  Forwarder&
  getForwarder(TopologyNode i)
  {
    return *m_forwarders.at(i);
  }

  /** \brief sets strategy on forwarder \p i
   *  \tparam S the strategy type
   *  \note Test scenario can also access StrategyChoice table directly.
   */
  template<typename S>
  void
  setStrategy(TopologyNode i, Name prefix = Name("ndn:/"))
  {
    Forwarder& forwarder = this->getForwarder(i);
    choose<S>(forwarder, prefix);
  }

  /** \brief makes a link that interconnects two or more forwarders
   *
   *  A face is created on each of \p forwarders .
   *  When a packet is sent onto one of the faces on this link,
   *  this packet will be received by all other faces on this link after \p delay .
   */
  shared_ptr<TopologyLink>
  addLink(const std::string& label, const time::nanoseconds& delay,
          std::initializer_list<TopologyNode> forwarders,
          bool forceMultiAccessFace = false);

  /** \brief makes a link to local application
   */
  shared_ptr<TopologyAppLink>
  addAppFace(const std::string& label, TopologyNode i);

  /** \brief makes a link to local application, and register a prefix
   */
  shared_ptr<TopologyAppLink>
  addAppFace(const std::string& label, TopologyNode i, const Name& prefix, uint64_t cost = 0);

  /** \brief registers a prefix on a forwarder face
   */
  void
  registerPrefix(TopologyNode i, const Face& face, const Name& prefix, uint64_t cost = 0);

  /** \brief creates a producer application that answers every Interest with Data of same Name
   */
  void
  addEchoProducer(ndn::Face& face, const Name& prefix = "/");

  /** \brief creates a consumer application that sends \p n Interests under \p prefix
   *         at \p interval fixed rate.
   */
  void
  addIntervalConsumer(ndn::Face& face, const Name& prefix,
                      const time::nanoseconds& interval, size_t n);

private:
  std::vector<unique_ptr<Forwarder>> m_forwarders;
  std::vector<std::string> m_forwarderLabels;
  std::vector<shared_ptr<TopologyLink>> m_links;
  std::vector<shared_ptr<TopologyAppLink>> m_appLinks;
};

} // namespace tests
} // namespace fw
} // namespace nfd

#endif // NFD_TESTS_NFD_FW_TOPOLOGY_TESTER_HPP
