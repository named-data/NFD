/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_GLOBAL_IO_HPP
#define NFD_CORE_GLOBAL_IO_HPP

#include "common.hpp"

namespace nfd {

/** \return the global io_service instance
 */
boost::asio::io_service&
getGlobalIoService();

#ifdef WITH_TESTS
/** \brief delete the global io_service instance
 *
 *  It will be recreated at the next invocation of getGlobalIoService.
 */
void
resetGlobalIoService();
#endif

} // namespace nfd

#endif // NFD_CORE_GLOBAL_IO_HPP
