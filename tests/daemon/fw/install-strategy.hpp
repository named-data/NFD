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

#ifndef NFD_TESTS_DAEMON_FW_INSTALL_STRATEGY_HPP
#define NFD_TESTS_DAEMON_FW_INSTALL_STRATEGY_HPP

namespace nfd {
namespace fw {
class Strategy;
} // namespace fw

namespace tests {

/** \brief install a strategy to forwarder
 *  \tparam S strategy type
 *  \param forwarder the forwarder
 *  \param args arguments to strategy constructor
 *  \throw std::bad_cast a strategy with duplicate strategyName is already installed
 *                       and has an incompatible type
 *  \return a reference to the strategy
 */
template<typename S, typename ...Args>
typename std::enable_if<std::is_base_of<fw::Strategy, S>::value, S&>::type
install(Forwarder& forwarder, Args&&... args)
{
  auto strategy = make_unique<S>(ref(forwarder), std::forward<Args>(args)...);
  fw::Strategy* installed = forwarder.getStrategyChoice().install(std::move(strategy)).second;
  return dynamic_cast<S&>(*installed);
}

/** \brief install a strategy to forwarder, and choose the strategy for a namespace
 *  \tparam S strategy type
 *  \param forwarder the forwarder
 *  \param prefix namespace to choose the strategy for
 *  \param args arguments to strategy constructor
 *  \throw std::bad_cast a strategy with duplicate strategyName is already installed
 *                       and has an incompatible type
 *  \return a reference to the strategy
 */
template<typename S, typename ...Args>
typename std::enable_if<std::is_base_of<fw::Strategy, S>::value, S&>::type
choose(Forwarder& forwarder, const Name& prefix, Args&&... args)
{
  S& strategy = install<S>(forwarder, std::forward<Args>(args)...);
  forwarder.getStrategyChoice().insert(prefix, strategy.getName());
  return strategy;
}

/** \brief install a strategy to forwarder, and choose the strategy as default
 *  \tparam S strategy type
 *  \param forwarder the forwarder
 *  \throw std::bad_cast a strategy with duplicate strategyName is already installed
 *                       and has an incompatible type
 *  \return a reference to the strategy
 */
template<typename S>
typename std::enable_if<std::is_base_of<fw::Strategy, S>::value, S&>::type
choose(Forwarder& forwarder)
{
  return choose<S>(forwarder, "ndn:/");
}

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FW_INSTALL_STRATEGY_HPP
