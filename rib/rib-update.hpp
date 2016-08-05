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

#ifndef NFD_RIB_RIB_UPDATE_HPP
#define NFD_RIB_RIB_UPDATE_HPP

#include "core/common.hpp"
#include "route.hpp"

namespace nfd {
namespace rib {

/** RibUpdate
 *  \brief represents a route that will be added to or removed from a namespace
 *
 *  \note This type is copyable so that it can be stored in STL containers.
 */
class RibUpdate
{
public:
  enum Action {
    REGISTER     = 0,
    UNREGISTER   = 1,

    /** \brief An update triggered by a face destruction notification
     *
     *  \note indicates a Route needs to be removed after a face is destroyed
     */
    REMOVE_FACE = 2
  };

  RibUpdate();

  RibUpdate&
  setAction(Action action);

  Action
  getAction() const;

  RibUpdate&
  setName(const Name& name);

  const Name&
  getName() const;

  RibUpdate&
  setRoute(const Route& route);

  const Route&
  getRoute() const;

private:
  Action m_action;
  Name m_name;
  Route m_route;
};

inline RibUpdate&
RibUpdate::setAction(Action action)
{
  m_action = action;
  return *this;
}

inline RibUpdate::Action
RibUpdate::getAction() const
{
  return m_action;
}

inline RibUpdate&
RibUpdate::setName(const Name& name)
{
  m_name = name;
  return *this;
}

inline const Name&
RibUpdate::getName() const
{
  return m_name;
}

inline RibUpdate&
RibUpdate::setRoute(const Route& route)
{
  m_route = route;
  return *this;
}

inline const Route&
RibUpdate::getRoute() const
{
  return m_route;
}

std::ostream&
operator<<(std::ostream& os, const RibUpdate::Action action);

std::ostream&
operator<<(std::ostream& os, const RibUpdate& update);

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_UPDATE_HPP
