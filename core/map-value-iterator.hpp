/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#ifndef NFD_CORE_MAP_VALUE_ITERATOR_H
#define NFD_CORE_MAP_VALUE_ITERATOR_H

#include "common.hpp"
#include <boost/iterator/transform_iterator.hpp>

namespace nfd {

/** \class MapValueIterator
 *  \brief ForwardIterator to iterator over map values
 */
template<typename Map>
class MapValueIterator
  : public boost::transform_iterator<
             function<const typename Map::mapped_type&(const typename Map::value_type&)>,
             typename Map::const_iterator>
{
public:
  explicit
  MapValueIterator(typename Map::const_iterator it)
    : boost::transform_iterator<
        function<const typename Map::mapped_type&(const typename Map::value_type&)>,
        typename Map::const_iterator>(it, &takeSecond)
  {
  }

private:
  static const typename Map::mapped_type&
  takeSecond(const typename Map::value_type& pair)
  {
    return pair.second;
  }
};

/** \class MapValueReverseIterator
 *  \brief ReverseIterator to iterator over map values
 */
template<typename Map>
class MapValueReverseIterator
  : public boost::transform_iterator<
             function<const typename Map::mapped_type&(const typename Map::value_type&)>,
             typename Map::const_reverse_iterator>
{
public:
  explicit
  MapValueReverseIterator(typename Map::const_reverse_iterator it)
    : boost::transform_iterator<
             function<const typename Map::mapped_type&(const typename Map::value_type&)>,
             typename Map::const_reverse_iterator>(it, &takeSecond)
  {
  }

private:
  static const typename Map::mapped_type&
  takeSecond(const typename Map::value_type& pair)
  {
    return pair.second;
  }
};

} // namespace nfd

#endif // NFD_CORE_MAP_VALUE_ITERATOR_H
