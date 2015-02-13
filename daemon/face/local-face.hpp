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

#ifndef NFD_DAEMON_FACE_LOCAL_FACE_HPP
#define NFD_DAEMON_FACE_LOCAL_FACE_HPP

#include "face.hpp"
#include <ndn-cxx/management/nfd-control-parameters.hpp>

namespace nfd {

using ndn::nfd::LocalControlFeature;
using ndn::nfd::LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID;
using ndn::nfd::LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID;

/** \brief represents a face
 */
class LocalFace : public Face
{
public:
  LocalFace(const FaceUri& remoteUri, const FaceUri& localUri);

  /** \brief get whether any LocalControlHeader feature is enabled
   *
   * \returns true if any feature is enabled.
   */
  bool
  isLocalControlHeaderEnabled() const;

  /** \brief get whether a specific LocalControlHeader feature is enabled
   *
   *  \param feature The feature.
   *  \returns true if the specified feature is enabled.
   */
  bool
  isLocalControlHeaderEnabled(LocalControlFeature feature) const;

  /** \brief enable or disable a LocalControlHeader feature
   *
   *  \param feature The feature. Cannot be LOCAL_CONTROL_FEATURE_ANY
   *                                     or LOCAL_CONTROL_FEATURE_MAX
   */
  void
  setLocalControlHeaderFeature(LocalControlFeature feature, bool enabled = true);

public:

  static const size_t LOCAL_CONTROL_FEATURE_MAX = 3; /// upper bound of LocalControlFeature enum
  static const size_t LOCAL_CONTROL_FEATURE_ANY = 0; /// any feature

protected:
  // statically overridden from Face

  /** \brief Decode block into Interest/Data, considering potential LocalControlHeader
   *
   *  If LocalControlHeader is present, the encoded data is filtered out, based
   *  on enabled features on the face.
   */
  bool
  decodeAndDispatchInput(const Block& element);

  // LocalFace-specific methods

  /** \brief Check if LocalControlHeader needs to be included, taking into account
   *         both set parameters in supplied LocalControlHeader and features
   *         enabled on the local face.
   */
  bool
  isEmptyFilteredLocalControlHeader(const ndn::nfd::LocalControlHeader& header) const;

  /** \brief Create LocalControlHeader, considering enabled features
   */
  template<class Packet>
  Block
  filterAndEncodeLocalControlHeader(const Packet& packet);

private:
  std::vector<bool> m_localControlHeaderFeatures;
};

inline
LocalFace::LocalFace(const FaceUri& remoteUri, const FaceUri& localUri)
  : Face(remoteUri, localUri, true)
  , m_localControlHeaderFeatures(LocalFace::LOCAL_CONTROL_FEATURE_MAX)
{
}

inline bool
LocalFace::isLocalControlHeaderEnabled() const
{
  return m_localControlHeaderFeatures[LOCAL_CONTROL_FEATURE_ANY];
}

inline bool
LocalFace::isLocalControlHeaderEnabled(LocalControlFeature feature) const
{
  BOOST_ASSERT(0 < feature &&
               static_cast<size_t>(feature) < m_localControlHeaderFeatures.size());
  return m_localControlHeaderFeatures[feature];
}

inline void
LocalFace::setLocalControlHeaderFeature(LocalControlFeature feature, bool enabled/* = true*/)
{
  BOOST_ASSERT(0 < feature &&
               static_cast<size_t>(feature) < m_localControlHeaderFeatures.size());

  m_localControlHeaderFeatures[feature] = enabled;

  m_localControlHeaderFeatures[LOCAL_CONTROL_FEATURE_ANY] =
    std::find(m_localControlHeaderFeatures.begin() + 1,
              m_localControlHeaderFeatures.end(), true) <
              m_localControlHeaderFeatures.end();
  // 'find(..) < .end()' instead of 'find(..) != .end()' due to LLVM Bug 16816
}

inline bool
LocalFace::decodeAndDispatchInput(const Block& element)
{
  try {
    const Block& payload = ndn::nfd::LocalControlHeader::getPayload(element);

    // If received LocalControlHeader, but it is not enabled on the face
    if ((&payload != &element) && !this->isLocalControlHeaderEnabled())
      return false;

    if (payload.type() == tlv::Interest)
      {
        shared_ptr<Interest> i = make_shared<Interest>();
        i->wireDecode(payload);
        if (&payload != &element)
          {
            uint8_t mask = 0;
            if (this->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID)) {
              mask |= ndn::nfd::LocalControlHeader::ENCODE_NEXT_HOP;
            }
            i->getLocalControlHeader().wireDecode(element, mask);
          }

        this->emitSignal(onReceiveInterest, *i);
      }
    else if (payload.type() == tlv::Data)
      {
        shared_ptr<Data> d = make_shared<Data>();
        d->wireDecode(payload);

        /// \todo Uncomment and correct the following when we have more
        ///       options in LocalControlHeader that apply for incoming
        ///       Data packets (if ever)
        // if (&payload != &element)
        //   {
        //
        //     d->getLocalControlHeader().wireDecode(element,
        //       false,
        //       false);
        //   }

        this->emitSignal(onReceiveData, *d);
      }
    else
      return false;

    return true;
  }
  catch (const tlv::Error&) {
    return false;
  }
}

inline bool
LocalFace::isEmptyFilteredLocalControlHeader(const ndn::nfd::LocalControlHeader& header) const
{
  if (!this->isLocalControlHeaderEnabled())
    return true;

  uint8_t mask = 0;
  if (this->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID)) {
    mask |= ndn::nfd::LocalControlHeader::ENCODE_INCOMING_FACE_ID;
  }
  return header.empty(mask);
}

template<class Packet>
inline Block
LocalFace::filterAndEncodeLocalControlHeader(const Packet& packet)
{
  uint8_t mask = 0;
  if (this->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID)) {
    mask |= ndn::nfd::LocalControlHeader::ENCODE_INCOMING_FACE_ID;
  }
  return packet.getLocalControlHeader().wireEncode(packet, mask);
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_LOCAL_FACE_HPP
