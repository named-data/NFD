/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#ifndef NFD_CORE_EVENT_EMITTER_HPP
#define NFD_CORE_EVENT_EMITTER_HPP

#include "common.hpp"

namespace nfd {

struct empty
{
};

/** \class EventEmitter
 *  \brief provides a lightweight event system
 *
 *  To declare an event:
 *    EventEmitter<TArgs> onEventName;
 *  To subscribe to an event:
 *    eventSource->onEventName += eventHandler;
 *    Multiple functions can subscribe to the same event.
 *  To trigger an event:
 *    onEventName(args);
 *  To clear event subscriptions:
 *    onEventName.clear();
 */

// four arguments
template<typename T1 = empty, typename T2 = empty,
         typename T3 = empty, typename T4 = empty>
class EventEmitter : noncopyable
{
public:
  /// represents a handler that can subscribe to the event
  typedef function<void(const T1&, const T2&,
                               const T3&, const T4&)> Handler;

  /// adds an subscription
  void
  operator+=(Handler handler);

  /// returns true if there is no subscription,
  /// otherwise returns false
  bool
  isEmpty();

  /// clears all subscriptions
  void
  clear();

  /// triggers the event
  void
  operator()(const T1& a1, const T2& a2, const T3& a3, const T4& a4);

private:
  /// stores all subscribed handlers
  std::vector<Handler> m_handlers;
};

// zero argument
template<>
class EventEmitter<empty, empty, empty, empty> : noncopyable
{
public:
  typedef function<void()> Handler;

  void
  operator+=(Handler handler);

  bool
  isEmpty();

  void
  clear();

  void
  operator()();

private:
  std::vector<Handler> m_handlers;
};


// one argument
template<typename T1>
class EventEmitter<T1, empty, empty, empty> : noncopyable
{
public:
  typedef function<void(const T1&)> Handler;

  void
  operator+=(Handler handler);

  bool
  isEmpty();

  void
  clear();

  void
  operator()(const T1& a1);

private:
  std::vector<Handler> m_handlers;
};


// two arguments
template<typename T1, typename T2>
class EventEmitter<T1, T2, empty, empty> : noncopyable
{
public:
  typedef function<void(const T1&, const T2&)> Handler;

  void
  operator+=(Handler handler);

  bool
  isEmpty();

  void
  clear();

  void
  operator()(const T1& a1, const T2& a2);

private:
  std::vector<Handler> m_handlers;
};


// three arguments
template<typename T1, typename T2, typename T3>
class EventEmitter<T1, T2, T3, empty> : noncopyable
{
public:
  typedef function<void(const T1&, const T2&, const T3&)> Handler;

  void
  operator+=(Handler handler);

  bool
  isEmpty();

  void
  clear();

  void
  operator()(const T1& a1, const T2& a2, const T3& a3);

private:
  std::vector<Handler> m_handlers;
};


// zero argument

inline void
EventEmitter<empty, empty, empty, empty>::operator+=(Handler handler)
{
  m_handlers.push_back(handler);
}

inline bool
EventEmitter<empty, empty, empty, empty>::isEmpty()
{
  return m_handlers.empty();
}

inline void
EventEmitter<empty, empty, empty, empty>::clear()
{
  return m_handlers.clear();
}

inline void
EventEmitter<empty, empty, empty, empty>::operator()()
{
  std::vector<Handler>::iterator it;
  for (it = m_handlers.begin(); it != m_handlers.end(); ++it) {
    (*it)();
    if (m_handlers.empty()) // .clear has been called
      return;
  }
}

// one argument

template<typename T1>
inline void
EventEmitter<T1, empty, empty, empty>::operator+=(Handler handler)
{
  m_handlers.push_back(handler);
}

template<typename T1>
inline bool
EventEmitter<T1, empty, empty, empty>::isEmpty()
{
  return m_handlers.empty();
}

template<typename T1>
inline void
EventEmitter<T1, empty, empty, empty>::clear()
{
  return m_handlers.clear();
}

template<typename T1>
inline void
EventEmitter<T1, empty, empty, empty>::operator()(const T1& a1)
{
  typename std::vector<Handler>::iterator it;
  for (it = m_handlers.begin(); it != m_handlers.end(); ++it) {
    (*it)(a1);
    if (m_handlers.empty()) // .clear has been called
      return;
  }
}

// two arguments

template<typename T1, typename T2>
inline void
EventEmitter<T1, T2, empty, empty>::operator+=(Handler handler)
{
  m_handlers.push_back(handler);
}

template<typename T1, typename T2>
inline bool
EventEmitter<T1, T2, empty, empty>::isEmpty()
{
  return m_handlers.empty();
}

template<typename T1, typename T2>
inline void
EventEmitter<T1, T2, empty, empty>::clear()
{
  return m_handlers.clear();
}

template<typename T1, typename T2>
inline void
EventEmitter<T1, T2, empty, empty>::operator()
    (const T1& a1, const T2& a2)
{
  typename std::vector<Handler>::iterator it;
  for (it = m_handlers.begin(); it != m_handlers.end(); ++it) {
    (*it)(a1, a2);
    if (m_handlers.empty()) // .clear has been called
      return;
  }
}

// three arguments

template<typename T1, typename T2, typename T3>
inline void
EventEmitter<T1, T2, T3, empty>::operator+=(Handler handler)
{
  m_handlers.push_back(handler);
}

template<typename T1, typename T2, typename T3>
inline bool
EventEmitter<T1, T2, T3, empty>::isEmpty()
{
  return m_handlers.empty();
}

template<typename T1, typename T2, typename T3>
inline void
EventEmitter<T1, T2, T3, empty>::clear()
{
  return m_handlers.clear();
}

template<typename T1, typename T2, typename T3>
inline void
EventEmitter<T1, T2, T3, empty>::operator()
    (const T1& a1, const T2& a2, const T3& a3)
{
  typename std::vector<Handler>::iterator it;
  for (it = m_handlers.begin(); it != m_handlers.end(); ++it) {
    (*it)(a1, a2, a3);
    if (m_handlers.empty()) // .clear has been called
      return;
  }
}

// four arguments

template<typename T1, typename T2, typename T3, typename T4>
inline void
EventEmitter<T1, T2, T3, T4>::operator+=(Handler handler)
{
  m_handlers.push_back(handler);
}

template<typename T1, typename T2, typename T3, typename T4>
inline bool
EventEmitter<T1, T2, T3, T4>::isEmpty()
{
  return m_handlers.empty();
}

template<typename T1, typename T2, typename T3, typename T4>
inline void
EventEmitter<T1, T2, T3, T4>::clear()
{
  return m_handlers.clear();
}

template<typename T1, typename T2, typename T3, typename T4>
inline void
EventEmitter<T1, T2, T3, T4>::operator()
    (const T1& a1, const T2& a2, const T3& a3, const T4& a4)
{
  typename std::vector<Handler>::iterator it;
  for (it = m_handlers.begin(); it != m_handlers.end(); ++it) {
    (*it)(a1, a2, a3, a4);
    if (m_handlers.empty()) // .clear has been called
      return;
  }
}


} // namespace nfd

#endif // NFD_CORE_EVENT_EMITTER_HPP
