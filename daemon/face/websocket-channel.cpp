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

#include "websocket-channel.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace nfd {

NFD_LOG_INIT("WebSocketChannel");

using namespace boost::asio;

WebSocketChannel::WebSocketChannel(const websocket::Endpoint& localEndpoint)
  : m_localEndpoint(localEndpoint)
  , m_isListening(false)
  , m_pingInterval(10000)
{
  // Setup WebSocket server
  m_server.clear_access_channels(websocketpp::log::alevel::all);
  m_server.clear_error_channels(websocketpp::log::alevel::all);

  m_server.set_message_handler(bind(&WebSocketChannel::handleMessage, this, _1, _2));
  m_server.set_open_handler(bind(&WebSocketChannel::handleOpen, this, _1));
  m_server.set_close_handler(bind(&WebSocketChannel::handleClose, this, _1));
  m_server.init_asio(&getGlobalIoService());
  // Always set SO_REUSEADDR flag
  m_server.set_reuse_addr(true);

  // Detect disconnection using PONG message
  m_server.set_pong_handler(bind(&WebSocketChannel::handlePong, this, _1, _2));
  m_server.set_pong_timeout_handler(bind(&WebSocketChannel::handlePongTimeout,
                                         this, _1, _2));

  this->setUri(FaceUri(localEndpoint, "ws"));
}

WebSocketChannel::~WebSocketChannel()
{
}

void
WebSocketChannel::setPongTimeout(time::milliseconds timeout)
{
  m_server.set_pong_timeout(static_cast<long>(timeout.count()));
}

void
WebSocketChannel::handlePongTimeout(websocketpp::connection_hdl hdl, std::string msg)
{
  ChannelFaceMap::iterator it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end())
    {
      it->second->close();
      NFD_LOG_DEBUG("handlePongTimeout: remove " << it->second->getRemoteUri());
      m_channelFaces.erase(it);
    }
}

void
WebSocketChannel::handlePong(websocketpp::connection_hdl hdl, std::string msg)
{
  ChannelFaceMap::iterator it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end())
    {
      NFD_LOG_TRACE("handlePong: from " << it->second->getRemoteUri());
    }
}

void
WebSocketChannel::handleMessage(websocketpp::connection_hdl hdl,
                                websocket::Server::message_ptr msg)
{
  ChannelFaceMap::iterator it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end())
    {
      it->second->handleReceive(msg->get_payload());
    }
}

void
WebSocketChannel::handleOpen(websocketpp::connection_hdl hdl)
{
  std::string remote;
  try
    {
      remote = "wsclient://" + m_server.get_con_from_hdl(hdl)->get_remote_endpoint();
    }
  catch (websocketpp::lib::error_code&)
    {
      NFD_LOG_DEBUG("handleOpen: cannot get remote uri");
      websocketpp::lib::error_code ecode;
      m_server.close(hdl, websocketpp::close::status::normal, "closed by channel", ecode);
    }
  shared_ptr<WebSocketFace> face = ndn::make_shared<WebSocketFace>(FaceUri(remote), this->getUri(),
                                                                   hdl, ref(m_server));
  m_onFaceCreatedCallback(face);
  m_channelFaces[hdl] = face;

  // Schedule PING message
  EventId pingEvent = scheduler::schedule(m_pingInterval,
                                          bind(&WebSocketChannel::sendPing, this, hdl));
  face->setPingEventId(pingEvent);
}

void
WebSocketChannel::sendPing(websocketpp::connection_hdl hdl)
{
  ChannelFaceMap::iterator it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end())
    {
      try
        {
          m_server.ping(hdl, "NFD-WebSocket");
        }
      catch (websocketpp::lib::error_code&)
        {
          it->second->close();
          NFD_LOG_DEBUG("sendPing: failed to ping " << it->second->getRemoteUri());
          m_channelFaces.erase(it);
        }

      NFD_LOG_TRACE("sendPing: to " << it->second->getRemoteUri());

      // Schedule next PING message
      EventId pingEvent = scheduler::schedule(m_pingInterval,
                                              bind(&WebSocketChannel::sendPing, this, hdl));
      it->second->setPingEventId(pingEvent);
    }
}

void
WebSocketChannel::handleClose(websocketpp::connection_hdl hdl)
{
  ChannelFaceMap::iterator it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end())
    {
      it->second->close();
      NFD_LOG_DEBUG("handleClose: remove " << it->second->getRemoteUri());
      m_channelFaces.erase(it);
    }
}


void
WebSocketChannel::listen(const FaceCreatedCallback& onFaceCreated)
{
  if (m_isListening)
    {
      throw Error("Listen already called on this channel");
    }
  m_isListening = true;

  m_onFaceCreatedCallback = onFaceCreated;

  try
    {
      m_server.listen(m_localEndpoint);
    }
  catch (websocketpp::lib::error_code ec)
    {
      throw Error("Failed to listen on local endpoint");
    }

  m_server.start_accept();
}

size_t
WebSocketChannel::size() const
{
  return m_channelFaces.size();
}

} // namespace nfd
