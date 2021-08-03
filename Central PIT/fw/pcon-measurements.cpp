#include "pcon-measurements.hpp"

namespace nfd {
namespace fw {

namespace pcon{
NFD_LOG_INIT("PconMeasurements");

PconStats::PconStats()
  : PI_number(0)
  , usage_percentage(0)
  , congested(false)
{
}

void
PconStats::incrementPINumber()
{
  PI_number++;
}

void
PconStats::decrementPINumber()
{
  PI_number--;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

FaceInfo::FaceInfo()
  : m_isTimeoutScheduled(false)
{
}

FaceInfo::~FaceInfo()
{
  cancelUncongestedTimeoutEvent();
  cancelTimeoutEvent();
  
  m_measurementExpirationId.cancel();
  //scheduler::cancel(m_measurementExpirationId);
}

void
FaceInfo::setTimeoutEvent(const scheduler::EventId& id, const Name& interestName)
{
  if (!m_isTimeoutScheduled) {
    m_timeoutEventId = id;
    m_isTimeoutScheduled = true;
    m_lastInterestName = interestName;
  }
  else {
    BOOST_THROW_EXCEPTION(FaceInfo::Error("Tried to schedule a timeout for a face that already has a timeout scheduled."));
  }
}

void
FaceInfo::cancelUncongestedTimeoutEvent()
{
  //scheduler::cancel(m_uncongestedTimeoutEventId);
  m_uncongestedTimeoutEventId.cancel();
}

void
FaceInfo::cancelTimeoutEvent()
{

m_timeoutEventId.cancel();
  //scheduler::cancel(m_timeoutEventId);
  
  
  m_isTimeoutScheduled = false;
}

void
FaceInfo::cancelTimeoutEvent(const Name& prefix)
{
  if (isTimeoutScheduled() && doesNameMatchLastInterest(prefix)) {
    cancelTimeoutEvent();
  }
}

bool
FaceInfo::doesNameMatchLastInterest(const Name& name)
{
  return m_lastInterestName.isPrefixOf(name);
}

void
FaceInfo::incrementPINumber(const shared_ptr<pit::Entry>& pitEntry, const Face& outFace)
{
  pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(outFace);

  if (outRecord == pitEntry->out_end()) { // no out-record
  	// TODO verifier que c'est bien utile ici
    NFD_LOG_TRACE(pitEntry->getInterest() << " dataFrom outFace=" << outFace.getId() << " no-out-record");
    return;
  }

  m_pconStats.incrementPINumber();

  NFD_LOG_TRACE("Incrementing PI number for FaceId: " << outFace.getId()
                << " PI number: " << m_pconStats.getPINumber());
}

void
FaceInfo::decrementPINumber(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace)
{
  pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(inFace);

  if (outRecord == pitEntry->out_end()) { // no out-record
    NFD_LOG_TRACE(pitEntry->getInterest() << " dataFrom inFace=" << inFace.getId() << " no-out-record");
    return;
  }

  m_pconStats.decrementPINumber();

  NFD_LOG_TRACE("Decrementing PI number for FaceId: " << inFace.getId()
                << " PI number: " << m_pconStats.getPINumber());
}

void
FaceInfo::recordTimeout(const Name& interestName)
{
  m_pconStats.decrementPINumber(); // pk on utilise pas celle de FaceInfo ?
  cancelTimeoutEvent(interestName);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ns3::TypeId
NamespaceInfo::GetTypeId(void)
{
  static ns3::TypeId tid =
    ns3::TypeId("ns3::ndn::NamespaceInfo")
      .SetParent<Object> ()
      .SetGroupName ("Ndn")
      .AddConstructor<NamespaceInfo>()
      .AddTraceSource("MeasurementsTracer",
                      "PI number, percentage and congested information of the face for this prefix",
                      MakeTraceSourceAccessor(&NamespaceInfo::m_measurements_tracer),
                      "ns3::ndn::NamespaceInfo::MeasurementsTracerCallback");

  return tid;
}

NamespaceInfo::NamespaceInfo()
{
}

FaceInfo*
NamespaceInfo::getFaceInfo(const fib::Entry& fibEntry, const Face& face)
{
  FaceInfoTable::iterator it = m_fit.find(face.getId());

  if (it != m_fit.end()) {
    return &it->second;
  }
  else {
    return nullptr;
  }
}

FaceInfo&
NamespaceInfo::getOrCreateFaceInfo(const fib::Entry& fibEntry, const Face& face)
{
  FaceInfoTable::iterator it = m_fit.find(face.getId());

  FaceInfo* info = nullptr;

  if (it == m_fit.end()) {
    const auto& pair = m_fit.insert(std::make_pair(face.getId(), FaceInfo()));
    info = &pair.first->second;

    extendFaceInfoLifetime(*info, face);
  }
  else {
    info = &it->second;
  }

  return *info;
}

void
NamespaceInfo::expireFaceInfo(nfd::face::FaceId faceId)
{
  m_fit.erase(faceId);
}

void
NamespaceInfo::extendFaceInfoLifetime(FaceInfo& info, const Face& face)
{
  // Cancel previous expiration
 // scheduler::cancel(info.getMeasurementExpirationEventId());
  info.getMeasurementExpirationEventId().cancel();

  // Refresh measurement
  ndn::scheduler::EventId id = getScheduler().schedule(PconMeasurements::MEASUREMENTS_LIFETIME,
    bind(&NamespaceInfo::expireFaceInfo, this, face.getId()));
    
    
    
    

  info.setMeasurementExpirationEventId(id);
}

void
NamespaceInfo::sendTrace(face::FaceId faceId, Name prefix, int PINumber, float usage_percentage, bool congested, int global_PI_number)
{
	m_measurements_tracer(faceId, prefix, PINumber, usage_percentage, congested, global_PI_number);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

constexpr time::microseconds PconMeasurements::MEASUREMENTS_LIFETIME;

PconMeasurements::PconMeasurements(MeasurementsAccessor& measurements)
  : m_measurements(measurements)
{
}

FaceInfo*
PconMeasurements::getFaceInfo(const fib::Entry& fibEntry, const Interest& interest, const Face& face)
{
  NamespaceInfo& info = getOrCreateNamespaceInfo(fibEntry, interest);
  return info.getFaceInfo(fibEntry, face);
}

FaceInfo&
PconMeasurements::getOrCreateFaceInfo(const fib::Entry& fibEntry, const Interest& interest, const Face& face)
{
  NamespaceInfo& info = getOrCreateNamespaceInfo(fibEntry, interest);
  int faceId = face.getId();
  Name name = fibEntry.getPrefix();
  if ( info.get(faceId) == nullptr )
  {
    this->incrementFaceNumber(name);
  }
  return info.getOrCreateFaceInfo(fibEntry, face);
}

NamespaceInfo*
PconMeasurements::getNamespaceInfo(const Name& prefix)
{
  measurements::Entry* me = m_measurements.findLongestPrefixMatch(prefix);
  if (me == nullptr) {
    return nullptr;
  }

  // Set or update entry lifetime
  extendLifetime(*me);

  NamespaceInfo* info = me->insertStrategyInfo<NamespaceInfo>().first;
  BOOST_ASSERT(info != nullptr);
  return info;
}

NamespaceInfo&
PconMeasurements::getOrCreateNamespaceInfo(const fib::Entry& fibEntry, const Interest& interest)
{
  measurements::Entry* me = m_measurements.get(fibEntry);

  // If the FIB entry is not under the strategy's namespace, find a part of the prefix
  // that falls under the strategy's namespace
  for (size_t prefixLen = fibEntry.getPrefix().size() + 1;
       me == nullptr && prefixLen <= interest.getName().size(); ++prefixLen) {
    me = m_measurements.get(interest.getName().getPrefix(prefixLen));
  }

  // Either the FIB entry or the Interest's name must be under this strategy's namespace
  BOOST_ASSERT(me != nullptr);

  // Set or update entry lifetime
  extendLifetime(*me);

  NamespaceInfo* info = me->insertStrategyInfo<NamespaceInfo>().first;
  BOOST_ASSERT(info != nullptr);
  info->setFibEntry(fibEntry);
  return *info;
}

void
PconMeasurements::extendLifetime(measurements::Entry& me)
{
  m_measurements.extendLifetime(me, MEASUREMENTS_LIFETIME);
}



}
}
}
