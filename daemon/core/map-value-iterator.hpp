/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

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
  : public boost::transform_iterator<function<const typename Map::mapped_type&(const typename Map::value_type&)>,
                                     typename Map::const_iterator>
{
public:
  explicit
  MapValueIterator(typename Map::const_iterator it)
    : boost::transform_iterator<function<const typename Map::mapped_type&(const typename Map::value_type&)>,
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

} // namespace nfd

#endif // NFD_CORE_MAP_VALUE_ITERATOR_H
