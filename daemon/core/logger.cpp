/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#include "logger.hpp"

namespace nfd
{

Logger::Logger(const std::string& name)
  : m_moduleName(name),
    m_isEnabled(true)
{
}

std::ostream&
operator<<(std::ostream& output, const Logger& obj)
{
  output << obj.getName();
  return output;
}

} // namespace nfd
