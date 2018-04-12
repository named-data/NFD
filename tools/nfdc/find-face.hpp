/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NFDC_FIND_FACE_HPP
#define NFD_TOOLS_NFDC_FIND_FACE_HPP

#include "execute-command.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

using ndn::nfd::FaceQueryFilter;
using ndn::nfd::FaceStatus;

/** \brief procedure to find a face
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

  /** \brief find face by FaceUri
   *  \pre execute has not been invoked
   */
  Code
  execute(const FaceUri& faceUri, bool allowMulti = false);

  /** \brief find face by FaceId
   *  \pre execute has not been invoked
   */
  Code
  execute(uint64_t faceId);

  /** \brief find face by FaceId or FaceUri
   *  \param faceIdOrUri a boost::any that contains uint64_t or FaceUri
   *  \param allowMulti effective only if \p faceIdOrUri contains a FaceUri
   *  \throw boost::bad_any_cast faceIdOrUri is neither uint64_t nor FaceUri
   */
  Code
  execute(const boost::any& faceIdOrUri, bool allowMulti = false);

  /** \brief find face by FaceQueryFilter
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

  /** \brief print results for disambiguation
   */
  void
  printDisambiguation(std::ostream& os, DisambiguationStyle style) const;

private:
  /** \brief canonize FaceUri
   *  \return canonical FaceUri if canonization succeeds, input if canonization is unsupported
   *  \retval nullopt canonization fails; m_errorReason describes the failure
   */
  optional<FaceUri>
  canonize(const std::string& fieldName, const FaceUri& input);

  /** \brief retrieve FaceStatus from filter
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

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_FIND_FACE_HPP
