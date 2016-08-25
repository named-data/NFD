/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP
#define NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP

#include "fw/strategy-info.hpp"

namespace nfd {

/** \brief base class for an entity onto which StrategyInfo items may be placed
 */
class StrategyInfoHost
{
public:
  /** \brief get a StrategyInfo item
   *  \tparam T type of StrategyInfo, must be a subclass of fw::StrategyInfo
   *  \return an existing StrategyInfo item of type T, or nullptr if it does not exist
   */
  template<typename T>
  T*
  getStrategyInfo() const
  {
    static_assert(std::is_base_of<fw::StrategyInfo, T>::value,
                  "T must inherit from StrategyInfo");

    auto it = m_items.find(T::getTypeId());
    if (it == m_items.end()) {
      return nullptr;
    }
    return static_cast<T*>(it->second.get());
  }

  /** \brief insert a StrategyInfo item
   *  \tparam T type of StrategyInfo, must be a subclass of fw::StrategyInfo
   *  \return a new or existing StrategyInfo item of type T,
   *          and true for new item, false for existing item
   */
  template<typename T, typename ...A>
  std::pair<T*, bool>
  insertStrategyInfo(A&&... args)
  {
    static_assert(std::is_base_of<fw::StrategyInfo, T>::value,
                  "T must inherit from StrategyInfo");

    unique_ptr<fw::StrategyInfo>& item = m_items[T::getTypeId()];
    bool isNew = (item == nullptr);
    if (isNew) {
      item.reset(new T(std::forward<A>(args)...));
    }
    return {static_cast<T*>(item.get()), isNew};
  }

  /** \brief erase a StrategyInfo item
   *  \tparam T type of StrategyInfo, must be a subclass of fw::StrategyInfo
   *  \return number of items erased
   */
  template<typename T>
  size_t
  eraseStrategyInfo()
  {
    static_assert(std::is_base_of<fw::StrategyInfo, T>::value,
                  "T must inherit from StrategyInfo");

    return m_items.erase(T::getTypeId());
  }

  /** \brief clear all StrategyInfo items
   */
  void
  clearStrategyInfo();

private:
  std::unordered_map<int, unique_ptr<fw::StrategyInfo>> m_items;
};

} // namespace nfd

#endif // NFD_DAEMON_TABLE_STRATEGY_INFO_HOST_HPP
