/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_STRATEGY_INFO_HPP
#define NFD_FW_STRATEGY_INFO_HPP

#include "common.hpp"

namespace nfd {
namespace fw {

/** \class StrategyInfo
 *  \brief contains arbitrary information forwarding strategy places on table entries
 */
class StrategyInfo
{
public:
  virtual
  ~StrategyInfo();
};


inline
StrategyInfo::~StrategyInfo()
{
}

} // namespace fw
} // namespace nfd

#endif // NFD_FW_STRATEGY_INFO_HPP
