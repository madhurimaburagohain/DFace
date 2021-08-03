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

#ifndef NFD_DAEMON_FW_PCON_STRATEGY_HPP
#define NFD_DAEMON_FW_PCON_STRATEGY_HPP

#include "pcon-measurements.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/retx-suppression-exponential.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/strategy.hpp"

namespace nfd {
namespace fw {
namespace pcon {

/** \brief Adaptive SRTT-based Forwarding Strategy
 *
 *  \see Vince Lehman, Ashlesh Gawande, Rodrigo Aldecoa, Dmitri Krioukov, Beichuan Zhang, Lixia Zhang, and Lan Wang,
 *       "An Experimental Investigation of Hyperbolic Routing with a Smart Forwarding Plane in NDN,"
 *       NDN Technical Report NDN-0042, 2016. http://named-data.net/techreports.html
 */
class PconStrategy : public Strategy, public ns3::Object
{
public:
  explicit
  PconStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

public: // triggers
  //virtual void
  //afterReceiveInterest(const Face& inFace, const Interest& interest,
                  //     const shared_ptr<pit::Entry>& pitEntry) override;
 void
  afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;
 // virtual void
  //beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                  //      const Face& inFace, const Data& data) override;
                  
                   
 void beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const FaceEndpoint& ingress, const Data& data) override;

  /*virtual void
  afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry) override;
*/
private:
  void
  forwardInterest(const Interest& interest,
                  const fib::Entry& fibEntry,
                  const shared_ptr<pit::Entry>& pitEntry,
                  Face& outFace,
                  bool wantNewNonce = false);

  Face*
  getFaceForForwarding(const fib::Entry& fibEntry, const fib::NextHopList& nexthops,
                       const Interest& interest, const Face& inFace);

  void
  onTimeout(const Name& interestName, face::FaceId faceId);

/*  void
  sendNoRouteNack(const Face& inFace, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry);
*/
  void
  scheduleUncongestedTimeoutEvent(FaceInfo& faceInfo, FaceId faceId);
  
  void
  endOfCongestion(FaceInfo& faceInfo, FaceId faceId); 

float
  calculateReduction(FaceInfo& faceInfo, int hop_count_to_congested_link);

  

private:
  PconMeasurements m_measurements;

private:
  RetxSuppressionExponential m_retxSuppression;

  static const time::milliseconds RETX_SUPPRESSION_INITIAL;
  static const time::milliseconds RETX_SUPPRESSION_MAX;
  static const float CHANGE_PERC;
};

} // namespace pcon

using pcon::PconStrategy;

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_PCON_STRATEGY_HPP
