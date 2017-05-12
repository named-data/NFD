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

#ifndef NFD_DAEMON_FACE_PCAP_HELPER_HPP
#define NFD_DAEMON_FACE_PCAP_HELPER_HPP

#include "core/common.hpp"

#ifndef HAVE_LIBPCAP
#error "Cannot include this file when libpcap is not available"
#endif

// forward declarations
struct pcap;
typedef pcap pcap_t;

namespace nfd {
namespace face {

/**
 * @brief Helper class for dealing with libpcap handles.
 */
class PcapHelper : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  /**
   * @brief Create a libpcap context for live packet capture on a network interface.
   * @throw Error on any error
   * @sa pcap_create(3pcap)
   */
  explicit
  PcapHelper(const std::string& interfaceName);

  ~PcapHelper();

  /**
   * @brief Start capturing packets.
   * @param dlt The link-layer header type to be used.
   * @throw Error on any error
   * @sa pcap_activate(3pcap), pcap_set_datalink(3pcap)
   */
  void
  activate(int dlt);

  /**
   * @brief Stop capturing and close the handle.
   * @sa pcap_close(3pcap)
   */
  void
  close();

  /**
   * @brief Obtain a file descriptor that can be used in calls such as select(2) and poll(2).
   * @pre activate() has been called.
   * @return A selectable file descriptor. It is the caller's responsibility to close the fd.
   * @throw Error on any error
   * @sa pcap_get_selectable_fd(3pcap)
   */
  int
  getFd() const;

  /**
   * @brief Get last error message.
   * @return Human-readable explanation of the last libpcap error.
   * @warning The behavior is undefined if no error occurred.
   * @sa pcap_geterr(3pcap)
   */
  std::string
  getLastError() const;

  /**
   * @brief Get the number of packets dropped by the kernel, as reported by libpcap.
   * @throw Error on any error
   * @sa pcap_stats(3pcap)
   */
  size_t
  getNDropped() const;

  /**
   * @brief Install a BPF filter on the receiving socket.
   * @param filter Null-terminated string containing the BPF program source.
   * @pre activate() has been called.
   * @throw Error on any error
   * @sa pcap_setfilter(3pcap), pcap-filter(7)
   */
  void
  setPacketFilter(const char* filter) const;

  /**
   * @brief Read the next packet captured on the interface.
   * @return If successful, returns a tuple containing a pointer to the received packet
   *         (including the link-layer header) and the size of the packet; the third
   *         element must be ignored. On failure, returns a tuple containing nullptr,
   *         0, and the reason for the failure.
   * @warning The returned pointer must not be freed by the caller, and is valid only
   *          until the next call to this function.
   * @sa pcap_next_ex(3pcap)
   */
  std::tuple<const uint8_t*, size_t, std::string>
  readNextPacket() const;

  operator pcap_t*() const
  {
    return m_pcap;
  }

private:
  pcap_t* m_pcap;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_PCAP_HELPER_HPP
