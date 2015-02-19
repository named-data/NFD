/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include <unordered_map>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include "fw/strategy.hpp"
#include "tests/test-common.hpp"
#include "../face/dummy-face.hpp"

namespace nfd {
namespace fw {
namespace tests {

using ndn::util::DummyClientFace;
using namespace nfd::tests;

/** \brief identifies a node (forwarder) in the topology
 */
typedef size_t TopologyNode;

/** \brief represents a network or local-app link
 */
class TopologyLinkBase : noncopyable
{
public:
  TopologyLinkBase()
    : m_isUp(true)
  {
  }

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

protected:
  bool m_isUp;
};

/** \brief represents a network link in the topology which connects two or more nodes
 */
class TopologyLink : public TopologyLinkBase
{
public:
  /** \return a face of forwarder \p i which is attached to this link
   */
  shared_ptr<DummyFace>
  getFace(TopologyNode i)
  {
    return m_faces.at(i)->face;
  }

private:
  explicit
  TopologyLink(const time::nanoseconds& delay)
    : m_delay(delay)
  {
    BOOST_ASSERT(delay >= time::nanoseconds::zero());
  }

  struct LinkFace
  {
    shared_ptr<DummyFace> face;
  };

  void
  addFace(TopologyNode i, shared_ptr<DummyFace> face)
  {
    BOOST_ASSERT(m_faces.count(i) == 0);

    LinkFace* lf = new LinkFace();
    lf->face = face;
    face->onSendInterest.connect(bind(&TopologyLink::transmitInterest, this, i, _1));
    face->onSendData.connect(bind(&TopologyLink::transmitData, this, i, _1));

    m_faces[i].reset(lf);
  }

  friend class TopologyTester;

private:
  void
  transmitInterest(TopologyNode i, const Interest& interest)
  {
    if (!m_isUp) {
      return;
    }

    // Interest object cannot be shared between faces because
    // Forwarder can set different IncomingFaceId.
    Block wire = interest.wireEncode();
    for (auto&& p : m_faces) {
      if (p.first == i) {
        continue;
      }
      shared_ptr<DummyFace> face = p.second->face;
      scheduler::schedule(m_delay, [wire, face] {
        auto interest = make_shared<Interest>(wire);
        face->receiveInterest(*interest);
      });
    }
  }

  void
  transmitData(TopologyNode i, const Data& data)
  {
    if (!m_isUp) {
      return;
    }

    // Data object cannot be shared between faces because
    // Forwarder can set different IncomingFaceId.
    Block wire = data.wireEncode();
    for (auto&& p : m_faces) {
      if (p.first == i) {
        continue;
      }
      shared_ptr<DummyFace> face = p.second->face;
      scheduler::schedule(m_delay, [wire, face] {
        auto data = make_shared<Data>(wire);
        face->receiveData(*data);
      });
    }
  }

private:
  time::nanoseconds m_delay;
  std::unordered_map<TopologyNode, unique_ptr<LinkFace>> m_faces;
};

/** \brief represents a link to a local application
 */
class TopologyAppLink : public TopologyLinkBase
{
public:
  /** \return face on forwarder side
   */
  shared_ptr<DummyLocalFace>
  getForwarderFace()
  {
    return m_face;
  }

  /** \return face on application side
   */
  shared_ptr<DummyClientFace>
  getClientFace()
  {
    return m_client;
  }

private:
  explicit
  TopologyAppLink(shared_ptr<DummyLocalFace> face)
    : m_face(face)
    , m_client(ndn::util::makeDummyClientFace(getGlobalIoService(), {false, false}))
  {
    m_client->onSendInterest.connect([this] (const Interest& interest) {
      if (!m_isUp) {
        return;
      }
      auto interest2 = interest.shared_from_this();
      getGlobalIoService().post([=] { m_face->receiveInterest(*interest2); });
    });

    m_client->onSendData.connect([this] (const Data& data) {
      if (!m_isUp) {
        return;
      }
      auto data2 = data.shared_from_this();
      getGlobalIoService().post([=] { m_face->receiveData(*data2); });
    });

    m_face->onSendInterest.connect([this] (const Interest& interest) {
      if (!m_isUp) {
        return;
      }
      auto interest2 = interest.shared_from_this();
      getGlobalIoService().post([=] { m_client->receive(*interest2); });
    });

    m_face->onSendData.connect([this] (const Data& data) {
      if (!m_isUp) {
        return;
      }
      auto data2 = data.shared_from_this();
      getGlobalIoService().post([=] { m_client->receive(*data2); });
    });
  }

  friend class TopologyTester;

private:
  shared_ptr<DummyLocalFace> m_face;
  shared_ptr<DummyClientFace> m_client;
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
  addForwarder()
  {
    size_t i = m_forwarders.size();
    m_forwarders.push_back(std::move(unique_ptr<Forwarder>(new Forwarder())));
    return i;
  }

  /** \return forwarder instance \p i
   */
  Forwarder&
  getForwarder(TopologyNode i)
  {
    return *m_forwarders.at(i);
  }

  /** \brief sets strategy on forwarder \p i
   *  \tparam the strategy type
   *  \note Test scenario can also access StrategyChoice table directly.
   */
  template<typename S>
  void
  setStrategy(TopologyNode i, Name prefix = Name("ndn:/"))
  {
    Forwarder& forwarder = this->getForwarder(i);
    StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
    shared_ptr<S> strategy = make_shared<S>(ref(forwarder));
    strategyChoice.install(strategy);
    strategyChoice.insert(prefix, strategy->getName());
  }

  /** \brief makes a link that interconnects two or more forwarders
   *
   *  A face is created on each of \p forwarders .
   *  When a packet is sent onto one of the faces on this link,
   *  this packet will be received by all other faces on this link after \p delay .
   */
  shared_ptr<TopologyLink>
  addLink(const time::nanoseconds& delay, std::initializer_list<TopologyNode> forwarders)
  {
    auto link = shared_ptr<TopologyLink>(new TopologyLink(delay));
    for (TopologyNode i : forwarders) {
      Forwarder& forwarder = this->getForwarder(i);
      shared_ptr<DummyFace> face = make_shared<DummyFace>();
      forwarder.addFace(face);
      link->addFace(i, face);
    }
    return link;
  }

  /** \brief makes a link to local application
   */
  shared_ptr<TopologyAppLink>
  addAppFace(TopologyNode i)
  {
    Forwarder& forwarder = this->getForwarder(i);
    auto face = make_shared<DummyLocalFace>();
    forwarder.addFace(face);

    return shared_ptr<TopologyAppLink>(new TopologyAppLink(face));
  }

  /** \brief makes a link to local application, and register a prefix
   */
  shared_ptr<TopologyAppLink>
  addAppFace(TopologyNode i, const Name& prefix, uint64_t cost = 0)
  {
    shared_ptr<TopologyAppLink> al = this->addAppFace(i);
    this->registerPrefix(i, al->getForwarderFace(), prefix, cost);
    return al;
  }

  /** \brief registers a prefix on a face
   *  \tparam F either DummyFace or DummyLocalFace
   */
  template<typename F>
  void
  registerPrefix(TopologyNode i, shared_ptr<F> face, const Name& prefix, uint64_t cost = 0)
  {
    Forwarder& forwarder = this->getForwarder(i);
    Fib& fib = forwarder.getFib();
    shared_ptr<fib::Entry> fibEntry = fib.insert(prefix).first;
    fibEntry->addNextHop(face, cost);
  }

  /** \brief creates a producer application that answers every Interest with Data of same Name
   */
  void
  addEchoProducer(DummyClientFace& face, const Name& prefix = "/")
  {
    face.setInterestFilter(prefix,
        [&face] (const ndn::InterestFilter&, const Interest& interest) {
          shared_ptr<Data> data = makeData(interest.getName());
          face.put(*data);
        });
  }

  /** \brief creates a consumer application that sends \p n Interests under \p prefix
   *         at \p interval fixed rate.
   */
  void
  addIntervalConsumer(DummyClientFace& face, const Name& prefix,
                      const time::nanoseconds& interval, size_t n)
  {
    Name name(prefix);
    name.appendTimestamp();
    shared_ptr<Interest> interest = makeInterest(name);
    face.expressInterest(*interest, bind([]{}));

    if (n > 1) {
      scheduler::schedule(interval, bind(&TopologyTester::addIntervalConsumer, this,
                                         ref(face), prefix, interval, n - 1));
    }
  }

private:
  std::vector<unique_ptr<Forwarder>> m_forwarders;
};

} // namespace tests
} // namespace fw
} // namespace nfd

#endif // NFD_TESTS_NFD_FW_TOPOLOGY_TESTER_HPP
