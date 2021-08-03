#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {


 void TraceInfo ( Ptr<Node> node, Time next) {

  
  long int interest, data, delay;
  double isr;
  int pit_size;
  ofstream f;
  string node_name=ns3::Names:: FindName (node);
  //string file_name= string("Data_")+  std::to_string(node->GetId())  + string(".txt");
string file_name=  node_name  + "_"+ std::to_string(node->GetId())+  string(".txt");
//string file_name= string("Data_")+  node_name  + string(".txt");
  f.open (file_name, std::ofstream :: out | std::ofstream :: app);
  	
  Ptr<ns3::ndn:: L3Protocol> l3= ns3::ndn::L3Protocol::getL3Protocol(node);
  shared_ptr<nfd::Forwarder> fw=  l3-> getForwarder();
  std::tie (interest, data, isr, pit_size, delay) =fw->CollectInfo();
  std::cout << "\n Delay and Node Id \t" << delay << "\t" << node_name << "\n"  ;


  Time now= Simulator::Now();
  f << now.ToDouble (Time::S)<< "\t" << (double)(interest)/5<< "\t" << (double) (data )/5<< "\t" << isr << "\t" << pit_size << "\t" << delay << "\n";
   
   
  Simulator::Schedule (next, TraceInfo, node,   next);
  
  }

int
main(int argc, char* argv[])
{
  std::string freq="65";
  CommandLine cmd;
  cmd.AddValue("intFreq","interest frequency", freq);
  cmd.Parse(argc, argv);

  std::cout << freq;


  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-pcon_scen4.2_equal.txt");
  topologyReader.Read();


  NodeContainer nc = NodeContainer::GetGlobal();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();

//ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast/%FD%03");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  Ptr<Node> consumer1 = Names::Find<Node>("consumer1");
  Ptr<Node> consumer2 = Names::Find<Node>("consumer2");
  Ptr<Node> consumer3 = Names::Find<Node>("consumer3");
  Ptr<Node> consumer4 = Names::Find<Node>("consumer4");
  //Ptr<Node> consumer4 = Names::Find<Node>("consumer5");


  Ptr<Node> producer1 = Names::Find<Node>("producer1");
  Ptr<Node> producer2 = Names::Find<Node>("producer2");
  Ptr<Node> producer3 = Names::Find<Node>("producer3");
  Ptr<Node> producer4 = Names::Find<Node>("producer4");

 ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPcon");
consumerHelper.SetAttribute("Size", StringValue("10000000"));


   ndn::AppHelper consumerHelper1("ns3::ndn::ConsumerCbr");
  consumerHelper1.SetPrefix("/dst1");
  consumerHelper1.SetAttribute("Frequency", StringValue("10000")); // 10 interests a second
  consumerHelper1.SetAttribute("LifeTime", StringValue("2s"));
  ApplicationContainer Aconsumer1= consumerHelper1.Install(consumer1);
  


consumerHelper.SetPrefix("/dst2");
 consumerHelper.SetAttribute("LifeTime", StringValue("2s"));
  ApplicationContainer Aconsumer2= consumerHelper.Install(consumer2);

  consumerHelper.SetPrefix("/dst3");
 consumerHelper.SetAttribute("LifeTime", StringValue("2s"));
 ApplicationContainer Aconsumer3= consumerHelper.Install(consumer3);
 consumerHelper.SetPrefix("/dst4");
 consumerHelper.SetAttribute("LifeTime", StringValue("2s"));
  ApplicationContainer Aconsumer4= consumerHelper.Install(consumer4);
 

 

 ndn::AppHelper producerHelper("ns3::ndn::Producer");
//  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

  // Register /dst1 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst1 namespace
  ndnGlobalRoutingHelper.AddOrigins("/dst1", producer1);
  producerHelper.SetPrefix("/dst1");
  producerHelper.Install(producer1);

  // Register /dst1 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst1 namespace
  ndnGlobalRoutingHelper.AddOrigins("/dst2", producer2);
  producerHelper.SetPrefix("/dst2");
  producerHelper.Install(producer2);

  ndnGlobalRoutingHelper.AddOrigins("/dst3", producer3);
  producerHelper.SetPrefix("/dst3");
  producerHelper.Install(producer3);

  ndnGlobalRoutingHelper.AddOrigins("/dst4", producer4);
  producerHelper.SetPrefix("/dst4");
  producerHelper.Install(producer4);


 // Calculate and install FIBs
  ndnGlobalRoutingHelper.InstallAll ();
ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();
 // NodeContainer nc = NodeContainer::GetGlobal();
  
  for (uint32_t i=0;i<nc.GetN();i++){
  
	Simulator::Schedule (Seconds (5), TraceInfo,  nc.Get (i), Seconds (5));
  }

  Simulator::Stop(Seconds(2.0));

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
