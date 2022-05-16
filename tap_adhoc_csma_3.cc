
#include <iostream>
#include <fstream>
 
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/animation-interface.h"

using namespace ns3;
 
NS_LOG_COMPONENT_DEFINE ("TapDumbbellExample");




class Com_group {
  
  private:
      NodeContainer nodesAP;
      NodeContainer nodesSTA;
      NetDeviceContainer devices1;
      NetDeviceContainer devices2;
      WifiHelper wifi; //helps to create wifi net devices
      NodeContainer nodesCars;
      std::string olsrprot;
      std::string mode;
      std::string tapName;
      std::string multiple_aps;
      Ipv4Address add_aps;
      Ipv4Address add_cars;
      Ipv4Address add_comb;
      NodeContainer car_bridge;
      YansWifiPhyHelper wifiPhy; //creates a phy object - attempts to provide an accurate MAC-level implementation of the 802.11 spec
      WifiMacHelper wifiMac; //creates mac layers for a net device
      Ipv4AddressHelper ipv4; //assigns ipv4 addresses
      OlsrHelper olsr;
      

  public:

    void set_options(std::string olsrprot, std::string mode, std::string tapName, std::string multiple_aps, Ipv4Address add_aps, Ipv4Address add_cars, Ipv4Address add_comb){
        this -> olsrprot = olsrprot;
        this -> mode = mode;
        this -> tapName = tapName;
        this -> multiple_aps = multiple_aps;
        this -> add_aps = add_aps;
        this -> add_cars = add_cars;
        this -> add_comb = add_comb;
    }
    NodeContainer get_car(){
        return car_bridge;
    }

    YansWifiPhyHelper highway_points_setup(){ //set one AP (tap device) and four STA devices (near highway)
      nodesAP.Create (1);
  
      
      nodesSTA.Create (4);

      
      YansWifiChannelHelper wifiChannel; //channel which connects phy objects
      wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel"); //calculates a propagation delay
      wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel" , "MaxRange", DoubleValue (4000.0)); //models the propagation loss through a transmission medium, RangePropagationLoss depends only on the distance between the transmitter and the receiver
      wifiPhy.SetChannel (wifiChannel.Create ());
 
      Ssid ssid = Ssid ("left");
      
      
      wifi.SetRemoteStationManager ("ns3::ArfWifiManager"); //holds a list of per-remote-station state
      
      wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
      devices1 = wifi.Install (wifiPhy, wifiMac, NodeContainer (nodesAP.Get (0))); //the 0th node will be an AP device
 
 
      wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
      devices1.Add (wifi.Install (wifiPhy, wifiMac, nodesSTA)); //the remaining nodes are STA devices


      
      return wifiPhy;
    }


    void tap_setup(){
        
        NodeContainer nodesLeft (nodesAP, nodesSTA); //merge all types of devices into one container
        
        InternetStackHelper internetLeft; //aggregates IP/UDP/TCP functionality to existing nodes
  
  

        if (olsrprot == "yes"){
            
            internetLeft.SetRoutingHelper (olsr); //using the OLSR protocol
            std::cout << "olsr on" << std::endl;
        }
        internetLeft.Install (nodesLeft);
 
        
        ipv4.SetBase (add_aps, "255.255.255.0");
        Ipv4InterfaceContainer interfacesLeft = ipv4.Assign (devices1);
  
        std::cout << interfacesLeft.GetAddress (1) << std::endl;
  
        TapBridgeHelper tapBridge (interfacesLeft.GetAddress (1)); //creates a tap bridge and specifies a gateway for the bridge
        tapBridge.SetAttribute ("Mode", StringValue (mode)); //uses a mode which creates the tap device on a real host for us and destroys it after the simulation ends
        tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
        tapBridge.Install (nodesLeft.Get (0), devices1.Get (0)); //installs a tap bridge on the specified node and forms the bridge with the specified net device
        Ptr<Node> appSink = nodesLeft.Get (0);
        Ipv4Address remoteAddr = appSink->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
        std::cout <<  remoteAddr << std::endl;
        std::cout << "gateway and ghost node addr"<< std::endl;
        return;
      }



     void cars_setup(){ //cars will be a part of an ad hoc network
       
        nodesCars.Create (2);
        car_bridge = nodesCars;
        WifiHelper wifi2;
        
        wifiMac.SetType ("ns3::AdhocWifiMac");
        wifi2.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue ("OfdmRate54Mbps"));
        
        wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
        YansWifiChannelHelper wifiChannel2;
        wifiChannel2.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
        wifiChannel2.AddPropagationLoss ("ns3::LogDistancePropagationLossModel" , "ReferenceDistance", DoubleValue (10.0));
        wifiPhy.SetChannel (wifiChannel2.Create ());
        NetDeviceContainer backboneDevices2 = wifi.Install (wifiPhy, wifiMac, nodesCars);
  
   
        InternetStackHelper internet;
        if (olsrprot == "yes"){
            
            internet.SetRoutingHelper (olsr); // has effect on the next Install ()
        }
        internet.Install (nodesCars);
 
        
        ipv4.SetBase (add_cars, "255.255.255.0");
        ipv4.Assign (backboneDevices2);
        return;
      }

     void car_highway_comm_setup(){ //highway transmitters will be AP devices and cars will be STA devices
        
        YansWifiChannelHelper wifiChannel3;
        wifiChannel3.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
        wifiChannel3.AddPropagationLoss ("ns3::LogDistancePropagationLossModel" , "ReferenceDistance", DoubleValue (10.0));
        wifiPhy.SetChannel (wifiChannel3.Create ());
 
        Ssid ssid3 = Ssid ("center");
        WifiHelper wifi3;
        
        wifi3.SetRemoteStationManager ("ns3::ArfWifiManager");
 

        
        ipv4.SetBase (add_comb, "255.255.255.0");
  

        if (multiple_aps == "yes"){
          wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid3));
          devices2 = wifi3.Install (wifiPhy, wifiMac, NodeContainer (nodesSTA.Get(1), nodesSTA.Get(2), nodesSTA.Get(3))); 
  
 
          wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid3),
                   "ActiveProbing", BooleanValue (false));
          devices2.Add (wifi3.Install (wifiPhy, wifiMac, nodesCars)); 
          Ipv4InterfaceContainer interfacesLeft3 = ipv4.Assign (devices2);
          std::cout << "multiple APs" << std::endl;
        }
        else {
          wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid3));
          devices2 = wifi3.Install (wifiPhy, wifiMac, nodesSTA.Get(2)); 
  
 
          wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid3),
                   "ActiveProbing", BooleanValue (false));
          devices2.Add (wifi3.Install (wifiPhy, wifiMac, nodesCars.Get(1))); 
          Ipv4InterfaceContainer interfacesLeft3 = ipv4.Assign (devices2);
          std::cout << "single AP" << std::endl;
      }

      

      return;
        
      }
      

    };




int 
main (int argc, char *argv[])
{
  std::string moving = "no";
  std::string mobility_file; // contains information about node movement and placement
  std::string mode = "ConfigureLocal"; //tap mode
  std::string tapName = "thetap";
  std::string tapName2 = "thetap2";
  std::string udpgen = "no";
  std::string olsrprot = "no";
  std::string multiple_aps = "no";
  std::string int_to_int = "no";



  

  CommandLine cmd (__FILE__);
 
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.AddValue ("tapName2", "Name of the OS tap device", tapName2);
  cmd.AddValue ("udpgen", "Generates outbound UDP traffic", udpgen);
  cmd.AddValue ("moving", "Generates moving nodes", moving);
  cmd.AddValue ("olsrprot", "OLSR protocol is used", olsrprot);
  cmd.AddValue ("multiple_aps", "Multiple APs for multiple STAs", multiple_aps);
  cmd.AddValue ("int_to_int", "interfaces will be able to communicate", int_to_int);
  cmd.Parse (argc, argv);
 
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl")); //to make sure that the simulation is real time
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true)); //to make sure that the simulation is real time
  Config::SetDefault ("ns3::ArpCache::DeadTimeout", TimeValue (Seconds (1))); //resending ARP packets !!! otherwise the default is 1000s which is too much and the resend never happens in case an ARP request gets lost
  
  if (moving == "yes"){
    mobility_file = "/home/jakub/sumo/tools/2021-11-01-11-08-58/final_3_aps.tcl";
    std::cout << "moving vehicles" << std::endl;
  }
  else{
    mobility_file = "/home/jakub/sumo/tools/2021-11-01-11-08-58/final_3_aps_simple.tcl";
    std::cout << "stationary vehicles" << std::endl;
  }
  

  YansWifiPhyHelper wifiPhy;
  
  int num_of_nets;
  num_of_nets = 2;
  Com_group net_obj[num_of_nets];

  for (int i = 0; i < num_of_nets; i++){ 
    if (i == 0){
      net_obj[i].set_options(olsrprot, mode, tapName, multiple_aps, "10.1.1.0", "10.1.3.0", "10.1.2.0"); 
    }
    else{
        net_obj[i].set_options(olsrprot, mode, tapName2, multiple_aps, "10.1.4.0", "10.1.6.0", "10.1.5.0"); 

    }
    wifiPhy = net_obj[i].highway_points_setup();
    net_obj[i].tap_setup();
    net_obj[i].cars_setup();
    net_obj[i].car_highway_comm_setup();

  }

  if (int_to_int == "yes"){ //connects both TAP interfaces through the simulation
        std::cout << "all nodes connected" << std::endl;
        NodeContainer car_br0;
        NodeContainer car_br1;
        car_br0 = net_obj[0].get_car();
        car_br1 = net_obj[1].get_car();
        NodeContainer cars_cross(car_br0.Get(0), car_br1.Get(0));

        WifiHelper wifi5;
        WifiMacHelper mac5;
        mac5.SetType ("ns3::AdhocWifiMac");
        wifi5.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue ("OfdmRate54Mbps"));
        YansWifiPhyHelper wifiPhy5;
        wifiPhy5.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
        YansWifiChannelHelper wifiChannel5;
        wifiChannel5.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
        wifiChannel5.AddPropagationLoss ("ns3::RangePropagationLossModel" , "MaxRange", DoubleValue (4000.0));
        wifiPhy5.SetChannel (wifiChannel5.Create ());
        NetDeviceContainer backboneDevices5 = wifi5.Install (wifiPhy5, mac5, cars_cross);
  
   
        InternetStackHelper internet;
        if (olsrprot == "yes"){
            OlsrHelper olsr5;
            internet.SetRoutingHelper (olsr5); 
        }
        
 
        Ipv4AddressHelper ipAddrs5;
        ipAddrs5.SetBase ("10.1.7.0", "255.255.255.0");
        ipAddrs5.Assign (backboneDevices5);

  }

  
  if (udpgen == "yes"){
    std::cout << "UDP generator on" << std::endl;
    Ptr<Node> appSource = NodeList::GetNode (13);
    OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress ("10.1.4.1", 9))); //creates udp packets and sends them to 10.1.4.1, port 9
    ApplicationContainer apps = onoff.Install (appSource);
    apps.Start (Seconds (3));
    apps.Stop (Seconds (100.)); //the length of simulation
  }
  
  

  Ns2MobilityHelper sumo_trace(mobility_file); 
  sumo_trace.Install();
 
 

  wifiPhy.EnablePcapAll ("final"); 

  AnimationInterface anim ("final.xml");

  if (olsrprot != "yes"){
    Ipv4GlobalRoutingHelper::PopulateRoutingTables (); //global routing instead of olsr
  }

 
  Simulator::Stop (Seconds (100.));
  Simulator::Run ();
  Simulator::Destroy ();
}