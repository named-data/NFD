/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NFDC_CANONIZER_HPP
#define NFD_TOOLS_NFDC_CANONIZER_HPP

#include "core/common.hpp"
#include "execute-command.hpp"
#include "find-face.hpp"

#include <ndn-cxx/net/face-uri.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

/** \brief canonize FaceUri
 *  \return pair of canonical FaceUri (nullopt if failure) and error string
 */
std::pair<optional<FaceUri>, std::string>
canonize(ExecuteContext& ctx, const FaceUri& uri);

/** \brief helper to generate exit code and error message for face canonization failures
 *  \param uri FaceUri
 *  \param error error string returned by canonization process
 *  \param field optional field identifier to include with message
 *  \return pair of exit code and error message
 */
std::pair<FindFace::Code, std::string>
canonizeErrorHelper(const FaceUri& uri,
                    const std::string& error,
                    const std::string& field = "");

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_CANONIZER_HPP
