/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_STRATEGY_INFO_HOST_HPP
#define NFD_TABLE_STRATEGY_INFO_HOST_HPP

#include "fw/strategy-info.hpp"

namespace nfd {

/** \class StrategyInfoHost
 *  \brief base class for an entity onto which StrategyInfo may be placed
 */
class StrategyInfoHost
{
public:
  template<typename T>
  void
  setStrategyInfo(shared_ptr<T> strategyInfo);
  
  template<typename T>
  shared_ptr<T>
  getStrategyInfo();
  
  void
  clearStrategyInfo();

private:
  shared_ptr<fw::StrategyInfo> m_strategyInfo;
};


template<typename T>
void
StrategyInfoHost::setStrategyInfo(shared_ptr<T> strategyInfo)
{
  m_strategyInfo = strategyInfo;
}

template<typename T>
shared_ptr<T>
StrategyInfoHost::getStrategyInfo()
{
  return static_pointer_cast<T, fw::StrategyInfo>(m_strategyInfo);
}

} // namespace nfd

#endif // NFD_TABLE_STRATEGY_INFO_HOST_HPP
