/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_INTERNAL_FACE_HPP
#define NFD_MGMT_INTERNAL_FACE_HPP

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
  processInterest(const Interest& interest);

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

#endif //NFD_MGMT_INTERNAL_FACE_HPP
