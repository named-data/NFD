/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#ifndef NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP
#define NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP

#include "fw/strategy-info.hpp"

namespace nfd {

/** \brief base class for an entity onto which StrategyInfo objects may be placed
 */
class StrategyInfoHost
{
public:
  /** \brief get a StrategyInfo item
   *  \tparam T type of StrategyInfo, must be a subclass of from nfd::fw::StrategyInfo
   *  \retval nullptr if no StrategyInfo of type T is stored
   */
  template<typename T>
  shared_ptr<T>
  getStrategyInfo() const;

  /** \brief set a StrategyInfo item
   *  \tparam T type of StrategyInfo, must be a subclass of from nfd::fw::StrategyInfo
   */
  template<typename T>
  void
  setStrategyInfo(shared_ptr<T> strategyInfo);

  /** \brief get or create a StrategyInfo item
   *  \tparam T type of StrategyInfo, must be a subclass of from nfd::fw::StrategyInfo
   *
   *  If no StrategyInfo of type T is stored, it's created with \p args;
   *  otherwise, the existing item is returned.
   */
  template<typename T, typename ...A>
  shared_ptr<T>
  getOrCreateStrategyInfo(A&&... args);

  /** \brief clear all StrategyInfo items
   */
  void
  clearStrategyInfo();

private:
  std::map<int, shared_ptr<fw::StrategyInfo>> m_items;
};


template<typename T>
shared_ptr<T>
StrategyInfoHost::getStrategyInfo() const
{
  static_assert(std::is_base_of<fw::StrategyInfo, T>::value,
                "T must inherit from StrategyInfo");

  auto it = m_items.find(T::getTypeId());
  if (it == m_items.end()) {
    return nullptr;
  }
  return static_pointer_cast<T, fw::StrategyInfo>(it->second);
}

template<typename T>
void
StrategyInfoHost::setStrategyInfo(shared_ptr<T> item)
{
  static_assert(std::is_base_of<fw::StrategyInfo, T>::value,
                "T must inherit from StrategyInfo");

  if (item == nullptr) {
    m_items.erase(T::getTypeId());
  }
  else {
    m_items[T::getTypeId()] = item;
  }
}

template<typename T, typename ...A>
shared_ptr<T>
StrategyInfoHost::getOrCreateStrategyInfo(A&&... args)
{
  static_assert(std::is_base_of<fw::StrategyInfo, T>::value,
                "T must inherit from StrategyInfo");

  shared_ptr<T> item = this->getStrategyInfo<T>();
  if (!static_cast<bool>(item)) {
    item = make_shared<T>(std::forward<A>(args)...);
    this->setStrategyInfo(item);
  }
  return item;
}

} // namespace nfd

#endif // NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP
