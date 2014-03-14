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
#include <ndn-cpp-dev/management/nfd-fib-management-options.hpp>
#include <ndn-cpp-dev/management/nfd-face-management-options.hpp>
#include <ndn-cpp-dev/management/nfd-strategy-choice-options.hpp>
#include <vector>

namespace nfdc {
    
class Controller : public ndn::nfd::Controller
{
public:
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };
    
  explicit
  Controller(ndn::Face& face);
  
  ~Controller();
  
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
   * @param cmdOptions           add command without leading 'insert' component
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
   * @param cmdOptions          delNext command without leading 'remove-nexthop' component
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
   * @param cmdOptions          create command without leading 'create' component
   */
  void
  faceCreate(const char* cmdOptions[]);
  /**
   * \brief destroy a face
   *
   * cmd format:
   *   faceId
   *
   * @param cmdOptions          destroy command without leading 'destroy' component
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
   * @param cmdOptions          Set command without leading 'Unset' component
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
   * @param cmdOptions          Unset command without leading 'Unset' component
   */
  void
  strategyChoiceUnset(const char* cmdOptions[]);
  
private:
  void
  onFibSuccess(const ndn::nfd::FibManagementOptions& fibOptions,
               const std::string& message);

  void
  onFaceSuccess(const ndn::nfd::FaceManagementOptions& faceOptions,
                const std::string& message);
  
  void
  onSetStrategySuccess(const ndn::nfd::StrategyChoiceOptions& resp,
                       const std::string& message);
  void
  onError(const std::string& error, const std::string& message);
  
public:
  const char* m_programName;
};

} // namespace nfdc

#endif // NFD_TOOLS_NFDC_HPP

