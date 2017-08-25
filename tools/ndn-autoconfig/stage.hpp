/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_STAGE_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_STAGE_HPP

#include "core/common.hpp"

#include <ndn-cxx/net/face-uri.hpp>
#include <ndn-cxx/util/signal.hpp>

#include <iostream>

namespace ndn {
namespace tools {
namespace autoconfig {

/** \brief a discovery stage
 */
class Stage : boost::noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  virtual ~Stage() = default;

  /** \brief get stage name
   *  \return stage name as a phrase, typically starting with lower case
   */
  virtual const std::string&
  getName() const = 0;

  /** \brief start running this stage
   *  \throw Error stage is already running
   */
  void
  start();

protected:
  /** \brief parse HUB FaceUri from string and declare success
   */
  void
  provideHubFaceUri(const std::string& s);

  void
  succeed(const FaceUri& hubFaceUri);

  void
  fail(const std::string& msg);

private:
  virtual void
  doStart() = 0;

public:
  /** \brief signal when a HUB FaceUri is found
   *
   *  Argument is HUB FaceUri, may not be canonical.
   */
  util::Signal<Stage, FaceUri> onSuccess;

  /** \brief signal when discovery fails
   *
   *  Argument is error message.
   */
  util::Signal<Stage, std::string> onFailure;

private:
  bool m_isInProgress = false;
};

} // namespace autoconfig
} // namespace tools
} // namespace ndn

#endif // NFD_TOOLS_NDN_AUTOCONFIG_STAGE_HPP
