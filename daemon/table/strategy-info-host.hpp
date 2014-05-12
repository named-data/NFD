/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#ifndef NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP
#define NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP

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
  getStrategyInfo() const;

  template<typename T>
  shared_ptr<T>
  getOrCreateStrategyInfo();

  template<typename T, typename T1>
  shared_ptr<T>
  getOrCreateStrategyInfo(T1& a1);

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
StrategyInfoHost::getStrategyInfo() const
{
  return static_pointer_cast<T, fw::StrategyInfo>(m_strategyInfo);
}

template<typename T>
shared_ptr<T>
StrategyInfoHost::getOrCreateStrategyInfo()
{
  shared_ptr<T> info = this->getStrategyInfo<T>();
  if (!static_cast<bool>(info)) {
    info = make_shared<T>();
    this->setStrategyInfo(info);
  }
  return info;
}

template<typename T, typename T1>
shared_ptr<T>
StrategyInfoHost::getOrCreateStrategyInfo(T1& a1)
{
  shared_ptr<T> info = this->getStrategyInfo<T>();
  if (!static_cast<bool>(info)) {
    info = make_shared<T>(ref(a1));
    this->setStrategyInfo(info);
  }
  return info;
}

} // namespace nfd

#endif // NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP
