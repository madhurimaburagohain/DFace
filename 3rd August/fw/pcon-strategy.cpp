/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "pcon-strategy.hpp"
//#include "hop-count-congestion-tag.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/algorithm.hpp"

//#include "ns3/ndnSIM/NFD/core/logger.hpp"

#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace fw {
namespace pcon {

NFD_LOG_INIT("PconStrategy");
NFD_REGISTER_STRATEGY(PconStrategy);

const time::milliseconds PconStrategy::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds PconStrategy::RETX_SUPPRESSION_MAX(250);
const float PconStrategy::CHANGE_PERC(0.03);


PconStrategy::PconStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , m_measurements(getMeasurements())
  , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                      RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                      RETX_SUPPRESSION_MAX)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    BOOST_THROW_EXCEPTION(std::invalid_argument("PconStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
      "PconStrategy does not support version " + std::to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
PconStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/pcon/%FD%02");
  return strategyName;
}

//void
//PconStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
      //                            const shared_ptr<pit::Entry>& pitEntry)
                                  
                                  
void PconStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                     const shared_ptr<pit::Entry>& pitEntry)
{
  // Should the Interest be suppressed?
  RetxSuppressionResult suppressResult = m_retxSuppression.decidePerPitEntry(*pitEntry);

  switch (suppressResult) {
  case RetxSuppressionResult::NEW:
  case RetxSuppressionResult::FORWARD:
    break;
  case RetxSuppressionResult::SUPPRESS:
    //NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " suppressed");
    //TODO Envoyer un NACK ?
    return;
  }

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  if (nexthops.size() == 0) {
    /*sendNoRouteNack(ingress.face, interest, pitEntry);
    this->rejectPendingInterest(pitEntry);
    return;*/
    
    
    lp::NackHeader nackHeader;
      nackHeader.setReason(lp::NackReason::NO_ROUTE);
      this->sendNack(pitEntry, ingress, nackHeader);

      this->rejectPendingInterest(pitEntry);
      return;
  }

 Face* faceToUse = getFaceForForwarding(fibEntry, nexthops, interest, ingress.face);

  if (faceToUse == nullptr) {
    /*sendNoRouteNack(inFace, interest, pitEntry);
    this->rejectPendingInterest(pitEntry);
    return;*/
    
    
    lp::NackHeader nackHeader;
      nackHeader.setReason(lp::NackReason::NO_ROUTE);
      this->sendNack(pitEntry, ingress, nackHeader);

      this->rejectPendingInterest(pitEntry);
      return;
  }

  NFD_LOG_TRACE("Forwarding interest to face: " << faceToUse->getId());

  forwardInterest(interest, fibEntry, pitEntry, *faceToUse);
}

//void
//PconStrategy::beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                                 //   const Face& inFace, const Data& data)
                                 
void PconStrategy:: beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const FaceEndpoint& ingress, const Data& data) 
{
  NamespaceInfo* namespaceInfo = m_measurements.getNamespaceInfo(pitEntry->getName());

  if (namespaceInfo == nullptr) {
    NFD_LOG_TRACE("Could not find measurements entry for " << pitEntry->getName());
    return;
  }

  // Get the FaceInfo
  FaceInfo* faceInfo = namespaceInfo->get(ingress.face.getId());
  if (faceInfo == nullptr) {
    return;
  }
int hop_count_to_congested_link;
  // If the data is marked
  if (data.getCongestionMark() != 0) {
  	// Get the Hop count to the congested link
  	
  	
  	auto hopCountCongestionTag = data.getTag<lp::HopCountCongestionTag>();
  	 if (hopCountCongestionTag != nullptr) { // e.g., packet came from local node's cache
     hop_count_to_congested_link = *hopCountCongestionTag;
  }
  	
    //std::shared_ptr<ndn::HopCountCongestionTag> tag = data.getTag<ndn::HopCountCongestionTag>();
   // ndn::HopCountCongestionTag* adb = tag.get();
  	//int hop_count_to_congested_link = adb->get();
  	
  	data.setTag(make_shared<lp::HopCountCongestionTag>(hop_count_to_congested_link + 1));
  	
  	// Increment the HopCountCongestionTag
  	//data.setTag(make_shared<ndn::HopCountCongestionTag>(hop_count_to_congested_link + 1));
  	// Flag the interface as congested
    faceInfo->setCongested(true);
    // Schedule when it will be non congested again
    scheduleUncongestedTimeoutEvent(*faceInfo, ingress.face.getId());
    // If there are multiple path to data
    Name name = m_measurements.findLongestPrefixMatch(pitEntry->getName())->getName();
    if ( m_measurements.getFaceNumber(name) != 1 )
	{
      // Recalculate the usage percentage
	  float reduction = calculateReduction(*faceInfo, hop_count_to_congested_link + 1);
	  // Reduce the percentage of the current face
	  faceInfo->reduceUsagePercentage(reduction);
	  //NFD_LOG_TRACE("Usage percentage of face " << inFace.getId() << " has decreased of " << reduction << "\% and has reached " << faceInfo-//////>getUsagePercentage());
	  // Increase the percentage of the others
	  float increase = reduction / (m_measurements.getFaceNumber(name) - 1);
	  FaceInfoTable::iterator it;
	  FaceInfoTable::iterator current_face_it = namespaceInfo->find(ingress.face.getId());
	  for (it = namespaceInfo->begin(); it != namespaceInfo->end(); it++) {
	    if (it == current_face_it)
	    {
	    	continue;
	    }
	    it->second.increaseUsagePercentage(increase);
		NFD_LOG_TRACE("Usage percentage of face " << it->first << " has increased of " << increase << "\% and has reached " << it->second.getUsagePercentage());
	  }
	}
  }

  // Decrement the PI number for this face and the global
  faceInfo->decrementPINumber(pitEntry, ingress.face);
  Name name = m_measurements.findLongestPrefixMatch(pitEntry->getName())->getName();
  m_measurements.decrementGlobalPINumber(name);
  namespaceInfo->sendTrace(ingress.face.getId(), pitEntry->getName(), faceInfo->getPINumber(), faceInfo->getUsagePercentage(), faceInfo->isCongested(), 0);

  // Extend lifetime for measurements associated with Face
  namespaceInfo->extendFaceInfoLifetime(*faceInfo, ingress.face);

  if (faceInfo->isTimeoutScheduled()) {
    faceInfo->cancelTimeoutEvent(data.getName());
  }
}

/*void
PconStrategy::afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                               const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("Nack for " << nack.getInterest() << " from=" << inFace.getId() << ": " << nack.getReason());
  // TODO Reenvoyer le NACK ?
  onTimeout(pitEntry->getName(), inFace.getId());
}*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void
PconStrategy::forwardInterest(const Interest& interest,
                              const fib::Entry& fibEntry,
                              const shared_ptr<pit::Entry>& pitEntry,
                              Face& outFace,
                              bool wantNewNonce)
{

 auto egress = FaceEndpoint(outFace, 0);
  if (wantNewNonce) {
    //Send probe: interest with new Nonce
    Interest probeInterest(interest);
    probeInterest.refreshNonce();
    NFD_LOG_TRACE("Sending probe for " << probeInterest << probeInterest.getNonce()
                                       << " to FaceId: " << outFace.getId());
                                       
   
  
                                   
                                       
    this->sendInterest(pitEntry, egress, probeInterest);
  }
  else {
    this->sendInterest(pitEntry, egress, interest);
  }

  FaceInfo& faceInfo = m_measurements.getOrCreateFaceInfo(fibEntry, interest, outFace);
  faceInfo.incrementPINumber(pitEntry, outFace);

  // Refresh measurements since Face is being used for forwarding
  NamespaceInfo& namespaceInfo = m_measurements.getOrCreateNamespaceInfo(fibEntry, interest);
  namespaceInfo.extendFaceInfoLifetime(faceInfo, outFace);
  namespaceInfo.sendTrace(outFace.getId(), fibEntry.getPrefix(), faceInfo.getPINumber(), faceInfo.getUsagePercentage(), faceInfo.isCongested(), 0);

  if (!faceInfo.isTimeoutScheduled()) {
    // Estimate and schedule timeout
  //  RttEstimator::Duration timeout = faceInfo.computeRto();
ndn::time::nanoseconds timeout = faceInfo.computeRto();
    //NFD_LOG_TRACE("Scheduling timeout for " << fibEntry.getPrefix()
                                         //   << " FaceId: " << outFace.getId()
                                          //  << " in " << time::duration_cast<time::milliseconds>(timeout) << " ms");

    scheduler::EventId id = getScheduler().schedule(timeout,
        bind(&PconStrategy::onTimeout, this, interest.getName(), outFace.getId()));

    faceInfo.setTimeoutEvent(id, interest.getName());
  }
}



Face*
PconStrategy::getFaceForForwarding(const fib::Entry& fibEntry, const fib::NextHopList& nexthops,
                                   const Interest& interest, const Face& inFace)
{
  NFD_LOG_TRACE("Looking for a face for " << fibEntry.getPrefix()
	            << " via percentage usage");

  NamespaceInfo namespaceinfo = m_measurements.getOrCreateNamespaceInfo(fibEntry, interest);
  Name name = m_measurements.findLongestPrefixMatch(fibEntry.getPrefix())->getName();
  fib::NextHopList::const_iterator it;

  bool all_percentage_null = true;
  m_measurements.incrementGlobalPINumber(name);
  int global_PI_number = m_measurements.getGlobalPINumber(fibEntry.getPrefix());


  for (it = nexthops.begin(); it != nexthops.end(); it++) {
    FaceInfo& faceInfo = m_measurements.getOrCreateFaceInfo(fibEntry, interest, it->getFace());
    int PI_number = faceInfo.getPINumber();
    float percentage = faceInfo.getUsagePercentage();
    float real_percentage = (float) PI_number / (float) global_PI_number;
    
   // std::cout << "real percentage "<< real_percentage <<  "\n";
    if (percentage != 0) {
      all_percentage_null = false;
	}
    if (real_percentage < percentage) {
      return &it->getFace();
	}
  }

  if (all_percentage_null) {
    it = nexthops.begin();
    if (it != nexthops.end()) {
      FaceInfo& faceInfo = m_measurements.getOrCreateFaceInfo(fibEntry, interest, it->getFace());
      faceInfo.setUsagePercentage(1);
      return &it->getFace();
    }
  }

  //TODO rajouter un log d'erreur ?
  return nullptr;
}



void
PconStrategy::onTimeout(const Name& interestName, face::FaceId faceId)
{
  NFD_LOG_TRACE("FaceId: " << faceId << " for " << interestName << " has timed-out");

  NamespaceInfo* namespaceInfo = m_measurements.getNamespaceInfo(interestName);

  if (namespaceInfo == nullptr) {
    NFD_LOG_TRACE("FibEntry for " << interestName << " has been removed since timeout scheduling");
    return;
  }

  FaceInfoTable::iterator it = namespaceInfo->find(faceId);

  if (it == namespaceInfo->end()) {
    it = namespaceInfo->insert(faceId);
    m_measurements.incrementFaceNumber(interestName);
  }

  FaceInfo& faceInfo = it->second;
  faceInfo.recordTimeout(interestName);
  Name name = m_measurements.findLongestPrefixMatch(interestName)->getName();
  m_measurements.decrementGlobalPINumber(name);
  namespaceInfo->sendTrace(faceId, interestName, faceInfo.getPINumber(), faceInfo.getUsagePercentage(), faceInfo.isCongested(), 0); //TODO find the prefix for interestName
}

/*

void
PconStrategy::sendNoRouteNack(const Face& inFace, const Interest& interest,
                              const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");

  lp::NackHeader nackHeader;
  nackHeader.setReason(lp::NackReason::NO_ROUTE);
  this->sendNack(pitEntry, inFace, nackHeader);
}
*/
void
PconStrategy::scheduleUncongestedTimeoutEvent(FaceInfo& faceInfo, FaceId faceId)
{
  time::microseconds marking_interval = time::milliseconds(110); // max marking_intervale from pcon-link-service
  faceInfo.cancelUncongestedTimeoutEvent();
  scheduler::EventId id = getScheduler().schedule(marking_interval, bind(&PconStrategy::endOfCongestion, this, faceInfo, faceId));
  faceInfo.setUncongestedTimeoutEventId(id);
}

float
PconStrategy::calculateReduction(FaceInfo& faceInfo, int hop_count_to_congested_link)
{
  return faceInfo.getUsagePercentage() * PconStrategy::CHANGE_PERC/hop_count_to_congested_link;
}

void
PconStrategy::endOfCongestion(FaceInfo& faceInfo, FaceId faceId)
{
  NFD_LOG_TRACE("End of congestion for face: " << faceId);
  // Set the congested flag to false
  faceInfo.setCongested(false);
  // Get the prefix consider
  Name name = m_measurements.findLongestPrefixMatch(faceInfo.getLastInterestName())->getName();
  // Get the best next hop
  NamespaceInfo* namespaceInfo = m_measurements.getNamespaceInfo(name);
  const fib::Entry* fibEntry = namespaceInfo->getFibEntry();
  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  fib::NextHopList::const_iterator bestNextHop = nexthops.begin();
  // If there are multiple path to data
  if ( m_measurements.getFaceNumber(name) != 1 )
  {
    // If the best next hop is not the current
	if ( bestNextHop->getFace().getId() != faceId )
	{
	  // Calculate the increase
      float increase = PconStrategy::CHANGE_PERC / 10;
	  FaceInfoTable::iterator it;
	  for (it = namespaceInfo->begin(); it != namespaceInfo->end(); it++) {
	    if (it->first == bestNextHop->getFace().getId())
		{
		  // Increase the percentage usage of the best face
		  it->second.increaseUsagePercentage(increase);
		  NFD_LOG_TRACE("Usage percentage of face " << it->first << " has increased of " << increase << "\% and has reached " << it->second.getUsagePercentage());
		  // Decrease the current
		  faceInfo.reduceUsagePercentage(increase);
		  NFD_LOG_TRACE("Usage percentage of face " << faceId << " has decreased of " << increase << "\% and has reached " << faceInfo.getUsagePercentage());
		  return;
		}
	  }
	}
  }
}

} // namespace pcon
} // namespace fw
} // namespace nfd
