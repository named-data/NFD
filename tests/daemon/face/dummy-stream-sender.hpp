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

#ifndef NFD_TESTS_DAEMON_FACE_DUMMY_STREAM_SENDER_HPP
#define NFD_TESTS_DAEMON_FACE_DUMMY_STREAM_SENDER_HPP

#include "core/scheduler.hpp"
#include "core/global-io.hpp"

namespace nfd {
namespace tests {


template<class Protocol, class Dataset>
class DummyStreamSender : public Dataset
{
public:
  typedef typename Protocol::endpoint Endpoint;
  typedef typename Protocol::socket Socket;

  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  DummyStreamSender()
    : socket(getGlobalIoService())
  {
  }

  void
  start(const Endpoint& endpoint)
  {
    socket.async_connect(endpoint,
                         bind(&DummyStreamSender::onSuccessfullConnect, this, _1));
  }

  void
  onSuccessfullConnect(const boost::system::error_code& error)
  {
    if (error)
      {
        BOOST_THROW_EXCEPTION(Error("Connection aborted"));
      }

    // This value may need to be adjusted if some dataset exceeds 100k
    socket.set_option(boost::asio::socket_base::send_buffer_size(100000));

    for (typename Dataset::Container::iterator i = this->data.begin();
         i != this->data.end(); ++i)
      {
        socket.async_send(boost::asio::buffer(*i),
                          bind(&DummyStreamSender::onSendFinished, this, _1, false));
      }

    socket.async_send(boost::asio::buffer(static_cast<const uint8_t*>(0), 0),
                      bind(&DummyStreamSender::onSendFinished, this, _1, true));
  }

  void
  onSendFinished(const boost::system::error_code& error, bool isFinal)
  {
    if (error) {
      BOOST_THROW_EXCEPTION(Error("Connection aborted"));
    }

    if (isFinal) {
      scheduler::schedule(ndn::time::seconds(1),
                          bind(&DummyStreamSender::stop, this));
    }
  }

  void
  stop()
  {
    // Terminate test
    boost::system::error_code error;
    socket.shutdown(Socket::shutdown_both, error);
    socket.close(error);
  }

public:
  Socket socket;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_DUMMY_STREAM_SENDER_HPP
