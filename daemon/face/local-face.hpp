/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_LOCAL_FACE_HPP
#define NFD_FACE_LOCAL_FACE_HPP

#include "face.hpp"

namespace nfd {

/* \brief indicates a feature in LocalControlHeader
 */
enum LocalControlHeaderFeature
{
  /// any feature
  LOCAL_CONTROL_HEADER_FEATURE_ANY,
  /// in-faceid
  LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID,
  /// out-faceid
  LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID,
  /// upper bound of enum
  LOCAL_CONTROL_HEADER_FEATURE_MAX
};


/** \brief represents a face
 */
class LocalFace : public Face
{
public:
  LocalFace();

  /** \brief get whether a LocalControlHeader feature is enabled
   *
   *  \param feature The feature. Cannot be LOCAL_CONTROL_HEADER_FEATURE_MAX
   *  LOCAL_CONTROL_HEADER_FEATURE_ANY returns true if any feature is enabled.
   */
  bool
  isLocalControlHeaderEnabled(LocalControlHeaderFeature feature =
                              LOCAL_CONTROL_HEADER_FEATURE_ANY) const;

  /** \brief enable or disable a LocalControlHeader feature
   *
   *  \param feature The feature. Cannot be LOCAL_CONTROL_HEADER_FEATURE_ANY
   *                                     or LOCAL_CONTROL_HEADER_FEATURE_MAX
   */
  void
  setLocalControlHeaderFeature(LocalControlHeaderFeature feature, bool enabled = true);

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
LocalFace::LocalFace()
  : Face(true)
  , m_localControlHeaderFeatures(LOCAL_CONTROL_HEADER_FEATURE_MAX)
{
}

inline bool
LocalFace::isLocalControlHeaderEnabled(LocalControlHeaderFeature feature) const
{
  BOOST_ASSERT(feature < m_localControlHeaderFeatures.size());
  return m_localControlHeaderFeatures[feature];
}

inline void
LocalFace::setLocalControlHeaderFeature(LocalControlHeaderFeature feature, bool enabled/* = true*/)
{
  BOOST_ASSERT(feature > LOCAL_CONTROL_HEADER_FEATURE_ANY &&
               feature < m_localControlHeaderFeatures.size());
  m_localControlHeaderFeatures[feature] = enabled;

  BOOST_STATIC_ASSERT(LOCAL_CONTROL_HEADER_FEATURE_ANY == 0);
  m_localControlHeaderFeatures[LOCAL_CONTROL_HEADER_FEATURE_ANY] =
    std::find(m_localControlHeaderFeatures.begin() + 1,
              m_localControlHeaderFeatures.end(), true) <
              m_localControlHeaderFeatures.end();
  // 'find(..) < .end()' instead of 'find(..) != .end()' due to LLVM Bug 16816
}

inline bool
LocalFace::decodeAndDispatchInput(const Block& element)
{
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
          i->getLocalControlHeader().wireDecode(element,
            false,
            this->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
        }

      this->onReceiveInterest(*i);
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

      this->onReceiveData(*d);
    }
  else
    return false;

  return true;
}

inline bool
LocalFace::isEmptyFilteredLocalControlHeader(const ndn::nfd::LocalControlHeader& header) const
{
  if (!this->isLocalControlHeaderEnabled())
    return true;

  return header.empty(this->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID),
                      false);
}

template<class Packet>
inline Block
LocalFace::filterAndEncodeLocalControlHeader(const Packet& packet)
{
  return packet.getLocalControlHeader().wireEncode(packet,
           this->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID),
           false);
}

} // namespace nfd

#endif // NFD_FACE_LOCAL_FACE_HPP
