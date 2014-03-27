/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TOOLS_NFDC_HPP
#define NFD_TOOLS_NFDC_HPP

#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/management/controller.hpp>
#include <ndn-cpp-dev/management/nfd-controller.hpp>
#include <vector>

namespace nfdc {

using namespace ndn::nfd;

class Nfdc
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

  explicit
  Nfdc(ndn::Face& face);

  ~Nfdc();

  bool
  dispatch(const std::string& cmd,
           const char* cmdOptions[],
           int nOptions);

  /**
   * \brief Adds a nexthop to a FIB entry.
   *
   * If the FIB entry does not exist, it is inserted automatically
   *
   * cmd format:
   *   name
   *
   * \param cmdOptions Add next hop command parameters without leading 'add-nexthop' component
   */
  void
  fibAddNextHop(const char* cmdOptions[], bool hasCost);

  /**
   * \brief Removes a nexthop from an existing FIB entry
   *
   * If the last nexthop record in a FIB entry is removed, the FIB entry is also deleted
   *
   * cmd format:
   *   name faceId
   *
   * \param cmdOptions Remove next hop command parameters without leading
   *                   'remove-nexthop' component
   */
  void
  fibRemoveNextHop(const char* cmdOptions[]);

  /**
   * \brief create new face
   *
   * This command allows creation of UDP unicast and TCP faces only.
   *
   * cmd format:
   *   uri
   *
   * \param cmdOptions Create face command parameters without leading 'create' component
   */
  void
  faceCreate(const char* cmdOptions[]);

  /**
   * \brief destroy a face
   *
   * cmd format:
   *   faceId
   *
   * \param cmdOptions Destroy face command parameters without leading 'destroy' component
   */
  void
  faceDestroy(const char* cmdOptions[]);

  /**
   * \brief Set the strategy for a namespace
   *
   *
   * cmd format:
   *   name strategy
   *
   * \param cmdOptions Set strategy choice command parameters without leading
   *                   'set-strategy' component
   */
  void
  strategyChoiceSet(const char* cmdOptions[]);

  /**
   * \brief Unset the strategy for a namespace
   *
   *
   * cmd format:
   *   name strategy
   *
   * \param cmdOptions Unset strategy choice command parameters without leading
   *                   'unset-strategy' component
   */
  void
  strategyChoiceUnset(const char* cmdOptions[]);

private:
  void
  onSuccess(const ControlParameters& parameters,
            const std::string& message);

  void
  onError(uint32_t code, const std::string& error, const std::string& message);

public:
  const char* m_programName;

private:
  Controller m_controller;
};

} // namespace nfdc

#endif // NFD_TOOLS_NFDC_HPP
