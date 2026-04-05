#include "core/common.hpp"
#include <chrono>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#ifndef NFD_DAEMON_FACE_AMS_MULTICAST_SUPPRESSION_HPP
#define NFD_DAEMON_FACE_AMS_MULTICAST_SUPPRESSION_HPP

namespace nfd {
namespace face {
namespace scheduler = ndn::scheduler;
namespace ams {

/* Component-based trie for NDN name prefix match */
class NameTree
{
public:
  std::map<std::string, std::unique_ptr<NameTree>> children;
  bool isLeaf;
  double suppressionTime;

  NameTree();

  void
  insert(const std::string& prefix, double value);

  double
  longestPrefixMatch(const std::string& prefix);

  time::milliseconds
  getSuppressionTimer(const std::string& prefix);

private:
  std::vector<std::string>
  parseNameComponents(const std::string& name);
};

class EMAMeasurements
{

public:
  EMAMeasurements(double expMovingAverage = 0, int lastDuplicateCount = 0, double suppressionTime = 1);

  void
  addUpdateEMA(int duplicateCount, bool wasForwarded);

  scheduler::ScopedEventId&
  getEMAExpiration()
  {
    return this->m_expirationId;
  }

  void
  setEMAExpiration(scheduler::EventId expirationId)
  {
    this->m_expirationId = std::move(expirationId);
  }

  float
  getEMACurrent()
  {
    return this->m_expMovingAverageCurrent;
  }

  float
  getEMAPrev()
  {
    return this->m_expMovingAveragePrev;
  }

  void
  updateDelayTime(bool wasForwarded);

  double
  getCurrentSuppressionTime()
  {
    return m_currentSuppressionTime;
  }

  void
  setSSthress(double val, int factor = 2)
  {
    m_ssthress = val/factor;
  }

private:
double m_expMovingAveragePrev;
double m_expMovingAverageCurrent;
double m_currentSuppressionTime;
scheduler::ScopedEventId m_expirationId;
int m_lastDuplicateCount;
int m_maxDuplicateCount;
double m_minSuppressionTime;
double m_ssthress;
int m_ignoreDuplicateRecoring;
};


class MulticastSuppression
{
public:

  struct ObjectHistory
  {
    int counter;
    bool isForwarded;
  };

  int
  recordInterest(const Interest& interest, bool isForwarded = false);

  int
  recordData(const Data& data, bool isForwarded = false);

  int
  getDuplicateCount(const Name name, char type)
  {
    auto temp_map = getRecorder(type);
    auto it = temp_map->find(name);
    if (it != temp_map->end())
      return it->second.counter;
    return 0;
  }

  std::map<Name, ObjectHistory>*
  getRecorder(char type)
  {
    return (type == 'i') ? &m_interestHistory : &m_dataHistory;
  }

  std::map<Name, std::shared_ptr<EMAMeasurements>>*
  getEMARecorder(char type)
  {
    return (type =='i') ? &m_EMA_interest : &m_EMA_data;
  }

  NameTree*
  getNameTree(char type)
  {
    return (type =='i') ? &m_interestNameTree : &m_dataNameTree; 
  }

  bool
  interestInflight(const Interest& interest) const
  {
    auto name = interest.getName();
    return (m_interestHistory.find(name) != m_interestHistory.end() );
  }

  bool
  dataInflight(const Data& data) const
  {
    auto name = data.getName();
    return (m_dataHistory.find(name) != m_dataHistory.end());
  }

time::milliseconds
getRandomTime()
  {
    return time::milliseconds(1 + (ndn::random::generateWord32() % 10));
  }

void
updateMeasurement(Name name, char type);

// set interest or data expiration
void
setUpdateExpiration(time::milliseconds entryLifetime, Name name, char type);

time::milliseconds
getDelayTimer(Name name, char type);

bool
getForwardedStatus(ndn::Name prefix, char type)
{
  auto recorder = getRecorder(type);
  auto it = recorder->find(prefix);
  return it != recorder->end() ? it->second.isForwarded : false; // if record exist, send whatever is the status else send false
}

private:

  std::map<ndn::Name, ObjectHistory> m_dataHistory;
  std::map<ndn::Name, ObjectHistory> m_interestHistory;
  std::map<Name, scheduler::ScopedEventId> m_objectExpirationTimer;
  std::map<Name, std::shared_ptr<EMAMeasurements>> m_EMA_data;
  std::map<Name, std::shared_ptr<EMAMeasurements>> m_EMA_interest;
  NameTree m_dataNameTree;
  NameTree m_interestNameTree;
};
} //namespace ams
} //namespace face
} //namespace nfd

#endif // NFD_DAEMON_FACE_SUPPRESSION_STRATEGY_HPP