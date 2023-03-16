/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NFDC_FACE_HELPERS_HPP
#define NFD_TOOLS_NFDC_FACE_HELPERS_HPP

#include "execute-command.hpp"

#include <ndn-cxx/mgmt/nfd/face-query-filter.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>

namespace nfd::tools::nfdc {

using ndn::nfd::FaceQueryFilter;
using ndn::nfd::FaceStatus;

/**
 * \brief Procedure to find a face.
 */
class FindFace : noncopyable
{
public:
  enum class Code {
    OK = 0, ///< found exactly one face, or found multiple faces when allowMulti is true
    ERROR = 1, ///< unspecified error
    NOT_FOUND = 3, ///< found zero face
    CANONIZE_ERROR = 4, ///< error during FaceUri canonization
    AMBIGUOUS = 5, ///< found multiple faces and allowMulti is false
    NOT_STARTED = -1, ///< for internal use
    IN_PROGRESS = -2, ///< for internal use
  };

  enum class DisambiguationStyle
  {
    LOCAL_URI = 1 ///< print FaceId and LocalUri
  };

  explicit
  FindFace(ExecuteContext& ctx);

  /** \brief Find face by FaceUri.
   *  \pre execute has not been invoked
   */
  Code
  execute(const FaceUri& faceUri, bool allowMulti = false);

  /** \brief Find face by FaceId.
   *  \pre execute has not been invoked
   */
  Code
  execute(uint64_t faceId);

  /** \brief Find face by FaceId or FaceUri.
   *  \param faceIdOrUri either a FaceId (uint64_t) or a FaceUri
   *  \param allowMulti effective only if \p faceIdOrUri contains a FaceUri
   *  \throw std::bad_any_cast faceIdOrUri is neither uint64_t nor FaceUri
   */
  Code
  execute(const std::any& faceIdOrUri, bool allowMulti = false);

  /** \brief Find face by FaceQueryFilter.
   *  \pre execute has not been invoked
   */
  Code
  execute(const FaceQueryFilter& filter, bool allowMulti = false);

  /** \return face status for all results
   */
  const std::vector<FaceStatus>&
  getResults() const
  {
    return m_results;
  }

  /** \return FaceId for all results
   */
  std::set<uint64_t>
  getFaceIds() const;

  /** \return a single face status
   *  \pre getResults().size() == 1
   */
  const FaceStatus&
  getFaceStatus() const;

  uint64_t
  getFaceId() const
  {
    return this->getFaceStatus().getFaceId();
  }

  const std::string&
  getErrorReason() const
  {
    return m_errorReason;
  }

  /** \brief Print results for disambiguation.
   */
  void
  printDisambiguation(std::ostream& os, DisambiguationStyle style) const;

private:
  std::optional<FaceUri>
  canonize(const std::string& fieldName, const FaceUri& uri);

  /** \brief Retrieve FaceStatus from filter.
   *  \post m_res == Code::OK and m_results is populated if retrieval succeeds
   *  \post m_res == Code::ERROR and m_errorReason is set if retrieval fails
   */
  void
  query();

private:
  ExecuteContext& m_ctx;
  FaceQueryFilter m_filter;
  Code m_res = Code::NOT_STARTED;
  std::vector<FaceStatus> m_results;
  std::string m_errorReason;
};

/**
 * \brief Canonize a FaceUri.
 * \return canonical FaceUri (nullopt on failure) and error string
 */
std::pair<std::optional<FaceUri>, std::string>
canonize(ExecuteContext& ctx, const FaceUri& uri);

/**
 * \brief Helper to generate exit code and error message for face canonization failures.
 * \param uri The FaceUri
 * \param error The error string returned by the canonization process
 * \param field An optional field identifier to include with the message
 * \return exit code and error message
 */
std::pair<FindFace::Code, std::string>
canonizeErrorHelper(const FaceUri& uri,
                    const std::string& error,
                    const std::string& field = "");

} // namespace nfd::tools::nfdc

#endif // NFD_TOOLS_NFDC_FACE_HELPERS_HPP
