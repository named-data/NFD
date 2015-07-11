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

#include "face/websocket-channel.hpp"
#include "face/websocket-face.hpp"
#include "face/websocket-factory.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> Client;

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceWebSocket, BaseFixture)

BOOST_AUTO_TEST_CASE(GetChannels)
{
  WebSocketFactory factory("19596");
  BOOST_REQUIRE_EQUAL(factory.getChannels().empty(), true);

  std::vector<shared_ptr<const Channel> > expectedChannels;

  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20070"));
  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20071"));
  expectedChannels.push_back(factory.createChannel("::1", "20071"));

  std::list<shared_ptr<const Channel> > channels = factory.getChannels();
  for (std::list<shared_ptr<const Channel> >::const_iterator i = channels.begin();
       i != channels.end(); ++i)
    {
      std::vector<shared_ptr<const Channel> >::iterator pos =
        std::find(expectedChannels.begin(), expectedChannels.end(), *i);

      BOOST_REQUIRE(pos != expectedChannels.end());
      expectedChannels.erase(pos);
    }

  BOOST_CHECK_EQUAL(expectedChannels.size(), 0);
}

BOOST_AUTO_TEST_CASE(UnsupportedFaceCreate)
{
  WebSocketFactory factory("19596");

  BOOST_CHECK_THROW(factory.createFace(FaceUri("ws://127.0.0.1:20070"),
                                       ndn::nfd::FACE_PERSISTENCY_PERMANENT,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);

  BOOST_CHECK_THROW(factory.createFace(FaceUri("ws://127.0.0.1:20070"),
                                       ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);

  BOOST_CHECK_THROW(factory.createFace(FaceUri("ws://127.0.0.1:20070"),
                                       ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);
}

class EndToEndFixture : protected BaseFixture
{
public:
  void
  channel1_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(face1));
    face1 = newFace;
    face1->onReceiveInterest.connect(bind(&EndToEndFixture::face1_onReceiveInterest, this, _1));
    face1->onReceiveData.connect(bind(&EndToEndFixture::face1_onReceiveData, this, _1));
    face1->onFail.connect(bind(&EndToEndFixture::face1_onFail, this));

    limitedIo.afterOp();
  }

  void
  face1_onReceiveInterest(const Interest& interest)
  {
    face1_receivedInterests.push_back(interest);
    limitedIo.afterOp();
  }

  void
  face1_onReceiveData(const Data& data)
  {
    face1_receivedDatas.push_back(data);
    limitedIo.afterOp();
  }

  void
  face1_onFail()
  {
    face1.reset();
    limitedIo.afterOp();
  }

  void
  client1_onOpen(websocketpp::connection_hdl hdl)
  {
    handle = hdl;
    limitedIo.afterOp();
  }

  void
  client1_onClose(websocketpp::connection_hdl hdl)
  {
    limitedIo.afterOp();
  }

  void
  client1_onFail(websocketpp::connection_hdl hdl)
  {
    limitedIo.afterOp();
  }

  bool
  client1_onPing(websocketpp::connection_hdl hdl, std::string msg)
  {
    limitedIo.afterOp();
    // Return false to suppress the pong response,
    // which will cause timeout in the websocket channel
    return false;
  }

  void
  client1_sendInterest(const Interest& interest)
  {
    const Block& payload = interest.wireEncode();
    client1.send(handle, payload.wire(), payload.size(), websocketpp::frame::opcode::binary);
  }

  void
  client1_sendData(const Data& data)
  {
    const Block& payload = data.wireEncode();
    client1.send(handle, payload.wire(), payload.size(), websocketpp::frame::opcode::binary);
  }

  void
  client1_onMessage(websocketpp::connection_hdl hdl,
                    websocketpp::config::asio_client::message_type::ptr msg)
  {
    bool isOk = false;
    Block element;
    const std::string& payload = msg->get_payload();
    std::tie(isOk, element) = Block::fromBuffer(reinterpret_cast<const uint8_t*>(payload.c_str()),
                                                payload.size());
    if (isOk)
      {
        try {
          if (element.type() == tlv::Interest)
            {
              shared_ptr<Interest> i = make_shared<Interest>();
              i->wireDecode(element);
              client1_onReceiveInterest(*i);
            }
          else if (element.type() == tlv::Data)
            {
              shared_ptr<Data> d = make_shared<Data>();
              d->wireDecode(element);
              client1_onReceiveData(*d);
            }
        }
        catch (tlv::Error&) {
          // Do something?
        }
      }
    limitedIo.afterOp();
  }

  void
  client1_onReceiveInterest(const Interest& interest)
  {
    client1_receivedInterests.push_back(interest);
    limitedIo.afterOp();
  }

  void
  client1_onReceiveData(const Data& data)
  {
    client1_receivedDatas.push_back(data);
    limitedIo.afterOp();
  }

public:
  LimitedIo limitedIo;

  shared_ptr<Face> face1;
  std::vector<Interest> face1_receivedInterests;
  std::vector<Data> face1_receivedDatas;
  Client client1;
  websocketpp::connection_hdl handle;
  std::vector<Interest> client1_receivedInterests;
  std::vector<Data> client1_receivedDatas;
};

BOOST_FIXTURE_TEST_CASE(EndToEnd4, EndToEndFixture)
{
  WebSocketFactory factory1("9696");

  shared_ptr<WebSocketChannel> channel1 = factory1.createChannel("127.0.0.1", "20070");
  channel1->setPingInterval(time::milliseconds(3000));
  channel1->setPongTimeout(time::milliseconds(1000));

  BOOST_CHECK_EQUAL(channel1->isListening(), false);

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1));

  BOOST_CHECK_EQUAL(channel1->isListening(), true);

  // Clear all logging info from websocketpp library
  client1.clear_access_channels(websocketpp::log::alevel::all);

  client1.init_asio(&getGlobalIoService());
  client1.set_open_handler(bind(&EndToEndFixture::client1_onOpen,   this, _1));
  client1.set_close_handler(bind(&EndToEndFixture::client1_onClose, this, _1));
  client1.set_fail_handler(bind(&EndToEndFixture::client1_onFail,   this, _1));
  client1.set_message_handler(bind(&EndToEndFixture::client1_onMessage, this, _1, _2));
  client1.set_ping_handler(bind(&EndToEndFixture::client1_onPing, this, _1, _2));

  websocketpp::lib::error_code ec;
  Client::connection_ptr con = client1.get_connection("ws://127.0.0.1:20070", ec);
  client1.connect(con);

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "WebSocketChannel error: cannot connect or cannot accept connection");

  BOOST_CHECK_EQUAL(channel1->size(), 1);

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_CHECK_EQUAL(face1->isLocal(), false);
  BOOST_CHECK_EQUAL(face1->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(face1->isMultiAccess(), false);
  BOOST_CHECK_EQUAL(face1->getLocalUri().toString(), "ws://127.0.0.1:20070");

  shared_ptr<Interest> interest1 = makeInterest("ndn:/TpnzGvW9R");
  shared_ptr<Data>     data1     = makeData("ndn:/KfczhUqVix");
  shared_ptr<Interest> interest2 = makeInterest("ndn:/QWiIMfj5sL");
  shared_ptr<Data>     data2     = makeData("ndn:/XNBV796f");

  std::string bigName("ndn:/");
  bigName.append(9000, 'a');
  shared_ptr<Interest> bigInterest = makeInterest(bigName);

  client1_sendInterest(*interest1);
  client1_sendInterest(*interest1);
  client1_sendInterest(*interest1);
  client1_sendInterest(*bigInterest);  // This one should be ignored by face1
  face1->sendData     (*data1);
  face1->sendInterest (*interest2);
  client1_sendData    (*data2);
  client1_sendData    (*data2);
  client1_sendData    (*data2);
  size_t nBytesSent = data1->wireEncode().size() + interest2->wireEncode().size();
  size_t nBytesReceived = interest1->wireEncode().size() * 3 + data2->wireEncode().size() * 3;

  BOOST_CHECK_MESSAGE(limitedIo.run(8, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "WebSocketChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 3);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 3);
  BOOST_REQUIRE_EQUAL(client1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(client1_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest1->getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2->getName());
  BOOST_CHECK_EQUAL(client1_receivedInterests[0].getName(), interest2->getName());
  BOOST_CHECK_EQUAL(client1_receivedDatas    [0].getName(), data1->getName());

  const FaceCounters& counters1 = face1->getCounters();
  BOOST_CHECK_EQUAL(counters1.getNInInterests() , 3);
  BOOST_CHECK_EQUAL(counters1.getNInDatas()     , 3);
  BOOST_CHECK_EQUAL(counters1.getNOutInterests(), 1);
  BOOST_CHECK_EQUAL(counters1.getNOutDatas()    , 1);
  BOOST_CHECK_EQUAL(counters1.getNInBytes(), nBytesReceived);
  BOOST_CHECK_EQUAL(counters1.getNOutBytes(), nBytesSent);

  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::seconds(8));
  BOOST_CHECK_EQUAL(channel1->size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
