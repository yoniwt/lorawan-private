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

#include "ns3/end-device-class-b-app.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/lora-net-device.h"
#include "src/core/model/log-macros-enabled.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("EndDeviceClassBApp");

NS_OBJECT_ENSURE_REGISTERED (EndDeviceClassBApp);

TypeId
EndDeviceClassBApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EndDeviceClassBApp")
    .SetParent<Application> ()
    .AddConstructor<EndDeviceClassBApp> ()
    .SetGroupName ("lorawan")
    .AddAttribute ("SendingInterval", "The interval between packet sends of this app",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&EndDeviceClassBApp::GetSendingInterval,
                                     &EndDeviceClassBApp::SetSendingInterval),
                   MakeTimeChecker ())
    .AddTraceSource ("FragmentsMissed",
                     "Current Fragments missed and total number of fragments missed in case of fragmented data",
                     MakeTraceSourceAccessor 
                      (&EndDeviceClassBApp::m_fragmentsMissed),
                     "ns3::EndDeviceClassBApp::FragmentsMissed");
  // .AddAttribute ("PacketSizeRandomVariable", "The random variable that determines the shape of the packet size, in bytes",
  //                StringValue ("ns3::UniformRandomVariable[Min=0,Max=10]"),
  //                MakePointerAccessor (&EndDeviceClassBApp::m_pktSizeRV),
  //                MakePointerChecker <RandomVariableStream>());
  return tid;
}

EndDeviceClassBApp::EndDeviceClassBApp ()
  : m_sendingInterval (Seconds (10)),
  m_initialSendingDelay (Seconds (1)),
  m_basePktSize (10),
  m_pktSizeRV (0),
  m_classBDelay (Minutes (1)),
  m_nAttempt (0),
  m_countAttempt (0),
  m_uplinkEnabled (false),
  m_maxAppPayloadForDataRate {51,51,51,115,222,222,222,222}  //Max MacPayload for EU863-870, taking FOpt to be empty
{
  NS_LOG_FUNCTION_NOARGS ();
  
  m_switchToClassBTimeOutEvent = EventId ();
  m_switchToClassBTimeOutEvent.Cancel ();
  m_enableFragmentedPacketDecoder = EndDeviceClassBApp::EnableFragmentedPacketDecoder();
}

EndDeviceClassBApp::~EndDeviceClassBApp ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
EndDeviceClassBApp::BeaconLockedCallback ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("m_uplinkEnabled " << m_uplinkEnabled);
  // Canceling time out event
  m_switchToClassBTimeOutEvent.Cancel ();
  
  if (!m_uplinkEnabled)
    {
      return; 
    }
  // Start scheduling packet after you successfully switched to class B
  Simulator::Cancel (m_sendEvent);
  NS_LOG_DEBUG ("Starting to send uplink once a device is in class B after " <<
                m_initialSendingDelay.GetSeconds () << " seconds delay");
  m_sendEvent = Simulator::Schedule (m_initialSendingDelay,
                                     &EndDeviceClassBApp::SendPacket, this);
  NS_LOG_DEBUG ("Event Id: " << m_sendEvent.GetUid ());
}

void
EndDeviceClassBApp::BeaconLostCallback ()
{
  NS_LOG_FUNCTION_NOARGS ();
  
  //Canceling the time out event
  m_switchToClassBTimeOutEvent.Cancel ();
  
  //Cancel Sending of packets if we loose beacon
  m_sendEvent.Cancel ();
  
  //Re attempting
  Simulator::Schedule (m_classBDelay, 
                       &EndDeviceClassBApp::SwitchToClassB,
                       this);
}

void
EndDeviceClassBApp::ClassBDownlinkCallback (EndDeviceLoraMac::ServiceType serviceType, Ptr<const Packet> packet, uint8_t pingIndex)
{
  NS_LOG_FUNCTION (this << serviceType << packet << pingIndex);
  //Check if the fragment packet decoder is enabled
  if (m_enableFragmentedPacketDecoder.enable)
    { 
      NS_LOG_DEBUG ("Device UC Address " << m_endDeviceLoraMac->GetDeviceAddress ());
      //std::cerr << "Device UC Address " << m_endDeviceLoraMac->GetDeviceAddress ().Print () << std::endl;
      
      //Find the received and the missed packets
      uint32_t fragmentExpected = m_fragmentedPacketDecoder.expectedFragment;
      uint32_t fragmentReceived;
      
      NS_LOG_DEBUG ("ExpectedFragment = " <<fragmentExpected);
      
      uint32_t totalMissedFragments = m_fragmentedPacketDecoder.FragmentReceived (packet, fragmentReceived);
      
      NS_LOG_DEBUG ("FragmentReceived = " << fragmentReceived);
      
      uint32_t currentMissedFragments = fragmentReceived - fragmentExpected;
      
      NS_LOG_DEBUG ("Current number Of fragments missed = " << currentMissedFragments);
      
      NS_LOG_DEBUG ("Total number of fragments missed = " << totalMissedFragments);
      
      //Change the trace source to fire
      // 
      if (currentMissedFragments > 0) 
        {
          m_fragmentsMissed (m_endDeviceLoraMac->GetMulticastDeviceAddress (),
                             m_endDeviceLoraMac->GetDeviceAddress (),
                             currentMissedFragments, 
                             totalMissedFragments);
        }
      
      //NS_LOG_DEBUG ("Number of missed fragments : " << (int) missedFragments);
      //std::cerr << "Number of missed fragments : " << (int) missedFragments << std::endl;
    
    }
  
  //Do Some Logging to file and printing 
  //Do some sequence checking for FUOTA
}

void
EndDeviceClassBApp::SetSwitchToClassBDelay (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_classBDelay = delay;
}

void
EndDeviceClassBApp::SetNumberOfAttempt (uint8_t nAttempt)
{
  NS_LOG_FUNCTION (this << nAttempt);
  m_nAttempt = nAttempt;
}

void 
EndDeviceClassBApp::SwitchToClassB (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("Unicast Address : " << m_endDeviceLoraMac->GetDeviceAddress ());
  
  if (m_nAttempt!=0 && m_countAttempt > m_nAttempt)
    {
      NS_LOG_DEBUG ("Maximum number of attempt to switch to class B is reached");
      return; 
    }
  NS_ASSERT (m_endDeviceLoraMac != 0);
  
  // Request the Mac to switch to Class B and the Mac will invoke 
  // either BeaconLockedCallback or BeaconLostCallback depending on the 
  // result
  m_endDeviceLoraMac->SwitchToClassB ();
  
  m_countAttempt++;
  
  NS_LOG_DEBUG ("Total Number of Attempts " << m_countAttempt);
 // if (maxWaitingTime == Seconds (0))
 //   {
 //     NS_LOG_DEBUG ("Not attempting again!");
 //     return; 
  //  }
  
  //m_switchToClassBTimeOutEvent = Simulator::Schedule (maxWaitingTime, 
  //                                                    &EndDeviceClassBApp::SwitchToClassB,
  //                                                    this);
}

void
EndDeviceClassBApp::EnablePeriodicUplinks ()
{
  m_uplinkEnabled = true;
}

void
EndDeviceClassBApp::DisablePeriodicUplinks ()
{
  m_uplinkEnabled = false;
}


void
EndDeviceClassBApp::SetSendingInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_sendingInterval = interval;
}

Time
EndDeviceClassBApp::GetSendingInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sendingInterval;
}

void
EndDeviceClassBApp::SetSendingInitialDelay (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_initialSendingDelay = delay;
}


void
EndDeviceClassBApp::SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv)
{
  m_pktSizeRV = rv;
}


void
EndDeviceClassBApp::SetPacketSize (uint8_t size)
{
  m_basePktSize = size;
}

void 
EndDeviceClassBApp::EnableFragmentedDataReception (uint32_t first, uint32_t last)
{
  m_enableFragmentedPacketDecoder.enable = true;
  m_enableFragmentedPacketDecoder.first = first;
  m_enableFragmentedPacketDecoder.last = last;
}

void
EndDeviceClassBApp::SendPacket (void)
{
  NS_LOG_FUNCTION (this);

  // Create and send a new packet
  Ptr<Packet> packet;
  if (m_pktSizeRV)
    {
      int randomsize = m_pktSizeRV->GetInteger ();
      packet = Create<Packet> (m_basePktSize + randomsize);
    }
  else
    {
      packet = Create<Packet> (m_basePktSize);
    }
  m_mac->Send (packet);

  // Schedule the next SendPacket event
  m_sendEvent = Simulator::Schedule (m_sendingInterval, &EndDeviceClassBApp::SendPacket,
                                     this);

  NS_LOG_DEBUG ("Sent a packet of size " << packet->GetSize ());
}

void
EndDeviceClassBApp::SetMaxAppPayloadForDataRate (std::vector<uint32_t> maxAppPayloadForDataRate)
{
  m_maxAppPayloadForDataRate = maxAppPayloadForDataRate;
}

void
EndDeviceClassBApp::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  // Make sure we have a MAC layer
  if (m_mac == 0)
    {
      // Assumes there's only one device
      Ptr<LoraNetDevice> loraNetDevice = m_node->GetDevice (0)->GetObject<LoraNetDevice> ();

      m_mac = loraNetDevice->GetMac ();
      NS_ASSERT (m_mac != 0);
    }
  
  //Install callbacks on the end device Mac
  m_endDeviceLoraMac = m_mac->GetObject<EndDeviceLoraMac> ();
  NS_ASSERT (m_endDeviceLoraMac != 0);
  m_endDeviceLoraMac->SetBeaconLockedCallback (MakeCallback (&EndDeviceClassBApp::BeaconLockedCallback,this));
  m_endDeviceLoraMac->SetBeaconLostCallback (MakeCallback (&EndDeviceClassBApp::BeaconLostCallback,this));
  m_endDeviceLoraMac->SetClassBDownlinkCallback (MakeCallback (&EndDeviceClassBApp::ClassBDownlinkCallback,this));
  
  //Check if packet fragmentation is enabled and then configure
  if (m_enableFragmentedPacketDecoder.enable)
    {
      uint32_t first = m_enableFragmentedPacketDecoder.first;
      uint32_t last = m_enableFragmentedPacketDecoder.last;
        
      //If the last fragment sequence number is invalid or zero set to the max 
      if (last < first || last == 0)  last = std::numeric_limits<uint32_t>::max ();
      uint8_t dr = m_endDeviceLoraMac->GetPingSlotReceiveWindowDataRate ();
      NS_LOG_DEBUG ("Setting size of a pcket");
      uint8_t size = (uint8_t)m_maxAppPayloadForDataRate[dr];
      NS_LOG_DEBUG ("Max size of a fragment is " << (int)size);
      m_fragmentedPacketDecoder = FragmentedPacketDecoder(first, last, size); 
    }
  
  Simulator::Schedule (m_classBDelay,
                       &EndDeviceClassBApp::SwitchToClassB,
                       this);
}

void
EndDeviceClassBApp::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);
}

}
}
