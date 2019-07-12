/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Padova
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
 * Authors: Davide Magrin <magrinda@dei.unipd.it>
 *          Martina Capuzzo <capuzzom@dei.unipd.it>
 */

#include "ns3/network-status.h"
#include "ns3/end-device-status.h"
#include "ns3/gateway-status.h"

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/lora-device-address.h"
#include "ns3/node-container.h"
#include "ns3/log.h"
#include "ns3/pointer.h"

#include "ns3/bcn-payload.h"
namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("NetworkStatus");

NS_OBJECT_ENSURE_REGISTERED (NetworkStatus);

TypeId
NetworkStatus::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetworkStatus")
    .SetParent<Object> ()
    .AddConstructor<NetworkStatus> ()
    .SetGroupName ("lorawan")
    .AddTraceSource ("LastBeaconTransmittingGateways",
                     "The number of gateways that were able to transmit the last beacon",
                     MakeTraceSourceAccessor
                       (&NetworkStatus::m_lastBeaconTransmittingGateways),
                     "ns3::TracedValueCallback::Uint8")
    .AddTraceSource ("LastMulticastTransmittingGateways",
                     "The number of gateways that were able to transmit the last multicast transmission",
                     MakeTraceSourceAccessor
                       (&NetworkStatus::m_lastMulticastTransmittingGateways),
                     "ns3::TracedValueCallback::Uint8");
  return tid;
}

NetworkStatus::NetworkStatus () :
m_beaconDr (3),
m_beaconFrequency (869.525),
m_lastBeaconTransmittingGateways (0),
m_lastMulticastTransmittingGateways (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

NetworkStatus::~NetworkStatus ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
NetworkStatus::AddNode (Ptr<EndDeviceLoraMac> edMac)
{
  NS_LOG_FUNCTION (this << edMac);

  // Check whether this device already exists in our list
  LoraDeviceAddress edAddress = edMac->GetDeviceAddress ();
  if (m_endDeviceStatuses.find (edAddress) == m_endDeviceStatuses.end ())
    {
      // The device doesn't exist. Create new EndDeviceStatus
      Ptr<EndDeviceStatus> edStatus = CreateObject<EndDeviceStatus>
          (edAddress, edMac->GetObject<EndDeviceLoraMac>());

      // Add it to the map
      //std::pair<LoraDeviceAddress, Ptr<EndDeviceStatus> > newDeviceStatus = (edAddress, edStatus)
      m_endDeviceStatuses.insert (std::pair<LoraDeviceAddress, Ptr<EndDeviceStatus> >
                                    (edAddress, edStatus));
      NS_LOG_DEBUG ("Added to the list a device with address " <<
                    edAddress.Print ());
      
      if (edMac->IsMulticastEnabled ())
        {
          LoraDeviceAddress mcEdAddress = edMac->GetMulticastDeviceAddress ();
          
          // Check if the the multicast address already exists
          if (m_mcEndDeviceStatuses.find (mcEdAddress) == m_mcEndDeviceStatuses.end ())
            {
              //
              m_mcEndDeviceStatuses[mcEdAddress] = {std::pair<LoraDeviceAddress, Ptr<EndDeviceStatus> >(edAddress, edStatus)};
              //m_mcEndDeviceStatuses.insert ( std::pair<LoraDeviceAddress, std::pair <LoraDeviceAddress, Ptr<EndDeviceStatus> > > 
              //                               (mcEdAddress, std::map<LoraDeviceAddress, Ptr<EndDeviceStatus>{std::pair<LoraDeviceAddress, Ptr<EndDeviceStatus> >(edAddress, edStatus)}));
            }
          else
            {
              m_mcEndDeviceStatuses[mcEdAddress][edAddress] = edStatus;
            }
        }
    }
  
//  //Checking with printing all available address
//  for (McEndDeviceStatusMap::iterator i = m_mcEndDeviceStatuses.begin (); i != m_mcEndDeviceStatuses.end (); ++i)
//    {
//    std::cerr << "Multicast Address : " << i->first << std::endl;
//      for (EndDeviceStatusMap::iterator j = i->second.begin (); j != i->second.end (); ++j)
//        {
//        std::cerr << "   UnicastAddress : " << j->first << std::endl; 
//        }
//    
//    }
//  
//  std::cerr << "All the Unicast Address " << std::endl;
//  for (EndDeviceStatusMap::iterator i = m_endDeviceStatuses.begin (); i != m_endDeviceStatuses.end (); ++i)
//    {
//      std::cerr << "   UnicastAddress : " << i->first << std::endl; 
//    }
}

void
NetworkStatus::AddGateway (Address& address, Ptr<GatewayStatus> gwStatus)
{
  NS_LOG_FUNCTION (this);

  // Check whether this device already exists in the list
  if (m_gatewayStatuses.find (address) == m_gatewayStatuses.end ())
    {
      // The device doesn't exist.

      // Add it to the map
      m_gatewayStatuses.insert (std::pair<Address, Ptr<GatewayStatus> >
                                  (address, gwStatus));
      NS_LOG_DEBUG ("Added to the list a gateway with address " << address);
    }
}

void
NetworkStatus::OnReceivedPacket (Ptr<const Packet> packet,
                                 const Address& gwAddress)
{
  NS_LOG_FUNCTION (this << packet << gwAddress);

  // Create a copy of the packet
  Ptr<Packet> myPacket = packet->Copy ();

  // Extract the headers
  LoraMacHeader macHdr;
  myPacket->RemoveHeader (macHdr);
  LoraFrameHeader frameHdr;
  frameHdr.SetAsUplink ();
  myPacket->RemoveHeader (frameHdr);

  // Update the correct EndDeviceStatus object
  LoraDeviceAddress edAddr = frameHdr.GetAddress ();
  NS_LOG_DEBUG ("Node address: " << edAddr);
  m_endDeviceStatuses.at (edAddr)->InsertReceivedPacket (packet, gwAddress);
}

bool
NetworkStatus::NeedsReply (LoraDeviceAddress deviceAddress)
{
  // Throws out of range if no device is found
  return m_endDeviceStatuses.at (deviceAddress)->NeedsReply ();
}

Address
NetworkStatus::GetBestGatewayForDevice (LoraDeviceAddress deviceAddress)
{
  // Get the endDeviceStatus we are interested in
  Ptr<EndDeviceStatus> edStatus = m_endDeviceStatuses.at (deviceAddress);

  // Get the list of gateways that this device can reach
  // NOTE: At this point, we could also take into account the whole network to
  // identify the best gateway according to various metrics. For now, we just
  // ask the EndDeviceStatus to pick the best gateway for us via its method.
  Address bestGwAddress = edStatus->GetBestGatewayForReply ();

  return bestGwAddress;
}

void
NetworkStatus::SendThroughGateway (Ptr<Packet> packet, Address gwAddress)
{
  NS_LOG_FUNCTION (packet << gwAddress);

  m_gatewayStatuses.find (gwAddress)->second->GetNetDevice ()->Send (packet,
                                                                     gwAddress,
                                                                     0x0800);
}

Ptr<Packet>
NetworkStatus::GetReplyForDevice (LoraDeviceAddress edAddress, int windowNumber)
{
  // Get the reply packet
  Ptr<EndDeviceStatus> edStatus = m_endDeviceStatuses.find (edAddress)->second;
  Ptr<Packet> packet = edStatus->GetCompleteReplyPacket ();

  // Apply the appropriate tag
  LoraTag tag;
  switch (windowNumber)
    {
    case 1:
      tag.SetDataRate (edStatus->GetMac ()->GetFirstReceiveWindowDataRate ());
      tag.SetFrequency (edStatus->GetFirstReceiveWindowFrequency ());
      break;
    case 2:
      tag.SetDataRate (edStatus->GetMac ()->GetSecondReceiveWindowDataRate ());
      tag.SetFrequency (edStatus->GetSecondReceiveWindowFrequency ());
      break;
    }

  packet->AddPacketTag (tag);
  return packet;
}

Ptr<EndDeviceStatus>
NetworkStatus::GetEndDeviceStatus (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Get the address
  LoraMacHeader mHdr;
  LoraFrameHeader fHdr;
  Ptr<Packet> myPacket = packet->Copy ();
  myPacket->RemoveHeader (mHdr);
  myPacket->RemoveHeader (fHdr);
  auto it = m_endDeviceStatuses.find (fHdr.GetAddress ());
  if (it != m_endDeviceStatuses.end ())
    {
      return (*it).second;
    }
  else
    {
      NS_LOG_ERROR ("EndDeviceStatus not found");
      return 0;
    }
}

Ptr<EndDeviceStatus>
NetworkStatus::GetEndDeviceStatus (LoraDeviceAddress address)
{
  NS_LOG_FUNCTION (this << address);

  auto it = m_endDeviceStatuses.find (address);
  if (it != m_endDeviceStatuses.end ())
    {
      return (*it).second;
    }
  else
    {
      NS_LOG_ERROR ("EndDeviceStatus not found");
      return 0;
    }
}

uint32_t
NetworkStatus::BroadcastBeacon ()
{
  NS_LOG_FUNCTION_NOARGS ();
  // Time stamp to be included in the beacon payload
  uint32_t bcnTime = 0;
  // Number of gateways that successfully transmitted the beacon 
  uint8_t numberOfSuccessfulGws = 0;
  
    std::map<Address, Ptr<GatewayStatus> >::iterator it;
      
    for (it = m_gatewayStatuses.begin (); it != m_gatewayStatuses.end (); ++it)
      {
         Ptr<GatewayStatus> gwStatus = it->second;
         Ptr<GatewayLoraMac>  gwLoraMac = gwStatus->GetGatewayMac ();
         if (gwLoraMac->IsBeaconTransmissionEnabled ())
           {
             if (gwStatus->IsAvailableForTransmission (m_beaconFrequency))
              {
                //Reserve Gateway for transmission
                gwStatus->SetNextTransmissionTime (Simulator::Now ());
               
                NS_LOG_DEBUG ("Transmit beacon at on Gateway " << it->first);
                // Create an empty packet
                Ptr<Packet> bcnPacket = Create<Packet> (0);
                
                // Create the header that is that contains the beacon payload
                BcnPayload bcnPayload;
                // Generate the time stamp
                bcnTime = static_cast<uint32_t>(Simulator::Now ().GetSeconds());
                bcnPayload.SetBcnTime (bcnTime);
                // \TODO Get location of the gateway node and add it to the latitude and longitude also  
                bcnPacket->AddHeader (bcnPayload);
                
                // Configure transmission parameter and type of packet
                LoraTag tag; 
                tag.SetAsBeaconPacket (true);
                tag.SetDataRate (m_beaconDr); // Default DR for now
                tag.SetFrequency (m_beaconFrequency); // Default Frequency for now
                bcnPacket->AddPacketTag (tag);
                
                //Send the beacon packet via the gateway
                gwStatus->GetNetDevice ()->Send(bcnPacket, it->first, 0x0800);
                numberOfSuccessfulGws++;
              }
             else
              {
                NS_LOG_INFO ("Gateway " << it->first << "Is not available for beacon transmission!");
              }           
           }
        }
    
   m_lastBeaconTransmittingGateways = numberOfSuccessfulGws; 
    
  return bcnTime;  
}

uint8_t
NetworkStatus::MulticastPacket (Ptr<const Packet> packet, LoraDeviceAddress mcAddress)
{
  //Working on copy of the multicast packet to broadcast
  Ptr<Packet> packetCopy = packet->Copy ();
  
  //Find all gateways that are multicast enabled and configured to serve 
  //the multicast address provided
  //\TODO do scheduling on some of the gateways that are closer (Start with simple round rubin)
  uint8_t successfulGateways = 0; 
  for (std::map<Address, Ptr<GatewayStatus> >::iterator it = m_gatewayStatuses.begin (); it != m_gatewayStatuses.end (); ++it)
    {
      Ptr<GatewayLoraMac> gwLoraMac = (it)->second->GetGatewayMac ();
      if (gwLoraMac->IsClassBTransmissionEnabled () && gwLoraMac->CheckMulticastGroup (mcAddress))
        {
          //Get the device status of one of the end devices in the multicast group.
          //This works because an end devices class B parameter is for multicast if multicast is enabled
          McEndDeviceStatusMap::iterator it2 = m_mcEndDeviceStatuses.find (mcAddress);
          if (it2 != m_mcEndDeviceStatuses.end ())
            {
              //Getting one of the devices device status from the ones available in the list
              EndDeviceStatusMap::iterator devStatusIterator = (it2)->second.begin ();
              Ptr<EndDeviceStatus> devStatus = devStatusIterator->second;
              
              //For gateways that are classB enabled and match the address check availability
              if ((it)->second->IsAvailableForTransmission ( devStatus->GetMac ()->GetPingSlotRecieveWindowFrequency () ))
                {
                  //Reserve Gateway for transmission
                  (it)->second->SetNextTransmissionTime (Simulator::Now ());
                
                  //Prepare header and send packet
                  LoraFrameHeader frameHeader;
                  frameHeader.SetAsDownlink ();
                  //frameHeader.SetAck ()
                  frameHeader.SetAddress (mcAddress);
                  //frameHeader.SetFPending ()
                  //frameHeader.SetFCnt ()
                  //frameHeader.SetFPort ()
                  
                  LoraMacHeader macHeader;
                  macHeader.SetMType (LoraMacHeader::UNCONFIRMED_DATA_DOWN); //Since it is multicast, if confirmed a collision will happen between end-devices
                  // Set up the ACK bit on the reply
                  
                  packetCopy->AddHeader (frameHeader);
                  packetCopy->AddHeader (macHeader);
                  
                  // Apply the appropriate tag
                  LoraTag tag;
                  tag.SetFrequency (devStatus->GetMac ()->GetPingSlotRecieveWindowFrequency ());
                  tag.SetDataRate (devStatus->GetMac ()->GetPingSlotReceiveWindowDataRate ());
                  
                  packetCopy->AddPacketTag (tag);
                  
                  //Send the packet
                  SendThroughGateway (packetCopy, (it)->first);
                  successfulGateways++; // the gateway available for transmission
                }
              else 
                {
                //\TODO fire tracesource for number of gateways that were not available for transmission.
                // Reasons can be that the gateway is busy, or that the duty cycle limitation or it is reserved. 
                }
              
            }
        
        }
    }
  
  m_lastMulticastTransmittingGateways = successfulGateways;
  
  NS_LOG_DEBUG ("Multicast Sent on " << (int)successfulGateways << " gateways!");
  
  return successfulGateways; // Number of gateways for which the multicast transmission was successfully sent
}

}
}
