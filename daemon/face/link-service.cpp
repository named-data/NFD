/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Regents of the University of California,
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

 #include "link-service.hpp"
 #include "face.hpp"
 
 namespace nfd {
 namespace face {
 
 NFD_LOG_INIT(LinkService);
 
 LinkService::LinkService()
   : m_face(nullptr)
   , m_transport(nullptr)
 {
 }
 
 LinkService::~LinkService()
 {
 }
 
 void
 LinkService::setFaceAndTransport(Face& face, Transport& transport)
 {
   BOOST_ASSERT(m_face == nullptr);
   BOOST_ASSERT(m_transport == nullptr);
 
   m_face = &face;
   m_transport = &transport;
 }
 
 void
 LinkService::sendInterest(const Interest& interest)
 {
   BOOST_ASSERT(m_transport != nullptr);
   NFD_LOG_FACE_TRACE(__func__);
 
   if (this->getFace()->getLinkType() != ndn::nfd::LINK_TYPE_MULTI_ACCESS || !m_suppressionEnabled)
   {
     ++this->nOutInterests;
     doSendInterest(interest);
     return;
   }
   // apply suppression algorithm if sending through multicast face
   // check if the interest is already in flight
   if (m_multicastSuppression.interestInflight(interest)) {
     NFD_LOG_INFO ("Interest drop by suppression, with name " <<  interest.getName() << " is in flight, drop the forwarding");
     return; // need to catch this, what should be the behaviour after dropping the interest?? 
   }
   // wait for suppression time before forwarding
   // check if another interest is overheard during the wait, if heard, cancle the forwarding
   auto suppressionTime = m_multicastSuppression.getDelayTimer(interest.getName(), 'i');
   NFD_LOG_INFO ("Interest " <<  interest.getName() << " not in flight, waiting" << suppressionTime << "before forwarding");
 
   auto entry_name = interest.getName();
   entry_name.appendNumber(0);
   auto eventId = getScheduler().schedule(suppressionTime, [this, interest, entry_name] {
     NFD_LOG_INFO ("Interest " <<  interest.getName() << " Analysis History Interest sent finally: ");
     int result = m_multicastSuppression.recordInterest(interest, true);
     if (result == 0) {
      NFD_LOG_INFO("Interest drop by suppression, Interest " <<  interest.getName() << " is in measurement table, drop the forwarding");
     }
     else{
      ++this->nOutInterests;
      doSendInterest(interest);
     }
     
     if (m_scheduledEntry.count(entry_name) > 0)
       m_scheduledEntry.erase(entry_name);
   });
   m_scheduledEntry.emplace(entry_name, std::move(eventId));
 }

 void
 LinkService::sendData(const Data& data)
 {
   BOOST_ASSERT(m_transport != nullptr);
   NFD_LOG_FACE_TRACE(__func__);
   if (this->getFace()->getLinkType() != ndn::nfd::LINK_TYPE_MULTI_ACCESS || !m_suppressionEnabled)
   {
     ++this->nOutData;
     doSendData(data);
     return;
   }
   if (m_multicastSuppression.dataInflight(data)) {
    NFD_LOG_INFO("Data drop by suppression, Data " <<  data.getName() << " is in measurement table, drop the forwarding");
    return; // need to catch this, what should be the behaviour after dropping the interest?? 
   }
   /*
     Same suppression logic as that of interest cannot be applied to multicast data.
     Doing so we might end up suppressing data for the interest received at different interval
     from different node, additionally it will also trigger multiple retransmission from the node
     that didn't received the data due to suppression. This might not happen if unsolicated data are
     cached, but the node that's supposed to send the data can't gurantee it.
 
     data sending should wait before forwarding, during this wait time if another data is overheard, need to drop the forwarding
     wait time should be determined based on the number of duplicate overhearing
   */
   //  for now, lets wait for some time before forwarding, if overheard, drop the reply
   auto suppressionTime = m_multicastSuppression.getDelayTimer(data.getName(), 'd');
   NFD_LOG_INFO("Waiting : " << suppressionTime<< "ms before sending data" << data.getName());
 
   auto entry_name = data.getName();
   entry_name.appendNumber(1);
   auto eventId = getScheduler().schedule(suppressionTime, [this, data, entry_name] {
 
     NFD_LOG_INFO("Sending data finally, via multicast face Analysis History Data sent finally:" << data.getName());
     int result = m_multicastSuppression.recordData(data, true);
     if (result == 0) {
      NFD_LOG_INFO("Data drop by suppression, Data " <<  data.getName() << " is in measurement table, drop the forwarding");
     }
     else{
      ++this->nOutData;
      doSendData(data);
     }
     if(m_scheduledEntry.count(entry_name) > 0)
         m_scheduledEntry.erase(entry_name);
 
   });
   m_scheduledEntry.emplace(entry_name, std::move(eventId));
 }


 void
 LinkService::sendNack(const ndn::lp::Nack& nack)
 {
   BOOST_ASSERT(m_transport != nullptr);
   NFD_LOG_FACE_TRACE(__func__);
 
   ++this->nOutNacks;
 
   doSendNack(nack);
 }
 
 bool
 LinkService::cancelIfSchdeuled(Name name, int type)
 {
   Name entry_name = name;
   entry_name.appendNumber(type);
   auto it = m_scheduledEntry.find(entry_name);
   if (it != m_scheduledEntry.end()) {
     it->second.cancel();
     m_scheduledEntry.erase(entry_name);
     return true;
   }
   return false;
 }
 
 void
 LinkService::receiveInterest(const Interest& interest, const EndpointId& endpoint)
 {
   NFD_LOG_FACE_TRACE(__func__);
   // record multicast interest
   if (this->getFace()->getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS && m_suppressionEnabled)
   {
     NFD_LOG_INFO("Multicast interest received: " << interest.getName());
     // check if a same interest is scheduled, if so drop it
     if (cancelIfSchdeuled(interest.getName(), 0))
       NDN_LOG_INFO("Interest drop by suppression, with name" << interest.getName() << " overheard, duplicate forwarding dropped");
     m_multicastSuppression.recordInterest(interest, false);
   }
   ++this->nInInterests;
   afterReceiveInterest(interest, endpoint);
 }
 
 void
 LinkService::receiveData(const Data& data, const EndpointId& endpoint)
 {
   NFD_LOG_FACE_TRACE(__func__);
   // record multicast Data received
   if (this->getFace()->getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS && m_suppressionEnabled)
   {
     NFD_LOG_INFO("Multicast data received: " << data.getName());
     if (cancelIfSchdeuled(data.getName(), 1))
       NDN_LOG_INFO("Data drop by suppression, with name  " << data.getName() << " overheard, duplicate forwarding dropped");

     if (cancelIfSchdeuled(data.getName(), 0)) // also can drop interest if shceduled for this data
       NDN_LOG_INFO("Interest drop by suppression, with name " << data.getName() << " overheard, drop the corresponding scheduled interest");

     m_multicastSuppression.recordData(data, false);
   }
 
   ++this->nInData;
   // record multicast data
   afterReceiveData(data, endpoint);
 }
 
 void
 LinkService::receiveNack(const ndn::lp::Nack& nack, const EndpointId& endpoint)
 {
   NFD_LOG_FACE_TRACE(__func__);
 
   ++this->nInNacks;
 
   afterReceiveNack(nack, endpoint);
 }
 
 void
 LinkService::notifyDroppedInterest(const Interest& interest)
 {
   ++this->nInterestsExceededRetx;
   onDroppedInterest(interest);
 }
 
 std::ostream&
 operator<<(std::ostream& os, const FaceLogHelper<LinkService>& flh)
 {
   const Face* face = flh.obj.getFace();
   if (face == nullptr) {
     os << "[id=0,local=unknown,remote=unknown] ";
   }
   else {
     os << "[id=" << face->getId() << ",local=" << face->getLocalUri()
        << ",remote=" << face->getRemoteUri() << "] ";
   }
   return os;
 }
 
 } // namespace face
 } // namespace nfd