/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "dns-srv.hpp"

#include <sys/types.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>

#ifdef __APPLE__
#include <arpa/nameser_compat.h>
#endif

#include <ndn-cxx/util/backports.hpp>
#include <ndn-cxx/util/exception.hpp>

#include <iostream>

#include <boost/endian/conversion.hpp>

namespace ndn::autoconfig {

using namespace std::string_literals;

union QueryAnswer
{
  HEADER header;
  uint8_t buf[NS_PACKETSZ];
};

/** \brief Parse SRV record
 *  \return FaceUri of the hub from the SRV record
 *  \throw DnsSrvError if SRV record cannot be parsed
 */
static std::string
parseSrvRr(const QueryAnswer& queryAnswer, int answerSize)
{
  // The references of the next classes are:
  // http://www.diablotin.com/librairie/networking/dnsbind/ch14_02.htm

  struct rechdr
  {
    uint16_t type;
    uint16_t iclass;
    uint32_t ttl;
    uint16_t length;
  };

  struct srv_t
  {
    uint16_t priority;
    uint16_t weight;
    uint16_t port;
    uint8_t* target;
  };

  uint16_t ancount = queryAnswer.header.ancount;
  if (boost::endian::big_to_native(ancount) == 0) {
    NDN_THROW(DnsSrvError("SRV record cannot be parsed"));
  }

  const uint8_t* blob = queryAnswer.buf + NS_HFIXEDSZ;
  blob += dn_skipname(blob, queryAnswer.buf + answerSize) + NS_QFIXEDSZ;

  char srvName[NS_MAXDNAME];
  int serverNameSize = dn_expand(queryAnswer.buf,               // message pointer
                                 queryAnswer.buf + answerSize,  // end of message
                                 blob,                          // compressed server name
                                 srvName,                       // expanded server name
                                 NS_MAXDNAME);
  if (serverNameSize <= 0) {
    NDN_THROW(DnsSrvError("SRV record cannot be parsed (error decoding domain name)"));
  }

  const srv_t* server = reinterpret_cast<const srv_t*>(&blob[sizeof(rechdr)]);
  uint16_t port = boost::endian::big_to_native(server->port);

  blob += serverNameSize + NS_HFIXEDSZ + NS_QFIXEDSZ;

  char hostName[NS_MAXDNAME];
  int hostNameSize = dn_expand(queryAnswer.buf,               // message pointer
                               queryAnswer.buf + answerSize,  // end of message
                               blob,                          // compressed host name
                               hostName,                      // expanded host name
                               NS_MAXDNAME);
  if (hostNameSize <= 0) {
    NDN_THROW(DnsSrvError("SRV record cannot be parsed (error decoding host name)"));
  }

  return "udp://"s + hostName + ":" + std::to_string(port);
}

std::string
querySrvRr(const std::string& fqdn)
{
  std::string srvDomain = "_ndn._udp." + fqdn;
  std::cerr << "Sending DNS query for SRV record for " << srvDomain << std::endl;

  res_init();

  _res.retrans = 1;
  _res.retry = 2;
  _res.ndots = 10;

  QueryAnswer queryAnswer;
  int answerSize = res_query(srvDomain.data(),
                             ns_c_in,
                             ns_t_srv,
                             queryAnswer.buf,
                             sizeof(queryAnswer));
  if (answerSize == 0) {
    NDN_THROW(DnsSrvError("No DNS SRV records found for " + srvDomain));
  }
  return parseSrvRr(queryAnswer, answerSize);
}

/**
 * @brief Send DNS SRV request using search domain list
 */
std::string
querySrvRrSearch()
{
  std::cerr << "Sending DNS query for SRV record for _ndn._udp" << std::endl;

  QueryAnswer queryAnswer;

  res_init();

  _res.retrans = 1;
  _res.retry = 2;
  _res.ndots = 10;

  int answerSize = res_search("_ndn._udp",
                              ns_c_in,
                              ns_t_srv,
                              queryAnswer.buf,
                              sizeof(queryAnswer));

  if (answerSize == 0) {
    NDN_THROW(DnsSrvError("No DNS SRV records found for _ndn._udp"));
  }

  return parseSrvRr(queryAnswer, answerSize);
}

} // namespace ndn::autoconfig
