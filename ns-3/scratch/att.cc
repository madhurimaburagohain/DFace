#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "model/ndn-net-device-transport.hpp"

using namespace std;
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
  //std::cout << "\n Delay and Node Id \t" << delay << "\t" << node_name << "\n"  ;
  
  
 
  



  Time now= Simulator::Now();
  f << now.ToDouble (Time::S)<< "\t" << (double)(interest)<< "\t" << (double) (data )<< "\t" << isr << "\t" << pit_size << "\t" << delay << "\n";
   
   
  Simulator::Schedule (next, TraceInfo, node,   next);
  
  }

int main (int argc, char *argv[])
{
	CommandLine cmd;
  	cmd.Parse(argc, argv);

  	AnnotatedTopologyReader topologyReader("", 1);
  	topologyReader.SetFileName("src/ndnSIM/examples/topologies/att.txt");
  	topologyReader.Read();


	/*CommandLine cmd;
  	cmd.Parse(argc, argv);
  	AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/att.txt");
  topologyReader.Read();
	int number_of_consumers=10, number_of_producers=5;
	*/
 	 ndn::StackHelper ndnHelper;
	 //ndn::Interest::setDefaultCanBePrefix(0);

  	 ndnHelper.setCsSize(1);
  	 ndnHelper.setPolicy("nfd::cs::lru");

  	 ndnHelper.InstallAll ();
  
  	 ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route/%FD%05");

  	 ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  	 ndnGlobalRoutingHelper.InstallAll();
  
  	 NodeContainer leaves;
    	 NodeContainer gw;
    	 NodeContainer bb;
    	for_each (NodeList::Begin (), NodeList::End (), [&] (Ptr<Node> node) {
         if (Names::FindName (node).compare (0, 1, "c")==0)
         {
              leaves.Add (node);
         }
        else if (Names::FindName (node).compare (0, 1, "r")==0)
         {
              gw.Add (node);
         }
        else if (Names::FindName (node).compare (0, 1, "p")==0)
        {
              bb.Add (node);
        }
   });
   

   std::cout << "Total_numbef_of_nodes      " << NodeList::GetNNodes () << endl;
   std::cout  << "Total_number_of_consumer_nodes " << leaves.GetN () << endl;
   std::cout << "Total_number_of_router_nodes   " << gw.GetN () << endl;
   std::cout << "Total_number_of_producer_nodes   " << bb.GetN () << endl;
  
  
  int i=0; 
  for (NodeContainer::Iterator node = leaves.begin (); node != leaves.end (); node++){
  
  ndn::AppHelper goodAppHelper ("ns3::ndn::ConsumerCbr");
  goodAppHelper .SetPrefix ("/data/"+to_string(i)+"/");
	i++;  
  goodAppHelper .SetAttribute ("Frequency", StringValue ("10")); // 100 interests a second
  ApplicationContainer consumer = goodAppHelper.Install(*node);

  }
  
  for (NodeContainer::Iterator node = bb.begin (); node != bb.end (); node++)
    {

        ndn::AppHelper producerHelper ("ns3::ndn::Producer");
	producerHelper.SetPrefix("/data/");
	 ndnGlobalRoutingHelper.AddOrigins ("/data/",*node);

	producerHelper.Install(*node);
   }
 
 
   // Manually configure FIB routes
 /* ndn::FibHelper::AddRoute("r1", "/data", "r2", 1); // link to n1
  ndn::FibHelper::AddRoute("c2", "/data", "n1", 1); // link to n1
  ndn::FibHelper::AddRoute("c3", "/data", "n1", 1); // link to n1
  ndn::FibHelper::AddRoute("c4", "/data", "n1", 1); // link to n1

  ndn::FibHelper::AddRoute("n1", "/data", "n2", 1);  // link to n2
  ndn::FibHelper::AddRoute("n1", "/data", "n12", 2); // link to n12

  ndn::FibHelper::AddRoute("n12", "/data", "n2", 1); // link to n2

  ndn::FibHelper::AddRoute("n2", "/data", "p1", 1); // link to p1
  ndn::FibHelper::AddRoute("n2", "/data/", "p2", 1); // link to p2
  ndn::FibHelper::AddRoute("n2", "/data/", "p3", 1); // link to p3
  ndn::FibHelper::AddRoute("n2", "/data/", "p4", 1); // link to p4*/
 
 
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();
  /*NodeContainer nc=ns3::NodeContainer::GetGlobal();
  
 for (uint32_t i=0;i<nc.GetN();i++){
  
	Simulator::Schedule (Seconds (1), TraceInfo, nc.Get (i), Seconds (1));
  }*/

	Simulator::Stop(Seconds(5.0));
	Simulator::Run ();
	Simulator::Destroy ();
	return 0;
}

} 






int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}

