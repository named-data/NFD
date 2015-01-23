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

#ifndef NFD_CORE_ALGORITHM_HPP
#define NFD_CORE_ALGORITHM_HPP

#include "common.hpp"
#include <boost/concept/requires.hpp>

namespace nfd {

/** \brief finds the last element satisfying a predicate
 *  \tparam It BidirectionalIterator
 *  \tparam Pred UnaryPredicate
 *
 *  \return Iterator to the last element satisfying the condition,
 *          or \p last if no such element is found.
 *
 *  Complexity: at most \p last-first invocations of \p p
 */
template<typename It, typename Pred>
BOOST_CONCEPT_REQUIRES(
  ((boost::BidirectionalIterator<It>))
  ((boost::UnaryPredicate<Pred, typename std::iterator_traits<It>::value_type>)),
  (It)
)
find_last_if(It first, It last, Pred p)
{
  std::reverse_iterator<It> firstR(first), lastR(last);
  auto found = std::find_if(lastR, firstR, p);
  return found == firstR ? last : std::prev(found.base());
}

} // namespace nfd

#endif // NFD_CORE_ALGORITHM_HPP
