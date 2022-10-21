/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_STRATEGY_HPP
#define NFD_DAEMON_FW_STRATEGY_HPP

#include "forwarder.hpp"
#include "table/measurements-accessor.hpp"

#include <boost/lexical_cast/try_lexical_convert.hpp>

namespace nfd::fw {

class StrategyParameters;

/**
 * \brief Base class of all forwarding strategies.
 */
class Strategy : noncopyable
{
public: // registry
  /**
   * \brief Register a strategy type.
   * \tparam S subclass of Strategy
   * \param strategyName strategy program name, must contain version
   * \note It is permitted to register the same strategy type under multiple names,
   *       which is useful in tests and for creating aliases.
   */
  template<typename S>
  static void
  registerType(const Name& strategyName = S::getStrategyName())
  {
    BOOST_ASSERT(strategyName.size() > 1);
    BOOST_ASSERT(strategyName.at(-1).isVersion());
    auto r = getRegistry().insert_or_assign(strategyName, [] (auto&&... args) {
      return make_unique<S>(std::forward<decltype(args)>(args)...);
    });
    BOOST_VERIFY(r.second);
  }

  /**
   * \brief Returns whether a strategy instance can be created from \p instanceName.
   * \param instanceName strategy instance name, may contain version and parameters
   * \note This function finds a strategy type using the same rules as create(),
   *       but does not attempt to construct an instance.
   */
  static bool
  canCreate(const Name& instanceName);

  /**
   * \brief Returns a strategy instance created from \p instanceName.
   * \retval nullptr if `canCreate(instanceName) == false`
   * \throw std::invalid_argument strategy type constructor does not accept the
   *                              specified version or parameters
   */
  static unique_ptr<Strategy>
  create(const Name& instanceName, Forwarder& forwarder);

  /**
   * \brief Returns whether two names will instantiate the same strategy type.
   */
  static bool
  areSameType(const Name& instanceNameA, const Name& instanceNameB);

  /**
   * \brief Returns all registered versioned strategy names.
   */
  [[nodiscard]] static std::set<Name>
  listRegistered();

public: // constructor, destructor, strategy info
  /** \brief Construct a strategy instance.
   *  \param forwarder a reference to the forwarder, used to enable actions and accessors.
   *  \note Strategy subclass constructor must not retain a reference to \p forwarder.
   */
  explicit
  Strategy(Forwarder& forwarder);

  virtual
  ~Strategy();

#ifdef DOXYGEN
  /**
   * \brief Returns the strategy's program name.
   *
   * The strategy name is defined by the strategy program.
   * It must end with a version component.
   */
  static const Name&
  getStrategyName();
#endif

  /**
   * \brief Returns the strategy's instance name.
   *
   * The instance name is assigned during instantiation.
   * It contains a version component and may have extra parameter components.
   */
  const Name&
  getInstanceName() const noexcept
  {
    return m_name;
  }

public: // triggers
  /**
   * \brief Trigger after an Interest is received.
   *
   * The Interest:
   *  - has not exceeded HopLimit
   *  - does not violate Scope
   *  - is not looped
   *  - cannot be satisfied by ContentStore
   *  - is under a namespace managed by this strategy
   *
   * The PIT entry is set to expire after InterestLifetime has elapsed at each downstream.
   *
   * The strategy should decide whether and where to forward this Interest.
   *  - If the strategy decides to forward this Interest,
   *    invoke sendInterest() for each upstream, either now or shortly after via a scheduler event,
   *    but before the PIT entry expires.
   *    Optionally, the strategy can invoke setExpiryTimer() to adjust how long it would wait for a response.
   *  - If the strategy has already forwarded this Interest previously and decides to continue
   *    waiting, do nothing.
   *    Optionally, the strategy can invoke setExpiryTimer() to adjust how long it would wait for a response.
   *  - If the strategy concludes that this Interest cannot be satisfied,
   *    invoke rejectPendingInterest() to erase the PIT entry.
   *
   * \warning The strategy must not retain a copy of the \p pitEntry shared_ptr after this function
   *          returns, otherwise undefined behavior may occur. However, the strategy is allowed to
   *          construct and keep a weak_ptr to \p pitEntry.
   */
  virtual void
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) = 0;

  /**
   * \brief Trigger after a matching Data is found in the Content Store.
   *
   * In the base class, this method sends \p data to \p ingress.
   *
   * \warning The strategy must not retain a copy of the \p pitEntry shared_ptr after this function
   *          returns, otherwise undefined behavior may occur. However, the strategy is allowed to
   *          construct and keep a weak_ptr to \p pitEntry.
   */
  virtual void
  afterContentStoreHit(const Data& data, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry);

  /**
   * \brief Trigger before a PIT entry is satisfied.
   *
   * This trigger is invoked when an incoming Data satisfies more than one PIT entry.
   * The strategy can collect measurements information, but cannot manipulate Data forwarding.
   * When an incoming Data satisfies only one PIT entry, afterReceiveData() is invoked instead
   * and given full control over Data forwarding. If a strategy does not override afterReceiveData(),
   * the default implementation invokes beforeSatisfyInterest().
   *
   * Normally, PIT entries are erased after receiving the first matching Data.
   * If the strategy wishes to collect responses from additional upstream nodes,
   * it should invoke setExpiryTimer() within this function to prolong the PIT entry lifetime.
   * If a Data arrives from another upstream during the extended PIT entry lifetime, this trigger
   * will be invoked again. At that time, the strategy must invoke setExpiryTimer() again to
   * continue collecting more responses.
   *
   * In the base class, this method does nothing.
   *
   * \warning The strategy must not retain a copy of the \p pitEntry shared_ptr after this function
   *          returns, otherwise undefined behavior may occur. However, the strategy is allowed to
   *          construct and keep a weak_ptr to \p pitEntry.
   */
  virtual void
  beforeSatisfyInterest(const Data& data, const FaceEndpoint& ingress,
                        const shared_ptr<pit::Entry>& pitEntry);

  /**
   * \brief Trigger after Data is received.
   *
   * This trigger is invoked when an incoming Data satisfies exactly one PIT entry,
   * and gives the strategy full control over Data forwarding.
   *
   * When this trigger is invoked:
   *  - The Data has been verified to satisfy the PIT entry.
   *  - The PIT entry expiry timer is set to now
   *
   * Inside this function:
   *  - A strategy should return Data to downstream nodes via sendData() or sendDataToAll().
   *  - A strategy can modify the Data as long as it still satisfies the PIT entry, such as
   *    adding or removing congestion marks.
   *  - A strategy can delay Data forwarding by prolonging the PIT entry lifetime via setExpiryTimer(),
   *    and later forward the Data before the PIT entry is erased.
   *  - A strategy can collect measurements about the upstream.
   *  - A strategy can collect responses from additional upstream nodes by prolonging the PIT entry
   *    lifetime via setExpiryTimer() every time a Data is received. Note that only one Data should
   *    be returned to each downstream node.
   *
   * In the base class, this method invokes beforeSatisfyInterest() and then returns the Data
   * to all downstream faces via sendDataToAll().
   *
   * \warning The strategy must not retain a copy of the \p pitEntry shared_ptr after this function
   *          returns, otherwise undefined behavior may occur. However, the strategy is allowed to
   *          construct and keep a weak_ptr to \p pitEntry.
   */
  virtual void
  afterReceiveData(const Data& data, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry);

  /**
   * \brief Trigger after a Nack is received.
   *
   * This trigger is invoked when an incoming Nack is received in response to
   * an forwarded Interest.
   * The Nack has been confirmed to be a response to the last Interest forwarded
   * to that upstream, i.e. the PIT out-record exists and has a matching Nonce.
   * The NackHeader has been recorded in the PIT out-record.
   *
   * If the PIT entry is not yet satisfied, its expiry timer remains unchanged.
   * Otherwise, the PIT entry will normally expire immediately after this function returns.
   *
   * If the strategy wishes to collect responses from additional upstream nodes,
   * it should invoke setExpiryTimer() within this function to prolong the PIT entry lifetime.
   * If a Nack arrives from another upstream during the extended PIT entry lifetime, this trigger
   * will be invoked again. At that time, the strategy must invoke setExpiryTimer() again to
   * continue collecting more responses.
   *
   * In the base class, this method does nothing.
   *
   * \warning The strategy must not retain a copy of the \p pitEntry shared_ptr after this function
   *          returns, otherwise undefined behavior may occur. However, the strategy is allowed to
   *          construct and keep a weak_ptr to \p pitEntry.
   */
  virtual void
  afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry);

  /**
   * \brief Trigger after an Interest is dropped (e.g., for exceeding allowed retransmissions).
   *
   * In the base class, this method does nothing.
   */
  virtual void
  onDroppedInterest(const Interest& interest, Face& egress);

  /**
   * \brief Trigger after a new nexthop is added.
   *
   * The strategy should decide whether to send the buffered Interests to the new nexthop.
   *
   * In the base class, this method does nothing.
   */
  virtual void
  afterNewNextHop(const fib::NextHop& nextHop, const shared_ptr<pit::Entry>& pitEntry);

protected: // actions
  /**
   * \brief Send an Interest packet.
   * \param interest the Interest packet
   * \param egress face through which to send out the Interest
   * \param pitEntry the PIT entry
   * \return A pointer to the out-record created or nullptr if the Interest was dropped
   */
  NFD_VIRTUAL_WITH_TESTS pit::OutRecord*
  sendInterest(const Interest& interest, Face& egress, const shared_ptr<pit::Entry>& pitEntry);

  /**
   * \brief Send a Data packet.
   * \param data the Data packet
   * \param egress face through which to send out the Data
   * \param pitEntry the PIT entry
   * \return Whether the Data was sent (true) or dropped (false)
   */
  NFD_VIRTUAL_WITH_TESTS bool
  sendData(const Data& data, Face& egress, const shared_ptr<pit::Entry>& pitEntry);

  /**
   * \brief Send a Data packet to all matched and qualified faces.
   *
   * A matched face qualifies if it is ad-hoc OR it is NOT \p inFace.
   *
   * \param data the Data packet
   * \param pitEntry the PIT entry
   * \param inFace face on which the Data arrived
   */
  NFD_VIRTUAL_WITH_TESTS void
  sendDataToAll(const Data& data, const shared_ptr<pit::Entry>& pitEntry, const Face& inFace);

  /**
   * \brief Schedule the PIT entry for immediate deletion.
   *
   * This helper function sets the PIT entry expiry time to zero.
   * The strategy should invoke this function when it concludes that the Interest cannot
   * be forwarded and it does not want to wait for responses from existing upstream nodes.
   */
  NFD_VIRTUAL_WITH_TESTS void
  rejectPendingInterest(const shared_ptr<pit::Entry>& pitEntry)
  {
    this->setExpiryTimer(pitEntry, 0_ms);
  }

  /**
   * \brief Send a Nack packet.
   *
   * The egress face must have a PIT in-record, otherwise this method has no effect.
   *
   * \param header the Nack header
   * \param egress face through which to send out the Nack
   * \param pitEntry the PIT entry
   * \return Whether the Nack was sent (true) or dropped (false)
   */
  NFD_VIRTUAL_WITH_TESTS bool
  sendNack(const lp::NackHeader& header, Face& egress, const shared_ptr<pit::Entry>& pitEntry)
  {
    return m_forwarder.onOutgoingNack(header, egress, pitEntry);
  }

  /**
   * \brief Send Nack to every face that has an in-record, except those in \p exceptFaces
   * \param header the Nack header
   * \param pitEntry the PIT entry
   * \param exceptFaces list of faces that should be excluded from sending Nacks
   * \note This is not an action, but a helper that invokes the sendNack() action.
   */
  void
  sendNacks(const lp::NackHeader& header, const shared_ptr<pit::Entry>& pitEntry,
            std::initializer_list<const Face*> exceptFaces = {});

  /**
   * \brief Schedule the PIT entry to be erased after \p duration.
   */
  void
  setExpiryTimer(const shared_ptr<pit::Entry>& pitEntry, time::milliseconds duration)
  {
    m_forwarder.setExpiryTimer(pitEntry, duration);
  }

protected: // accessors
  /**
   * \brief Performs a FIB lookup, considering Link object if present.
   */
  const fib::Entry&
  lookupFib(const pit::Entry& pitEntry) const;

  MeasurementsAccessor&
  getMeasurements() noexcept
  {
    return m_measurements;
  }

  Face*
  getFace(FaceId id) const noexcept
  {
    return getFaceTable().get(id);
  }

  const FaceTable&
  getFaceTable() const noexcept
  {
    return m_forwarder.m_faceTable;
  }

protected: // instance name
  struct ParsedInstanceName
  {
    Name strategyName; ///< Strategy name without parameters
    std::optional<uint64_t> version; ///< The strategy version number, if present
    PartialName parameters; ///< Parameter components, may be empty
  };

  /** \brief Parse a strategy instance name
   *  \param input strategy instance name, may contain version and parameters
   *  \throw std::invalid_argument input format is unacceptable
   */
  static ParsedInstanceName
  parseInstanceName(const Name& input);

  /** \brief Construct a strategy instance name
   *  \param input strategy instance name, may contain version and parameters
   *  \param strategyName strategy name with version but without parameters;
   *                      typically this should be \p getStrategyName()
   *
   *  If \p input contains a version component, return \p input unchanged.
   *  Otherwise, return \p input plus the version component taken from \p strategyName.
   *  This allows a strategy instance to be constructed with an unversioned name,
   *  but its final instance name should contain the version.
   */
  static Name
  makeInstanceName(const Name& input, const Name& strategyName);

  /** \brief Set strategy instance name
   *  \note This must be called by strategy subclass constructor.
   */
  void
  setInstanceName(const Name& name) noexcept
  {
    m_name = name;
  }

NFD_PUBLIC_WITH_TESTS_ELSE_PROTECTED:
  /**
   * \brief Parse strategy parameters encoded in a strategy instance name
   * \param params encoded parameters, typically obtained from a call to parseInstanceName()
   * \throw std::invalid_argument the encoding format is invalid or unsupported by this implementation
   */
  static StrategyParameters
  parseParameters(const PartialName& params);

private: // registry
  using CreateFunc = std::function<unique_ptr<Strategy>(Forwarder&, const Name& /*strategyName*/)>;
  using Registry = std::map<Name, CreateFunc>; // indexed by strategy name

  static Registry&
  getRegistry();

  static Registry::const_iterator
  find(const Name& instanceName);

protected: // accessors
  signal::Signal<FaceTable, Face>& afterAddFace;
  signal::Signal<FaceTable, Face>& beforeRemoveFace;

private: // instance fields
  Name m_name;
  Forwarder& m_forwarder;
  MeasurementsAccessor m_measurements;
};

class StrategyParameters : public std::map<std::string, std::string>
{
public:
  // Note: only arithmetic types are supported by getOrDefault() for now

  template<typename T>
  std::enable_if_t<std::is_signed_v<T>, T>
  getOrDefault(const key_type& key, const T& defaultVal) const
  {
    auto it = find(key);
    if (it == end()) {
      return defaultVal;
    }

    T val{};
    if (!boost::conversion::try_lexical_convert(it->second, val)) {
      NDN_THROW(std::invalid_argument(key + " value is malformed"));
    }
    return val;
  }

  template<typename T>
  std::enable_if_t<std::is_unsigned_v<T>, T>
  getOrDefault(const key_type& key, const T& defaultVal) const
  {
    auto it = find(key);
    if (it == end()) {
      return defaultVal;
    }

    if (it->second.find('-') != std::string::npos) {
      NDN_THROW(std::invalid_argument(key + " cannot be negative"));
    }

    T val{};
    if (!boost::conversion::try_lexical_convert(it->second, val)) {
      NDN_THROW(std::invalid_argument(key + " value is malformed"));
    }
    return val;
  }
};

} // namespace nfd::fw

/** \brief Registers a strategy
 *
 *  This macro should appear once in .cpp of each strategy.
 */
#define NFD_REGISTER_STRATEGY(S)                       \
static class NfdAuto ## S ## StrategyRegistrationClass \
{                                                      \
public:                                                \
  NfdAuto ## S ## StrategyRegistrationClass()          \
  {                                                    \
    ::nfd::fw::Strategy::registerType<S>();            \
  }                                                    \
} g_nfdAuto ## S ## StrategyRegistrationVariable

#endif // NFD_DAEMON_FW_STRATEGY_HPP
