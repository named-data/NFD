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
   * \brief Create a new FIB entry if it doesn't exist
   *
   * cmd format:
   *   name
   *
   * @param cmdOptions           add command without leading 'insert' component
   */
  void
  fibInsert(const char* cmdOptions[]);
  /**
   * \brief Delete a FIB entry if it exists
   *
   * cmd format:
   *   name
   *
   * @param cmdOptions          del command without leading 'delete' component
   */
  void
  fibDelete(const char* cmdOptions[]);
  /**
   * \brief Adds a nexthop to an existing FIB entry
   *
   *  If a nexthop of same FaceId exists on the FIB entry, its cost is updated.
   *  FaceId is the FaceId returned in NFD Face Management protocol.
   *  If FaceId is set to zero, it is implied as the face of the entity sending this command.
   * cmd format:
   *   name faceId cost
   *
   * @param cmdOptions          addNextHop command without leading 'add-nexthop' component
   */
  void
  fibAddNextHop(const char* cmdOptions[], bool hasCost);
  /**
   * \brief Remove a nexthop from an existing FIB entry
   *
   *  This command removes a nexthop from a FIB entry. 
   *  Removing the last nexthop in a FIB entry will not automatically delete the FIB entry.
   *
   * cmd format:
   *   name faceId
   *
   * @param cmdOptions          delNext command without leading 'remove-nexthop' component
   */
  void
  fibRemoveNextHop(const char* cmdOptions[]);
  /**
   * \brief Sets a forwarding strategy for a namespace
   *
   *  This command sets a forwarding strategy for a namespace.
   *
   * cmd format:
   *   name strategy
   *
   * @param cmdOptions          setStrategy command without leading 'setStrategy' component
   */
  void
  fibSetStrategy(const char* cmdOptions[]);
  
  /**
   * \brief create new face
   *
   *  This command allows creation of UDP unicast and TCP faces only.
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

private:
  void
  onFibSuccess(const ndn::nfd::FibManagementOptions& fibOptions, const std::string& message);

  void
  onFaceSuccess(const ndn::nfd::FaceManagementOptions& faceOptions, const std::string& message);
  
  void
  onError(const std::string& error, const std::string& message);
  
public:
  const char* m_programName;
};

}// namespace nfdc

#endif // NFD_TOOLS_NFDC_HPP

