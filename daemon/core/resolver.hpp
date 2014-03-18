/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_RESOLVER_H
#define NFD_CORE_RESOLVER_H

#include "common.hpp"
#include "core/global-io.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace resolver {

typedef function<bool (const boost::asio::ip::address& address)> AddressSelector;

struct AnyAddress {
  bool
  operator()(const boost::asio::ip::address& address)
  {
    return true;
  }
};

struct Ipv4Address {
  bool
  operator()(const boost::asio::ip::address& address)
  {
    return address.is_v4();
  }
};

struct Ipv6Address {
  bool
  operator()(const boost::asio::ip::address& address)
  {
    return address.is_v6();
  }
};

} // namespace resolver

template<class Protocol>
class Resolver
{
public:
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };

  typedef function<void (const typename Protocol::endpoint& endpoint)> SuccessCallback;
  typedef function<void (const std::string& reason)> ErrorCallback;

  typedef boost::asio::ip::basic_resolver< Protocol > resolver;

  /** \brief Asynchronously resolve host and port
   *
   * If an address selector predicate is specified, then each resolved IP address
   * is checked against the predicate.
   *
   * Available address selector predicates:
   *
   * - resolver::AnyAddress()
   * - resolver::Ipv4Address()
   * - resolver::Ipv6Address()
   */
  static void
  asyncResolve(const std::string& host, const std::string& port,
               const SuccessCallback& onSuccess,
               const ErrorCallback& onError,
               const nfd::resolver::AddressSelector& addressSelector = nfd::resolver::AnyAddress(),
               const time::seconds& timeout = time::seconds(4))
  {
    shared_ptr<Resolver> resolver =
      shared_ptr<Resolver>(new Resolver(onSuccess, onError,
                                        addressSelector));

    resolver->asyncResolve(host, port, timeout, resolver);
    // resolver will be destroyed when async operation finishes or global IO service stops
  }

  /** \brief Synchronously resolve host and port
   *
   * If an address selector predicate is specified, then each resolved IP address
   * is checked against the predicate.
   *
   * Available address selector predicates:
   *
   * - resolver::AnyAddress()
   * - resolver::Ipv4Address()
   * - resolver::Ipv6Address()
   */
  static typename Protocol::endpoint
  syncResolve(const std::string& host, const std::string& port,
              const nfd::resolver::AddressSelector& addressSelector = nfd::resolver::AnyAddress())
  {
    Resolver resolver(SuccessCallback(), ErrorCallback(), addressSelector);

    typename resolver::query query(host, port,
                                   resolver::query::all_matching);

    typename resolver::iterator remoteEndpoint = resolver.m_resolver.resolve(query);
    typename resolver::iterator end;
    for (; remoteEndpoint != end; ++remoteEndpoint)
      {
        if (addressSelector(typename Protocol::endpoint(*remoteEndpoint).address()))
          return *remoteEndpoint;
      }
    throw Error("No endpoint matching the specified address selector found");
  }

private:
  Resolver(const SuccessCallback& onSuccess,
           const ErrorCallback& onError,
           const nfd::resolver::AddressSelector& addressSelector)
    : m_resolver(getGlobalIoService())
    , m_addressSelector(addressSelector)
    , m_onSuccess(onSuccess)
    , m_onError(onError)
  {
  }

  void
  asyncResolve(const std::string& host, const std::string& port,
               const time::seconds& timeout,
               const shared_ptr<Resolver>& self)
  {
    typename resolver::query query(host, port,
                                   resolver::query::all_matching);

    m_resolver.async_resolve(query,
                             bind(&Resolver::onResolveSuccess, this, _1, _2, self));

    m_resolveTimeout = scheduler::schedule(timeout,
                                           bind(&Resolver::onResolveError, this,
                                                "Timeout", self));
  }

  void
  onResolveSuccess(const boost::system::error_code& error,
                   typename resolver::iterator remoteEndpoint,
                   const shared_ptr<Resolver>& self)
  {
    scheduler::cancel(m_resolveTimeout);

    if (error)
      {
        if (error == boost::system::errc::operation_canceled)
          return;

        return m_onError("Remote endpoint hostname or port cannot be resolved: " +
                         error.category().message(error.value()));
      }

    typename resolver::iterator end;
    for (; remoteEndpoint != end; ++remoteEndpoint)
      {
        if (m_addressSelector(typename Protocol::endpoint(*remoteEndpoint).address()))
          return m_onSuccess(*remoteEndpoint);
      }

    m_onError("No endpoint matching the specified address selector found");
  }

  void
  onResolveError(const std::string& errorInfo,
                 const shared_ptr<Resolver>& self)
  {
    m_resolver.cancel();
    m_onError(errorInfo);
  }

private:
  resolver m_resolver;
  EventId m_resolveTimeout;

  nfd::resolver::AddressSelector m_addressSelector;
  SuccessCallback m_onSuccess;
  ErrorCallback m_onError;
};

typedef Resolver<boost::asio::ip::tcp> TcpResolver;
typedef Resolver<boost::asio::ip::udp> UdpResolver;

} // namespace nfd

#endif // NFD_CORE_RESOLVER_H
