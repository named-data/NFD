/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_FW_CHOOSE_STRATEGY_HPP
#define NFD_TESTS_DAEMON_FW_CHOOSE_STRATEGY_HPP

#include "fw/forwarder.hpp"
#include "table/strategy-choice.hpp"
#include <boost/lexical_cast.hpp>

namespace nfd {
namespace fw {
class Strategy;
} // namespace fw

namespace tests {

/** \brief choose the strategy for a namespace
 *  \tparam S strategy type, must be a complete type
 *  \param forwarder the forwarder
 *  \param prefix namespace to choose the strategy for
 *  \param instanceName strategy instance name; the strategy must already be registered
 *  \throw std::invalid_argument cannot create strategy by instanceName
 *  \throw std::bad_cast strategy type is incompatible with S
 *  \return a reference to the strategy
 */
template<typename S>
typename std::enable_if<std::is_base_of<fw::Strategy, S>::value, S&>::type
choose(Forwarder& forwarder, const Name& prefix = "/",
       const Name& instanceName = S::getStrategyName())
{
  StrategyChoice& sc = forwarder.getStrategyChoice();
  auto insertRes = sc.insert(prefix, instanceName);
  if (!insertRes) {
    BOOST_THROW_EXCEPTION(std::invalid_argument(boost::lexical_cast<std::string>(insertRes)));
  }
  return dynamic_cast<S&>(sc.findEffectiveStrategy(prefix));
}

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FW_CHOOSE_STRATEGY_HPP
