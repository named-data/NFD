/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "privilege-helper.hpp"
#include "core/logger.hpp"

#include <pwd.h>
#include <grp.h>

namespace nfd {

NFD_LOG_INIT("PrivilegeHelper");

#ifdef HAVE_PRIVILEGE_DROP_AND_ELEVATE
uid_t PrivilegeHelper::s_normalUid = ::geteuid();
gid_t PrivilegeHelper::s_normalGid = ::getegid();

uid_t PrivilegeHelper::s_privilegedUid = ::geteuid();
gid_t PrivilegeHelper::s_privilegedGid = ::getegid();
#endif // HAVE_PRIVILEGE_DROP_AND_ELEVATE

void
PrivilegeHelper::initialize(const std::string& userName, const std::string& groupName)
{
#ifdef HAVE_PRIVILEGE_DROP_AND_ELEVATE
  static const size_t MAX_GROUP_BUFFER_SIZE = 16384; // 16kB
  static const size_t MAX_PASSWD_BUFFER_SIZE = 16384;

  static const size_t FALLBACK_GROUP_BUFFER_SIZE = 1024;
  static const size_t FALLBACK_PASSWD_BUFFER_SIZE = 1024;

  NFD_LOG_TRACE("initializing privilege helper with user \"" << userName << "\""
                << " group \"" << groupName << "\"");

  // workflow from man getpwnam_r

  if (!groupName.empty())
    {
      static int groupSize = ::sysconf(_SC_GETGR_R_SIZE_MAX);

      if (groupSize == -1)
        {
          groupSize = FALLBACK_GROUP_BUFFER_SIZE;
        }

      std::vector<char> groupBuffer(groupSize);
      struct group group;
      struct group* groupResult = 0;

      int errorCode = getgrnam_r(groupName.c_str(), &group,
                                 &groupBuffer[0], groupSize, &groupResult);

      while (errorCode == ERANGE)
        {
          if (groupBuffer.size() * 2 > MAX_GROUP_BUFFER_SIZE)
            {
              throw PrivilegeHelper::Error("Cannot allocate large enough buffer for struct group");
            }

          groupBuffer.resize(groupBuffer.size() * 2);

          errorCode = getgrnam_r(groupName.c_str(), &group,
                                 &groupBuffer[0], groupBuffer.size(), &groupResult);
        }

      if (errorCode != 0 || !groupResult)
        {
          throw PrivilegeHelper::Error("Failed to get gid for \"" + groupName + "\"");
        }

      s_normalGid = group.gr_gid;
    }

  if (!userName.empty())
    {
      static int passwdSize = ::sysconf(_SC_GETPW_R_SIZE_MAX);

      if (passwdSize == -1)
        {
          passwdSize = FALLBACK_PASSWD_BUFFER_SIZE;
        }

      std::vector<char> passwdBuffer(passwdSize);
      struct passwd passwd;
      struct passwd* passwdResult = 0;

      int errorCode =
        getpwnam_r(userName.c_str(), &passwd,
                   &passwdBuffer[0], passwdBuffer.size(), &passwdResult);

      while (errorCode == ERANGE)
        {
          if (passwdBuffer.size() * 2 > MAX_PASSWD_BUFFER_SIZE)
            {
              throw PrivilegeHelper::Error("Cannot allocate large enough buffer for struct passwd");
            }

          passwdBuffer.resize(passwdBuffer.size() * 2);

          errorCode = getpwnam_r(userName.c_str(), &passwd,
                                 &passwdBuffer[0], passwdBuffer.size(), &passwdResult);
        }

      if (errorCode != 0 || !passwdResult)
        {
          throw PrivilegeHelper::Error("Failed to get uid for \"" + userName + "\"");
        }

      s_normalUid = passwd.pw_uid;
    }
#else
  if (!userName.empty() || !groupName.empty()) {
    throw Error("Dropping and raising privileges is not supported on this platform");
  }
#endif // HAVE_PRIVILEGE_DROP_AND_ELEVATE
}

void
PrivilegeHelper::drop()
{
#ifdef HAVE_PRIVILEGE_DROP_AND_ELEVATE
  NFD_LOG_TRACE("dropping to effective gid=" << s_normalGid);
  if (::setegid(s_normalGid) != 0)
    {
      std::stringstream error;
      error << "Failed to drop to effective gid=" << s_normalGid;

      throw PrivilegeHelper::Error(error.str());
    }

  NFD_LOG_TRACE("dropping to effective uid=" << s_normalUid);
  if (::seteuid(s_normalUid) != 0)
    {
      std::stringstream error;
      error << "Failed to drop to effective uid=" << s_normalUid;

      throw PrivilegeHelper::Error(error.str());
    }

  NFD_LOG_INFO("dropped to effective uid=" << ::geteuid() << " gid=" << ::getegid());
#else
  NFD_LOG_WARN("Dropping privileges is not supported on this platform");
#endif // HAVE_PRIVILEGE_DROP_AND_ELEVATE
}

void
PrivilegeHelper::raise()
{
#ifdef HAVE_PRIVILEGE_DROP_AND_ELEVATE
  NFD_LOG_TRACE("elevating to effective uid=" << s_privilegedUid);
  if (::seteuid(s_privilegedUid) != 0)
    {
      std::stringstream error;
      error << "Failed to elevate to effective uid=" << s_privilegedUid;

      throw PrivilegeHelper::Error(error.str());
    }

  NFD_LOG_TRACE("elevating to effective gid=" << s_privilegedGid);
  if (::setegid(s_privilegedGid) != 0)
    {
      std::stringstream error;
      error << "Failed to elevate to effective gid=" << s_privilegedGid;

      throw PrivilegeHelper::Error(error.str());
    }
  NFD_LOG_INFO("elevated to effective uid=" << ::geteuid() << " gid=" << ::getegid());
#else
  NFD_LOG_WARN("Elevating privileges is not supported on this platform");
#endif // HAVE_PRIVILEGE_DROP_AND_ELEVATE
}

void
PrivilegeHelper::runElevated(function<void()> f)
{
  raise();

  try
    {
      f();
    }
  catch (...)
    {
      drop();
      throw;
    }
  drop();
}

} // namespace nfd
