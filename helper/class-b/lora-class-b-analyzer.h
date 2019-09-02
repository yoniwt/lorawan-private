/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Delft University of Technology
 *
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
 *
 * Author: Yonatan Woldeleul Shiferaw <yoniwt@gmail.com>
 */

#ifndef LORACLASSBANALYZER_H
#define LORACLASSBANALYZER_H

#include "ns3/end-device-status.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/address.h"
#include "ns3/lora-device-address.h"
#include "ns3/node-container.h"
#include "ns3/end-device-class-b-app.h"
#include "src/lorawan/model/network-scheduler.h"
#include <map>
#include <string>

namespace ns3 {
namespace lorawan {
 
class LoraClassBAnalyzer {
public:
  
  /**
   * Constructor 
   *
   * \param filename a file name to which to log the output
   * \param append whether to append or override existing file in the disc
   * \param endDevices class B multicast devices
   * \param gateways associated gateways
   * \param networkServer the networkServer
   */
  LoraClassBAnalyzer(std::string filenameNs, std::string filenameEd, std::string verboseLocation, bool append, NodeContainer endDevices, NodeContainer gateways, NodeContainer networkServer);
  virtual ~LoraClassBAnalyzer();
  
  //////////////////////////////////////////////////
  // Trace connection and Analyzer configuration //
  ////////////////////////////////////////////////
  
  void ConnectAllTraceSinks (NodeContainer endDevices, NodeContainer gateways, NodeContainer networkServer);
  
  void CreateInformationContainers (NodeContainer endDevices, NodeContainer gateways, NodeContainer networkServer);
  
  
  
  ////////////////
  // PHY layer //
  //////////////
  //\TODO To analyze beacon and class B downlink packets and beacons that failed.
  // Failures could be device sleeping or packet destroyed and other reasons 
  
  
  ////////////////
  // MAC layer //
  //////////////
  
  
  ///////////////////////
  // NetworkScheduler //
  /////////////////////
  
  /**
   * Callback when multicast packet is sent via gateway
   */
  void McPingSentCallback (LoraDeviceAddress mcAddress, uint8_t numberOfGateways, uint8_t pingSlotPeriodicity, 
                 uint8_t slotIndex, Time time, Ptr<Packet const> packet, bool isSequentialPacket, uint32_t sequenceNumber);
  
  void TotalBeaconBroadcastedCallback (uint32_t oldCount, uint32_t newCount);
  
  void TotalBeaconSkippedCallback (uint32_t oldCount, uint32_t newCount);
  
  void NumberOfBeaconTransmittingGateways (uint8_t oldCount, uint8_t newCount);
  
  /**
   * Trace sink for the trace source fired when the beacon status changes from 
   * successfully sending to skipping or vise versa
   * 
   * The first time the beacon is fired it will be called with a parameter 
   * false and 0 as we didn't sent any beacons nor did we skip any beacon. 
   * For the other times it will be invoked with: 
   * if isSent == false : the number of beacons skipped continuously
   * if isSent == true: the number of beacons continuously sent
   * 
   * \param isSent beacon skipped if false and beacon sent if true
   * \param continuousCount if true this indicates the number of beacons 
   * continuously transmitted, otherwise it indicates the number of beacons 
   * continuously skipped. If there, it means it is the first beacon.
   *  
   */
  void BeaconStatusCallback (bool isSent, uint32_t continuousCount);
  
  ///////////////////////
  // EndDeviceLoraMac //
  /////////////////////
  
  /**
   * Callback when changing device-class
   */
  void DeviceClassChangeCallback (enum EndDeviceLoraMac::DeviceClass oldClass, enum EndDeviceLoraMac::DeviceClass newClass);
  
  /**
   * Callback when receiving ping packets
   */
  void ReceivedPingPacket (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, Ptr<const Packet> packet, uint8_t slotIndex);
  
  /**
   * Callback when missing beacons
   */
  void BeaconReceived (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t numberOfBeaconsReceived);
  
  /**
   * Callback for missed beacons
   */
  void BeaconMissed (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t currentMissedBeacons);
  
  /**
   * Beacon Missed Run Length
   */
  void CurrentBeaconMissedRunLength (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint8_t currentBeaconMissedRunLength);
  
  /**
   * Number of overheard packets 
   */
  void NumberOfOverhearedPackets (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t numberOfOverheardPacket);
  
  /**
   * Number of failed ping downlinks
   */
  void NumberOfFailedPings (uint32_t oldValue, uint32_t newValue);
  
  
  ///////////////////////////
  // EndDeviceApplication //
  /////////////////////////
  
  /**
   * Tracedcallback when fragments are missed
   * 
   * \param mcAddress multicast address of this device
   * \param ucAddress unicast address of this device
   * \param currentNumberOfFragementsMissed the number of fragments that are identified to be missed
   * \param totalNumberOfFragmentsMissed the total number of fragments missed up until now
   */
  void FragmentsMissed (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t currentNumberOfFragmentsMissed, uint32_t totalNumberOfFragmentsMissed);
  
  ///////////////////////////
  // Various Computations //
  /////////////////////////

  
  /**
   * It checks the status of the previous packet sent as a downlink
   * 
   * it will check whether the transmitted packet has been received by all the 
   * end devices and record it as lost for devices which did not receive the packet. 
   * 
   * \param mcAddress the multicast address of the transmission for which to check
   * the status of the previous packet transmission.
   */
  void ProcessPreviousPacketStatus (LoraDeviceAddress mcAddress);
  
  
  ////////////////
  // Utilities //
  //////////////
  
  /**
   * Stream beacon related information from the NetworkServer
   *
   * \param output stream object to log to after finalizing
   * \param verbose list performance for the network server if true, only put summary if false
   */
  void FinalizeNsBeaconRelatedInformation (std::ostream& output, bool verbose);
  
  /**
   * Stream downlink related information from the NetworkServer
   *
   * \param output stream object to log to after finalizing
   * \param verbose list performance for the network server if true, only put summary if false
   */
  void FinalizeNsDownlinkRelatedInformation (std::ostream& output, bool verbose);
   
  
  /**
   * Stream beacon related information from the End Devices
   *
   * \param output stream object to log to after finalizing
   * \param verbose list performance for each end device if true, only put summary 
   * for all the end-device if false
   */
  void FinalizeEdsBeaconRelatedInformation (std::ostream& output, bool verbose);
  
  /**
   * Stream downlink related information from the end device side
   * 
   * \param output stream object to log to after finalizing
   * \param verbose list performance for each end device if true, only put summary 
   * for all the end-device if false
   */
  void FinalizeEdsDownlinkRelatedInformation (std::ostream& output, bool verbose);
  
  
  /**
   * To analyze different metrics and log
   * 
   * \param appStopTime is the time where the application stop
   * \param simulationSetup information about the simulation setup of append before the output
   * \param nsVerbose enable verbose logging for the network server
   * \param edVerbose enable verbose logging for the end device
   */
  void Analayze (Time appStopTime, std::ostringstream& simulationSetup, bool nsVerbose, bool edVerbose);
  
  
private:
  
  std::string m_nsLogFileName;
  
  std::string m_edLogFileName;
  
  /**
   * Location to write performance for each node
   * 
   * The simulation will write a csv file to the location provided in the following manner
   *  - prr-<groupIndex>-<dr>-<ping-slot periodicty>-<numberofnodes>-<filePostFix>.csv, 
   *  - throughput-<groupIndex>-<dr>-<ping-slot periodicty>-<numberofnodes>-<filePostFix>.csv,
   *  - maxRunLength-<groupIndex>-<dr>-<ping-slot periodicty>-<numberofnodes>-<filePostFix>.csv,
   *  - avgRunLength-<groupIndex>-<dr>-<ping-slot periodicty>-<numberofnodes>-<filePostFix>.csv
   */
  std::string m_verboseLocation; 
  
  bool m_appendInformation; ///< Whether to append to files when writting 
  
  Time m_startTime; ///< Time from which we start to analyze the performance including throughput 
  Time m_endTime;  ///<Time on which we end to analyze the performance including throughput
  
  // Members to store the beacon and class-b packets at their various stages
  // Also for the fragmented data along with the sequence number in-order to 
  // do the mapping
  
  // When transmitted
  // When received
  // When failed because the device is sleeping (you might need to collect in the phy the failed packets)
  //
  
  
  // Cout
  // Configuration: 
  // number of devices, number of multicast groups, number of multicast devices, 
  // number of unicast devices 
  // Number of Gw
  // Average distance between the Gws if Number of Gw is more than one
  // number of devices with DR0, 1, 2, 3, 4, 5
  // number of devices with PingSlot periodicity 0, 1, 2, 3, 4, 5, 6, 7
  
  // Members to store information that apply to the state of the class B devices and gateways
  // Beacon-related-informations
  // Do also the continues version 
  // 
  
  
  ////////////////////////////////////
  // NS and Gw Related Information //
  //////////////////////////////////
  
  struct NSBeaconRelatedPerformance
  {
    uint32_t numberOfBeaconsSentByNs = 0; ///< Total number of beacons sent by NS
    uint32_t numberOfBeaconsSkippedByNs = 0; ///< Total number of beacons skipped by NS
    double averageNumberOfContinuousBeaconsSentByNs = 0;
    uint32_t maximumNumberOfContinuousBeaconsSentByNs = 0;
    uint32_t minimumNumberOfContinuousBeaconsSentByNs = 0;
    double averageNumberOfContinuousBeaconsSkippedByNs = 0;
    uint32_t maximumNumberOfContinuousBeaconsSkippedByNs = 0;
    uint32_t minimumNumberOfContinuousBeaconsSkippedByNs = 0;
    uint32_t sumOfBeaconsTxByGws  = 0; ///< Total number of beacons actually transmitted. Eg: If 10 Gw broadcasted a single beacon the sum will be 10.
    double effectiveNumberOfBeaconsTxByGws = 0; ///< m_sumOfBeaconsTxByGws / m_maxBeaconingGateways
    uint32_t maximumNumberOfBeaconsTxbyGws = 0; ///< \TODO Extract from GwTrace or NetworkStatus
    uint32_t minimumNumberOfBeaconsSentbyGws = 0; ///< \TODO Extract from GwTrace or NetworkStatus
    double averageNumberOfContinuousBeaconsSkippedByGws = 0;
    uint32_t maximumNumberOfContinuousBeaconsSkippedByGws = 0;
    uint32_t minimumNumberOfContinuousBeaconsSkippedByGws = 0;
    uint32_t lastBeaconingGateways = 0;
    uint32_t maxBeaconingGateways = 0;
    Ptr<NetworkScheduler> networkScheduler = NULL; ///< To extract the pingDownlinkPacketSize later, If 0 the packet size if randomely selected, and if it is above the size supported by the DR used upto 255 the maximum possible size will be selected 
  };
  
  struct NSBeaconRelatedPerformance m_nSBeaconRelatedPerformance;    
  
  // Fragmented-packet-information
  // Normalize it with the packet size
  
  struct NsDownlinkRelatedPerformance
  {
    uint32_t numberOfFragmentsSentbyNs = 0;
    uint32_t totalBytesSent = 0; 
    double nSThroughput = 0; ///< throughput of the NetworkServe for a multicast group
    uint32_t cummulativeNumberOfGwsForAllTransmissions = 0; ///< This is just used to calculate the average number of Gws Used;
    uint32_t maximumNumberOfGwsUsed = 0; ///< Maximum number of Gws used per transmission
    uint32_t avarageNumberOfGwsUsed = 0; ///< cummulativeNumberOfGwsForAllTransmissions/numberOfFragmentsSentbyNs
    uint32_t minimumNumberOfGwsUsed = 0; ///< Minimum number of Gws used per transmission
    
    Ptr<Packet> latestPacketSent = 0; ///< The latest packet that is sent 
  };
  
  std::map<LoraDeviceAddress, struct NsDownlinkRelatedPerformance> m_mcNsDownlinkRelatedPerformance; ///< Multicast Related Network Server Downlink Performance  
  
  double m_averageNumberOfFragementsSentbyNs; ///< Can be derived from m_mcNsDownlinkRelatedPerformance
  uint32_t m_maximumNumberOfFragementsSentbyNs; ///< Can be derived from m_mcNsDownlinkRelatedPerformance
  uint32_t m_minimumNumberOfFragementsSentbyNs; ///< Can be derived from m_mcNsDownlinkRelatedPerformance  
  // What about bytes?
  
  /// Overall NS performance
  uint32_t m_totalFragementsSentbyNs; 
  uint32_t m_totalBytesSentbyNs;
  double m_aggregateNsThroughput; //Throughput in bits per second
  
  
  
//  // Throughput-calculations
//  double m_averageGwBytesSent;
//  uint32_t m_maximumGwBytesSent;
//  uint32_t m_minimumGwBytesSent;  
//  
//  double m_averageGwThroughput;
//  double m_maximumGwThroughput;
//  double m_minimumGwThroughput;
  
  ///////////////////////////////////////
  //   End-Device related information //
  /////////////////////////////////////
  
  struct EdBeaconRelatedPerformance
  {
    uint32_t totalBeaconLost = 0;
    uint32_t totalBeaconReceived = 0;
    uint32_t numberOfSwitchToBeaconLessOperationModes = 0;    
    uint32_t totalBeaconLostInBeaconlessOperationMode = 0;
    uint32_t lastBeaconLossRunLength = 0; ///< Number of beacons lost continuously in minimal beaconless operation mode    
    uint32_t maximumBeaconLostInBeaconlessOperationMode = 0;
    uint32_t minimumBeaconLostInBeaconlessOperationMode = 0;
    double averageBeaconLostInBeaconlessOperationMode = 0;
    double brr = 0; // beacon reception ratio
  };
  
  struct McEdBeaconRelatedPerformance
  {
    double averageNumberOfBeaconsReceiveByEds = 0;
    uint32_t maximumNumberOfBeaconsReceivedByEds = 0;
    uint32_t minimumNumberOfBeaconsReceivedByEds = 0;
    double averageNumberOfBeaconsLostByEds = 0;
    uint32_t maximumNumberOfBeaconsLostByEds = 0;
    uint32_t minimumNumberOfBeaconsLostByEds = 0;   
    
    std::map<LoraDeviceAddress, struct EdBeaconRelatedPerformance> edBeaconRelatedPerformance;
  };
  
  std::map<LoraDeviceAddress, struct McEdBeaconRelatedPerformance> m_mcEdBeaconRelatedPerformance;
  
  
  struct EdDownlinkRelatedPerformance
  {
    uint32_t totalNumberOfFragmentsReceived = 0;
    uint32_t totalNumberOfFragmentsLost = 0;
    uint32_t totalBytesReceived = 0;
    uint32_t totalBytesLost = 0;
    double throughput = 0; ///< Throughput in bits per second
    
    uint32_t maximumNumberOfSequentialBytesLost = 0; ///< From all the Eds in the Mc group
    uint32_t minimumNumberOfSequentialBytesLost = 0; ///< From all the Eds in the Mc group
    double averageNumberOfSequentialBytesLost = 0; ///< Sum (Average sequences lost per Ed)/Number of Eds in Mc

    uint32_t maximumNumberOfSequentialFragmentsLost = 0; ///< From all the Eds in the Mc group
    uint32_t minimumNumberOfSequentialFragmentsLost = 0; ///< From all the Eds in the Mc group
    double averageNumberOfSequentialFragmentsLost = 0; ///< Sum (Average sequences lost per Ed)/Number of Eds in Mc     
    uint32_t numberOfDiscontinuties = 0; ///< Number of times where fragments are lost
    
    uint32_t currentByteLossRunLength = 0; ///< this is incremented when packet is lost and set to zero when packet is received
    uint32_t currentByteSuccessRunLength = 0; ///< this is increamented when packet is received and set to zero when packet is lost
    uint32_t currentPacketLossRunLength = 0; ///< this is incremented when packet is lost and set to zero when packet is received
    uint32_t currentPacketSuccessRunLength = 0; ///< this is increamented when packet is received and set to zero when packet is lost
    
    double prr = 0; ///<Packet reception ratio;
    
    Ptr<Packet> latestPacketReceived = 0; ///< the latest packet that is received; this can be used to check lost packets from sent packets. 
    
    uint32_t numberOfOverhearedPackets = 0; ///< Number of packets an end-device has overheared
  };
  
  struct McEdDownlinkRelatedPerformance
  {
    uint32_t totalNumberOfFragmentsReceivedByEds = 0;
    double averageNumberOfFragementsReceivedByEds = 0; //along with multicastAddress : Total fragments received by Eds in Mc/ Number of Eds in MC
    uint32_t largestNumberOfFragmentReceivedByEds = 0;
    uint32_t smallestNumberOfFragmentReceivedByEds = 0;
    
    uint32_t totalBytesReceivedByEds = 0; ///< Sum of all bytes from all devices in multicast device 
    double averageBytesReceivedByEds = 0; ///< Along with multicastAddress : Total bytes received by Eds in Mc/ Number of Eds in MC
    uint32_t maximumBytesReceivedByEds = 0; ///< From all the Eds in the mc the maximum bytes received 
    uint32_t minimumBytesReceivedByEds = 0; ///< From all the Eds in the mc the minimum bytes received   
    
    uint32_t maximumNumberOfSequentialFragmentsLost = 0; ///< From all the Eds in the Mc group
    uint32_t minimumNumberOfSequentialFragmentsLost = 0; ///< From all the Eds in the Mc group
    double averageNumberOfSequentialFragmentsLost = 0; ///< Sum(ForAllEds(number of lost fragements in a chunk))/Sum(ForAllEds(Number_of_chuncks_continually_lost))
    ///where a chunk is a set of packets that are continually lost        
    
    uint32_t maximumNumberOfSequentialBytesLost = 0; ///< From all the Eds in the Mc group
    uint32_t minimumNumberOfSequentialBytesLost = 0; ///< From all the Eds in the Mc group
    double averageNumberOfSequentialBytesLost = 0; ///< Sum(ForAllEds(bytes continually lost in a chunk))/Sum(ForAllEds(Number_of_chuncks_continually_lost))
    ///where a chunk is a set of packets that are continually lost
    
    double averageThroughput = 0; ///< ThroughputOfAllEdsinMc/NumberOfEds
    double minimumThroughput = 0; ///< The throughput of the Ed with the smallest throughput value among all the Eds in the MC group 
    double maximumThroughput = 0; ///< The throughput of the Ed with the largest throughput value among all the Eds in the MC group
    
    uint32_t numberOfBytesLostInAllEds = 0; ///< This are bytes that are not received by any of the Eds
    double averageSequencialBytesLostInAllEds = 0; ///< This measures average bytes that are not received by any of the Eds
    uint32_t minimumSequencialBytesLostInAllEds = 0; ///< This measures minimum continues bytes that are not received by any of the Eds
    uint32_t maximumSequencialBytesLostInAllEds = 0; ///< This measures maximum bytes that are not received by any of the Eds
    
    uint32_t numberOfFragmentsLostInAllEds = 0; ///< This are fragments that are not received by any of the Eds
    double averageSequencialFragmentsLostInAllEds = 0; ///< This measures average fragments that are not received by any of the Eds
    uint32_t minimumSequencialFragmentsLostInAllEds = 0; ///< This measures minimum continues fragments that are not received by any of the Eds
    uint32_t maximumSequencialFragmentsLostInAllEds = 0; ///< This measures maximum fragments that are not received by any of the Eds    
    
    uint8_t dr = 3; ///< Data Rate Used for the multicast group, for printing purposes in verbose mode
    uint8_t periodicity = 0; ///< Ping slot periodicity for the multicast group, for prining puposes in verbose mode
    int numberOfEds = 0; ///< Number of end devices in the multicast group
    
    std::map<LoraDeviceAddress, struct EdDownlinkRelatedPerformance> edDownlinkRelatedPerformance;
  };
    
  std::map<LoraDeviceAddress, struct McEdDownlinkRelatedPerformance> m_mcEdDownlinkRelatedPerformance; ///< Multicast Related Network Server Downlink Performance  
  
  uint32_t m_failedPings; // Total number of failed ping downlinks
  
};

}
}
#endif /* LORACLASSBANALYZER_H */


