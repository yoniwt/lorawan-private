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

#include "ns3/lora-class-b-analyzer.h"
#include "ns3/lora-device-address.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/lora-mac-header.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/network-server.h"
#include "ns3/network-scheduler.h"
#include "src/core/model/assert.h"
#include "src/core/model/log-macros-enabled.h"
#include "ns3/lora-class-b-analyzer.h"
#include "ns3/ptr.h"
#include "src/core/model/trace-source-accessor.h"
#include "ns3/trace-source-accessor.h"

#include <iostream>
#include <fstream>

namespace ns3 {
namespace lorawan{

NS_LOG_COMPONENT_DEFINE ("LoraClassBAnalyzer");

LoraClassBAnalyzer::LoraClassBAnalyzer (std::string filenameNs, std::string filenameEd, std::string verboseLocation, 
  bool append, NodeContainer endDevices, NodeContainer gateways, NodeContainer networkServer) :
  m_nsLogFileName (filenameNs),
  m_edLogFileName (filenameEd),
  m_verboseLocation (verboseLocation),
  m_appendInformation (append),
  m_nSBeaconRelatedPerformance(LoraClassBAnalyzer::NSBeaconRelatedPerformance ()),
  m_averageNumberOfFragementsSentbyNs (0),
  m_maximumNumberOfFragementsSentbyNs (0),
  m_minimumNumberOfFragementsSentbyNs (std::numeric_limits<uint32_t>::max()),
  m_totalFragementsSentbyNs (0),
  m_totalBytesSentbyNs (0),
  m_aggregateNsThroughput (0)

{ 
  //Create two file: one for Network Server and one of the End Devices
  std::ofstream nsLogFile;
  std::ofstream edLogFile;
  if (append) 
    {
      nsLogFile.open (m_nsLogFileName, std::ofstream::out | std::ofstream::app); 
      edLogFile.open (m_edLogFileName, std::ofstream::out | std::ofstream::app);
    }
  else 
    {
      nsLogFile.open (m_nsLogFileName, std::ofstream::out | std::ofstream::trunc);
      edLogFile.open (m_edLogFileName, std::ofstream::out | std::ofstream::trunc);      
    }
  
  nsLogFile.close ();
  edLogFile.close ();
  
  ConnectAllTraceSinks (endDevices, gateways, networkServer);
  
  CreateInformationContainers (endDevices, gateways, networkServer);
  
}

LoraClassBAnalyzer::~LoraClassBAnalyzer ()
{
}

void
LoraClassBAnalyzer::ConnectAllTraceSinks (NodeContainer endDevices, NodeContainer gateways, NodeContainer networkServer)
{
  //For checking the success of a trace connection
  bool success = false; 
  
  for (NodeContainer::Iterator i = endDevices.Begin () ; i != endDevices.End (); ++i)
    {
      Ptr<NetDevice> netDevice = (*i)->GetDevice (0);
  
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      NS_ASSERT (mac != 0);      
      
      success = mac->TraceConnectWithoutContext ("ReceivedPingMessages",
                                                  MakeCallback 
                                                    (&LoraClassBAnalyzer::ReceivedPingPacket, this));
      NS_ASSERT (success == true);

      success = mac->TraceConnectWithoutContext ("DeviceClass",
                                                  MakeCallback 
                                                    (&LoraClassBAnalyzer::DeviceClassChangeCallback, this));
      NS_ASSERT (success == true);
      
      success =mac->TraceConnectWithoutContext ("TotalSuccessfulBeaconPacketsTracedCallback",
                                                 MakeCallback
                                                   (&LoraClassBAnalyzer::BeaconReceived, this));
      NS_ASSERT (success == true);
      
      success =mac->TraceConnectWithoutContext ("MissedBeaconTracedCallback",
                                                 MakeCallback
                                                   (&LoraClassBAnalyzer::BeaconMissed, this));
      NS_ASSERT (success == true);
      
      success =mac->TraceConnectWithoutContext ("CurrentConsecutiveBeaconsMissedTracedCallback",
                                                 MakeCallback
                                                   (&LoraClassBAnalyzer::CurrentBeaconMissedRunLength, this));
      NS_ASSERT (success == true);      
      
      // We assume that there is only one application on each node
      Ptr<Application> app = (*i)->GetApplication (0);
      NS_ASSERT (app != 0);
      
      Ptr<EndDeviceClassBApp> edApp = app->GetObject<EndDeviceClassBApp> ();
      NS_ASSERT (edApp != 0);
      
      success = edApp->TraceConnectWithoutContext ("FragmentsMissed",
                                                   MakeCallback
                                                     (&LoraClassBAnalyzer::FragmentsMissed, this));
      NS_ASSERT (success == true);
    }
  
  for (NodeContainer::Iterator i = networkServer.Begin () ; i != networkServer.End (); ++i)
    {
     //We assume the NetworkServer application to be the first application
      Ptr<Application> networkServerApp = (*i)->GetApplication (0);
      NS_ASSERT (networkServerApp != 0);
     
      //Get the NetworkServer
      Ptr<NetworkServer> networkServer = networkServerApp->GetObject<NetworkServer> ();
      NS_ASSERT (networkServer != 0);
      
      // NetworkStatus TraceConnect
      Ptr<NetworkStatus> networkStatus = networkServer->GetNetworkStatus ()->GetObject<NetworkStatus> ();
      NS_ASSERT (networkStatus != 0);
      
      success = networkStatus->TraceConnectWithoutContext ("LastBeaconTransmittingGateways", 
                                                            MakeCallback
                                                              (&LoraClassBAnalyzer::NumberOfBeaconTransmittingGateways, this));
      NS_ASSERT (success == true);
      
      //NetworkScheduler TraceConnect
      Ptr<NetworkScheduler> networkScheduler = networkServer->GetNetworkScheduler ();
      NS_ASSERT (networkScheduler != 0);
      
      
      success = networkScheduler->TraceConnectWithoutContext ("TotalBeaconsBroadcasted", 
                                                              MakeCallback 
                                                                (&LoraClassBAnalyzer::TotalBeaconBroadcastedCallback, this));      
      NS_ASSERT (success == true);
      
      success = networkScheduler->TraceConnectWithoutContext ("TotalBeaconsBlocked",
                                                              MakeCallback
                                                                (&LoraClassBAnalyzer::TotalBeaconSkippedCallback, this));
      NS_ASSERT (success == true);
      
      success = networkScheduler->TraceConnectWithoutContext ("BeaconStatusCallback",
                                                              MakeCallback
                                                                (&LoraClassBAnalyzer::BeaconStatusCallback, this));
      NS_ASSERT (success == true);
      
      success = networkScheduler->TraceConnectWithoutContext ("McPingSent", 
                                         MakeCallback 
                                           (&LoraClassBAnalyzer::McPingSentCallback, this));
      NS_ASSERT (success == true);
    }

}

void
LoraClassBAnalyzer::CreateInformationContainers (NodeContainer endDevices, NodeContainer gateways, NodeContainer networkServer)
{
  for (NodeContainer::Iterator i = endDevices.Begin () ; i != endDevices.End (); ++i)
    {
      // Creating Entries for all multicast devices, to track the packets for
      // performance analysis here
      Ptr<NetDevice> netDevice = (*i)->GetDevice (0);
  
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      NS_ASSERT (mac != 0);   
      
      // If the device is not a multicast device 
      // Don't put it in the entry
      if (!mac->IsMulticastEnabled ())
        {
          // Add unicast devices here 
          continue; 
        }
      
      LoraDeviceAddress mcAddress = mac->GetMulticastDeviceAddress ();
      LoraDeviceAddress ucAddress = mac->GetDeviceAddress ();
  
      //Network Server Related Information container for each multicast group
      if (m_mcNsDownlinkRelatedPerformance.find (mcAddress) == m_mcNsDownlinkRelatedPerformance.end ())
        {
          m_mcNsDownlinkRelatedPerformance.emplace (std::piecewise_construct, 
                                                    std::forward_as_tuple (mcAddress),
                                                    std::forward_as_tuple ());
        }
      
      //End device related downlink information
      if (m_mcEdDownlinkRelatedPerformance.find (mcAddress) == m_mcEdDownlinkRelatedPerformance.end ())
        {
          m_mcEdDownlinkRelatedPerformance.emplace (std::piecewise_construct, 
                                                    std::forward_as_tuple (mcAddress),
                                                    std::forward_as_tuple ()); 
        }
      
      m_mcEdDownlinkRelatedPerformance.at (mcAddress).dr = mac->GetPingSlotReceiveWindowDataRate ();
      m_mcEdDownlinkRelatedPerformance.at (mcAddress).periodicity = mac->GetPingSlotPeriodicity ();
      
        
      if (m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
            .find (ucAddress) ==  m_mcEdDownlinkRelatedPerformance.at (mcAddress)
                                    .edDownlinkRelatedPerformance.end())
        {
          m_mcEdDownlinkRelatedPerformance.at (mcAddress)
            .edDownlinkRelatedPerformance.emplace (std::piecewise_construct,
                                                   std::forward_as_tuple (ucAddress),       
                                                   std::forward_as_tuple ());
          
          //For each new device increment the number nodes in a group
          m_mcEdDownlinkRelatedPerformance.at (mcAddress).numberOfEds++;
          
        }
      
      //End device related beacon information
   
      if (m_mcEdBeaconRelatedPerformance.find (mcAddress) == m_mcEdBeaconRelatedPerformance.end())
        {
          m_mcEdBeaconRelatedPerformance.emplace (std::piecewise_construct, 
                                                    std::forward_as_tuple (mcAddress),
                                                    std::forward_as_tuple ());
        }
      if (m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance
            .find (ucAddress) == m_mcEdBeaconRelatedPerformance.at (mcAddress)
                                  .edBeaconRelatedPerformance.end ())
        {
          m_mcEdBeaconRelatedPerformance.at (mcAddress)
            .edBeaconRelatedPerformance.emplace (std::piecewise_construct,
                                                   std::forward_as_tuple (ucAddress),       
                                                   std::forward_as_tuple ());     
        }     
      
      
    }
}



///////////////////////////////////////////
// NetworkScheduler  and Network Status //
/////////////////////////////////////////

void
LoraClassBAnalyzer::McPingSentCallback (LoraDeviceAddress mcAddress, uint8_t numberOfGateways, uint8_t pingSlotPeriodicity, uint8_t slotIndex, Time time, Ptr<Packet const> packet, bool isSequentialPacket, uint32_t sequenceNumber)
{
  NS_LOG_FUNCTION (this << mcAddress << numberOfGateways << pingSlotPeriodicity << slotIndex << time << packet << isSequentialPacket << sequenceNumber);
  
  NS_LOG_DEBUG ("Packet Sent UID : " << packet->GetUid ());
  
  if (m_mcNsDownlinkRelatedPerformance.find (mcAddress) == m_mcNsDownlinkRelatedPerformance.end ())
    {
      NS_ASSERT_MSG (false, "Multicast address not found");
    }
  
  // Check about the reception of the previous packet that is sent before updating
  // to find lost packets and update information accordingly
  // if it is the first packet that is being sent, skip processing
  if (m_mcNsDownlinkRelatedPerformance.at (mcAddress).totalBytesSent != 0)
    {
      ProcessPreviousPacketStatus (mcAddress);  
    }
  
  uint32_t size = packet->GetSize ();
  
  // Total byte sent by Network Server
  m_totalBytesSentbyNs+= size;
  m_totalFragementsSentbyNs++;
  
  // Add the new packet to the list
  m_mcNsDownlinkRelatedPerformance.at (mcAddress).latestPacketSent = packet->Copy ();
  m_mcNsDownlinkRelatedPerformance.at (mcAddress).totalBytesSent += size;
  m_mcNsDownlinkRelatedPerformance.at (mcAddress).numberOfFragmentsSentbyNs += 1;
  m_mcNsDownlinkRelatedPerformance.at (mcAddress).cummulativeNumberOfGwsForAllTransmissions += numberOfGateways;
  m_mcNsDownlinkRelatedPerformance.at (mcAddress).avarageNumberOfGwsUsed = 
    m_mcNsDownlinkRelatedPerformance.at (mcAddress).cummulativeNumberOfGwsForAllTransmissions /
    m_mcNsDownlinkRelatedPerformance.at (mcAddress).numberOfFragmentsSentbyNs;
  uint8_t currentMinimumGwsUsed = m_mcNsDownlinkRelatedPerformance.at (mcAddress).minimumNumberOfGwsUsed;
  
  // if it is still zero that means that this is being called for the first time
  if (currentMinimumGwsUsed == 0)
    {
      currentMinimumGwsUsed = std::numeric_limits<uint8_t>::max ();
    }
  uint8_t currentMaximumGwsUsed = m_mcNsDownlinkRelatedPerformance.at (mcAddress).maximumNumberOfGwsUsed;
  m_mcNsDownlinkRelatedPerformance.at (mcAddress).minimumNumberOfGwsUsed = std::min (currentMinimumGwsUsed, numberOfGateways);
  m_mcNsDownlinkRelatedPerformance.at (mcAddress).maximumNumberOfGwsUsed = std::max (currentMaximumGwsUsed, numberOfGateways);
  
  // Network throughput calculation
  // Throughput calculation for each multicast group
  m_mcNsDownlinkRelatedPerformance.at (mcAddress).nSThroughput = (double)(m_mcNsDownlinkRelatedPerformance.at (mcAddress).totalBytesSent * 8)/
                                                                  (Simulator::Now ().GetSeconds () - m_startTime.GetSeconds ());
  
  // Overall NS performance
  m_aggregateNsThroughput = (double)(m_totalBytesSentbyNs*8)/(Simulator::Now ().GetSeconds () - m_startTime.GetSeconds ());
  
}

void
LoraClassBAnalyzer::TotalBeaconBroadcastedCallback (uint32_t oldCount, uint32_t newCount)
{
  NS_LOG_FUNCTION (this << oldCount << newCount);
  //Number of beacons that broadcasted is already taken care by the LoraClassBAnalyzer::NumberOfBeaconTransmittingGateways
  m_nSBeaconRelatedPerformance.numberOfBeaconsSentByNs++;
  
  //Calculate the average number of Beacons transmitted by Gws
  // Sum (BeaconTransmittingGateways for each beacon sent by NS) / Sum(Gateways that participated in the broadcast)
  m_nSBeaconRelatedPerformance.sumOfBeaconsTxByGws+= m_nSBeaconRelatedPerformance.lastBeaconingGateways;
  m_nSBeaconRelatedPerformance.effectiveNumberOfBeaconsTxByGws = (double)m_nSBeaconRelatedPerformance.sumOfBeaconsTxByGws
                                                                    /
                                                                  m_nSBeaconRelatedPerformance.maxBeaconingGateways;
  
  //You can plot this for real-time plot the averageNumberOfGateways and the total
  // << to a file or string stream
}

void
LoraClassBAnalyzer::TotalBeaconSkippedCallback (uint32_t oldCount, uint32_t newCount)
{
  NS_LOG_FUNCTION (this << oldCount << newCount);
  m_nSBeaconRelatedPerformance.numberOfBeaconsSkippedByNs++;
}

void
LoraClassBAnalyzer::NumberOfBeaconTransmittingGateways (uint8_t oldCount, uint8_t newCount)
{
  NS_LOG_FUNCTION (this << oldCount << newCount);
  //When this call back is called it means the number of transmitting gateways has changed
  m_nSBeaconRelatedPerformance.lastBeaconingGateways = newCount;
  
  //Keep track of the maximum number of Gateways that are involved in beacon broadcasting
  m_nSBeaconRelatedPerformance.maxBeaconingGateways = (newCount > m_nSBeaconRelatedPerformance.maxBeaconingGateways)
                                                           ?
                                                           newCount : 
                                                           m_nSBeaconRelatedPerformance.maxBeaconingGateways;
}

void
LoraClassBAnalyzer::BeaconStatusCallback (bool isSent, uint32_t continuousCount)
{
  NS_LOG_FUNCTION (this << isSent << continuousCount);
  // For calculating averageNumberOfContinuousBeaconasSkippedByNs
  static uint32_t numberOfContinuousBeaconSkippedChuncks = 0; 
  // For calculating averageNumberOfContinuousBeaconasSentByNs
  static uint32_t numberOfContinuousBeaconSentChuncks = 0;
  
  if (!isSent && continuousCount == 0)
    {
      // Beacon is fired for the first time
      // Record the starting time
      m_startTime = Simulator::Now ();
      return;
    }
  
  if (isSent)
    {
      numberOfContinuousBeaconSentChuncks++;

      uint32_t minimumNumberOfContinuousBeaconsSentByNs = m_nSBeaconRelatedPerformance.minimumNumberOfContinuousBeaconsSentByNs;
      // if it is still zero that means that this is being called for the first time
      if (minimumNumberOfContinuousBeaconsSentByNs == 0)
      {
        minimumNumberOfContinuousBeaconsSentByNs = std::numeric_limits<uint32_t>::max ();
      }    
      m_nSBeaconRelatedPerformance.minimumNumberOfContinuousBeaconsSentByNs = 
        std::min(minimumNumberOfContinuousBeaconsSentByNs,
                 continuousCount);
      m_nSBeaconRelatedPerformance.maximumNumberOfContinuousBeaconsSentByNs = 
        std::max(m_nSBeaconRelatedPerformance.maximumNumberOfContinuousBeaconsSentByNs,
                 continuousCount);
      m_nSBeaconRelatedPerformance.averageNumberOfContinuousBeaconsSentByNs = 
        (double)m_nSBeaconRelatedPerformance.numberOfBeaconsSentByNs/numberOfContinuousBeaconSentChuncks;
    }
  else
    {
      numberOfContinuousBeaconSkippedChuncks++;
        
      uint32_t minimumNumberOfContinuousBeaconsSkippedByNs = m_nSBeaconRelatedPerformance.minimumNumberOfContinuousBeaconsSkippedByNs;
      // if it is still zero that means that this is being called for the first time
      if (minimumNumberOfContinuousBeaconsSkippedByNs == 0)
      {
        minimumNumberOfContinuousBeaconsSkippedByNs = std::numeric_limits<uint32_t>::max ();
      }     
      m_nSBeaconRelatedPerformance.minimumNumberOfContinuousBeaconsSkippedByNs = 
        std::min(minimumNumberOfContinuousBeaconsSkippedByNs,
                 continuousCount);
      m_nSBeaconRelatedPerformance.maximumNumberOfContinuousBeaconsSkippedByNs = 
        std::max(m_nSBeaconRelatedPerformance.maximumNumberOfContinuousBeaconsSkippedByNs,
                 continuousCount);
      m_nSBeaconRelatedPerformance.averageNumberOfContinuousBeaconsSkippedByNs = 
        (double)m_nSBeaconRelatedPerformance.numberOfBeaconsSkippedByNs/numberOfContinuousBeaconSkippedChuncks;
    }
  
}


/////////////////////////
// EndDeviceLoraMac   //
///////////////////////

void
LoraClassBAnalyzer::DeviceClassChangeCallback (EndDeviceLoraMac::DeviceClass oldClass, EndDeviceLoraMac::DeviceClass newClass)
{
  NS_LOG_FUNCTION (this << oldClass << newClass);
  
}

void
LoraClassBAnalyzer::ReceivedPingPacket (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, Ptr<const Packet> packet, uint8_t slotIndex)
{
  NS_LOG_FUNCTION (this << mcAddress << ucAddress << packet << slotIndex);
  NS_LOG_DEBUG ("Ping Packet Received UID : " << packet->GetUid ());
  
  uint32_t size = packet->GetSize ();
  
  
  if (m_mcEdDownlinkRelatedPerformance.find (mcAddress) == m_mcEdDownlinkRelatedPerformance.end ())
    {
      if (mcAddress == 1)
        {
          // Unicast device is not being analyzed for now
          //\TODO  In the future add it to the unicast list 
          NS_LOG_WARN ("Unicast devices not analyzed for now! Future update");
          return;
        }
      
       NS_ASSERT_MSG (false, "Multicast address not found");
    }
  
  if (m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
        .find (ucAddress) ==  m_mcEdDownlinkRelatedPerformance.at (mcAddress)
                                .edDownlinkRelatedPerformance.end())
    {
       NS_ASSERT_MSG (false, "Device not found");  
    }
  //Add the received packet to the device
  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
      .at (ucAddress).latestPacketReceived = packet->Copy ();
  
  //May be member function of the structure that will help in inserting and searching
  
  uint32_t currentTotalBytes = m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
                                .at (ucAddress).totalBytesReceived += size;  
  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
      .at (ucAddress).totalNumberOfFragmentsReceived += 1;
  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
        .at (ucAddress).throughput = (double)(currentTotalBytes*8)/ (Simulator::Now ().GetSeconds () - m_startTime.GetSeconds ()); 
}

void
LoraClassBAnalyzer::BeaconReceived (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t numberOfBeaconsReceived)
{
  NS_LOG_FUNCTION (this << mcAddress << ucAddress << numberOfBeaconsReceived);  
  
  if (m_mcEdBeaconRelatedPerformance.find (mcAddress) == m_mcEdBeaconRelatedPerformance.end())
    {
      if (mcAddress == 1)
        {
          // Unicast device is not being analyzed for now
          //\TODO  In the future add it to the unicast list 
          NS_LOG_WARN ("Unicast devices not analyzed for now! Future update");
          return;
        }    
      NS_ASSERT_MSG (false, "Multicast address not found");
    }
  if (m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance
        .find (ucAddress) == m_mcEdBeaconRelatedPerformance.at (mcAddress)
                              .edBeaconRelatedPerformance.end ())
    {
      NS_ASSERT_MSG (false, "Device not found");
    }

m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).totalBeaconReceived++;

m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).brr = 
   m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).totalBeaconReceived / 
    (m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).totalBeaconReceived +
     m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).totalBeaconLost);
  
}

void
LoraClassBAnalyzer::BeaconMissed (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t currentMissedBeacons)
{
  NS_LOG_FUNCTION (this << mcAddress << ucAddress << currentMissedBeacons);
  
  if (m_mcEdBeaconRelatedPerformance.find (mcAddress) == m_mcEdBeaconRelatedPerformance.end())
    {
      if (mcAddress == 1)
        {
          // Unicast device is not being analyzed for now
          //\TODO  In the future add it to the unicast list 
          NS_LOG_WARN ("Unicast devices not analyzed for now! Future update");
          return;
        }
      NS_ASSERT_MSG (false, "Multicast address not found");
    }
  if (m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance
        .find (ucAddress) == m_mcEdBeaconRelatedPerformance.at (mcAddress)
                              .edBeaconRelatedPerformance.end ())
    {
      NS_ASSERT_MSG (false, "Device not found");
    }  
  
  m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).totalBeaconLost++;
  m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).brr = 
     m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).totalBeaconReceived / 
      (m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).totalBeaconReceived +
       m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance.at (ucAddress).totalBeaconLost);
  
  
}

void
LoraClassBAnalyzer::CurrentBeaconMissedRunLength (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint8_t currentBeaconMissedRunLength)
{
  NS_LOG_FUNCTION (this << mcAddress << ucAddress << currentBeaconMissedRunLength);
  
  
  if (m_mcEdBeaconRelatedPerformance.find (mcAddress) == m_mcEdBeaconRelatedPerformance.end())
    {
      if (mcAddress == 1)
        {
          // Unicast device is not being analyzed for now
          //\TODO  In the future add it to the unicast list 
          NS_LOG_WARN ("Unicast devices not analyzed for now! Future update");
          return;
        }    
      NS_ASSERT_MSG (false, "Multicast address not found");
    }
  if (m_mcEdBeaconRelatedPerformance.at (mcAddress).edBeaconRelatedPerformance
        .find (ucAddress) == m_mcEdBeaconRelatedPerformance.at (mcAddress)
                              .edBeaconRelatedPerformance.end ())
    {
      NS_ASSERT_MSG (false, "Device not found");
    }

  if (m_mcEdBeaconRelatedPerformance
        .at (mcAddress).edBeaconRelatedPerformance
                          .at (ucAddress).lastBeaconLossRunLength != 0
      &&
      currentBeaconMissedRunLength == 0)
    {
      //  Selecting the maximum beaconless operation mode
      m_mcEdBeaconRelatedPerformance
        .at (mcAddress).edBeaconRelatedPerformance
                          .at (ucAddress).maximumBeaconLostInBeaconlessOperationMode = 
        std::max (m_mcEdBeaconRelatedPerformance
                    .at (mcAddress).edBeaconRelatedPerformance
                                      .at (ucAddress).maximumBeaconLostInBeaconlessOperationMode,
                  m_mcEdBeaconRelatedPerformance
                    .at (mcAddress).edBeaconRelatedPerformance
                                      .at (ucAddress).lastBeaconLossRunLength);
      
      //  Selecting the minimum beaconless operation mode
      m_mcEdBeaconRelatedPerformance
        .at (mcAddress).edBeaconRelatedPerformance
                          .at (ucAddress).minimumBeaconLostInBeaconlessOperationMode = 
        m_mcEdBeaconRelatedPerformance
                    .at (mcAddress).edBeaconRelatedPerformance
                                      .at (ucAddress).minimumBeaconLostInBeaconlessOperationMode == 0 ?
                                        m_mcEdBeaconRelatedPerformance
                                          .at (mcAddress).edBeaconRelatedPerformance
                                                            .at (ucAddress).lastBeaconLossRunLength :                                
                                        std::min (m_mcEdBeaconRelatedPerformance
                                                    .at (mcAddress).edBeaconRelatedPerformance
                                                                      .at (ucAddress).minimumBeaconLostInBeaconlessOperationMode,
                                                  m_mcEdBeaconRelatedPerformance
                                                    .at (mcAddress).edBeaconRelatedPerformance
                                                                      .at (ucAddress).lastBeaconLossRunLength);      
      //  Calculating the average beaconless operation mode                        
      m_mcEdBeaconRelatedPerformance
        .at (mcAddress).edBeaconRelatedPerformance
                          .at (ucAddress).totalBeaconLostInBeaconlessOperationMode +=
                            m_mcEdBeaconRelatedPerformance
                              .at (mcAddress).edBeaconRelatedPerformance
                                                .at (ucAddress).lastBeaconLossRunLength;
      
      m_mcEdBeaconRelatedPerformance
      .at (mcAddress).edBeaconRelatedPerformance
                        .at (ucAddress).numberOfSwitchToBeaconLessOperationModes++;  
      
      m_mcEdBeaconRelatedPerformance
        .at (mcAddress).edBeaconRelatedPerformance
                          .at (ucAddress).averageBeaconLostInBeaconlessOperationMode = 
                          (double)m_mcEdBeaconRelatedPerformance
                                    .at (mcAddress).edBeaconRelatedPerformance
                                                      .at (ucAddress).totalBeaconLostInBeaconlessOperationMode /
                                  m_mcEdBeaconRelatedPerformance
                                    .at (mcAddress).edBeaconRelatedPerformance
                                                      .at (ucAddress).numberOfSwitchToBeaconLessOperationModes;
        
    }
  
  m_mcEdBeaconRelatedPerformance
    .at (mcAddress).edBeaconRelatedPerformance
                      .at (ucAddress).lastBeaconLossRunLength =   currentBeaconMissedRunLength;
                          
}


///////////////////////////
// EndDeviceApplication //
/////////////////////////

void
LoraClassBAnalyzer::FragmentsMissed (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t currentNumberOfFragmentsMissed, uint32_t totalNumberOfFragmentsMissed)
{
  NS_LOG_FUNCTION (this << mcAddress << ucAddress << currentNumberOfFragmentsMissed << totalNumberOfFragmentsMissed);
  
//  if (m_mcEdDownlinkRelatedPerformance.find (mcAddress) == m_mcEdDownlinkRelatedPerformance.end ())
//    {
//      m_mcEdDownlinkRelatedPerformance.emplace (std::piecewise_construct, 
//                                                std::forward_as_tuple (mcAddress),
//                                                std::forward_as_tuple ()); 
//    }
//  
//  if (m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//        .find (ucAddress) ==  m_mcEdDownlinkRelatedPerformance.at (mcAddress)
//                                .edDownlinkRelatedPerformance.end())
//    {
//      m_mcEdDownlinkRelatedPerformance.at (mcAddress)
//        .edDownlinkRelatedPerformance.emplace (std::piecewise_construct,
//                                               std::forward_as_tuple (ucAddress),       
//                                               std::forward_as_tuple ());       
//    }
//  
//  //May be member function of the structure that will help in inserting and searching
//  
//  uint32_t minimumNumberOfSequentialFragmentsLost = m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                .at (ucAddress).minimumNumberOfSequentialFragmentsLost;
//  uint32_t maximumNumberOfSequentialFragmentsLost = m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                .at (ucAddress).maximumNumberOfSequentialFragmentsLost;
//  
//  //If both minimum and maximum are 0 that means we are receiving packet for the first time
//  if (minimumNumberOfSequentialFragmentsLost == 0 && maximumNumberOfSequentialFragmentsLost == 0)
//    {
//      minimumNumberOfSequentialFragmentsLost = 
//        m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                  .at (ucAddress).minimumNumberOfSequentialFragmentsLost = currentNumberOfFragmentsMissed;
//      maximumNumberOfSequentialFragmentsLost = 
//        m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                  .at (ucAddress).maximumNumberOfSequentialFragmentsLost = currentNumberOfFragmentsMissed;
//    }
//  
//  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                  .at (ucAddress).minimumNumberOfSequentialFragmentsLost =
//                                    std::min(minimumNumberOfSequentialFragmentsLost,
//                                             currentNumberOfFragmentsMissed);
//  
//  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                  .at (ucAddress).maximumNumberOfSequentialFragmentsLost = 
//                                    std::max(maximumNumberOfSequentialFragmentsLost,
//                                             currentNumberOfFragmentsMissed);
//  
//  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                .at (ucAddress).numberOfDiscontinuties++;
//  
//  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                .at (ucAddress).totalNumberOfFragmentsLost += currentNumberOfFragmentsMissed;
// 
//  NS_ASSERT (m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                .at (ucAddress).totalNumberOfFragmentsLost ==
//             totalNumberOfFragmentsMissed);  
//
//  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                .at (ucAddress).averageNumberOfSequentialFragmentsLost = 
//                                  totalNumberOfFragmentsMissed/
//                                  m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance
//                                                                  .at (ucAddress).numberOfDiscontinuties;
  
  
}

///////////////////////////
// Various Computations //
/////////////////////////


void
LoraClassBAnalyzer::ProcessPreviousPacketStatus (LoraDeviceAddress mcAddress)
{
  NS_LOG_FUNCTION (this << mcAddress);
  
  Ptr<Packet> packetSent = m_mcNsDownlinkRelatedPerformance.at (mcAddress).latestPacketSent;
  
  for (std::map<LoraDeviceAddress, struct EdDownlinkRelatedPerformance>::iterator it = 
         m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance.begin (); 
       it != m_mcEdDownlinkRelatedPerformance.at (mcAddress).edDownlinkRelatedPerformance.end ();
       ++it)
    {
      if ((*it).second.latestPacketReceived == 0 || ((*it).second.latestPacketReceived->GetUid () != packetSent->GetUid ()))
        {
          // the packet success run length has ended
          NS_LOG_DEBUG ("Packet " << packetSent->GetUid () << " Not received by " << mcAddress << "(McAddress) and " << (*it).first << "(UnicastAdress)");
          //NS_LOG_DEBUG ("LastPacketReceived "<<(*it).second.latestPacketReceived->GetUid ());
          // calculate information related to byte loss run-length
          if ( ((*it).second.currentByteSuccessRunLength != 0 &&
                (*it).second.currentPacketSuccessRunLength != 0) 
                                  || 
               ((*it).second.latestPacketReceived == 0 &&   //Check if packet reception start with a packet loss
                (*it).second.currentByteLossRunLength == 0) )
            {
              (*it).second.numberOfDiscontinuties++;
              
              // reset current run length values for successful reception
              (*it).second.currentByteSuccessRunLength = 0;
              (*it).second.currentPacketSuccessRunLength = 0;
            }
          
          // update information upon packet lost
          (*it).second.currentByteLossRunLength += packetSent->GetSize ();
          (*it).second.currentPacketLossRunLength += 1;
          (*it).second.totalBytesLost += packetSent->GetSize ();
          (*it).second.totalNumberOfFragmentsLost += 1;    
          (*it).second.prr = (double)(*it).second.totalNumberOfFragmentsReceived /
                               ( (*it).second.totalNumberOfFragmentsLost + 
                                 (*it).second.totalNumberOfFragmentsReceived );
          
          // calculate information related to byte loss run-length
          (*it).second.maximumNumberOfSequentialBytesLost = 
            std::max ((*it).second.maximumNumberOfSequentialBytesLost,
                      (*it).second.currentByteLossRunLength);

          (*it).second.averageNumberOfSequentialBytesLost =
              (double)(*it).second.totalBytesLost/(*it).second.numberOfDiscontinuties;
            
          // calculate information related to packet loss run-length
          (*it).second.maximumNumberOfSequentialFragmentsLost = 
            std::max ((*it).second.maximumNumberOfSequentialFragmentsLost,
                      (*it).second.currentPacketLossRunLength);
            
          //             
          (*it).second.averageNumberOfSequentialFragmentsLost =
            (double)(*it).second.totalNumberOfFragmentsLost/(*it).second.numberOfDiscontinuties;        
          
        
        }
      else
        {
        
        if ((*it).second.currentByteLossRunLength != 0 &&
            (*it).second.currentPacketLossRunLength != 0)
          {
            // the packet loss run length has ended
            NS_LOG_DEBUG ("Number of discontinuities for " << (*it).first << " : " << (*it).second.numberOfDiscontinuties);
 
// Minimum byte and packet loss run length
// requires adjustment, gives zero if no packet is received, because run length not terminated by success is not included           
//            (*it).second.minimumNumberOfSequentialBytesLost =
//              (*it).second.minimumNumberOfSequentialBytesLost == 0 ?
//                (*it).second.currentByteLossRunLength :
//                std::min ((*it).second.minimumNumberOfSequentialBytesLost,
//                          (*it).second.currentByteLossRunLength);
//            
//            
//            (*it).second.minimumNumberOfSequentialFragmentsLost =
//              (*it).second.minimumNumberOfSequentialFragmentsLost == 0 ?
//                (*it).second.currentPacketLossRunLength :
//                std::min ((*it).second.minimumNumberOfSequentialFragmentsLost,
//                          (*it).second.currentPacketLossRunLength);
            
            
            // reset current run length values for lost packets
            (*it).second.currentByteLossRunLength = 0;
            (*it).second.currentPacketLossRunLength = 0;
          }
        
          (*it).second.currentByteSuccessRunLength += packetSent->GetSize ();
          (*it).second.currentPacketSuccessRunLength += 1;
          
          (*it).second.prr = (double)(*it).second.totalNumberOfFragmentsReceived /
                               ( (*it).second.totalNumberOfFragmentsLost + 
                                 (*it).second.totalNumberOfFragmentsReceived );          
        }
    }
}

  
////////////////
// Utilities //
//////////////
// \TODO Add various control to print selectively
void
LoraClassBAnalyzer::FinalizeNsBeaconRelatedInformation (std::ostream& output)
{
  //Finalize
  output << "Beacon Related Performance" << std::endl;
  output << "==========================" << std::endl;
  output << "NetworkServer and Gateway Related" << std::endl;
  output << "---------------------------------" << std::endl;
  output << "Number Of Beacons Sent By Ns : " << m_nSBeaconRelatedPerformance.numberOfBeaconsSentByNs << std::endl;
  output << "Effective Number Of Beacons Tx By Gws : " << m_nSBeaconRelatedPerformance.effectiveNumberOfBeaconsTxByGws << std::endl;
  output << "Number Of Beacons Skipped By Ns : " << m_nSBeaconRelatedPerformance.numberOfBeaconsSkippedByNs << std::endl;
  output << "Average number of continuous beacons skipped by Ns : " << m_nSBeaconRelatedPerformance.averageNumberOfContinuousBeaconsSkippedByNs << std::endl;
  output << "Maximum number of continuous  beacons skipped by Ns : " << m_nSBeaconRelatedPerformance.maximumNumberOfContinuousBeaconsSkippedByNs << std::endl;
  output << "Minimum number of continuous beacons skipped by Ns : " << m_nSBeaconRelatedPerformance.minimumNumberOfContinuousBeaconsSkippedByNs << std::endl;

}

void
LoraClassBAnalyzer::FinalizeNsDownlinkRelatedInformation (std::ostream& output)
{
  output << "Class B downlink related Performance (NS)" << std::endl;
  output << "====================================" << std::endl;
  output << "Total fragments sent by NS : "<< m_totalFragementsSentbyNs << std::endl;
  output << "Total bytes send by Ns : " << m_totalBytesSentbyNs << std::endl;
  output << "Aggregate Network Throughput (bits/Sec) : " << m_aggregateNsThroughput << std::endl;
}


void
LoraClassBAnalyzer::FinalizeEdsBeaconRelatedInformation (std::ostream& output, bool verbose)
{
  output << "Class B beacon related Performance (ED)" << std::endl;
  output << "=========================================" << std::endl; 
  
  for (auto& mcGroup : m_mcEdBeaconRelatedPerformance)
    {
      output << "McGroup:" << mcGroup.first.Print () << std::endl;
    
      int numberOfDevices = 0;
    
      //Beacon reception ration
      //double averageBrr = 0; 
      //int minimimBrr = 0;
      //int maximumBrr = 0;
    
      double averageBeaconLost = 0;
      uint32_t minimumBeaconLost = 0;
      uint32_t maximumBeaconLost = 0;
    
      double averageBeaconLostRunLength = 0;
      uint32_t maximumBeaconLostRunLength = 0;
      uint32_t minimumBeaconLostRunLength = 0;
    
    
   
      for (auto& device : mcGroup.second.edBeaconRelatedPerformance)
        {
          if (verbose)
            {
              output << "   MemberDevice:" << device.first.Print () << std::endl;
              output << "       Total beacon lost : " << device.second.totalBeaconLost << std::endl; 
              output << "       Total beacon received : " << device.second.totalBeaconReceived << std::endl; 
              output << "       Total beacon lost in beaconless operation mode : " << device.second.totalBeaconLostInBeaconlessOperationMode << std::endl;              
              output << "       Average beacon lost run length " << device.second.averageBeaconLostInBeaconlessOperationMode << std::endl;
              output << "       Maximum beacon lost run length : " << device.second.maximumBeaconLostInBeaconlessOperationMode << std::endl;
              output << "       Minimum beacon lost run length : " << device.second.minimumBeaconLostInBeaconlessOperationMode << std::endl;
            }
          numberOfDevices++;
          
          averageBeaconLost += device.second.totalBeaconLost;
          averageBeaconLostRunLength +=  device.second.averageBeaconLostInBeaconlessOperationMode;
          
          if (numberOfDevices == 1)
            {
              maximumBeaconLost = device.second.totalBeaconLost;
              minimumBeaconLost = device.second.totalBeaconLost;
              
              maximumBeaconLostRunLength = device.second.maximumBeaconLostInBeaconlessOperationMode;
              minimumBeaconLostRunLength = device.second.minimumBeaconLostInBeaconlessOperationMode;
            }
          else
            {
              maximumBeaconLost = std::max (maximumBeaconLost, device.second.totalBeaconLost);
              minimumBeaconLost = std::min (minimumBeaconLost, device.second.totalBeaconLost);
              
              maximumBeaconLostRunLength = std::max (maximumBeaconLostRunLength, device.second.maximumBeaconLostInBeaconlessOperationMode);
              minimumBeaconLostRunLength = std::min (minimumBeaconLostRunLength, device.second.minimumBeaconLostInBeaconlessOperationMode);                
            }
          
        }
    
      averageBeaconLost /= numberOfDevices;
      averageBeaconLostRunLength /= numberOfDevices;
    
      output << std::endl;
      output << "AverageBeaconLost:" << averageBeaconLost << std::endl;
      output << "MaximumBeaconLost:"<< maximumBeaconLost << std::endl;
      output << "MinimumBeaconLost" << minimumBeaconLost << std::endl;
      output << "AverageBeaconLostRunLenth:"<< averageBeaconLostRunLength << std::endl;
      output << "MaximumBeaconLostRunLength:"<< maximumBeaconLostRunLength << std::endl; 
      output << "MinimumBeaconLostRunLength:"<< minimumBeaconLostRunLength << std::endl;
    
    }
}

void
LoraClassBAnalyzer::FinalizeEdsDownlinkRelatedInformation (std::ostream& output, bool verbose)
{
  output << "Class B downlink related Performance (ED)" << std::endl;
  output << "=========================================" << std::endl; 
  
  int groupIndex = 0;
  
  for (auto& mcGroup : m_mcEdDownlinkRelatedPerformance)
    {
      output << "McGroup:" << mcGroup.first.Print () << std::endl;

      int numberOfDevices = 0;
      
      //Packet reception ration = received packet/lost packet + receieved packet
      double averagePrr = 0; 
      double minPrr = 0;
      double maxPrr = 0;
    
      double averageThroughput = 0;
      double minThroughput = 0;
      double maxThroughput = 0;
    
      double averagePacketReceived = 0;
      uint32_t maxPacketReceived = 0;
      uint32_t minPacketReceived = 0;
    
      double averageBytesReceived = 0;
      uint32_t maxBytesReceived = 0;
      uint32_t minBytesReceived = 0;
    
      double averageBytesLost = 0;
      uint32_t minBytesLost = 0;
      uint32_t maxBytesLost = 0;
    
      double averagePacketLost = 0;
      uint32_t minPacketLost = 0;
      uint32_t maxPacketLost = 0;
    
      double averagePacketLostRunLength = 0;
      uint32_t minPacketLostRunLength = 0;
      uint32_t maxPacketLostRunLength = 0;
    
      double averageByteLostRunLength = 0;
      uint32_t minByteLostRunLength = 0;
      uint32_t maxByteLostRunLength = 0;
      
      // All file name will have groupIndex-dr-periodicity-numberOfEds format
      std::ofstream prr; 
      std::string prrLoc = m_verboseLocation+"prr"+std::to_string(groupIndex)+"-"
        +std::to_string ((int)mcGroup.second.dr)+"-"+std::to_string ((int)mcGroup.second.periodicity)
        +"-"+std::to_string (mcGroup.second.numberOfEds)+".csv";
      std::ofstream throughput;
      std::string throughputLoc = m_verboseLocation+"throughput"+std::to_string(groupIndex)+"-"
        +std::to_string ((int)mcGroup.second.dr)+"-"+std::to_string ((int)mcGroup.second.periodicity)
        +"-"+std::to_string (mcGroup.second.numberOfEds)+".csv";
      std::ofstream maxPacketLossRunLength;
      std::string maxPacketLossRunLengthLoc = m_verboseLocation+"maxPacketLossRunLength"+std::to_string(groupIndex)+"-"
        +std::to_string ((int)mcGroup.second.dr)+"-"+std::to_string ((int)mcGroup.second.periodicity)
        +"-"+std::to_string (mcGroup.second.numberOfEds)+".csv";
      std::ofstream avgPacketLossRunLength;
      std::string avgPacketLossRunLengthLoc = m_verboseLocation+"avgPacketLossRunLength"+std::to_string(groupIndex)+"-"
        +std::to_string ((int)mcGroup.second.dr)+"-"+std::to_string ((int)mcGroup.second.periodicity)
        +"-"+std::to_string (mcGroup.second.numberOfEds)+".csv";
      //\TODO you can add more verbose information
      
      if (verbose)
        {
          if (m_appendInformation)
            {
              prr.open (prrLoc, std::ofstream::out | std::ofstream::app); 
              throughput.open (throughputLoc, std::ofstream::out | std::ofstream::app); 
              maxPacketLossRunLength.open (maxPacketLossRunLengthLoc, std::ofstream::out | std::ofstream::app); 
              avgPacketLossRunLength.open (avgPacketLossRunLengthLoc, std::ofstream::out | std::ofstream::app);               
            }
          else 
            {
              prr.open (prrLoc, std::ofstream::out | std::ofstream::trunc); 
              throughput.open (throughputLoc, std::ofstream::out | std::ofstream::trunc); 
              maxPacketLossRunLength.open (maxPacketLossRunLengthLoc, std::ofstream::out | std::ofstream::trunc); 
              avgPacketLossRunLength.open (avgPacketLossRunLengthLoc, std::ofstream::out | std::ofstream::trunc);
            }
        }
    
      for (auto& device : mcGroup.second.edDownlinkRelatedPerformance)
        {
          if (verbose)
            {          
              output << "   MemberDevice:" << device.first.Print () << std::endl;
              
              output << "       PRR : " << device.second.prr << std::endl;
              output << "       Throughput(bits/sec) : " << device.second.throughput << std::endl; 
              output << "       TotalBytesReceived : " << device.second.totalBytesReceived << std::endl; 
              output << "       TotalFragmentsReceived : " << device.second.totalNumberOfFragmentsReceived << std::endl;              
              output << "       TotalBytesLost : " << device.second.totalBytesLost << std::endl;
              output << "       TotalFragmentsLost : " << device.second.totalNumberOfFragmentsLost << std::endl;
              
              output << "       AverageByteLostRunLength : " << device.second.averageNumberOfSequentialBytesLost << std::endl;
              output << "       MaximumByteLostRunLength : " << device.second.maximumNumberOfSequentialBytesLost << std::endl;
              output << "       MinimumByteLostRunLength : " << device.second.minimumNumberOfSequentialBytesLost << std::endl;     
              
              output << "       AveragePacketLostRunLength : " << device.second.averageNumberOfSequentialFragmentsLost << std::endl;
              output << "       MaximumPacketLostRunLength : " << device.second.maximumNumberOfSequentialFragmentsLost << std::endl;
              output << "       MinimumPacketLostRunLength : " << device.second.minimumNumberOfSequentialFragmentsLost << std::endl; 
            }

          numberOfDevices++;
          
          averagePrr += device.second.prr;
          averageThroughput += device.second.throughput;
          averagePacketReceived += device.second.totalNumberOfFragmentsReceived;
          averageBytesReceived += device.second.totalBytesReceived;
          averageBytesLost += device.second.totalBytesLost;
          averagePacketLost += device.second.totalNumberOfFragmentsLost;
            
          averagePacketLostRunLength += device.second.averageNumberOfSequentialFragmentsLost;
          averageByteLostRunLength += device.second.averageNumberOfSequentialBytesLost;
              
              
          if (numberOfDevices == 1)
            {
              maxPrr = device.second.prr;
              maxThroughput = device.second.throughput;
              maxPacketReceived = device.second.totalNumberOfFragmentsReceived;
              maxBytesReceived = device.second.totalBytesReceived;
              maxBytesLost = device.second.totalBytesLost;
              maxPacketLost = device.second.totalNumberOfFragmentsLost;
              maxPacketLostRunLength = device.second.maximumNumberOfSequentialFragmentsLost;
              maxByteLostRunLength = device.second.maximumNumberOfSequentialBytesLost; 
              
              minPrr = device.second.prr;
              minThroughput = device.second.throughput;
              minPacketReceived = device.second.totalNumberOfFragmentsReceived;
              minBytesReceived = device.second.totalBytesReceived;
              minBytesLost = device.second.totalBytesLost;
              minPacketLost = device.second.totalNumberOfFragmentsLost;
              minPacketLostRunLength = device.second.minimumNumberOfSequentialFragmentsLost;
              minByteLostRunLength = device.second.minimumNumberOfSequentialBytesLost; 
            }
          else
            {
              maxPrr = std::max (maxPrr, device.second.prr);
              maxThroughput = std::max (maxThroughput, device.second.throughput);
              maxPacketReceived = std::max (maxPacketReceived, device.second.totalNumberOfFragmentsReceived);
              maxBytesReceived = std::max (maxBytesReceived, device.second.totalBytesReceived);
              maxBytesLost = std::max (maxBytesLost, device.second.totalBytesLost);
              maxPacketLost = std::max (maxPacketLost, device.second.totalNumberOfFragmentsLost);
              maxPacketLostRunLength = std::max (maxPacketLostRunLength, device.second.maximumNumberOfSequentialFragmentsLost);
              maxByteLostRunLength = std::max (maxByteLostRunLength, device.second.maximumNumberOfSequentialBytesLost); 
              
              minPrr = std::min (minPrr, device.second.prr);
              minThroughput = std::min (minThroughput, device.second.throughput);
              minPacketReceived = std::min (minPacketReceived, device.second.totalNumberOfFragmentsReceived);
              minBytesReceived = std::min (minBytesReceived, device.second.totalBytesReceived);
              minBytesLost = std::min (minBytesLost, device.second.totalBytesLost);
              minPacketLost = std::min (minPacketLost, device.second.totalNumberOfFragmentsLost);
              minPacketLostRunLength = std::min (minPacketLostRunLength, device.second.minimumNumberOfSequentialFragmentsLost);
              minByteLostRunLength = std::min (minByteLostRunLength, device.second.minimumNumberOfSequentialBytesLost);                 
            }
          
          // If verbose, print the prr, throughput, max run-length, avg run-length
          
          if (verbose)
            {
              prr << device.second.prr << ",";
              throughput << device.second.throughput << ",";
              maxPacketLossRunLength << device.second.maximumNumberOfSequentialFragmentsLost << ",";
              avgPacketLossRunLength << device.second.averageNumberOfSequentialFragmentsLost << ",";
            }
          
      }
      
      // close the file if verbose
      if (verbose)
        {
          prr << "\n";
          throughput << "\n";
          maxPacketLossRunLength << "\n";
          avgPacketLossRunLength << "\n";
        
          
          prr.close (); 
          throughput.close (); 
          maxPacketLossRunLength.close (); 
          avgPacketLossRunLength.close ();           
        }
      
      NS_ASSERT_MSG (mcGroup.second.numberOfEds == numberOfDevices, "Counting of the number of Eds is not consistent");
      
      averagePrr /= numberOfDevices;
      averageThroughput /= numberOfDevices;
      averagePacketReceived /= numberOfDevices;
      averageBytesReceived /= numberOfDevices;
      averageBytesLost /= numberOfDevices;
      averagePacketLost /= numberOfDevices;   
      averagePacketLostRunLength /= numberOfDevices;
      averageByteLostRunLength /= numberOfDevices;       

    
      output << std::endl;  
    
      output << "averagePrr:" << averagePrr << std::endl;
      output << "minPrr:" << minPrr << std::endl;
      output << "maxPrr:" << maxPrr << std::endl;
      output << "averageThroughput:" << averageThroughput << std::endl;
      output << "minThroughput:" << minThroughput << std::endl;
      output << "maxThroughput:" << maxThroughput << std::endl;
      output << "averagePacketReceived:" << averagePacketReceived << std::endl;
      output << "maxPacketReceived:" << maxPacketReceived << std::endl;
      output << "minPacketReceived:" << minPacketReceived << std::endl;
      output << "averageBytesReceived:" << averageBytesReceived << std::endl;
      output << "maxBytesReceived:" << maxBytesReceived << std::endl;
      output << "minBytesReceived:" << minBytesReceived << std::endl;
      output << "averageBytesLost:" << averageBytesLost << std::endl;
      output << "minBytesLost:" << minBytesLost << std::endl;
      output << "maxBytesLost:" << maxBytesLost << std::endl;
      output << "averagePacketLost:" << averagePacketLost << std::endl;
      output << "minPacketLost:" << minPacketLost << std::endl;
      output << "maxPacketLost:" << maxPacketLost << std::endl;
      output << "averagePacketLostRunLength:" << averagePacketLostRunLength << std::endl;
//      output << "minPacketLostRunLength:" << minPacketLostRunLength << std::endl; ///< requires adjustment, gives zero if no packet is received, because run length not terminated by success is not includded
      output << "maxPacketLostRunLength:" << maxPacketLostRunLength << std::endl;
      output << "averageByteLostRunLength:" << averageByteLostRunLength << std::endl;
//      output << "minByteLostRunLength:" << minByteLostRunLength << std::endl; ///< requires adjustment, gives zero if no packet is received, because run length not terminated by success is not includded
      output << "maxByteLostRunLength:" << maxByteLostRunLength << std::endl;
    
      groupIndex++;
    }
  
}


void
LoraClassBAnalyzer::Analayze (Time appStopTime, std::ostringstream& simulationSetup)
{
  //Calculate throughput
  
  // We need to check for std::numeric_limites<T>::max() on the minimum computations
  // and put zero if it hasn't changed.
  
  
  
  ///////////////////////////
  std::stringstream nsOutput;
  
  // Append simulation setup information
  nsOutput << "Simulation Information (NS) " << std::endl;
  nsOutput << "=========================== " << std::endl;
  nsOutput << simulationSetup.str () << std::endl << std::endl;
  FinalizeNsBeaconRelatedInformation (nsOutput);
  FinalizeNsDownlinkRelatedInformation (nsOutput);
  nsOutput << "-------------------------------------" << std::endl << std::endl;
  
  std::cout << nsOutput.str ();
  
  std::ofstream nsfile;
  nsfile.open (m_nsLogFileName, std::ofstream::out | std::ofstream::app);
  nsfile << nsOutput.str ();
  nsfile.close ();

  ///////////////////////////  
  std::stringstream edOutput;
  
  edOutput << "Simulation Information (ED) " << std::endl;
  edOutput << "=========================== " << std::endl;  
  edOutput << simulationSetup.str () << std::endl << std::endl;
  FinalizeEdsBeaconRelatedInformation (edOutput, true);
  FinalizeEdsDownlinkRelatedInformation (edOutput, true);
  edOutput << "-------------------------------------" << std::endl << std::endl;
  
  std::cout << edOutput.str ();
  
  std::ofstream edfile;
  edfile.open (m_edLogFileName, std::ofstream::out | std::ofstream::app);
  edfile << edOutput.str ();
  edfile.close ();
  
  ///////////////////////////
}


}
}