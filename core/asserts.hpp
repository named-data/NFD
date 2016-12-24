/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_CORE_ASSERTS_HPP
#define NFD_CORE_ASSERTS_HPP

#include "common.hpp"
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <type_traits>

namespace nfd {
namespace detail {

// As of Boost 1.54.0, the internal implementation of BOOST_CONCEPT_ASSERT does not allow
// multiple assertions on the same line, so we have to combine multiple concepts together.

template<typename T>
class StlForwardIteratorConcept : public boost::ForwardIterator<T>
                                , public boost::DefaultConstructible<T>
{
};

} // namespace detail
} // namespace nfd

/** \brief assert T is default constructible
 *  \sa http://en.cppreference.com/w/cpp/concept/DefaultConstructible
 */
#define NFD_ASSERT_DEFAULT_CONSTRUCTIBLE(T) \
  static_assert(std::is_default_constructible<T>::value, \
                #T " must be default-constructible"); \
  BOOST_CONCEPT_ASSERT((boost::DefaultConstructible<T>))

/** \brief assert T is a forward iterator
 *  \sa http://en.cppreference.com/w/cpp/concept/ForwardIterator
 *  \note A forward iterator should be default constructible, but boost::ForwardIterator follows
 *        SGI standard which doesn't require DefaultConstructible, so a separate check is needed.
 */
#define NFD_ASSERT_FORWARD_ITERATOR(T) \
  BOOST_CONCEPT_ASSERT((::nfd::detail::StlForwardIteratorConcept<T>)); \
  static_assert(std::is_default_constructible<T>::value, \
                #T " must be default-constructible")

#endif // NFD_CORE_ASSERTS_HPP
