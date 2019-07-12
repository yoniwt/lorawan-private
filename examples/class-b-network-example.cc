/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "ns3/end-device-lora-phy.h"
#include "ns3/lora-class-b-analyzer.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/end-device-class-b-app-helper.h"
#include "ns3/command-line.h"
#include "ns3/network-server-helper.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"
#include "ns3/forwarder-helper.h"
#include "ns3/bcn-payload.h"
#include "src/lorawan/helper/class-b/lora-class-b-analyzer.h"
#include <algorithm>
#include <ctime>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE ("ClassBLorawanNetworkExample");

// Network settings
int nUcDevices = 0;//10;
int nMcDevices = 6;//6;//20;
int nMcDevicesPerGroup = 6;//3;//2;

bool mixOfDrs = false; //Whether to have mixed Dr in simulation for different multicast groups

int dr = 3; //Dr to be used for all the multicast groups created, if mixOfDrs = false

int nMcDr0 = 0; // Number of multicast groups that use Dr0, if mixOfDrs = true
int nMcDr1 = 0; // Number of multicast groups that use Dr1, if mixOfDrs = true
int nMcDr2 = 0; // Number of multicast groups that use Dr2, if mixOfDrs = true
int nMcDr3 = 0; // Number of multicast groups that use Dr3, if mixOfDrs = true
int nMcDr4 = 0; // Number of multicast groups that use Dr4, if mixOfDrs = true
int nMcDr5 = 0; // Number of multicast groups that use Dr5, if mixOfDrs = true

bool mixOfPeriodicity = false; //Whether to have mixed Periodicity in simulation for different multicast groups

int periodicity = 0; //periodicity to be used for all the multicast groups created, if mixOfPeriodicity = false

int nMcPeriodicity0 = 0; // Number of multicast groups that use ping slot periodicity 0, if mixOfPeriodicity = true
int nMcPeriodicity1 = 0; // Number of multicast groups that use ping slot periodicity 1, if mixOfPeriodicity = true
int nMcPeriodicity2 = 0; // Number of multicast groups that use ping slot periodicity 2, if mixOfPeriodicity = true
int nMcPeriodicity3 = 0; // Number of multicast groups that use ping slot periodicity 3, if mixOfPeriodicity = true
int nMcPeriodicity4 = 0; // Number of multicast groups that use ping slot periodicity 4, if mixOfPeriodicity = true
int nMcPeriodicity5 = 0; // Number of multicast groups that use ping slot periodicity 5, if mixOfPeriodicity = true
int nMcPeriodicity6 = 0; // Number of multicast groups that use ping slot periodicity 6, if mixOfPeriodicity = true
int nMcPeriodicity7 = 0; // Number of multicast groups that use ping slot periodicity 7, if mixOfPeriodicity = true

int nGateways = 1;

int nBeaconGateways = 1; //For now only this one is used // all gateways placed at the same location
int nClassBGateways = 1; 

double radius = 7500;
double simulationTime = 86400;//1 day //2400;

// Channel model
bool realisticChannelModel = false;//true;

int appPeriodSeconds = 100;//600; 

bool enableUplink = false; // For enabling uplinks for devices 
int delayToSwitchToClassB = 100; //Period of uplink after switching to class B
int periodsToSimulate = 1;
int transientPeriods = 0;



// Output control
bool print = true;
bool append = false; // append the new simulation file to existing file
bool addInfoOnFileName = true; // add the simulation setup information on the file name
int filePostFix = 0; // A post fix to be appended at the end of the simulation, used if addInfoOnFileName is false

int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.AddValue ("realisticChannelModel",
                "Include shadowing loss and building penetration loss",
                realisticChannelModel);  
  cmd.AddValue ("nUcDevices",
                "Number of unicast class B end devices to include in the simulation",
                nUcDevices);
  cmd.AddValue ("nMcDevices",
                "Number of multicast end devices to include in the simulation",
                nMcDevices);
  cmd.AddValue ("nMcDevicesPerGroup",
                "Number of multicast end devices per multicast group",
                nMcDevicesPerGroup);  
  cmd.AddValue ("mixOfDrs",
                "Whether to have mixed Dr in simulation for different multicast groups, default = false",
                mixOfDrs);
  cmd.AddValue ("dr",
                "Dr to be used for all the multicast groups created if mixOfDrs is false, default = 3",
                dr);
  cmd.AddValue ("mixOfPeriodicity",
                "Whether to have mixed Periodicity in simulation for different multicast groups, default = false",
                mixOfPeriodicity);
  cmd.AddValue ("periodicity",
                "ping slot periodicity to be used for all the multicast groups created, if mixOfPeriodicity = false, default = 0",
                periodicity);  
  cmd.AddValue ("radius",
                "The radius of the area to simulate",
                radius);
  cmd.AddValue ("nBeaconGateways",
                "The number of gateways that are placed both for Beacon transmission (for now also for class B downlink and Class A operation also",
                nBeaconGateways);
  cmd.AddValue ("simulationTime",
                "The time for which to simulate",
                simulationTime);
  cmd.AddValue ("appPeriod",
                "The period in seconds to be used by periodically transmitting applications after switching to Class B",
                appPeriodSeconds);
  cmd.AddValue ("print",
                "Whether or not to print various informations",
                print);
  cmd.AddValue ("append",
                "append the new simulation file to existing file",
                append);
  cmd.AddValue ("addInfoOnFileName",
                "add the simulation setup information on the file name",
                addInfoOnFileName);  
  cmd.AddValue ("filePostFix",
                "A post fix to be appended at the end of the simulation, used if addInfoOnFileName is false",
                filePostFix);    
  
  //Also add argument of class B paramters
  cmd.Parse (argc, argv);

  
  // Set up logging
  //-LogComponentEnable ("ClassBLorawanNetworkExample", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
  // LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
  //-LogComponentEnable ("EndDeviceLoraPhy", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  // LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
  //-LogComponentEnable ("LoraMac", LOG_LEVEL_ALL);
  //-LogComponentEnable ("EndDeviceLoraMac", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  //-LogComponentEnable ("GatewayLoraMac", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  // LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
   //LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraMacHelper", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  // LogComponentEnable ("PeriodicSenderHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("PeriodicSender", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraMacHeader", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);
  // LogComponentEnable ("NetworkScheduler", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  // LogComponentEnable ("NetworkServer", LOG_LEVEL_ALL);
  //-LogComponentEnable ("NetworkStatus", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  // LogComponentEnable ("NetworkController", LOG_LEVEL_ALL);
  //-LogComponentEnable ("BcnPayload", LOG_LEVEL_ALL);
  // LogComponentEnable ("EndDeviceClassBAppHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("EndDeviceClassBApp", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  // LogComponentEnable ("SimpleGatewayLoraPhy", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  // LogComponentEnable ("SimpleEndDeviceLoraPhy", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  // LogComponentEnable ("LoraClassBAnalyzer", (LogLevel)(LOG_PREFIX_TIME | LOG_LEVEL_ALL));
  
  /*********************
   * Check user inputs *
   *********************/
  NS_ASSERT (dr <= 5 && dr >= 0);
  
  /***********
   *  Setup  *
   ***********/

  // Create the time value from the period
  Time appPeriod = Seconds (appPeriodSeconds);

  // Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "rho", DoubleValue (radius),
                                 "X", DoubleValue (0.0),
                                 "Y", DoubleValue (0.0));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  /************************
   *  Create the channel  *
   ************************/

  // Create the lora channel object
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent (3.76);
  loss->SetReference (1, 7.7);
  
  if (realisticChannelModel)
    {
      // Create the correlated shadowing component
      Ptr<CorrelatedShadowingPropagationLossModel> shadowing = CreateObject<CorrelatedShadowingPropagationLossModel> ();

      // Aggregate shadowing to the logdistance loss
      loss->SetNext (shadowing);

      // Add the effect to the channel propagation loss
      Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss> ();

      shadowing->SetNext (buildingLoss);
    }

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  /************************
   *  Create the helpers  *
   ************************/

  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LoraMacHelper
  LoraMacHelper macHelper = LoraMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();
  helper.EnablePacketTracking ("performance"); // Output filename
  // helper.EnableSimulationTimePrinting ();

  //Create the NetworkServerHelper
  NetworkServerHelper nsHelper = NetworkServerHelper ();

  //Create the ForwarderHelper
  ForwarderHelper forHelper = ForwarderHelper ();

  /************************
   *  Create End Devices  *
   ************************/

  // Container containing all the end devices
  NodeContainer endDevices;
  
  //Manual creation of multicast groups 
//  //Uncomment if the automatic is commented
//  // Create multicast end nodes
//  NodeContainer mcEndDevices;
//  mcEndDevices.Create (nMcDevicesPerGroup);
//  
//  // Create multicast 2 end nodes
//  NodeContainer mcEndDevices2;
//  mcEndDevices2.Create (nMcDevicesPerGroup);
  
  //Automatic creation of multicast groups
  //Comment if the manual is uncommented
  
  NodeContainer mcTotalEndDevices;
  
  if (nMcDevices > 0)
    {
      mcTotalEndDevices.Create (nMcDevices);
      endDevices.Add (mcTotalEndDevices);    
    }  
  
  // Create unicast end nodes
  NodeContainer ucEndDevices;  
  
  if (nUcDevices > 0)
    {
      ucEndDevices.Create (nUcDevices);
      endDevices.Add (ucEndDevices);    
    }

//  // Uncomment this if the manual multicast group creation is uncommented
//  endDevices.Add (mcEndDevices);
//  endDevices.Add (mcEndDevices2);
  
  // Comment this if the automatic group creation is uncommented


  // Assign a mobility model to each node
  mobility.Install (endDevices);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j)
    {
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      position.z = 1.2;
      mobility->SetPosition (position);
    }

  // Create the LoraNetDevices of the end devices
  uint8_t nwkId = 54;
  uint32_t nwkAddr = 1864;
  Ptr<LoraDeviceAddressGenerator> addrGen = CreateObject<LoraDeviceAddressGenerator> (nwkId,nwkAddr);

  // Create the LoraNetDevices of the end devices
  macHelper.SetAddressGenerator (addrGen);
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  helper.Install (phyHelper, macHelper, endDevices);

  // Now end devices are connected to the channel

  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
      //ClassBdownlink reception tracesources
    }

  /*********************
   *  Create Gateways  *
   *********************/
  
  // Create the Beacon Transmitting gateways
  NodeContainer beaconingGateways;
  beaconingGateways.Create (nBeaconGateways);
  
  // Create the ClassBdownlink transmitting gateways
  // Or Add if they are the same with beaconing ones 
  NodeContainer classBGateways;
  //ClassBGateways.Create (nClassBGateways);
  classBGateways.Add (beaconingGateways);
  
  // Collect all gateway nodes
  NodeContainer gateways;
  gateways.Add (beaconingGateways);
  //gateways.Add (classBGateways); // if they were different

  // Place it at the center of the Disc you allocated (0,0)
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  // Make it so that nodes are at a certain height > 0
  allocator->Add (Vector (0.0, 0.0, 15.0));
  //allocator->Add (  ) //if you have more disc or locations on a disc to place them 
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);


  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);
  
  //EnableBeaconTransmission and class B downlink transmissions on selected gateways
  macHelper.EnableBeaconTransmission (beaconingGateways);
  macHelper.EnableClassBDownlinkTransmission (classBGateways);

  /***********************
   * Creating multicast 
   ***********************/
  //Now configure for Class B multicast
  //Pass in a container the gateways which serve a particular multicast group
  //For now, its one gateway serving all
  
  //Select multicast and unicast devices
  //LoraDeviceAddress mcDevAddr1 = macHelper.CreateMulticastGroup (mcEndDevices);
//  macHelper.CreateMulticastGroup (mcEndDevices, classBGateways);
//    macHelper.CreateMulticastGroup (mcEndDevices, classBGateways, 3, 0);

  //Create Another multicastGroup
//  macHelper.CreateMulticastGroup (mcEndDevices2, classBGateways);
//  macHelper.CreateMulticastGroup (mcEndDevices2, classBGateways, 3, 0);
  
  //macHelper.CreateNMulticastGroup (mcTotalEndDevices, classBGateways, nMcDevicesPerGroup);    
  if (nMcDevices > 0)
    {
      macHelper.CreateNMulticastGroup (mcTotalEndDevices, classBGateways, nMcDevicesPerGroup, dr, periodicity);      
    } 
  
  /**********************
   *  Handle buildings  *
   **********************/

  double xLength = 130;
  double deltaX = 32;
  double yLength = 64;
  double deltaY = 17;
  int gridWidth = 2 * radius / (xLength + deltaX);
  int gridHeight = 2 * radius / (yLength + deltaY);
  if (realisticChannelModel == false)
    {
      gridWidth = 0;
      gridHeight = 0;
    }
  Ptr<GridBuildingAllocator> gridBuildingAllocator;
  gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (gridWidth));
  gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (xLength));
  gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (yLength));
  gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (deltaX));
  gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (deltaY));
  gridBuildingAllocator->SetAttribute ("Height", DoubleValue (6));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (2));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (4));
  gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (2));
  gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (-gridWidth * (xLength + deltaX) / 2 + deltaX / 2));
  gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (-gridHeight * (yLength + deltaY) / 2 + deltaY / 2));
  BuildingContainer bContainer = gridBuildingAllocator->Create (gridWidth * gridHeight);

  BuildingsHelper::Install (endDevices);
  BuildingsHelper::Install (gateways);
  BuildingsHelper::MakeMobilityModelConsistent ();

  // Print the buildings
  if (print)
    {
      std::ofstream myfile;
      myfile.open ("buildings.txt");
      std::vector<Ptr<Building> >::const_iterator it;
      int j = 1;
      for (it = bContainer.Begin (); it != bContainer.End (); ++it, ++j)
        {
          Box boundaries = (*it)->GetBoundaries ();
          myfile << "set object " << j << " rect from " << boundaries.xMin << "," << boundaries.yMin << " to " << boundaries.xMax << "," << boundaries.yMax << std::endl;
        }
      myfile.close ();

    }

  /**********************************************
   *  Set up the end device's spreading factor  *
   **********************************************/

  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  NS_LOG_DEBUG ("Completed configuration");

  /*********************************************
   *  Install applications on the end devices  *
   *********************************************/

  Time appStopTime = Seconds (simulationTime);
  
  /*PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (appPeriodSeconds));
  appHelper.SetPacketSize (23);
  Ptr <RandomVariableStream> rv = CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (0), "Max", DoubleValue (10));
  ApplicationContainer appContainer = appHelper.Install (endDevices);

  appContainer.Start (Seconds (0));
  appContainer.Stop (appStopTime);
  */
  //class B App
  
  EndDeviceClassBAppHelper appHelper = EndDeviceClassBAppHelper ();
  //\TODO .. Try switching to class B randomly
  
  //Try enabling uplinks
  appHelper.SetSendingPeriod (Seconds(appPeriodSeconds));
  //appHelper.SetPacketSize (23);
  Ptr <RandomVariableStream> rv = CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (0), "Max", DoubleValue (10));
  appHelper.SetPacketSizeRandomVariable (rv);
  
  //Uncomment bellow to Enable Uplink 
  //appHelper.PeriodicUplinks (true);
  
  //Uncomment bellow for Enable Fragmented data reception 
  //appHelper.EnableFragmentedDataReception (0);
  
  ApplicationContainer appContainer = appHelper.Install (endDevices);
  NS_LOG_DEBUG ("Installed!");
  appContainer.Start (Seconds (0));
  appContainer.Stop (appStopTime);
  
  
  NS_LOG_DEBUG ("Completed Installing Class B application on the end device");
    
 
  /**************************
   * Configuring downlink
   **************************/
  
  
  
  /**************************
   *  Create Network Server  *
   ***************************/
//\TODO
  
  // Create the NS node
  NodeContainer networkServer;
  networkServer.Create (1);

  //Enabling Beacon Broadcasting on the NS
  nsHelper.EnableBeaconTransmission (true);
  
  //Enable Fragmented Data Generation 
  //nsHelper.EnableSequencedPacketGeneration (true);
  
  // Create a NS for the network
  nsHelper.SetEndDevices (endDevices);
  nsHelper.SetGateways (gateways);
  nsHelper.Install (networkServer);
  

  //Create a forwarder for each gateway
  forHelper.Install (gateways);

  /**********************
   * Setup Class B Analyzer
   **********************/
  //Print to a file that matches the simulation setting
  
  std::stringstream outputStringNs;
  std::stringstream outputStringEd;
  
  if (addInfoOnFileName && !mixOfDrs && !mixOfPeriodicity)
    {
      // LoraClassBAnalyzer-Ns/Ed-numberOfUnicastNode-numberOfMulticastNode-NumberOfMulticastGroup-NumberOfCells-ETC, from the simulation setting put in the command line. 
      outputStringNs << "ClassBAnalyzerOutput-Ns-" << nUcDevices << "-" 
                     << nMcDevices << "-" << nMcDevicesPerGroup << "-"
                     << dr << "-" << periodicity << "-"
                     << radius << "-" << simulationTime << "-" 
                     << nBeaconGateways << ".txt";    
      
      outputStringEd << "ClassBAnalyzerOutput-Ed-" << nUcDevices << "-" 
                     << nMcDevices << "-" << nMcDevicesPerGroup << "-"
                     << dr << "-" << periodicity << "-"
                     << radius << "-" << simulationTime << "-" 
                     << nBeaconGateways << ".txt";          
    }
  else if (addInfoOnFileName)
    {
      //If the Drs and periodicity are not uniform, 
      outputStringNs << "ClassBAnalyzerOutput-Ns-" << nUcDevices << "-" << nMcDevices << "-" << nMcDevicesPerGroup << "-" << radius << "-" << simulationTime << "-" << nBeaconGateways << "-" << ".txt";
      outputStringEd << "ClassBAnalyzerOutput-Ed-" << nUcDevices << "-" << nMcDevices << "-" << nMcDevicesPerGroup << "-" << radius << "-" << simulationTime << "-" << nBeaconGateways << "-" << ".txt";
    }
  else
    {
      outputStringNs << "ClassBAnalyzerOutput-Ns-"<<filePostFix<<".txt";
      outputStringEd << "ClassBAnalyzerOutput-Ed-"<<filePostFix<<".txt";    
    }
  
  std::string outputFileNameNs = outputStringNs.str ();
  std::string outputFileNameEd = outputStringEd.str ();
  
  // verboseLocation is the location for which to write the performance log for each node in a simulation 
  // Each simulation is on raw of csv
  // prr-<groupIndex>-<dr>-<ping-slot periodicty>-<numberofnodes>-<filePostFix>.csv, 
  // throughput-<groupIndex>-<dr>-<ping-slot periodicty>-<numberofnodes>-<filePostFix>.csv,
  // maxRunLength-<groupIndex>-<dr>-<ping-slot periodicty>-<numberofnodes>-<filePostFix>.csv,
  // avgRunLength-<groupIndex>-<dr>-<ping-slot periodicty>-<numberofnodes>-<filePostFix>.csv
  std::string verboseLocation = "/home/yoni/matlab/LorawanClassB-refining/"; //"home/yoni/matlab/LorawanClassB/";
  LoraClassBAnalyzer classBAnalyzer(outputFileNameNs, outputFileNameEd, verboseLocation, append, endDevices, gateways, networkServer); 
  
  /**********************
   * Print output files *
   **********************/
  if (print)
    {
      helper.PrintEndDevices (endDevices, gateways,
                              "endDevices.dat");
    }

  ////////////////
  // Simulation //
  ////////////////

  Simulator::Stop (appStopTime); // + Hours (1)

  NS_LOG_INFO ("Running simulation...");
  Simulator::Run ();

  Simulator::Destroy ();

  ///////////////////////////
  // Print results to file //
  ///////////////////////////
  //NS_LOG_INFO ("Computing performance metrics...");
  //helper.PrintPerformance (transientPeriods * appPeriod, appStopTime);
  std::ostringstream simulationSetup;
  simulationSetup << "Number of unicast devices = " << nUcDevices << std::endl
                  << "Number of multicast devices = " << nMcDevices << std::endl 
                  << "Number of multicast groups = " << std::ceil ((double)nMcDevices/nMcDevicesPerGroup) << std::endl
                  << "Dr used if all multicast groups have same = " << dr << std::endl 
                  << "Ping Slot Periodicity used if all multicast groups use the same = " << periodicity << std::endl 
                  << "Radius = " << radius << std::endl 
                  << "SimulationTime (Seconds) = " << simulationTime << std::endl
                  << "Number of beaconing gateways = " << nBeaconGateways << std::endl;
  
  classBAnalyzer.Analayze (appStopTime, simulationSetup);

  return 0;
}
