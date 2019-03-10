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

#include "common/global.hpp"
#include "face/face.hpp"
#include "face/tcp-channel.hpp"
#include "face/udp-channel.hpp"

#include <boost/exception/diagnostic_information.hpp>

#include <fstream>
#include <iostream>

#ifdef HAVE_VALGRIND
#include <valgrind/callgrind.h>
#endif

namespace nfd {
namespace tests {

class FaceBenchmark
{
public:
  FaceBenchmark(const char* configFileName)
    : m_terminationSignalSet{getGlobalIoService()}
    , m_tcpChannel{tcp::Endpoint{boost::asio::ip::tcp::v4(), 6363}, false,
                   bind([] { return ndn::nfd::FACE_SCOPE_NON_LOCAL; })}
    , m_udpChannel{udp::Endpoint{boost::asio::ip::udp::v4(), 6363}, 10_min, false}
  {
    m_terminationSignalSet.add(SIGINT);
    m_terminationSignalSet.add(SIGTERM);
    m_terminationSignalSet.async_wait([] (const auto& error, int) {
      if (!error)
        getGlobalIoService().stop();
    });

    parseConfig(configFileName);

    m_tcpChannel.listen(bind(&FaceBenchmark::onLeftFaceCreated, this, _1),
                        bind(&FaceBenchmark::onFaceCreationFailed, _1, _2));
    std::clog << "Listening on " << m_tcpChannel.getUri() << std::endl;

    m_udpChannel.listen(bind(&FaceBenchmark::onLeftFaceCreated, this, _1),
                        bind(&FaceBenchmark::onFaceCreationFailed, _1, _2));
    std::clog << "Listening on " << m_udpChannel.getUri() << std::endl;
  }

private:
  void
  parseConfig(const char* configFileName)
  {
    std::ifstream file{configFileName};
    std::string uriStrL;
    std::string uriStrR;

    while (file >> uriStrL >> uriStrR) {
      FaceUri uriL{uriStrL};
      FaceUri uriR{uriStrR};

      if (uriL.getScheme() != "tcp4" && uriL.getScheme() != "udp4") {
        std::clog << "Unsupported protocol '" << uriL.getScheme() << "'" << std::endl;
      }
      else if (uriR.getScheme() != "tcp4" && uriR.getScheme() != "udp4") {
        std::clog << "Unsupported protocol '" << uriR.getScheme() << "'" << std::endl;
      }
      else {
        m_faceUris.push_back(std::make_pair(uriL, uriR));
      }
    }

    if (m_faceUris.empty()) {
      NDN_THROW(std::runtime_error("No supported FaceUri pairs found in config file"));
    }
  }

  void
  onLeftFaceCreated(const shared_ptr<Face>& faceL)
  {
    std::clog << "Left face created: remote=" << faceL->getRemoteUri()
              << " local=" << faceL->getLocalUri() << std::endl;

    // find a matching right uri
    FaceUri uriR;
    for (const auto& pair : m_faceUris) {
      if (pair.first.getHost() == faceL->getRemoteUri().getHost() &&
          pair.first.getScheme() == faceL->getRemoteUri().getScheme()) {
        uriR = pair.second;
      }
      else if (pair.second.getHost() == faceL->getRemoteUri().getHost() &&
               pair.second.getScheme() == faceL->getRemoteUri().getScheme()) {
        uriR = pair.first;
      }
    }

    if (uriR == FaceUri()) {
      std::clog << "No FaceUri matched, ignoring..." << std::endl;
      faceL->close();
      return;
    }

    // create the right face
    auto addr = boost::asio::ip::address::from_string(uriR.getHost());
    auto port = boost::lexical_cast<uint16_t>(uriR.getPort());
    if (uriR.getScheme() == "tcp4") {
      m_tcpChannel.connect(tcp::Endpoint(addr, port), {},
                           bind(&FaceBenchmark::onRightFaceCreated, this, faceL, _1),
                           bind(&FaceBenchmark::onFaceCreationFailed, _1, _2));
    }
    else if (uriR.getScheme() == "udp4") {
      m_udpChannel.connect(udp::Endpoint(addr, port), {},
                           bind(&FaceBenchmark::onRightFaceCreated, this, faceL, _1),
                           bind(&FaceBenchmark::onFaceCreationFailed, _1, _2));
    }
  }

  void
  onRightFaceCreated(const shared_ptr<Face>& faceL, const shared_ptr<Face>& faceR)
  {
    std::clog << "Right face created: remote=" << faceR->getRemoteUri()
              << " local=" << faceR->getLocalUri() << std::endl;

    tieFaces(faceR, faceL);
    tieFaces(faceL, faceR);
  }

  static void
  tieFaces(const shared_ptr<Face>& face1, const shared_ptr<Face>& face2)
  {
    face1->afterReceiveInterest.connect([face2] (const Interest& interest, const EndpointId&) {
      face2->sendInterest(interest, 0);
    });
    face1->afterReceiveData.connect([face2] (const Data& data, const EndpointId&) {
      face2->sendData(data, 0);
    });
    face1->afterReceiveNack.connect([face2] (const ndn::lp::Nack& nack, const EndpointId&) {
      face2->sendNack(nack, 0);
    });
  }

  static void
  onFaceCreationFailed(uint32_t status, const std::string& reason)
  {
    NDN_THROW(std::runtime_error("Failed to create face: [" + to_string(status) + "] " + reason));
  }

private:
  boost::asio::signal_set m_terminationSignalSet;
  face::TcpChannel m_tcpChannel;
  face::UdpChannel m_udpChannel;
  std::vector<std::pair<FaceUri, FaceUri>> m_faceUris;
};

} // namespace tests
} // namespace nfd

int
main(int argc, char** argv)
{
#ifdef _DEBUG
  std::cerr << "Benchmark compiled in debug mode is unreliable, please compile in release mode.\n";
#endif

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <config-file>" << std::endl;
    return 2;
  }

  try {
    nfd::tests::FaceBenchmark bench{argv[1]};
#ifdef HAVE_VALGRIND
    CALLGRIND_START_INSTRUMENTATION;
#endif
    nfd::getGlobalIoService().run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << boost::diagnostic_information(e);
    return 1;
  }

  return 0;
}
