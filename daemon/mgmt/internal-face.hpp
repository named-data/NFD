/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_INTERNAL_FACE_HPP
#define NFD_MGMT_INTERNAL_FACE_HPP

#include "face/face.hpp"
#include "app-face.hpp"

namespace nfd {

class InternalFace : public Face, public AppFace
{
public:
  /**
   * \brief InternalFace-related error
   */
  struct Error : public Face::Error
  {
    Error(const std::string& what) : Face::Error(what) {}
  };

  InternalFace();

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

  virtual
  ~InternalFace();

private:

  // void
  // onConfig(ConfigFile::Node section, bool isDryRun);

  std::map<Name, OnInterest> m_interestFilters;
};

} // namespace nfd

#endif //NFD_MGMT_INTERNAL_FACE_HPP
