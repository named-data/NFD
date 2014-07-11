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

#ifndef NFD_DAEMON_MGMT_INTERNAL_FACE_HPP
#define NFD_DAEMON_MGMT_INTERNAL_FACE_HPP

#include "face/face.hpp"
#include "app-face.hpp"

#include "command-validator.hpp"

namespace nfd {

class InternalFace : public Face, public AppFace
{
public:
  /**
   * \brief InternalFace-related error
   */
  class Error : public Face::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : Face::Error(what)
    {
    }
  };

  InternalFace();

  CommandValidator&
  getValidator();

  virtual
  ~InternalFace();

  // Overridden Face methods for forwarder

  virtual void
  sendInterest(const Interest& interest);

  virtual void
  sendData(const Data& data);

  virtual void
  close();

  // Methods implementing AppFace interface. Do not invoke from forwarder.

  virtual void
  setInterestFilter(const Name& filter,
                    OnInterest onInterest);

  virtual void
  put(const Data& data);

private:
  void
  processInterest(const shared_ptr<const Interest>& interest);

private:
  std::map<Name, OnInterest> m_interestFilters;
  CommandValidator m_validator;
};

inline CommandValidator&
InternalFace::getValidator()
{
  return m_validator;
}


} // namespace nfd

#endif // NFD_DAEMON_MGMT_INTERNAL_FACE_HPP
