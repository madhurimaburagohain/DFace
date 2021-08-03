#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

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
  std::cout << "\n Delay and Node Id \t" << delay << "\t" << node_name << "\n"  ;


  Time now= Simulator::Now();
  f << now.ToDouble (Time::S)<< "\t" << (double)(interest)<< "\t" << (double) (data )<< "\t" << isr << "\t" << pit_size << "\t" << delay << "\n";
   
   
  Simulator::Schedule (next, TraceInfo, node,   next);
  
  }

int main (int argc, char *argv[])
{

	CommandLine cmd;
  	cmd.Parse(argc, argv);
  	AnnotatedTopologyReader topologyReader("", 25);
 	topologyReader.SetFileName ("ndnSIM-sample-topologies/topologies/bw-delay-rand-1/1755.r0-conv-annotated.txt");
	topologyReader.Read();
	
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
         if (Names::FindName (node).compare (0, 5, "leaf-")==0)
         {
              leaves.Add (node);
         }
        else if (Names::FindName (node).compare (0, 3, "gw-")==0)
         {
              gw.Add (node);
         }
        else if (Names::FindName (node).compare (0, 3, "bb-")==0)
        {
              bb.Add (node);
        }
   });
   

   std::cout << "Total_numbef_of_nodes      " << NodeList::GetNNodes () << endl;
   std::cout  << "Total_number_of_leaf_nodes " << leaves.GetN () << endl;
   std::cout << "Total_number_of_gw_nodes   " << gw.GetN () << endl;
   std::cout << "Total_number_of_bb_nodes   " << bb.GetN () << endl;
   
  
 
 NodeContainer consumers,  producers;
 

 
   
    while (consumers.size () < 10)
    {

	double min1 = 0.0;
        double max1 = leaves.GetN ();
	 Ptr<UniformRandomVariable> x1 = CreateObject<UniformRandomVariable> ();
	 
	 x1->SetAttribute ("Min", DoubleValue (min1));
         x1->SetAttribute ("Max", DoubleValue (max1));

	
         int node_id= int (x1->GetValue ());
     
	 Ptr<Node> node = leaves.Get (node_id);


     
     /* if (consumers.find (node) != consumers.end ())
          
        continue;*/
      
     consumers.Add (node);
      string name = Names::FindName (node);
      
      std::cout << "\nProducer Name \t" << name << "\n";
      Names::Rename (name, "consumer-"+name);
    }
   
   string producerLocation = "bb";
   while (producers.size () < 5)
    {
       Ptr<Node> node ;
      if (producerLocation == "gw")
        {


	double min1 = 0.0;
        double max1 = gw.GetN ();
	 Ptr<UniformRandomVariable> x1 = CreateObject<UniformRandomVariable> ();
	 
	 x1->SetAttribute ("Min", DoubleValue (min1));
         x1->SetAttribute ("Max", DoubleValue (max1));

	
         int node_id= int (x1->GetValue ());
     
	 node = gw.Get (node_id);
          //UniformVariable randVar (0, gw.GetN ());
          //node = gw.Get (randVar.GetValue ());
        }
      else if (producerLocation == "bb")
        {


	double min1 = 0.0;
        double max1 = bb.GetN ();
	 Ptr<UniformRandomVariable> x1 = CreateObject<UniformRandomVariable> ();
	 
	 x1->SetAttribute ("Min", DoubleValue (min1));
         x1->SetAttribute ("Max", DoubleValue (max1));

	
         int node_id= int (x1->GetValue ());
     
	  node = bb.Get (node_id);
          //UniformVariable randVar (0, bb.GetN ());
          ///node = bb.Get (randVar.GetValue ());
        }
      
	/* if (producers.find (node) != producers.end ())
               continue;*/
         producers.Add (node);

	
      string name = Names::FindName (node);

	std::cout << "\nProducer Name \t" << name << "\n";
      Names::Rename (name, "producer-"+name);
    }
    int i=0;
    
    for (NodeContainer::Iterator node = consumers.begin (); node != consumers.end (); node++)
    {
     
     
   
   
   /*if (i==5)
   {
   	ndn::AppHelper goodAppHelper ("ns3::ndn::ConsumerCbr");
   	goodAppHelper.SetAttribute ("Frequency", StringValue ("500")); // 100 interests a second
   	goodAppHelper .SetPrefix ("/good/"+to_string(i)+"/");
   	ApplicationContainer consumer = goodAppHelper.Install(*node);
   }*/
   //else
   {
   	
   	ndn::AppHelper goodAppHelper ("ns3::ndn::ConsumerPcon");
        goodAppHelper .SetPrefix ("/good/"+to_string(i)+"/");
        ApplicationContainer consumer = goodAppHelper.Install(*node);
        
   }
   i++;
	
	
    
   
    }
  

   
   for (NodeContainer::Iterator node = producers.begin (); node != producers.end (); node++)
    {

        ndn::AppHelper producerHelper ("ns3::ndn::Producer");

	

        producerHelper.SetPrefix("/good");

        ndnGlobalRoutingHelper.AddOrigins ("/good",*node);

	producerHelper.Install(*node);

	
         
  }
  
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes();
  
   for (uint32_t i=0;i<consumers.GetN();i++){
  
	Simulator::Schedule (Seconds (1), TraceInfo,  consumers.Get (i), Seconds (1));
  }

	Simulator::Stop(Seconds(3.0));
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

