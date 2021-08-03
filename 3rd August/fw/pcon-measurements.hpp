#ifndef NFD_DAEMON_FW_PCON_MEASUREMENTS_HPP
#define NFD_DAEMON_FW_PCON_MEASUREMENTS_HPP


#include <ndn-cxx/util/rtt-estimator.hpp>
#include "ndn-cxx/util/time.hpp"
#include "ndn-cxx/name.hpp"
#include "ndn-cxx/util/scheduler.hpp"
#include "table/measurements-accessor.hpp"
#include "ns3/traced-callback.h"

#include "common/global.hpp"

#include "table/pit.hpp"


namespace nfd {
namespace fw {
namespace pcon{


class PconStats
{
public:
  PconStats();

  void
  incrementPINumber();

  void
  decrementPINumber();

  int
  getPINumber() const
  {
    return PI_number;
  }

  float
  getUsagePercentage() const
  {
  	return usage_percentage;
  }

  void
  setUsagePercentage(float new_percentage)
  {
  	usage_percentage = new_percentage;
  }

  bool
  isCongested() const
  {
  	return congested;
  }

  void
  setCongested(bool new_value)
  {
	congested = new_value;
  }


ndn::time::nanoseconds
  computeRto() const
  {
  	return m_rttEstimator.getEstimatedRto();
  }

private:
  int PI_number;
  float usage_percentage;
  bool congested;
  
  ndn::util::RttEstimator m_rttEstimator;
 // ndn::time::nanoseconds maxRto = 1_min;
  
};



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** \brief Strategy information for each face in a namespace
*/
class FaceInfo
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  FaceInfo();

  ~FaceInfo();

  void
  setTimeoutEvent(const ndn::scheduler::EventId& id, const ndn::Name& interestName);

  void
  setMeasurementExpirationEventId(const ndn::scheduler::EventId& id)
  {
    m_measurementExpirationId = id;
  }

  const ndn::scheduler::EventId&
  getMeasurementExpirationEventId()
  {
    return m_measurementExpirationId;
  }

  void
  setUncongestedTimeoutEventId(const ndn::scheduler::EventId& id)
  {
    m_uncongestedTimeoutEventId = id;
  }

  const ndn::scheduler::EventId&
  getUncongestedTimeoutEventId()
  {
    return m_uncongestedTimeoutEventId;
  }

  void
  cancelTimeoutEvent(const ndn::Name& prefix);

  bool
  isTimeoutScheduled() const
  {
    return m_isTimeoutScheduled;
  }

  void
  incrementPINumber(const std::shared_ptr<pit::Entry>& pitEntry, const Face& inFace);

  void
  decrementPINumber(const std::shared_ptr<pit::Entry>& pitEntry, const Face& inFace);

  void
  recordTimeout(const ndn::Name& interestName);

  ndn::Name getLastInterestName()
  {
  	return m_lastInterestName;
  }

  int
  getPINumber() const
  {
    return m_pconStats.getPINumber();
  }

  bool
  isCongested() const
  {
  	return m_pconStats.isCongested();
  }

  void
  setCongested(bool new_value)
  {
	m_pconStats.setCongested(new_value);
  }

  float
  getUsagePercentage() const
  {
    return m_pconStats.getUsagePercentage();
  }

  void
  setUsagePercentage(float new_percentage)
  {
    m_pconStats.setUsagePercentage(new_percentage);
  }

  void reduceUsagePercentage(float reduction)
  {
  	m_pconStats.setUsagePercentage(m_pconStats.getUsagePercentage() - reduction);
  }

  void increaseUsagePercentage(float increase)
  {
  	m_pconStats.setUsagePercentage(m_pconStats.getUsagePercentage() + increase);
  }

  time::nanoseconds
  computeRto() const
  {
  	return m_pconStats.computeRto();
  }

  void
  cancelUncongestedTimeoutEvent();

private:
  void
  cancelTimeoutEvent();

  bool
  doesNameMatchLastInterest(const ndn::Name& name);

private:
  PconStats m_pconStats;
  ndn::Name m_lastInterestName;

  // Event needed to consider the face uncongested again
ndn::scheduler::EventId m_uncongestedTimeoutEventId;

  // Timeout associated with measurement
 ndn::scheduler::EventId m_measurementExpirationId;

  // RTO associated with Interest
 ndn:: scheduler::EventId m_timeoutEventId;
  bool m_isTimeoutScheduled;
};


typedef std::unordered_map<face::FaceId, FaceInfo> FaceInfoTable;



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** \brief stores stategy information about each face in this namespace
 */
class NamespaceInfo : public StrategyInfo, public ns3::Object
{
public:
  static ns3::TypeId
  GetTypeId();

  NamespaceInfo();

  static constexpr int
  getTypeId()
  {
    return 1030;
  }

  FaceInfo&
  getOrCreateFaceInfo(const fib::Entry& fibEntry, const Face& face);

  FaceInfo*
  getFaceInfo(const fib::Entry& fibEntry, const Face& face);

  void
  expireFaceInfo(nfd::face::FaceId faceId);

  void
  extendFaceInfoLifetime(FaceInfo& info, const Face& face);

  const fib::Entry*
  getFibEntry()
  {
  	return m_fibEntry;
  }

  void
  setFibEntry(const fib::Entry& fibEntry)
  {
  	m_fibEntry = &fibEntry;
  }

  FaceInfo*
  get(nfd::face::FaceId faceId)
  {
    if (m_fit.find(faceId) != m_fit.end()) {
      return &m_fit.at(faceId);
    }
    else {
      return nullptr;
    }
  }

  FaceInfoTable::iterator
  find(nfd::face::FaceId faceId)
  {
    return m_fit.find(faceId);
  }

  FaceInfoTable::iterator
  begin()
  {
    return m_fit.begin();
  }

  FaceInfoTable::iterator
  end()
  {
    return m_fit.end();
  }

  const FaceInfoTable::iterator
  insert(nfd::face::FaceId faceId)
  {
    const auto& pair = m_fit.insert(std::make_pair(faceId, FaceInfo()));
    return pair.first;
  }

  void
  sendTrace(face::FaceId faceId, Name prefix, int PINumber, float usage_percentage, bool congested, int global_PI_number);

private:
  const fib::Entry* m_fibEntry;
  FaceInfoTable m_fit;
  ns3::TracedCallback<face::FaceId /* id of the face */, Name /* prefix */, int /* PI number */, float /* usage percentage */, bool /* congested */, int /* global_PI_number */> m_measurements_tracer;
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** \brief Helper class to retrieve and create strategy measurements
 */
class PconMeasurements : noncopyable
{
public:
 explicit
  PconMeasurements(MeasurementsAccessor& measurements);

  FaceInfo*
  getFaceInfo(const fib::Entry& fibEntry, const Interest& interest, const Face& face);

  FaceInfo&
  getOrCreateFaceInfo(const fib::Entry& fibEntry, const Interest& interest, const Face& face);

  NamespaceInfo*
  getNamespaceInfo(const Name& prefix);

  NamespaceInfo&
  getOrCreateNamespaceInfo(const fib::Entry& fibEntry, const Interest& interest);

  int
  getFaceNumber(const Name name)
  {
    std::map<Name, int>::iterator it = face_numbers.find(name);
    if (it != face_numbers.end())
	{
	  return it->second;
	}
	face_numbers.insert(std::pair<Name, int>(name, 0));
	return 0;
  }

  void
  incrementFaceNumber(const Name name)
  {
    std::map<Name, int>::iterator it = face_numbers.find(name);
    if (it != face_numbers.end())
	{
	  it->second++;
	}
	else
	{
	  face_numbers.insert(std::pair<Name, int>(name, 1));
	}
  }

  int
  getGlobalPINumber(const Name name)
  {
    std::map<Name, int>::iterator it = global_PI_numbers.find(name);
    if (it != global_PI_numbers.end())
	{
	  return it->second;
	}
	global_PI_numbers.insert(std::pair<Name, int>(name, 0));
	return 0;
  }

  void
  incrementGlobalPINumber(const Name name)
  {
    std::map<Name, int>::iterator it = global_PI_numbers.find(name);
    if (it != global_PI_numbers.end())
	{
	  it->second++;
	} else {
	  global_PI_numbers.insert(std::pair<Name, int>(name, 1));
	}
  }

  void
  decrementGlobalPINumber(const Name name)
  {
    std::map<Name, int>::iterator it = global_PI_numbers.find(name);
    if (it != global_PI_numbers.end())
	{
	  it->second--;
	}
  }

  measurements::Entry*
  findLongestPrefixMatch(Name prefix)
  {
  	return m_measurements.findLongestPrefixMatch(prefix);
  }

private:
  void
  extendLifetime(measurements::Entry& me);

public:
  static constexpr time::microseconds MEASUREMENTS_LIFETIME = time::seconds(300);

private:


  MeasurementsAccessor& m_measurements;
  std::map<Name, int> global_PI_numbers;
  std::map<Name, int> face_numbers;
};


}
}
}

#endif

