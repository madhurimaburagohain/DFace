
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/traffic-control-module.h"

namespace ns3 {


 void TraceInfo ( Ptr<Node> node, Time next) {

  long int interest, data, delay;
  double isr;
  int pit_size;
  ofstream f;
  string file_name= string("REsultTracer_")+  std::to_string(node->GetId())  + string(".txt");
  f.open (file_name, std::ofstream :: out | std::ofstream :: app);
  	
  Ptr<ns3::ndn:: L3Protocol> l3= ns3::ndn::L3Protocol::getL3Protocol(node);
  shared_ptr<nfd::Forwarder> fw=  l3-> getForwarder();
  std::tie (interest, data, isr, pit_size, delay) =fw->CollectInfo();
  
  Time now= Simulator::Now();
  f << now.ToDouble (Time::S)<< "\t" << interest << "\t" << data << "\t" << isr <<"\t" << pit_size << "\t" << delay << "\n";
  Simulator::Schedule (next, TraceInfo, node,   next);
   
   }

/**
 *
 *   /------\ 0                                                 0 /------\
 *   |  c1  |<-----+                                       +----->|  p1  |
 *   \------/       \                                     /       \------/
 *                   \              /-----\              /
 *   /------\ 0       \         +==>| r12 |<==+         /       0 /------\
 *   |  c2  |<--+      \       /    \-----/    \       /      +-->|  p2  |
 *   \------/    \      \     |                 |     /      /    \------/
 *                \      |    |   1Mbps links   |    |      /
 *                 \  1  v0   v5               1v   2v  3  /
 *                  +->/------\                 /------\<-+
 *                    2|  r1  |<===============>|  r2  |4
 *                  +->\------/4               0\------/<-+
 *                 /    3^                           ^5    \
 *                /      |                           |      \
 *   /------\ 0  /      /                             \      \  0 /------\
 *   |  c3  |<--+      /                               \      +-->|  p3  |
 *   \------/         /                                 \         \------/
 *                   /     "All consumer-router and"     \
 *   /------\ 0     /      "router-producer links are"    \    0 /------\
 *   |  c4  |<-----+       "10Mbps"                        +---->|  p4  |
 *   \------/                                                    \------/
 *
 *   "Numbers near nodes denote face IDs. Face ID is assigned based on the order of link"
 *   "definitions in the topology file"
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-congestion-alt-topo-plugin
 */

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);
// Config::SetDefault ("ns3::PointToPointNetDevice::TxQueue", StringValue ("ns3::DropTailQueue1"));
  
  //Config::SetDefault ("ns3::PointToPointNetDevice::TxQueue", StringValue ("ns3::CoDelQueue2"));
  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-11-node-two-bottlenecks.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.setCsSize(1);
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Getting containers for the consumer/producer
  Ptr<Node> consumers[4] = {Names::Find<Node>("c1"), Names::Find<Node>("c2"),
                            Names::Find<Node>("c3"), Names::Find<Node>("c4")};
  Ptr<Node> producers[4] = {Names::Find<Node>("p1"), Names::Find<Node>("p2"),
                            Names::Find<Node>("p3"), Names::Find<Node>("p4")};

  if (consumers[0] == 0 || consumers[1] == 0 || consumers[2] == 0 || consumers[3] == 0
      || producers[0] == 0 || producers[1] == 0 || producers[2] == 0 || producers[3] == 0) {
    NS_FATAL_ERROR("Error in topology: one nodes c1, c2, c3, c4, p1, p2, p3, or p4 is missing");
  }

  for (int i = 0; i < 4; i++) {
   // std::string prefix = "/data/" + Names::FindName(producers[i]);
    
    std::string prefix = "/data/" ;

   
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPcon");


    consumerHelper.SetPrefix(prefix);
    ApplicationContainer consumer = consumerHelper.Install(consumers[i]);
  //  consumer.Start(Seconds(i));     // start consumers at 0s, 1s, 2s, 3s
  //  consumer.Stop(Seconds(19 - i)); // stop consumers at 19s, 18s, 17s, 16s

    ///////////////////////////////////////////////
    // install producer app on producer node p_i //
    ///////////////////////////////////////////////

    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

    // install producer that will satisfy Interests in /dst1 namespace
    producerHelper.SetPrefix(prefix);
    ApplicationContainer producer = producerHelper.Install(producers[i]);
    // when Start/Stop time is not specified, the application is running throughout the simulation
  }
  
  
  TrafficControlHelper tchCoDel;
  tchCoDel.SetRootQueueDisc ("ns3::CoDelQueueDisc");


  // Manually configure FIB routes
  ndn::FibHelper::AddRoute("c1", "/data", "n1", 1); // link to n1
  ndn::FibHelper::AddRoute("c2", "/data", "n1", 1); // link to n1
  ndn::FibHelper::AddRoute("c3", "/data", "n1", 1); // link to n1
  ndn::FibHelper::AddRoute("c4", "/data", "n1", 1); // link to n1

  ndn::FibHelper::AddRoute("n1", "/data", "n2", 1);  // link to n2
  ndn::FibHelper::AddRoute("n1", "/data", "n12", 2); // link to n12

  ndn::FibHelper::AddRoute("n12", "/data", "n2", 1); // link to n2

  ndn::FibHelper::AddRoute("n2", "/data/p1", "p1", 1); // link to p1
  ndn::FibHelper::AddRoute("n2", "/data/p2", "p2", 1); // link to p2
  ndn::FibHelper::AddRoute("n2", "/data/p3", "p3", 1); // link to p3
  ndn::FibHelper::AddRoute("n2", "/data/p4", "p4", 1); // link to p4

  // Schedule simulation time and run the simulation
  
 NodeContainer nc=ns3::NodeContainer::GetGlobal();
  
  for (uint32_t i=0;i<nc.GetN();i++){
  
	Simulator::Schedule (Seconds (1), TraceInfo,  nc.Get (i), Seconds (1));
  }
  
  Simulator::Stop(Seconds(5.0));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
