/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
// Network topology
//
//  +----------+
//  | external |
//  |  Linux   |
//  |   Host   |
//  |          |
//  | "mytap"  |
//  +----------+
//       |           n0                n3                n4
//       |       +--------+     +------------+     +------------+
//       +-------|  tap   |     |            |     |            |
//               | bridge | ... |            |     |            |
//               +--------+     +------------+     +------------+
//               |  Wifi  |     | Wifi | P2P |-----| P2P | CSMA |
//               +--------+     +------+-----+     +-----+------+
//                   |              |           ^           |
//                 ((*))          ((*))         |           |
//                                          P2P 10.1.2      |
//                 ((*))          ((*))                     |    n5  n6   n7
//                   |              |                       |     |   |    |
//                  n1             n2                       ================
//                     Wifi 10.1.1                           CSMA LAN 10.1.3
//
// The Wifi device on node zero is:  10.1.1.1
// The Wifi device on node one is:   10.1.1.2
// The Wifi device on node two is:   10.1.1.3
// The Wifi device on node three is: 10.1.1.4
// The P2P device on node three is:  10.1.2.1
// The P2P device on node four is:   10.1.2.2
// The CSMA device on node four is:  10.1.3.1
// The CSMA device on node five is:  10.1.3.2
// The CSMA device on node six is:   10.1.3.3
// The CSMA device on node seven is: 10.1.3.4
//
// Some simple things to do:
//
// 1) Ping one of the simulated nodes on the left side of the topology.
//
//    ./ns3 run tap-wifi-dumbbell&
//    ping 10.1.1.3
//
// 2) Configure a route in the linux host and ping once of the nodes on the 
//    right, across the point-to-point link.  You will see relatively large
//    delays due to CBR background traffic on the point-to-point (see next
//    item).
//
//    ./ns3 run tap-wifi-dumbbell&
//    sudo route add -net 10.1.3.0 netmask 255.255.255.0 dev thetap gw 10.1.1.2
//    ping 10.1.3.4
//
//    Take a look at the pcap traces and note that the timing reflects the 
//    addition of the significant delay and low bandwidth configured on the 
//    point-to-point link along with the high traffic.
//
// 3) Fiddle with the background CBR traffic across the point-to-point 
//    link and watch the ping timing change.  The OnOffApplication "DataRate"
//    attribute defaults to 500kb/s and the "PacketSize" Attribute defaults
//    to 512.  The point-to-point "DataRate" is set to 512kb/s in the script,
//    so in the default case, the link is pretty full.  This should be 
//    reflected in large delays seen by ping.  You can crank down the CBR 
//    traffic data rate and watch the ping timing change dramatically.
//
//    ./ns3 run "tap-wifi-dumbbell --ns3::OnOffApplication::DataRate=100kb/s"&
//    sudo route add -net 10.1.3.0 netmask 255.255.255.0 dev thetap gw 10.1.1.2
//    ping 10.1.3.4
//
// 4) Try to run this in UseBridge mode.  This allows you to bridge an ns-3
//    simulation to an existing pre-configured bridge.  This uses tap devices
//    just for illustration, you can create your own bridge if you want.
//
//    sudo tunctl -t mytap1
//    sudo ifconfig mytap1 0.0.0.0 promisc up
//    sudo tunctl -t mytap2
//    sudo ifconfig mytap2 0.0.0.0 promisc up
//    sudo brctl addbr mybridge
//    sudo brctl addif mybridge mytap1
//    sudo brctl addif mybridge mytap2
//    sudo ifconfig mybridge 10.1.1.5 netmask 255.255.255.0 up
//    ./ns3 run "tap-wifi-dumbbell --mode=UseBridge --tapName=mytap2"&
//    ping 10.1.1.3
 
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








 //zrus olsr ak chces ping, urob one-to-one comm
int 
main (int argc, char *argv[])//move 0 and 1 closer together, check these node separations separately if taht does not work
{
  //13-2pcap has a strange long distance communication
  std::string moving = "no";
  std::string mobility_file; //odtialto berieme pohyb vozidiel(nodes)
  std::string mode = "ConfigureLocal";
  std::string tapName = "thetap";
  std::string tapNamed = "thetap2";
  std::string udpgen = "no";
  std::string olsrprot = "no";
  std::string multiple_aps = "no";


  

  CommandLine cmd (__FILE__);
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.AddValue ("udpgen", "Generates outbound UDP traffic", udpgen);
  cmd.AddValue ("moving", "Generates moving nodes", moving);
  cmd.AddValue ("olsrprot", "OLSR protocol is used", olsrprot);
  cmd.AddValue ("multiple_aps", "Multiple APs for multiple STAs", multiple_aps);

  cmd.Parse (argc, argv);
 
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  
  if (moving == "yes"){
    mobility_file = "/home/jakub/sumo/tools/2021-11-01-11-08-58/final_2_aps.tcl";
    std::cout << "movy" << std::endl;
  }
  else{
    mobility_file = "/home/jakub/sumo/tools/2021-11-01-11-08-58/final_2_aps_simple.tcl";
    std::cout << "movn" << std::endl;
  }
  
  //
  // The topology has a Wifi network of four nodes on the left side.  We'll make
  // node zero the AP and have the other three will be the STAs.
  //
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  NodeContainer nodesLeftAP; //test for a different IP e.g. 10.1.1.3
  nodesLeftAP.Create (1);
  
  NodeContainer nodesLeftSTA;
  nodesLeftSTA.Create (4);

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel" , "MaxRange", DoubleValue (4000.0));
  wifiPhy.SetChannel (wifiChannel.Create ());
 
  Ssid ssid = Ssid ("left");
  WifiHelper wifi;
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
 
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer devicesLeft = wifi.Install (wifiPhy, wifiMac, NodeContainer (nodesLeftAP.Get (0)));
 
 
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
  devicesLeft.Add (wifi.Install (wifiPhy, wifiMac, nodesLeftSTA));


  NodeContainer nodesLeft (nodesLeftAP, nodesLeftSTA);


  //22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
  NodeContainer nodesLeftAPd; //test for a different IP e.g. 10.1.1.3
  nodesLeftAPd.Create (1);
  
  NodeContainer nodesLeftSTAd;
  nodesLeftSTAd.Create (4);

  YansWifiPhyHelper wifiPhyd;
  YansWifiChannelHelper wifiChanneld;
  wifiChanneld.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChanneld.AddPropagationLoss ("ns3::RangePropagationLossModel" , "MaxRange", DoubleValue (4000.0));
  wifiPhyd.SetChannel (wifiChanneld.Create ());
 
  Ssid ssidd = Ssid ("leftd");
  WifiHelper wifid;
  WifiMacHelper wifiMacd;
  wifid.SetRemoteStationManager ("ns3::ArfWifiManager");
 
  wifiMacd.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssidd));
  NetDeviceContainer devicesLeftd = wifid.Install (wifiPhyd, wifiMacd, NodeContainer (nodesLeftAPd.Get (0)));
 
 
  wifiMacd.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssidd),
                   "ActiveProbing", BooleanValue (false));
  devicesLeftd.Add (wifid.Install (wifiPhyd, wifiMacd, nodesLeftSTAd));


  NodeContainer nodesLeftd (nodesLeftAPd, nodesLeftSTAd);
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
  MobilityHelper mobility2;
   mobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   mobility2.Install (nodesLeft);
   */
  /*
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (20.0),
                                  "MinY", DoubleValue (20.0),
                                  "DeltaX", DoubleValue (20.0),
                                  "DeltaY", DoubleValue (20.0),
                                  "GridWidth", UintegerValue (5),
                                  "LayoutType", StringValue ("RowFirst"));
   mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                              "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
                              "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                              "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
   //mobility.Install (nodesRight);
  mobility.Install (nodesLeft);
  */

  
  InternetStackHelper internetLeft;
  
  

  if (olsrprot == "yes"){
      OlsrHelper olsr;
      internetLeft.SetRoutingHelper (olsr);
      std::cout << "olsry" << std::endl;
  }
  internetLeft.Install (nodesLeft);
 
  Ipv4AddressHelper ipv4Left;
  ipv4Left.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesLeft = ipv4Left.Assign (devicesLeft);
  
  std::cout << interfacesLeft.GetAddress (1) << std::endl;
  
  TapBridgeHelper tapBridge (interfacesLeft.GetAddress (1)); //1 is 1.1.2
  tapBridge.SetAttribute ("Mode", StringValue (mode));
  tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
  tapBridge.Install (nodesLeft.Get (0), devicesLeft.Get (0));
  Ptr<Node> appSink = nodesLeft.Get (0);//1.1.1
  Ipv4Address remoteAddr = appSink->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
  std::cout <<  remoteAddr << std::endl;
  //
  // Now, create the right side.
  //
  //2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
  
  InternetStackHelper internetLeftd;
  if (olsrprot == "yes"){
      OlsrHelper olsrd;
      internetLeft.SetRoutingHelper (olsrd);
  }
  
  internetLeft.Install (nodesLeftd);//zmena
 
  Ipv4AddressHelper ipv4Leftd;
  ipv4Leftd.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesLeftd = ipv4Leftd.Assign (devicesLeftd);
  
  std::cout << interfacesLeftd.GetAddress (1) << std::endl;
  
  TapBridgeHelper tapBridged (interfacesLeftd.GetAddress (1)); //funguje, ale nie ping
  tapBridged.SetAttribute ("Mode", StringValue (mode));
  tapBridged.SetAttribute ("DeviceName", StringValue (tapNamed));
  tapBridged.Install (nodesLeftd.Get (0), devicesLeftd.Get (0));
  Ptr<Node> appSinkd = nodesLeftd.Get (0);//1.1.1
  Ipv4Address remoteAddrd = appSinkd->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
  std::cout <<  remoteAddrd << std::endl;

//////////////////////////////////////////////////////////////////////////////////////////////////
   NodeContainer nodesRight;
   nodesRight.Create (2);
   //
   // Create the backbone wifi net devices and install them into the nodes in
   // our container
   //
   WifiHelper wifi2;
   WifiMacHelper mac2;
   mac2.SetType ("ns3::AdhocWifiMac");
   wifi2.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue ("OfdmRate54Mbps"));
   YansWifiPhyHelper wifiPhy2;
   wifiPhy2.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
   YansWifiChannelHelper wifiChannel2;
   wifiChannel2.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   wifiChannel2.AddPropagationLoss ("ns3::LogDistancePropagationLossModel" , "ReferenceDistance", DoubleValue (10.0));
   wifiPhy2.SetChannel (wifiChannel2.Create ());
   NetDeviceContainer backboneDevices2 = wifi.Install (wifiPhy2, mac2, nodesRight);
  
   // We enable OLSR (which will be consulted at a higher priority than
   // the global routing) on the backbone ad hoc nodes
   //NS_LOG_INFO ("Enabling OLSR routing on all backbone nodes");

   //OlsrHelper olsr2;
   //
   // Add the IPv4 protocol stack to the nodes in our container
   //
   InternetStackHelper internet;
   if (olsrprot == "yes"){
      OlsrHelper olsr2;
      internet.SetRoutingHelper (olsr2); // has effect on the next Install ()
    }
   //internet.SetRoutingHelper (olsr2); // has effect on the next Install ()
   internet.Install (nodesRight);
  
   //
   // Assign IPv4 addresses to the device drivers (actually to the associated
   // IPv4 interfaces) we just created.
   //
   Ipv4AddressHelper ipAddrs2;
   ipAddrs2.SetBase ("10.1.3.0", "255.255.255.0");
   ipAddrs2.Assign (backboneDevices2);
   //22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
   NodeContainer nodesRightd;
   nodesRightd.Create (2);
   //
   // Create the backbone wifi net devices and install them into the nodes in
   // our container
   //
   WifiHelper wifi2d;
   WifiMacHelper mac2d;
   mac2d.SetType ("ns3::AdhocWifiMac");
   wifi2d.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue ("OfdmRate54Mbps"));
   YansWifiPhyHelper wifiPhy2d;
   wifiPhy2d.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
   YansWifiChannelHelper wifiChannel2d;
   wifiChannel2d.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   wifiChannel2d.AddPropagationLoss ("ns3::LogDistancePropagationLossModel" , "ReferenceDistance", DoubleValue (10.0));
   wifiPhy2d.SetChannel (wifiChannel2d.Create ());
   NetDeviceContainer backboneDevices2d = wifid.Install (wifiPhy2d, mac2d, nodesRightd);
  
   // We enable OLSR (which will be consulted at a higher priority than
   // the global routing) on the backbone ad hoc nodes
   //NS_LOG_INFO ("Enabling OLSR routing on all backbone nodes");
   //OlsrHelper olsr2d;
   //
   // Add the IPv4 protocol stack to the nodes in our container
   //
   InternetStackHelper internetd;
   if (olsrprot == "yes"){
      OlsrHelper olsr2d;
      internetd.SetRoutingHelper (olsr2d); // has effect on the next Install ()
    }
   //internetd.SetRoutingHelper (olsr2d); // has effect on the next Install ()
   internetd.Install (nodesRightd);
  
   //
   // Assign IPv4 addresses to the device drivers (actually to the associated
   // IPv4 interfaces) we just created.
   //
   Ipv4AddressHelper ipAddrs2d;
   ipAddrs2d.SetBase ("10.1.6.0", "255.255.255.0");
   ipAddrs2d.Assign (backboneDevices2d);
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
   MobilityHelper mobility;
   mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   mobility.Install (nodesRight);
   */
   /*
   MobilityHelper mobility2;
   mobility2.SetPositionAllocator ("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (20.0),
                                  "MinY", DoubleValue (20.0),
                                  "DeltaX", DoubleValue (20.0),
                                  "DeltaY", DoubleValue (20.0),
                                  "GridWidth", UintegerValue (5),
                                  "LayoutType", StringValue ("RowFirst"));
   mobility2.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                              "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
                              "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                              "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
   mobility2.Install (nodesRight);
   */
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  //does not require a
  YansWifiPhyHelper wifiPhy3; //try also point-to-point
  YansWifiChannelHelper wifiChannel3;
  wifiChannel3.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel3.AddPropagationLoss ("ns3::LogDistancePropagationLossModel" , "ReferenceDistance", DoubleValue (10.0));
  wifiPhy3.SetChannel (wifiChannel3.Create ());
 
  Ssid ssid3 = Ssid ("center");
  WifiHelper wifi3;
  WifiMacHelper wifiMac3;
  wifi3.SetRemoteStationManager ("ns3::ArfWifiManager");
 

  Ipv4AddressHelper ipv4Left3;
  ipv4Left3.SetBase ("10.1.2.0", "255.255.255.0");
  

  if (multiple_aps == "yes"){
    wifiMac3.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid3));
    NetDeviceContainer devicesLeft3 = wifi3.Install (wifiPhy3, wifiMac3, NodeContainer (nodesLeftSTA.Get(1), nodesLeftSTA.Get(2), nodesLeftSTA.Get(3))); //nodesLeft.Get(3)
  
 
    wifiMac3.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid3),
                   "ActiveProbing", BooleanValue (false));
    devicesLeft3.Add (wifi3.Install (wifiPhy3, wifiMac3, nodesRight)); //ARP posiela len na prvy node v skupine??
    Ipv4InterfaceContainer interfacesLeft3 = ipv4Left3.Assign (devicesLeft3);
    std::cout << "apy" << std::endl;
  }
  else {
    wifiMac3.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid3));
    NetDeviceContainer devicesLeft3 = wifi3.Install (wifiPhy3, wifiMac3, nodesLeftSTA.Get(2)); //nodesLeft.Get(3)
  
 
    wifiMac3.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid3),
                   "ActiveProbing", BooleanValue (false));
    devicesLeft3.Add (wifi3.Install (wifiPhy3, wifiMac3, nodesRight.Get(1))); //ARP posiela len na prvy node v skupine??
    Ipv4InterfaceContainer interfacesLeft3 = ipv4Left3.Assign (devicesLeft3);
    std::cout << "apn" << std::endl;
  }
  

  

  //2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
  YansWifiPhyHelper wifiPhy3d; //try also point-to-point
  YansWifiChannelHelper wifiChannel3d;
  wifiChannel3d.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel3d.AddPropagationLoss ("ns3::LogDistancePropagationLossModel" , "ReferenceDistance", DoubleValue (10.0));
  wifiPhy3d.SetChannel (wifiChannel3d.Create ());
 
  Ssid ssid3d = Ssid ("centerd");
  WifiHelper wifi3d;
  WifiMacHelper wifiMac3d;
  wifi3d.SetRemoteStationManager ("ns3::ArfWifiManager");

  Ipv4AddressHelper ipv4Left3d;
  ipv4Left3d.SetBase ("10.1.5.0", "255.255.255.0");

  if (multiple_aps == "yes"){
    wifiMac3d.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid3d));
    NetDeviceContainer devicesLeft3d = wifi3d.Install (wifiPhy3d, wifiMac3d, NodeContainer(nodesLeftSTAd.Get(1), nodesLeftSTAd.Get(2), nodesLeftSTAd.Get(3))); //nodesLeft.Get(3)
 
 
    wifiMac3d.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid3d),
                   "ActiveProbing", BooleanValue (false));
    devicesLeft3d.Add (wifi3d.Install (wifiPhy3d, wifiMac3d, nodesRightd)); //ARP posiela len na prvy node v skupine??

  
    Ipv4InterfaceContainer interfacesLeft3d = ipv4Left3d.Assign (devicesLeft3d);
    std::cout << "apy" << std::endl;

  }
  else{
    wifiMac3d.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid3d));
    NetDeviceContainer devicesLeft3d = wifi3d.Install (wifiPhy3d, wifiMac3d, nodesLeftSTAd.Get(1)); //nodesLeft.Get(3)
 
 
    wifiMac3d.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid3d),
                   "ActiveProbing", BooleanValue (false));
    devicesLeft3d.Add (wifi3d.Install (wifiPhy3d, wifiMac3d, nodesRightd.Get(0))); //ARP posiela len na prvy node v skupine??

  
    Ipv4InterfaceContainer interfacesLeft3d = ipv4Left3d.Assign (devicesLeft3d);
    std::cout << "apn" << std::endl;
  }

  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /*
  YansWifiPhyHelper wifiPhy4; //try also point-to-point
  YansWifiChannelHelper wifiChannel4;
  wifiChannel4.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel4.AddPropagationLoss ("ns3::LogDistancePropagationLossModel" , "ReferenceDistance", DoubleValue (10.0));
  wifiPhy4.SetChannel (wifiChannel4.Create ());
 
  Ssid ssid4 = Ssid ("center4");
  WifiHelper wifi4;
  WifiMacHelper wifiMac4;
  wifi4.SetRemoteStationManager ("ns3::ArfWifiManager");
 
  wifiMac4.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid4));
  NetDeviceContainer devicesLeft4 = wifi4.Install (wifiPhy4, wifiMac4, nodesLeftSTA.Get(2)); //nodesLeft.Get(3)
 
 
  wifiMac4.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid4),
                   "ActiveProbing", BooleanValue (false));
  devicesLeft4.Add (wifi4.Install (wifiPhy4, wifiMac4, nodesRight.Get(3))); //ARP posiela len na prvy node v skupine??

  Ipv4AddressHelper ipv4Left4;
  ipv4Left4.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesLeft4 = ipv4Left4.Assign (devicesLeft4);
  */
  /*
  NodeContainer nodesRight;
  nodesRight.Create (4);
 
  CsmaHelper csmaRight;
  csmaRight.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csmaRight.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
 
  NetDeviceContainer devicesRight = csmaRight.Install (nodesRight);
 
  InternetStackHelper internetRight;
  internetRight.Install (nodesRight);
 
  Ipv4AddressHelper ipv4Right;
  ipv4Right.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesRight = ipv4Right.Assign (devicesRight);
  */
  //
  // Stick in the point-to-point line between the sides.
  //
  //------------------------------------------------------------------
  /*
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("512kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
 
  NodeContainer nodes = NodeContainer (nodesLeft.Get (3), nodesRight.Get (0));
  NetDeviceContainer devices = p2p.Install (nodes);
 
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.2.0", "255.255.255.192");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  */
  //--------------------------------------------------------------------
  //////////////////////////////////////////////////////////////////////////
  //fungujuca cast pre posielani packetov von zo simulacie
  if (udpgen == "yes"){
    std::cout << "here" << std::endl;
    Ptr<Node> appSource = NodeList::GetNode (13);
    OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress ("10.1.4.1", 9))); //otestuj aj s appsink ak toto nepojde
    ApplicationContainer apps = onoff.Install (appSource);
    apps.Start (Seconds (3));
    apps.Stop (Seconds (100.)); //dlzka simulacie
  }
  
  
  
  ///////////////////////////////////////////////////////////////////////////
  Ns2MobilityHelper sumo_trace(mobility_file); //instaluje sa mobility file
  sumo_trace.Install();
 
 

  wifiPhy.EnablePcapAll ("final_tap");
  wifiPhy3.EnablePcapAll ("final_apsta");
  wifiPhy2.EnablePcapAll ("final_adhoc");
  AnimationInterface anim ("final.xml");
  //anim.SetConstantPosition(nodesLeft.Get(0), 0.0, 0.0);
  //anim.SetConstantPosition(nodesLeft.Get(1), 0.0, 0.0);
  //anim.SetConstantPosition(nodesLeft.Get(2), 0.0, 0.0);
  //anim.SetConstantPosition(nodesLeft.Get(3), 2.0, 2.0);
  //anim.SetConstantPosition(nodesLeft.Get(4), 2.5, 2.5);
  //anim.SetConstantPosition(nodesRight.Get(0), 4.0, 4.0);
  //anim.SetConstantPosition(nodesRight.Get(1), 5.0, 5.0);
  //anim.SetConstantPosition(nodesRight.Get(2), 6.0, 6.0);
  //anim.SetConstantPosition(nodesRight.Get(3), 7.0, 7.0);

  //csmaRight.EnablePcapAll ("tap-wifi-dumbbell", false);
  if (olsrprot == "no"){
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  }

 
  Simulator::Stop (Seconds (100.));
  Simulator::Run ();
  Simulator::Destroy ();
}