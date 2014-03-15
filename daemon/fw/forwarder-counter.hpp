/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_FORWARDER_COUNTER_HPP
#define NFD_FW_FORWARDER_COUNTER_HPP

#include "face/face-counter.hpp"

namespace nfd {

/** \class ForwarderCounter
 *  \brief represents a counter on forwarder
 *
 *  \todo This class should be noncopyable
 */
typedef uint64_t ForwarderCounter;


/** \brief contains counters on forwarder
 */
class ForwarderCounters : public FaceCounters
{
};


} // namespace nfd

#endif // NFD_FW_FORWARDER_COUNTER_HPP
