/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Padova, Delft University of Technology
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
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 *         Martina Capuzzo <capuzzom@dei.unipd.it>
 *         Yonatan Woldeleul Shiferaw <yoniwt@gmail.com>
 */

#include "ns3/end-device-lora-mac.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/bcn-payload.h"
#include "src/core/model/log-macros-enabled.h"
#include "src/core/model/assert.h"
#include "ns3/hop-count-tag.h"
#include <algorithm>
#include <complex>

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("EndDeviceLoraMac");

NS_OBJECT_ENSURE_REGISTERED (EndDeviceLoraMac);

TypeId
EndDeviceLoraMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EndDeviceLoraMac")
    .SetParent<LoraMac> ()
    .SetGroupName ("lorawan")
    .AddTraceSource ("RequiredTransmissions",
                     "Total number of transmissions required to deliver this packet",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_requiredTxCallback),
                     "ns3::TracedValueCallback::uint8_t")
    .AddTraceSource ("DataRate",
                     "Data Rate currently employed by this end device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_dataRate),
                     "ns3::TracedValueCallback::uint8_t")
    .AddTraceSource ("TxPower",
                     "Transmission power currently employed by this end device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_txPower),
                     "ns3::TracedValueCallback::Double")
    .AddTraceSource ("LastKnownLinkMargin",
                     "Last known demodulation margin in "
                     "communications between this end device "
                     "and a gateway",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_lastKnownLinkMargin),
                     "ns3::TracedValueCallback::Double")
    .AddTraceSource ("LastKnownGatewayCount",
                     "Last known number of gateways able to "
                     "listen to this end device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_lastKnownGatewayCount),
                     "ns3::TracedValueCallback::Int")
    .AddTraceSource ("AggregatedDutyCycle",
                     "Aggregate duty cycle, in fraction form, "
                     "this end device must respect",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_aggregatedDutyCycle),
                     "ns3::TracedValueCallback::Double")
    .AddTraceSource ("MacState",
                     "The current Mac state of the device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_macState),
                     "ns3::TracedValueCallback::EndDeviceLoraMac::MacState")
    .AddTraceSource ("DeviceClass",
                     "The current device class of the device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_deviceClass),
                     "ns3::TracedValueCallback::EndDeviceLoraMac::DeviceClass")
    .AddTraceSource ("BeaconState",
                     "The current beacon state of the device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_beaconState),
                     "ns3::TracedValueCallback::EndDeviceLoraMac::BeaconState")    
    .AddTraceSource ("ReceivedPingMessages",
                     "The packet received via ping slot",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_receivedPingPacket),
                     "ns3::EndDeviceLoraMac::ReceivedPingPacket")
    .AddTraceSource ("FailedPings",
                     "Number of packets failed while receiving in the ping slots",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_failedPings),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("TotalSuccessfulBeaconPackets",
                     "Number of beacons successfully received during the simulation time",
                     MakeTraceSourceAccessor
                      (&EndDeviceLoraMac::m_totalSuccessfulBeaconPackets),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("TotalSuccessfulBeaconPacketsTracedCallback",
                     "Number of beacons successfully received during the simulation time",
                     MakeTraceSourceAccessor
                      (&EndDeviceLoraMac::m_totalSuccessfulBeaconPacketsTracedCallback),
                     "ns3::EndDeviceLoraMac::CustomTracedValue")  
    .AddTraceSource ("MissedBeaconCount",
                     "Number of beacons missed throughout the simulation period including during switch to class B attempts",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_missedBeaconCount),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("MissedBeaconTracedCallback",
                     "Number of beacons missed throughout the simulation period including during switch to class B attempts",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_missedBeaconTracedCallback),
                     "ns3::EndDeviceLoraMac::CustomTracedValue")  
    .AddTraceSource ("MaximumConsecutiveBeaconsMissed",
                     "The maximum number of beacons missed consecutively",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_maximumConsecutiveBeaconsMissed),
                     "ns3::TracedValueCallback::Uint8")    
    .AddTraceSource ("CurrentConsecutiveBeaconsMissed",
                     "The number of beacons missed until now consecutively if the device is in minimal beaconless operation mode",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_currentConsecutiveBeaconsMissed),
                     "ns3::TracedValueCallback::Uint8")
    .AddTraceSource ("CurrentConsecutiveBeaconsMissedTracedCallback",
                     "The number of beacons missed until now consecutively if the device is in minimal beaconless operation mode",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_currentConsecutiveBeaconsMissedTracedCallback),
                     "ns3::EndDeviceLoraMac::CustomTracedValue")  
    .AddTraceSource ("AttemptToClassB",
                     "The number of attempt in the simulation time to switch to class B",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_attemptToClassB),
                     "ns3::TracedValueCallback::Uint32") 
    .AddTraceSource ("TotalBytesReceived",
                     "The number of downlink bytes received by the device",
                     MakeTraceSourceAccessor
                       (&EndDeviceLoraMac::m_totalBytesReceived),
                     "ns3::TracedValueCallback::Uint32")  
    .AddConstructor<EndDeviceLoraMac> ();
  return tid;
}

EndDeviceLoraMac::EndDeviceLoraMac ()
  : m_enableDRAdapt (false),
  m_maxNumbTx (8),
  m_dataRate (0),
  m_txPower (14),
  m_codingRate (1),
  // LoraWAN default
  m_headerDisabled (0),
  // LoraWAN default
  m_receiveDelay1 (Seconds (1)),
  // LoraWAN default
  m_receiveDelay2 (Seconds (2)),
  // LoraWAN default
  m_receiveWindowDurationInSymbols (5),
  m_address (LoraDeviceAddress (0)),
  m_mcAddress (LoraDeviceAddress (1)),
  m_rx1DrOffset (0),
  // LoraWAN default
  m_lastKnownLinkMargin (0),
  m_lastKnownGatewayCount (0),
  m_aggregatedDutyCycle (1),
  m_mType (LoraMacHeader::UNCONFIRMED_DATA_UP),
  m_currentFCnt (0),
  m_macState (EndDeviceLoraMac::MAC_IDLE),      
  m_deviceClass (EndDeviceLoraMac::CLASS_A),
  m_beaconState (EndDeviceLoraMac::BEACON_UNLOCKED),
  m_slotIndexLastOpened (255),
  m_failedPings (0),
  m_totalSuccessfulBeaconPackets (0),
  m_missedBeaconCount (0),
  m_maximumConsecutiveBeaconsMissed (0),
  m_currentConsecutiveBeaconsMissed (0),
  m_attemptToClassB(0),
  m_totalBytesReceived (0),
  m_enableMulticast (false),
  m_relayActivated (false),
  m_relayPending (false),
  m_maxBandTxPower (27),
  m_marginTxPower (0),
  m_relayPower (14),
  maxHop (2)
{
  NS_LOG_FUNCTION (this);

  // Initialize the random variable we'll use to decide which channel to
  // transmit on.
  m_uniformRV = CreateObject<UniformRandomVariable> ();

  // Void the two receiveWindow events
  m_closeFirstWindow = EventId ();
  m_closeFirstWindow.Cancel ();
  m_closeSecondWindow = EventId ();
  m_closeSecondWindow.Cancel ();
  m_secondReceiveWindow = EventId ();
  m_secondReceiveWindow.Cancel ();

  // Void the transmission event
  m_nextTx = EventId ();
  m_nextTx.Cancel ();

  // Initialize structure for retransmission parameters
  m_retxParams = EndDeviceLoraMac::LoraRetxParameters ();
  m_retxParams.retxLeft = m_maxNumbTx;
  
  //Initializing structure for class B beacon and ping
  m_beaconInfo = EndDeviceLoraMac::BeaconInfo ();
  m_pingSlotInfo = EndDeviceLoraMac::PingSlotInfo ();
  m_classBReceiveWindowInfo = EndDeviceLoraMac::ClassBReceiveWindowInfo ();
  
  //Initialize and void ping slot Events
  //used to cancel events when device Class is switched from Class B to A
  m_pingSlotInfo.pendingPingSlotEvents.resize (128);
  for (EventId& ping : m_pingSlotInfo.pendingPingSlotEvents)
  {
    ping = EventId ();
    ping.Cancel ();
  }
  
}

EndDeviceLoraMac::~EndDeviceLoraMac ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

////////////////////////
//  Sending methods   //
////////////////////////

void
EndDeviceLoraMac::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Check that payload length is below the allowed maximum
  if (packet->GetSize () > m_maxAppPayloadForDataRate.at (m_dataRate))
    {
      NS_LOG_WARN ("Attempting to send a packet larger than the maximum allowed"
                   << " size at this DataRate (DR" << unsigned(m_dataRate) <<
                   "). Transmission canceled.");
      return;
    }
  
  // If it is not possible to transmit now because of the duty cycle,
  // or because we are receiving, schedule a tx/retx later

  Time netxTxDelay = GetNextTransmissionDelay ();
  if (netxTxDelay != Seconds (0))
    {
      // Add the ACK_TIMEOUT random delay if it is a retransmission.
      if (m_retxParams.waitingAck)
        {
          double ack_timeout = m_uniformRV->GetValue (1,3);
          netxTxDelay = netxTxDelay + Seconds (ack_timeout);
        }
      postponeTransmission (netxTxDelay, packet);
    }

  // Pick a channel on which to transmit the packet
  Ptr<LogicalLoraChannel> txChannel = GetChannelForTx ();

  if (!(txChannel && m_retxParams.retxLeft > 0))
    {
      if (!txChannel)
        {
          m_cannotSendBecauseDutyCycle (packet);
        }
      else
        {
          NS_LOG_INFO ("Max number of transmission achieved: packet not transmitted.");
        }
    }
  else
  // the transmitting channel is available and we have not run out the maximum number of retransmissions
    {
      // Make sure we can transmit at the current power on this channel
      NS_ASSERT_MSG (m_txPower <= m_channelHelper.GetTxPowerForChannel (txChannel),
                     " The selected power is too hight to be supported by this channel.");
      DoSend (packet);
    }
}

void
EndDeviceLoraMac::postponeTransmission (Time netxTxDelay, Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this);
  // Delete previously scheduled transmissions if any.
  Simulator::Cancel (m_nextTx);
  m_nextTx = Simulator::Schedule (netxTxDelay, &EndDeviceLoraMac::DoSend, this, packet);
  NS_LOG_WARN ("Attempting to send, but the aggregate duty cycle won't allow it. Scheduling a tx at a delay "
               << netxTxDelay.GetSeconds () << ".");
}


void
EndDeviceLoraMac::DoSend (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this);
  
  Time timeToSend = ResolveWithClassBAndGetTime (packet);
  
  if (timeToSend != Seconds (0))
  {
    NS_LOG_DEBUG ("Rescheduling transmission to " << Simulator::Now ()+timeToSend);
    Simulator::Schedule (timeToSend,
                         &EndDeviceLoraMac::DoSend,
                         this,
                         packet);
    return;
  }
  
  // Checking if this is the transmission of a new packet
  if (packet != m_retxParams.packet)
    {
      NS_LOG_DEBUG ("Received a new packet from application. Resetting retransmission parameters.");
      m_currentFCnt++;
      NS_LOG_DEBUG ("APP packet: " << packet << ".");

      // Add the Lora Frame Header to the packet
      LoraFrameHeader frameHdr;
      ApplyNecessaryOptions (frameHdr);
      packet->AddHeader (frameHdr);

      NS_LOG_INFO ("Added frame header of size " << frameHdr.GetSerializedSize () <<
                   " bytes.");

      // Add the Lora Mac header to the packet
      LoraMacHeader macHdr;
      ApplyNecessaryOptions (macHdr);
      packet->AddHeader (macHdr);

      // Reset MAC command list
      m_macCommandList.clear ();

      if (m_retxParams.waitingAck)
        {
          // Call the callback to notify about the failure
          uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
          m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
          NS_LOG_DEBUG (" Received new packet from the application layer: stopping retransmission procedure. Used " <<
                        unsigned(txs) << " transmissions out of a maximum of " << unsigned(m_maxNumbTx) << ".");
        }

      // Reset retransmission parameters
      resetRetransmissionParameters ();

      // If this is the first transmission of a confirmed packet, save parameters for the (possible) next retransmissions.
      if (m_mType == LoraMacHeader::CONFIRMED_DATA_UP)
        {
          m_retxParams.packet = packet->Copy ();
          m_retxParams.retxLeft = m_maxNumbTx;
          m_retxParams.waitingAck = true;
          m_retxParams.firstAttempt = Simulator::Now ();
          m_retxParams.retxLeft = m_retxParams.retxLeft - 1;   // decreasing the number of retransmissions

          NS_LOG_DEBUG ("Message type is " << m_mType);
          NS_LOG_DEBUG ("It is a confirmed packet. Setting retransmission parameters and decreasing the number of transmissions left.");

          NS_LOG_INFO ("Added MAC header of size " << macHdr.GetSerializedSize () <<
                       " bytes.");

          // Sent a new packet
          NS_LOG_DEBUG ("Copied packet: " << m_retxParams.packet);
          m_sentNewPacket (m_retxParams.packet);

          SendToPhy (m_retxParams.packet);
        }
      else
        {
          SendToPhy (packet);
        }

    }
  // this is a retransmission
  else
    {
      if (m_retxParams.waitingAck)
        {
          m_retxParams.retxLeft = m_retxParams.retxLeft - 1;   // decreasing the number of retransmissions
          NS_LOG_DEBUG ("Retransmitting an old packet.");

          SendToPhy (m_retxParams.packet);
        }
    }

}

void
EndDeviceLoraMac::SendToPhy (Ptr<Packet> packetToSend)
{
  /////////////////////////////////////////////////////////
  // Add headers, prepare TX parameters and send the packet
  /////////////////////////////////////////////////////////

  NS_LOG_DEBUG ("PacketToSend: " << packetToSend);

  // Data Rate Adaptation as in LoRaWAN specification, V1.0.2 (2016)
  if (m_enableDRAdapt && (m_dataRate > 0) && (m_retxParams.retxLeft < m_maxNumbTx) && (m_retxParams.retxLeft % 2 == 0) )
    {
      m_dataRate = m_dataRate - 1;
    }

  // Craft LoraTxParameters object
  LoraTxParameters params;
  params.sf = GetSfFromDataRate (m_dataRate);
  params.headerDisabled = m_headerDisabled;
  params.codingRate = m_codingRate;
  params.bandwidthHz = GetBandwidthFromDataRate (m_dataRate);
  params.nPreamble = m_nPreambleSymbols;
  params.crcEnabled = 1;
  params.lowDataRateOptimizationEnabled = 0;

  // Wake up PHY layer and directly send the packet

  Ptr<LogicalLoraChannel> txChannel = GetChannelForTx ();

  NS_LOG_DEBUG ("PacketToSend: " << packetToSend);
  m_phy->Send (packetToSend, params, txChannel->GetFrequency (), m_txPower);

  //////////////////////////////////////////////
  // Register packet transmission for duty cycle
  //////////////////////////////////////////////

  // Compute packet duration
  Time duration = m_phy->GetOnAirTime (packetToSend, params);

  // Register the sent packet into the DutyCycleHelper
  m_channelHelper.AddEvent (duration, txChannel);

  // Mac is now busy transmitting 
  SetMacState (MAC_TX);
  
  //////////////////////////////
  // Prepare for the downlink //
  //////////////////////////////

  // Switch the PHY to the channel so that it will listen here for downlink
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency (txChannel->GetFrequency ());

  // Instruct the PHY on the right Spreading Factor to listen for during the window
  uint8_t replyDataRate = GetFirstReceiveWindowDataRate ();
  NS_LOG_DEBUG ("m_dataRate: " << unsigned (m_dataRate) <<
                ", m_rx1DrOffset: " << unsigned (m_rx1DrOffset) <<
                ", replyDataRate: " << unsigned (replyDataRate) << ".");

  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor
    (GetSfFromDataRate (replyDataRate));
}

//////////////////////////
//  Receiving methods   //
//////////////////////////

void
EndDeviceLoraMac::Receive (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Work on a copy of the packet
  Ptr<Packet> packetCopy = packet->Copy ();
  
  BcnPayload bcnPayload; 
  packetCopy->PeekHeader(bcnPayload);
  
  NS_LOG_DEBUG ("MacState while receiving " << m_macState);
  
  if (m_macState == MAC_BEACON_RESERVED)
    { 
      if (bcnPayload.GetBcnTime() != 0)
        {
          NS_LOG_DEBUG ("BeaconRecieved!");
          BeaconReceived (packetCopy);
        }
      else
        {
          //We received a non-beacon packet so basically beacon is missed
          BeaconMissed ();
        }
   }
  
  if (m_macState == MAC_PING_SLOT || m_macState == MAC_PING_SLOT_BEACON_GUARD)
    {
      if (bcnPayload.GetBcnTime () != 0)
        {
          NS_LOG_DEBUG ("Dropping packet! BcnPacket received in a wrong slot (Ping Slot)");
          //\TODO may be fire trace source here
        }
      else
        {
          //\TODO When ping and rx2 window parameter matches: what if we receive a 
          // class A downlink during the ping slot 
      
          PingReceived (packetCopy);
        }
    }  
  
  if (m_macState == MAC_RX1 || m_macState == MAC_RX2 || m_macState == MAC_RX_BEACON_GUARD)
    {
      if (bcnPayload.GetBcnTime () != 0)
        {
          //This happens if we configure the Rx2 SF and the Beacon SF with same value
          NS_LOG_DEBUG ("Dropping packet! BcnPacket received in a wrong slot (Class A Rx2-slot)");
          //\TODO may be fire trace source here
        }
      else 
        {    
          // Remove the Mac Header to get some information
          LoraMacHeader mHdr;
          packetCopy->RemoveHeader (mHdr);

          NS_LOG_DEBUG ("Mac Header: " << mHdr);

          // Only keep analyzing the packet if it's downlink
          if (!mHdr.IsUplink ())
            {
              NS_LOG_INFO ("Found a downlink packet.");

              // Remove the Frame Header
              LoraFrameHeader fHdr;
              fHdr.SetAsDownlink ();
              packetCopy->RemoveHeader (fHdr);

              NS_LOG_DEBUG ("Frame Header: " << fHdr);

              // Determine whether this packet is for us
              bool messageForUs = (m_address == fHdr.GetAddress ());

              if (messageForUs)
                {
                  NS_LOG_INFO ("The message is for us!");

                  // If it exists, cancel the second receive window event
                  Simulator::Cancel (m_secondReceiveWindow);

                  // Parse the MAC commands
                  ParseCommands (fHdr);

                  // TODO Pass the packet up to the NetDevice


                  // Call the trace source
                  m_receivedPacket (packet);
                }
              else
                {
                  NS_LOG_DEBUG ("The message is intended for another recipient.");

                  // In this case, we are either receiving in the first receive window
                  // and finishing reception inside the second one, or receiving a
                  // packet in the second receive window and finding out, after the
                  // fact, that the packet is not for us. In either case, if we no
                  // longer have any retransmissions left, we declare failure.
                  if (m_retxParams.waitingAck && m_secondReceiveWindow.IsExpired ())
                    {
                      if (m_retxParams.retxLeft == 0)
                        {
                          uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
                          m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
                          NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

                          // Reset retransmission parameters
                          resetRetransmissionParameters ();
                        }
                    else   // Reschedule
                      {
                        this->Send (m_retxParams.packet);
                        NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
                      }
                    }
                }
              }
            else if (m_retxParams.waitingAck && m_secondReceiveWindow.IsExpired ())
              {
                NS_LOG_INFO ("The packet we are receiving is in uplink.");
                if (m_retxParams.retxLeft > 0)
                  {
                    this->Send (m_retxParams.packet);
                    NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
                  }
                else
                  {
                    uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
                    m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
                    NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

                    // Reset retransmission parameters
                    resetRetransmissionParameters ();
                  }
              }
        }
      if (m_macState == MAC_RX_BEACON_GUARD)
        {
          //packet finished being received during guard, put it back to guard
          SetMacState (MAC_BEACON_GUARD);
        }
      else
        {
          SetMacState (MAC_IDLE);
        }
    }

  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();
}

void
EndDeviceLoraMac::FailedReception (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Switch to sleep after a failed reception
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();
  
  if (m_macState == MAC_BEACON_RESERVED)
    { 
      BeaconMissed ();
    }
  
  if (m_macState == MAC_PING_SLOT || m_macState == MAC_PING_SLOT_BEACON_GUARD)
    {
      m_failedPings++;
      
      //Once the packet has failed free the MAC
      if (m_macState == MAC_PING_SLOT_BEACON_GUARD)
        {
          //If beacon guard started before end of the packet then switch back 
          //to the beacon reserved
          NS_LOG_DEBUG ("Ping failed! Switching back to Beacon Guard");
          SetMacState (MAC_BEACON_GUARD);
        }
      else if (m_macState == MAC_PING_SLOT)
        {
          NS_LOG_DEBUG ("Ping failed! Switching to IDLE");
          SetMacState (MAC_IDLE);
        }
      else
        {
          NS_LOG_ERROR ("Invalid MAC State at the End of failed ping!");
        }

    }  

  if (m_macState == MAC_RX1 || m_macState == MAC_RX2 || m_macState == MAC_RX_BEACON_GUARD)
    {
    
    if (m_secondReceiveWindow.IsExpired () && m_retxParams.waitingAck)
      {
        if (m_retxParams.retxLeft > 0)
          {
            this->Send (m_retxParams.packet);
            NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
          }
        else
          {
            uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
            m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
            NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

            // Reset retransmission parameters
            resetRetransmissionParameters ();
          }
      }
    }
}

void
EndDeviceLoraMac::ParseCommands (LoraFrameHeader frameHeader)
{
  NS_LOG_FUNCTION (this << frameHeader);

  if (m_retxParams.waitingAck)
    {
      if (frameHeader.GetAck ())
        {
          NS_LOG_INFO ("The message is an ACK, not waiting for it anymore.");

          NS_LOG_DEBUG ("Reset retransmission variables to default values and cancel retransmission if already scheduled.");

          uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
          m_requiredTxCallback (txs, true, m_retxParams.firstAttempt, m_retxParams.packet);
          NS_LOG_DEBUG ("Received ACK packet after " << unsigned(txs) << " transmissions: stopping retransmission procedure. ");

          // Reset retransmission parameters
          resetRetransmissionParameters ();

        }
      else
        {
          NS_LOG_ERROR ("Received downlink message not containing an ACK while we were waiting for it!");
        }
    }

  std::list<Ptr<MacCommand> > commands = frameHeader.GetCommands ();
  std::list<Ptr<MacCommand> >::iterator it;
  for (it = commands.begin (); it != commands.end (); it++)
    {
      NS_LOG_DEBUG ("Iterating over the MAC commands...");
      enum MacCommandType type = (*it)->GetCommandType ();
      switch (type)
        {
        case (LINK_CHECK_ANS):
          {
            NS_LOG_DEBUG ("Detected a LinkCheckAns command.");

            // Cast the command
            Ptr<LinkCheckAns> linkCheckAns = (*it)->GetObject<LinkCheckAns> ();

            // Call the appropriate function to take action
            OnLinkCheckAns (linkCheckAns->GetMargin (), linkCheckAns->GetGwCnt ());

            break;
          }
        case (LINK_ADR_REQ):
          {
            NS_LOG_DEBUG ("Detected a LinkAdrReq command.");

            // Cast the command
            Ptr<LinkAdrReq> linkAdrReq = (*it)->GetObject<LinkAdrReq> ();

            // Call the appropriate function to take action
            OnLinkAdrReq (linkAdrReq->GetDataRate (), linkAdrReq->GetTxPower (),
                          linkAdrReq->GetEnabledChannelsList (),
                          linkAdrReq->GetRepetitions ());

            break;
          }
        case (DUTY_CYCLE_REQ):
          {
            NS_LOG_DEBUG ("Detected a DutyCycleReq command.");

            // Cast the command
            Ptr<DutyCycleReq> dutyCycleReq = (*it)->GetObject<DutyCycleReq> ();

            // Call the appropriate function to take action
            OnDutyCycleReq (dutyCycleReq->GetMaximumAllowedDutyCycle ());

            break;
          }
        case (RX_PARAM_SETUP_REQ):
          {
            NS_LOG_DEBUG ("Detected a RxParamSetupReq command.");

            // Cast the command
            Ptr<RxParamSetupReq> rxParamSetupReq = (*it)->GetObject<RxParamSetupReq> ();

            // Call the appropriate function to take action
            OnRxParamSetupReq (rxParamSetupReq->GetRx1DrOffset (),
                               rxParamSetupReq->GetRx2DataRate (),
                               rxParamSetupReq->GetFrequency ());

            break;
          }
        case (DEV_STATUS_REQ):
          {
            NS_LOG_DEBUG ("Detected a DevStatusReq command.");

            // Cast the command
            Ptr<DevStatusReq> devStatusReq = (*it)->GetObject<DevStatusReq> ();

            // Call the appropriate function to take action
            OnDevStatusReq ();

            break;
          }
        case (NEW_CHANNEL_REQ):
          {
            NS_LOG_DEBUG ("Detected a NewChannelReq command.");

            // Cast the command
            Ptr<NewChannelReq> newChannelReq = (*it)->GetObject<NewChannelReq> ();

            // Call the appropriate function to take action
            OnNewChannelReq (newChannelReq->GetChannelIndex (), newChannelReq->GetFrequency (), newChannelReq->GetMinDataRate (), newChannelReq->GetMaxDataRate ());

            break;
          }
        case (RX_TIMING_SETUP_REQ):
          {
            break;
          }
        case (TX_PARAM_SETUP_REQ):
          {
            break;
          }
        case (DL_CHANNEL_REQ):
          {
            break;
          }
        default:
          {
            NS_LOG_ERROR ("CID not recognized");
            break;
          }
        }
    }

}

void
EndDeviceLoraMac::ApplyNecessaryOptions (LoraFrameHeader& frameHeader)
{
  NS_LOG_FUNCTION_NOARGS ();

  frameHeader.SetAsUplink ();
  frameHeader.SetFPort (1);             // TODO Use an appropriate frame port based on the application
  frameHeader.SetAddress (m_address);
  frameHeader.SetAdr (0);             // TODO Set ADR if a member variable is true
  frameHeader.SetAdrAckReq (0);             // TODO Set ADRACKREQ if a member variable is true
  if (m_mType == LoraMacHeader::CONFIRMED_DATA_UP)
    {
      frameHeader.SetAck (1);
    }
  else
    {
      frameHeader.SetAck (0);
    }
  // FPending indicate ClassB for uplink (\TODO Add separate Set/Get Method for classB in LoraFrameHeader)
  frameHeader.SetClassB ((m_deviceClass == CLASS_B));
  
  frameHeader.SetFCnt (m_currentFCnt);

  // Add listed MAC commands
  for (const auto &command : m_macCommandList)
    {
      NS_LOG_INFO ("Applying a MAC Command of CID " <<
                   unsigned(MacCommand::GetCIDFromMacCommand
                              (command->GetCommandType ())));

      frameHeader.AddCommand (command);
    }

}

void
EndDeviceLoraMac::ApplyNecessaryOptions (LoraMacHeader& macHeader)
{
  NS_LOG_FUNCTION_NOARGS ();

  macHeader.SetMType (m_mType);
  macHeader.SetMajor (1);
}

void
EndDeviceLoraMac::SetMType (LoraMacHeader::MType mType)
{
  m_mType = mType;
  NS_LOG_DEBUG ("Message type is set to " << mType);
}

LoraMacHeader::MType
EndDeviceLoraMac::GetMType (void)
{
  return m_mType;
}

void
EndDeviceLoraMac::TxFinished (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION_NOARGS ();

  //If it was transmitting during the ping slot, it is cooprative relaying
  if (m_macState == MAC_PING_SLOT )
    {
      NS_LOG_DEBUG ("Relaying done!");
      
      // Switch the PHY to sleep
      m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();
  
      NS_LOG_DEBUG ("Switching to idle!");
      
      // Mac state is free now 
      SetMacState (MAC_IDLE);
      
      //Don't schedule receive windows
      return;
    }
  else if (m_macState == MAC_PING_SLOT_BEACON_GUARD)
    {
      NS_LOG_DEBUG ("Relaying done!");
      
      // Switch the PHY to sleep
      m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();
  
      NS_LOG_DEBUG ("Switching to beacon guard!");
      
      // Mac state is free now 
      SetMacState (MAC_BEACON_GUARD);
      
      //Don't schedule receive windows
      return;    
    }
  
  // Schedule the opening of the first receive window
  Simulator::Schedule (m_receiveDelay1,
                       &EndDeviceLoraMac::OpenFirstReceiveWindow, this);

  // Schedule the opening of the second receive window
  m_secondReceiveWindow = Simulator::Schedule (m_receiveDelay2,
                                               &EndDeviceLoraMac::OpenSecondReceiveWindow,
                                               this);

  // Switch the PHY to sleep
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();
  
  // Mac state is free now 
  SetMacState (MAC_IDLE);
}

void
EndDeviceLoraMac::OpenFirstReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  //Calculate the duration of a single symbol for the first receive window DR
  double tSym = pow (2, GetSfFromDataRate (GetFirstReceiveWindowDataRate ())) / GetBandwidthFromDataRate ( GetFirstReceiveWindowDataRate ());
  
  // Schedule return to sleep after "at least the time required by the end
  // device's radio transceiver to effectively detect a downlink preamble"
  // (LoraWAN specification)
  m_closeFirstWindow = Simulator::Schedule (Seconds (m_receiveWindowDurationInSymbols*tSym),
                                            &EndDeviceLoraMac::CloseFirstReceiveWindow, this); //m_receiveWindowDuration
  
  // Mac state is serving first receive window 
  SetMacState (MAC_RX1);
}

void
EndDeviceLoraMac::CloseFirstReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // Check the Phy layer's state:
  // - RX -> We are receiving a preamble.
  // - STANDBY -> Nothing was received.
  // - SLEEP -> We have received a packet.
  // We should never be in TX or SLEEP mode at this point
  switch (phy->GetState ())
    {
    case EndDeviceLoraPhy::TX:
      NS_ABORT_MSG ("PHY was in TX mode when attempting to " <<
                    "close a receive window.");
      break;
    case EndDeviceLoraPhy::RX:
      // PHY is receiving: let it finish. The Receive method will switch it back to SLEEP.
      break;
    case EndDeviceLoraPhy::SLEEP:
      // PHY has received, and the MAC's Receive already put the device to sleep
      break;
    case EndDeviceLoraPhy::STANDBY:
      // Turn PHY layer to SLEEP
      phy->SwitchToSleep ();
      // Mac state is free now 
      SetMacState (MAC_IDLE);
      break;
    }
}

void
EndDeviceLoraMac::OpenSecondReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Check for receiver status: if it's locked on a packet, don't open this
  // window at all.
  if (m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::RX)
    {
      NS_LOG_INFO ("Won't open second receive window since we are in RX mode.");

      return;
    }

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  // Switch to appropriate channel and data rate
  NS_LOG_INFO ("Using parameters: " << m_secondReceiveWindowFrequency << "Hz, DR"
                                    << unsigned(m_secondReceiveWindowDataRate));

  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency
    (m_secondReceiveWindowFrequency);
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor (GetSfFromDataRate
                                                               (m_secondReceiveWindowDataRate));
  
  //Calculate the duration of a single symbol for the second receive window DR
  double tSym = pow (2, GetSfFromDataRate (GetSecondReceiveWindowDataRate ())) / GetBandwidthFromDataRate ( GetSecondReceiveWindowDataRate ());
    
  // Schedule return to sleep after "at least the time required by the end
  // device's radio transceiver to effectively detect a downlink preamble"
  // (LoraWAN specification)
  m_closeSecondWindow = Simulator::Schedule (Seconds (m_receiveWindowDurationInSymbols*tSym),
                                             &EndDeviceLoraMac::CloseSecondReceiveWindow, this);
  
  // Mac state is serving second receive window 
  SetMacState (MAC_RX2);
}

void
EndDeviceLoraMac::CloseSecondReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // NS_ASSERT (phy->m_state != EndDeviceLoraPhy::TX &&
  // phy->m_state != EndDeviceLoraPhy::SLEEP);

  // Check the Phy layer's state:
  // - RX -> We have received a preamble.
  // - STANDBY -> Nothing was detected.
  switch (phy->GetState ())
    {
    case EndDeviceLoraPhy::TX:
      break;
    case EndDeviceLoraPhy::SLEEP:
      break;
    case EndDeviceLoraPhy::RX:
      // PHY is receiving: let it finish
      NS_LOG_DEBUG ("PHY is receiving: Receive will handle the result.");
      return;
    case EndDeviceLoraPhy::STANDBY:
      // Turn PHY layer to sleep
      phy->SwitchToSleep ();
      // Mac state is free now 
     SetMacState (MAC_IDLE);
      break;
    }

  if (m_retxParams.waitingAck)
    {
      NS_LOG_DEBUG ("No reception initiated by PHY: rescheduling transmission.");
      if (m_retxParams.retxLeft > 0 )
        {
          NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
          this->Send (m_retxParams.packet);
        }

      else if (m_retxParams.retxLeft == 0 && m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () != EndDeviceLoraPhy::RX)
        {
          uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
          m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
          NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

          // Reset retransmission parameters
          resetRetransmissionParameters ();
        }

      else
        {
          NS_LOG_ERROR ("The number of retransmissions left is negative ! ");
        }
    }
  else
    {
      uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft );
      m_requiredTxCallback (txs, true, m_retxParams.firstAttempt, m_retxParams.packet);
      NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) <<
                   " transmissions left. We were not transmitting confirmed messages.");

      // Reset retransmission parameters
      resetRetransmissionParameters ();
    }
}

Time
EndDeviceLoraMac::GetNextTransmissionDelay (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  //    Check duty cycle    //

  // Pick a random channel to transmit on
  std::vector<Ptr<LogicalLoraChannel> > logicalChannels;
  logicalChannels = m_channelHelper.GetEnabledChannelList ();             // Use a separate list to do the shuffle
  //logicalChannels = Shuffle (logicalChannels);

  NS_LOG_DEBUG ("lungh lista " << logicalChannels.size ());

  Time waitingTime = Time::Max ();

  // Try every channel
  std::vector<Ptr<LogicalLoraChannel> >::iterator it;
  for (it = logicalChannels.begin (); it != logicalChannels.end (); ++it)
    {
      // Pointer to the current channel
      Ptr<LogicalLoraChannel> logicalChannel = *it;
      double frequency = logicalChannel->GetFrequency ();

      waitingTime = std::min (waitingTime, m_channelHelper.GetWaitingTime (logicalChannel));

      NS_LOG_DEBUG ("Waiting time before the next transmission in channel with frequecy " <<
                    frequency << " is = " << waitingTime.GetSeconds () << ".");
    }


  //    Check if there are receiving windows    //

  if (!m_closeFirstWindow.IsExpired () || !m_closeSecondWindow.IsExpired () || !m_secondReceiveWindow.IsExpired () )
    {
      NS_LOG_WARN ("Attempting to send when there are receive windows:" <<
                   " Transmission postponed.");
      
      //Calculate the duration of a single symbol for the second receive window DR
      double tSym = pow (2, GetSfFromDataRate (GetSecondReceiveWindowDataRate ())) / GetBandwidthFromDataRate ( GetSecondReceiveWindowDataRate ());
      
      Time endSecondRxWindow = (m_receiveDelay2 + Seconds (m_receiveWindowDurationInSymbols*tSym));
      waitingTime = std::max (waitingTime, endSecondRxWindow);
    }
  
  //  Check if there are any pending transmissions //

  return waitingTime;
}

Ptr<LogicalLoraChannel>
EndDeviceLoraMac::GetChannelForTx (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Pick a random channel to transmit on
  std::vector<Ptr<LogicalLoraChannel> > logicalChannels;
  logicalChannels = m_channelHelper.GetEnabledChannelList ();             // Use a separate list to do the shuffle
  logicalChannels = Shuffle (logicalChannels);

  // Try every channel
  std::vector<Ptr<LogicalLoraChannel> >::iterator it;
  for (it = logicalChannels.begin (); it != logicalChannels.end (); ++it)
    {
      // Pointer to the current channel
      Ptr<LogicalLoraChannel> logicalChannel = *it;
      double frequency = logicalChannel->GetFrequency ();

      NS_LOG_DEBUG ("Frequency of the current channel: " << frequency);

      // Verify that we can send the packet
      Time waitingTime = m_channelHelper.GetWaitingTime (logicalChannel);

      NS_LOG_DEBUG ("Waiting time for current channel = " <<
                    waitingTime.GetSeconds ());

      // Send immediately if we can
      if (waitingTime == Seconds (0))
        {
          return *it;
        }
      else
        {
          NS_LOG_DEBUG ("Packet cannot be immediately transmitted on " <<
                        "the current channel because of duty cycle limitations.");
        }
    }
  return 0;             // In this case, no suitable channel was found
}


std::vector<Ptr<LogicalLoraChannel> >
EndDeviceLoraMac::Shuffle (std::vector<Ptr<LogicalLoraChannel> > vector)
{
  NS_LOG_FUNCTION_NOARGS ();

  int size = vector.size ();

  for (int i = 0; i < size; ++i)
    {
      uint16_t random = std::floor (m_uniformRV->GetValue (0, size));
      Ptr<LogicalLoraChannel> temp = vector.at (random);
      vector.at (random) = vector.at (i);
      vector.at (i) = temp;
    }

  return vector;
}

/////////////////////////
// Setters and Getters //
/////////////////////////

void EndDeviceLoraMac::resetRetransmissionParameters ()
{
  m_retxParams.waitingAck = false;
  m_retxParams.retxLeft = m_maxNumbTx;
  m_retxParams.packet = 0;
  m_retxParams.firstAttempt = Seconds (0);

  // Cancel next retransmissions, if any
  Simulator::Cancel (m_nextTx);
}

void
EndDeviceLoraMac::SetDataRateAdaptation (bool adapt)
{
  NS_LOG_FUNCTION (this << adapt);
  m_enableDRAdapt = adapt;
}

bool
EndDeviceLoraMac::GetDataRateAdaptation (void)
{
  return m_enableDRAdapt;
}

void
EndDeviceLoraMac::SetMaxNumberOfTransmissions (uint8_t maxNumbTx)
{
  NS_LOG_FUNCTION (this << unsigned(maxNumbTx));
  m_maxNumbTx = maxNumbTx;
  m_retxParams.retxLeft = maxNumbTx;
}

uint8_t
EndDeviceLoraMac::GetMaxNumberOfTransmissions (void)
{
  NS_LOG_FUNCTION (this );
  return m_maxNumbTx;
}


void
EndDeviceLoraMac::SetDataRate (uint8_t dataRate)
{
  NS_LOG_FUNCTION (this << unsigned (dataRate));

  m_dataRate = dataRate;
}

uint8_t
EndDeviceLoraMac::GetDataRate (void)
{
  NS_LOG_FUNCTION (this);

  return m_dataRate;
}

void
EndDeviceLoraMac::SetDeviceAddress (LoraDeviceAddress address)
{
  NS_LOG_FUNCTION (this << address);

  m_address = address;
}

LoraDeviceAddress
EndDeviceLoraMac::GetDeviceAddress (void)
{
  NS_LOG_FUNCTION (this);

  return m_address;
}

void
EndDeviceLoraMac::SetMulticastDeviceAddress(LoraDeviceAddress address)
{
  NS_LOG_FUNCTION (this << address);
  
  if (address.Get () == 1)
    {
      NS_LOG_ERROR ("multicast address has to be different from 1");
      return;
    }
  
  NS_ASSERT_MSG (address != m_address, "Multicast and Unicast can not have same Address");
  
  m_mcAddress = address;
}

LoraDeviceAddress
EndDeviceLoraMac::GetMulticastDeviceAddress()
{
  NS_LOG_FUNCTION (this);
  
  return m_mcAddress;
}

void
EndDeviceLoraMac::OnLinkCheckAns (uint8_t margin, uint8_t gwCnt)
{
  NS_LOG_FUNCTION (this << unsigned(margin) << unsigned(gwCnt));

  m_lastKnownLinkMargin = margin;
  m_lastKnownGatewayCount = gwCnt;
}

void
EndDeviceLoraMac::OnLinkAdrReq (uint8_t dataRate, uint8_t txPower,
                                std::list<int> enabledChannels, int repetitions)
{
  NS_LOG_FUNCTION (this << unsigned (dataRate) << unsigned (txPower) <<
                   repetitions);

  // Three bools for three requirements before setting things up
  bool channelMaskOk = true;
  bool dataRateOk = true;
  bool txPowerOk = true;

  // Check the channel mask
  /////////////////////////
  // Check whether all specified channels exist on this device
  auto channelList = m_channelHelper.GetChannelList ();
  int channelListSize = channelList.size ();

  for (auto it = enabledChannels.begin (); it != enabledChannels.end (); it++)
    {
      if ((*it) > channelListSize)
        {
          channelMaskOk = false;
          break;
        }
    }

  // Check the dataRate
  /////////////////////
  // We need to know we can use it at all
  // To assess this, we try and convert it to a SF/BW combination and check if
  // those values are valid. Since GetSfFromDataRate and
  // GetBandwidthFromDataRate return 0 if the dataRate is not recognized, we
  // can check against this.
  uint8_t sf = GetSfFromDataRate (dataRate);
  double bw = GetBandwidthFromDataRate (dataRate);
  NS_LOG_DEBUG ("SF: " << unsigned (sf) << ", BW: " << bw);
  if (sf == 0 || bw == 0)
    {
      dataRateOk = false;
      NS_LOG_DEBUG ("Data rate non valid");
    }

  // We need to know we can use it in at least one of the enabled channels
  // Cycle through available channels, stop when at least one is enabled for the
  // specified dataRate.
  if (dataRateOk && channelMaskOk)             // If false, skip the check
    {
      bool foundAvailableChannel = false;
      for (auto it = enabledChannels.begin (); it != enabledChannels.end (); it++)
        {
          NS_LOG_DEBUG ("MinDR: " << unsigned (channelList.at (*it)->GetMinimumDataRate ()));
          NS_LOG_DEBUG ("MaxDR: " << unsigned (channelList.at (*it)->GetMaximumDataRate ()));
          if (channelList.at (*it)->GetMinimumDataRate () <= dataRate
              && channelList.at (*it)->GetMaximumDataRate () >= dataRate)
            {
              foundAvailableChannel = true;
              break;
            }
        }

      if (!foundAvailableChannel)
        {
          dataRateOk = false;
          NS_LOG_DEBUG ("Available channel not found");
        }
    }

  // Check the txPower
  ////////////////////
  // Check whether we can use this transmission power
  if (GetDbmForTxPower (txPower) == 0)
    {
      txPowerOk = false;
    }

  NS_LOG_DEBUG ("Finished checking. " <<
                "ChannelMaskOk: " << channelMaskOk << ", " <<
                "DataRateOk: " << dataRateOk << ", " <<
                "txPowerOk: " << txPowerOk);

  // If all checks are successful, set parameters up
  //////////////////////////////////////////////////
  if (channelMaskOk && dataRateOk && txPowerOk)
    {
      // Cycle over all channels in the list
      for (uint32_t i = 0; i < m_channelHelper.GetChannelList ().size (); i++)
        {
          if (std::find (enabledChannels.begin (), enabledChannels.end (), i) != enabledChannels.end ())
            {
              m_channelHelper.GetChannelList ().at (i)->SetEnabledForUplink ();
              NS_LOG_DEBUG ("Channel " << i << " enabled");
            }
          else
            {
              m_channelHelper.GetChannelList ().at (i)->DisableForUplink ();
              NS_LOG_DEBUG ("Channel " << i << " disabled");
            }
        }

      // Set the data rate
      m_dataRate = dataRate;

      // Set the transmission power
      m_txPower = GetDbmForTxPower (txPower);
    }

  // Craft a LinkAdrAns MAC command as a response
  ///////////////////////////////////////////////
  m_macCommandList.push_back (CreateObject<LinkAdrAns> (txPowerOk, dataRateOk,
                                                        channelMaskOk));
}

void
EndDeviceLoraMac::OnDutyCycleReq (double dutyCycle)
{
  NS_LOG_FUNCTION (this << dutyCycle);

  // Make sure we get a value that makes sense
  NS_ASSERT (0 <= dutyCycle && dutyCycle < 1);

  // Set the new duty cycle value
  m_aggregatedDutyCycle = dutyCycle;

  // Craft a DutyCycleAns as response
  NS_LOG_INFO ("Adding DutyCycleAns reply");
  m_macCommandList.push_back (CreateObject<DutyCycleAns> ());
}

void
EndDeviceLoraMac::OnRxParamSetupReq (uint8_t rx1DrOffset, uint8_t rx2DataRate, double frequency)
{
  NS_LOG_FUNCTION (this << unsigned (rx1DrOffset) << unsigned (rx2DataRate) <<
                   frequency);

  bool offsetOk = true;
  bool dataRateOk = true;

  // Check that the desired offset is valid
  if ( !(0 <= rx1DrOffset && rx1DrOffset <= 5))
    {
      offsetOk = false;
    }

  // Check that the desired data rate is valid
  if (GetSfFromDataRate (rx2DataRate) == 0
      || GetBandwidthFromDataRate (rx2DataRate) == 0)
    {
      dataRateOk = false;
    }

  // For now, don't check for validity of frequency
  if (offsetOk && dataRateOk)
    {
      m_secondReceiveWindowDataRate = rx2DataRate;
      m_rx1DrOffset = rx1DrOffset;
      m_secondReceiveWindowFrequency = frequency;
    }

  // Craft a RxParamSetupAns as response
  NS_LOG_INFO ("Adding RxParamSetupAns reply");
  m_macCommandList.push_back (CreateObject<RxParamSetupAns> (offsetOk,
                                                             dataRateOk, true));
}

void
EndDeviceLoraMac::OnDevStatusReq (void)
{
  NS_LOG_FUNCTION (this);

  uint8_t battery = 10;             // XXX Fake battery level
  uint8_t margin = 10;             // XXX Fake margin

  // Craft a RxParamSetupAns as response
  NS_LOG_INFO ("Adding DevStatusAns reply");
  m_macCommandList.push_back (CreateObject<DevStatusAns> (battery, margin));
}

void
EndDeviceLoraMac::OnNewChannelReq (uint8_t chIndex, double frequency, uint8_t minDataRate, uint8_t maxDataRate)
{
  NS_LOG_FUNCTION (this);

  bool dataRateRangeOk = true;             // XXX Check whether the new data rate range is ok
  bool channelFrequencyOk = true;             // XXX Check whether the frequency is ok

  // TODO Return false if one of the checks above failed
  // TODO Create new channel in the LogicalLoraChannelHelper

  SetLogicalChannel (chIndex, frequency, minDataRate, maxDataRate);

  NS_LOG_INFO ("Adding NewChannelAns reply");
  m_macCommandList.push_back (CreateObject<NewChannelAns> (dataRateRangeOk,
                                                           channelFrequencyOk));
}

void
EndDeviceLoraMac::AddLogicalChannel (double frequency)
{
  NS_LOG_FUNCTION (this << frequency);

  m_channelHelper.AddChannel (frequency);
}

void
EndDeviceLoraMac::AddLogicalChannel (Ptr<LogicalLoraChannel> logicalChannel)
{
  NS_LOG_FUNCTION (this << logicalChannel);

  m_channelHelper.AddChannel (logicalChannel);
}

void
EndDeviceLoraMac::SetLogicalChannel (uint8_t chIndex, double frequency,
                                     uint8_t minDataRate, uint8_t maxDataRate)
{
  NS_LOG_FUNCTION (this << unsigned (chIndex) << frequency <<
                   unsigned (minDataRate) << unsigned(maxDataRate));

  m_channelHelper.SetChannel (chIndex, CreateObject<LogicalLoraChannel>
                                (frequency, minDataRate, maxDataRate));
}

void
EndDeviceLoraMac::AddSubBand (double startFrequency, double endFrequency, double dutyCycle, double maxTxPowerDbm)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_channelHelper.AddSubBand (startFrequency, endFrequency, dutyCycle, maxTxPowerDbm);
}

uint8_t
EndDeviceLoraMac::GetFirstReceiveWindowDataRate (void)
{
  return m_replyDataRateMatrix.at (m_dataRate).at (m_rx1DrOffset);
}

void
EndDeviceLoraMac::SetSecondReceiveWindowDataRate (uint8_t dataRate)
{
  m_secondReceiveWindowDataRate = dataRate;
}

uint8_t
EndDeviceLoraMac::GetSecondReceiveWindowDataRate (void)
{
  return m_secondReceiveWindowDataRate;
}

void
EndDeviceLoraMac::SetSecondReceiveWindowFrequency (double frequencyMHz)
{
  m_secondReceiveWindowFrequency = frequencyMHz;
}

double
EndDeviceLoraMac::GetSecondReceiveWindowFrequency (void)
{
  return m_secondReceiveWindowFrequency;
}

double
EndDeviceLoraMac::GetAggregatedDutyCycle (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  return m_aggregatedDutyCycle;
}

void
EndDeviceLoraMac::AddMacCommand (Ptr<MacCommand> macCommand)
{
  NS_LOG_FUNCTION (this << macCommand);

  m_macCommandList.push_back (macCommand);
}

uint8_t
EndDeviceLoraMac::GetTransmissionPower (void)
{
  return m_txPower;
}

//////////////////////////////////////////////////////
// Conflict resolution Between Class A and Class B //
////////////////////////////////////////////////////

Time
EndDeviceLoraMac::ResolveWithClassBAndGetTime(Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION_NOARGS ();
  // Here we can put different algorithms with if-else so that another API 
  // could set what algorithm to use for certain end device from the MAC 
  // Helper function
  
  //Experiment with your algorithms here
  
  // Algorithm 1
  {
    if (m_macState == MAC_BEACON_GUARD)
      {
        NS_LOG_LOGIC ("Collision with beacon guard! implement Algorithm 1");
        return  Simulator::GetDelayLeft (m_beaconInfo.endBeaconGuardEvent)
                +
                Simulator::GetDelayLeft (m_beaconInfo.endBeaconReservedEvent)
                + 
                Seconds (m_uniformRV->GetValue (0, (m_pingSlotInfo.pingOffset)*m_pingSlotInfo.slotLen.GetSeconds()));
      }
    else if (m_macState == MAC_BEACON_RESERVED)
      {
        NS_LOG_LOGIC ("Collision with beacon reserved! implement Algorithm 1");
        return Simulator::GetDelayLeft(m_beaconInfo.endBeaconReservedEvent)
               +
               Seconds (m_uniformRV->GetValue (0, (m_pingSlotInfo.pingOffset)*m_pingSlotInfo.slotLen.GetSeconds()));
      }
    else if (m_macState == MAC_PING_SLOT)
      {
        NS_LOG_LOGIC ("Collision with ping slot! implement Algorithm 1");
        return Seconds (m_uniformRV->GetValue (0, (m_pingSlotInfo.pingOffset)*m_pingSlotInfo.slotLen.GetSeconds()));
      }
    else if (m_deviceClass == CLASS_A)
      {
        NS_LOG_LOGIC ("No Collision with Class B! Device operating in Class A");
        // If we are in idle or the class is even not Class B
        return Seconds (0);
      }
    else if (m_deviceClass == CLASS_B)
     {
       //Make sure the transmission and the rx windows do not pass beyond the 
       //beacon guard. Now for easy computation take the worst case
       //Variation of this with perfect airtime calculation could be tried.
      
       //Calculating the time when the beacon will start   
       Time nextGuard = Simulator::GetDelayLeft (m_beaconInfo.nextBeaconGuardEvent);
       
       //Calculating the total time required for TX+RX1+RX2 time
       //The Longest Tx time on SF 12 is 2,465.79ms (2.457seconds)
       Time longestTx = Seconds (2.5);
       //Either of the rx1 or the rx2 window will be used each opened for 5 symbols
       //worst case rx1
       Time rx1 = MilliSeconds (163.84);
       //default rx2
       Time rx2 = MilliSeconds (163.84);
       
       if (nextGuard < longestTx+m_receiveDelay1+rx1+m_receiveDelay2+rx2)
         {
           //If the tx+rx1+rx2 don't fit postpone at the end of the transmission
           return nextGuard + Seconds (5.12) + Seconds (m_uniformRV->GetValue (0, (m_pingSlotInfo.pingOffset)*m_pingSlotInfo.slotLen.GetSeconds()));
         }
       else 
         {
           return Seconds (0);
         }
     }
    else
      {
        NS_LOG_LOGIC ("No immediate collision with Class B!");
      }
  
  }
  
  //Algorithm 2 (Smart device solution): Have an if else statement control the API
  {
    // When collision happens with beacon guard or reserved we can end until the
    // end and randomly schedule
  
    // When the device is already in receive mode
    
    // When the tx and its rx windows fit with in a ping by calculating air time
  
    // When only the tx fits within the ping
  
    // When nothing fits
  
  }
  
  // Algorithm 3 
  // With zero effect on Class A. Class A always gets priority
  {
  
  
  
  
  }
  
  //Algorithm 4
  // With zero effect on Class B. Class B always gets priority
  {
  
  
  
  }
  
  
  return Seconds (0);
}

///////////////////////////////////////////////////
//  LoRaWAN Class B related  procedures         //
/////////////////////////////////////////////////

void 
EndDeviceLoraMac::SwitchToClassB (void)
{
  NS_LOG_FUNCTION_NOARGS (); 
  
  if (m_deviceClass == CLASS_C)
    {
      NS_LOG_ERROR ("Can't switch to Class B from Class C! You need to switch to class A first");
      return;
    }
  
  if (m_deviceClass == CLASS_B)
    {
      NS_LOG_INFO ("Device already operating in Class B!");
      return;
    }
  
  if (m_beaconState != BEACON_UNLOCKED)
    {
      NS_LOG_INFO ("Invalid request to SwitchToclassB! Check previous request made to SwitchToClassB");
      return;
    }
  
  //\todo Class B Mac exchanges before beacon searching
  //For now assume the DeviceTimeAns/BeaconTimingAns is already received from
  
  // k is the smallest integer for which k * 128 > T, where 
  // T = seconds since 00:00:00, Sunday 5th of January 1980 (start of the GPS time)
  // which is the Simulator::Now in this ns3 simulation */
  NS_LOG_DEBUG ("SwitchToClassb requested at" << Simulator::Now ());
  
  int k = 0; 
  
  while (Simulator::Now () >=  Seconds (k*128))
  {
    k++;
  }
   NS_LOG_DEBUG ("k(" << k <<")" <<"*128>T(" << Simulator::Now () << ")"); 
  
  // The time when beacon is sent
  Time bT = Seconds (k*128) + m_beaconInfo.tBeaconDelay; 
  
  // Calculate the beacon guard start time from the beacon time
  Time nextAbsoluteBeaconGuardTime = bT - m_beaconInfo.beaconGuard; 
  
  // Calculate the relative time to schedule the beacon guard
  Time nextBeaconGuard = nextAbsoluteBeaconGuardTime - Simulator::Now ();
  
  // Schedule the beaconGuard 
  Simulator::Schedule (nextBeaconGuard,
                       &EndDeviceLoraMac::StartBeaconGuard,
                       this);
  
  // Update beaconState
  m_beaconState = BEACON_SEARCH;
  
  NS_LOG_DEBUG ( "BeaconGuard scheduled at " << nextAbsoluteBeaconGuardTime);
  
  //Reset beacon and class b window to default
  m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols = EndDeviceLoraMac::ClassBReceiveWindowInfo().beaconReceiveWindowDurationInSymbols;
  m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols = EndDeviceLoraMac::ClassBReceiveWindowInfo().pingReceiveWindowDurationInSymbols;
  
  NS_LOG_DEBUG ("Beacon Receive Window Reset to " << (int)(m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols));
  NS_LOG_DEBUG ("Ping Receive Window Reset to " << (int)(m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols));
  
  m_attemptToClassB++;
}

void
EndDeviceLoraMac::SwitchFromClassB (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  if (m_deviceClass != CLASS_B)
  {
    //Nothing to do if device is not in class B
    NS_LOG_DEBUG ("The device is not in Class A");
    return;
  }
  
  m_beaconState = BEACON_UNLOCKED;
  
  m_deviceClass = CLASS_A;
  
  // Cancel all pending ping slots if any
  for (EventId& ping : m_pingSlotInfo.pendingPingSlotEvents)
  {
    Simulator::Cancel(ping);
  }
  
  // Cancel upcoming beacon guard if any
  Simulator::Cancel(m_beaconInfo.nextBeaconGuardEvent);
  
  NS_LOG_DEBUG ("Beacon unlocked, ping slot canceled, beaconGuard canceled and device switched to class A");
  //\todo Schedule an uplink with class B field set
}


void 
EndDeviceLoraMac::StartBeaconGuard (void)
{
  
  NS_LOG_FUNCTION_NOARGS ();
  
  if (m_deviceClass == CLASS_A)
    {
      if (m_macState == MAC_IDLE)
        {
          SetMacState (MAC_BEACON_GUARD);
        }
      else 
        {
          NS_LOG_INFO ("Can not start the beacon guard; device mac is not in IDLE state");
          return;
        }
      
    }
  //where is beacon guard from beacon less
  // If m_deviceClass == CLASS _B, device could be receiving in the beacon guard
  // So check the state and put intermediate stqte accordingly
  if (m_deviceClass == CLASS_B)
    {
      switch (m_macState)
        {
        case MAC_IDLE: 
          SetMacState (MAC_BEACON_GUARD);
          break;
        case MAC_RX1:
          SetMacState (MAC_RX_BEACON_GUARD);
          break;
        case MAC_RX2:
          SetMacState (MAC_RX_BEACON_GUARD);
          break;
        case MAC_PING_SLOT:
          SetMacState (MAC_PING_SLOT_BEACON_GUARD);
          break;
        default:
          NS_LOG_INFO ("Can not start the beacon guard; mac is in an improper state");
          return;
        }
    }
  
  m_beaconInfo.endBeaconGuardEvent = Simulator::Schedule (m_beaconInfo.beaconGuard,
                                                          &EndDeviceLoraMac::EndBeaconGuard,
                                                          this);
}

void
EndDeviceLoraMac::EndBeaconGuard (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  //Now Start the Beacon_reserved period as the beacon guard is done
  Simulator::Schedule (Seconds (0),
                       &EndDeviceLoraMac::StartBeaconReserved,
                       this);
}

void
EndDeviceLoraMac::StartBeaconReserved (void)
{

  NS_LOG_FUNCTION_NOARGS ();
  
  NS_ASSERT_MSG (m_macState == MAC_BEACON_GUARD, "Beacon guard should be right before the beacon reserved");
  
  //Open the beacon slot receive window to detect preamble for the beacon
  
  SetMacState (MAC_BEACON_RESERVED);
  
  NS_LOG_DEBUG ("Beacon receive window opened at " << Simulator::Now ().GetSeconds () << "Seconds");

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  // Switch to appropriate channel and data rate
  NS_LOG_INFO ("Beacon parameters: " << m_classBReceiveWindowInfo.beaconReceiveWindowFrequency << "Hz, DR"
                                    << unsigned(m_classBReceiveWindowInfo.beaconReceiveWindowDataRate));

  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency
    (m_classBReceiveWindowInfo.beaconReceiveWindowFrequency);
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor (GetSfFromDataRate
                                                               (m_classBReceiveWindowInfo.beaconReceiveWindowDataRate));
  
  //Calculate the duration of a single symbol for the beacon slot receive window
  double tSym = pow (2, GetSfFromDataRate (m_classBReceiveWindowInfo.beaconReceiveWindowDataRate)) 
                 / 
                GetBandwidthFromDataRate (m_classBReceiveWindowInfo.beaconReceiveWindowDataRate);
    
  // Schedule return to sleep after current beacon slot receive window duration
  Simulator::Schedule (Seconds (m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols*tSym),
                       &EndDeviceLoraMac::CloseBeaconReceiveWindow, 
                       this);
  
  NS_LOG_DEBUG ("The receive window opened for : " << Seconds (m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols*tSym));
  
  //Schedule release from beacon reserved so that the mac could start using the 
  //device for transmission and also schedule ping slots
  m_beaconInfo.endBeaconReservedEvent = Simulator::Schedule (m_beaconInfo.beaconReserved,
                                                             &EndDeviceLoraMac::EndBeaconReserved,
                                                             this);
  
  NS_LOG_DEBUG ("The beacon reserved finishes at: " << (Simulator::Now() + m_beaconInfo.beaconReserved));
}

void
EndDeviceLoraMac::CloseBeaconReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();
   //Beacon preamble not detected
  
  NS_ASSERT_MSG (m_macState == MAC_BEACON_RESERVED, "Beacon receive window should reside in beacon reserved");
  
  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // Check the Phy layer's state:
  // - TX -> Its an error, transmission can't happen in beacon reserved
  // - RX -> We have received a preamble.
  // - STANDBY -> Nothing was detected.
  switch (phy->GetState ())
    {
    case EndDeviceLoraPhy::TX:
      NS_LOG_ERROR ("TX can't happen in beacon reserved");
      break;
    case EndDeviceLoraPhy::SLEEP:
      break;
    case EndDeviceLoraPhy::RX:
      // PHY is receiving: let it finish
      NS_LOG_DEBUG ("PHY is receiving: Receive will handle the result.");
      return;
    case EndDeviceLoraPhy::STANDBY:
      // Turn PHY layer to sleep
      phy->SwitchToSleep ();
      // Report beacon is missed to execute procedure that matches the device state
      BeaconMissed ();
      break;
    }
}

void
EndDeviceLoraMac::EndBeaconReserved (void)
{ 
  NS_LOG_FUNCTION_NOARGS ();
  
  NS_ASSERT_MSG (m_macState == MAC_BEACON_RESERVED, "Error happened in the beacon reserved time!");
  
  // Turn back mac to idle
  SetMacState (MAC_IDLE);
  
  // If beacon is unlocked there is nothing to do
  if (m_beaconState == BEACON_UNLOCKED)
  {
    return;
  }
  
  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();
 
  // If beacon is locked or beacon less 
  if (m_beaconState == BEACON_LOCKED || m_beaconState == BEACONLESS)
  {
    // If device locked beacon or is operating in beacon less operation and if
    // radio is still in receive mode error as beacon payload can't exceed beacon
    // reserved
    if ( phy->GetState () == EndDeviceLoraPhy::RX)
    {
      NS_LOG_ERROR("Beacon payload can't exceed the beacon reserved time!");
      return;
    }
    
    // If beacon is locked we reset the beacon information
    if (m_beaconState == BEACON_LOCKED)
      {
        //Beacon information, beacon receive window, ping slot receive window
        //will be reseted immediately after the receiving the packet
      
        //Check and update for the maximum number of beacons missed in the 
        //minimal beaconless operation mode if the device was in the mode
        if (m_currentConsecutiveBeaconsMissed > m_maximumConsecutiveBeaconsMissed) 
          m_maximumConsecutiveBeaconsMissed = m_currentConsecutiveBeaconsMissed; 
        
        //device is not in minimal beaconless operation mode anymore if it was
        //therefore, reset
        m_currentConsecutiveBeaconsMissed = 0;
        
        //Fire traced-callback for informing the run length has ended
        m_currentConsecutiveBeaconsMissedTracedCallback (GetMulticastDeviceAddress (),
                                                         GetDeviceAddress (), 
                                                         m_currentConsecutiveBeaconsMissed);
      }
    
    switch (m_deviceClass)
    {
      case CLASS_A:
        // If device is still in class A switch to Class B and continue to /
        // scheduling the beacon and the ping slots
        // \todo Think if we have to leave this procedure to the Application layer
        //       for the m_beaconLockedCallback to do this.
        m_deviceClass = CLASS_B;  
      case CLASS_B:
        // If device is already in Class B then schedule the coming ping slots
        SchedulePingSlots ();
        
        // The time gap between the end of Beacon_reserved and the start of 
        // the next Beacon_guard is after Beacon_window.
        // Need to store the event to cancel incase a switch to class A is requested in the middle
        m_beaconInfo.nextBeaconGuardEvent = Simulator::Schedule (m_beaconInfo.beaconWindow,  
                                                                 &EndDeviceLoraMac::StartBeaconGuard,
                                                                 this);
          
        break;
      case CLASS_C:
        //\todo Class C not implimented yet
        NS_LOG_ERROR ("A switch to Class B is possible only from Class A!");
        break;
    }
  }
}

// This function must be called at the end of BeaconResered which is
// EndDeviceLoraMac::EndBeaconReserved as it assumes the current time to be the 
// end of the BeaconReserved time.
void
EndDeviceLoraMac::SchedulePingSlots (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  // AES part taken with modification from Joseph Finnegan 
  // <https://github.com/ConstantJoe/ns3-lorawan-class-B/blob/master/lorawan/model/lorawan-enddevice-application.cc>
  
  
  //Key = 16 times 0x00 (4x4 block)
  uint8_t key[16] = {0}; //  AES encryption with a fixed key of all zeros is used to randomize
  
  //Rand is also a 4x4 block (16 byte)
  uint8_t rand[16] = {0};
   
  //Serialize the beaconPayload
  uint32_t bcnPayload = (uint32_t) (m_beaconInfo.deviceBcnTime.GetSeconds());
  uint8_t *beaconTime = (uint8_t *)&bcnPayload;
  rand[0] = beaconTime[0];
  rand[1] = beaconTime[1];
  rand[2] = beaconTime[2];
  rand[3] = beaconTime[3];
  
  // Serialize the devAddr
  // Serialize method doesn't have any side effect so could be directly used
  uint8_t devAddr[4] = {0};
  
  //Give priority to MC
  if (IsMulticastEnabled ())
    {
      m_mcAddress.Serialize (devAddr);
    }
  else
    {
      m_address.Serialize(devAddr); 
    }
    
  rand[4] = devAddr[0];
  rand[5] = devAddr[1];
  rand[6] = devAddr[2];
  rand[7] = devAddr[3];

  //Rand = aes128_encrypt (key, beaconTime(4byte)|DevAddr(4byte)|Pad16)
  AES aes;
  aes.SetKey(key, 16);
  aes.Encrypt(rand, 16);
  
  // pingOffset = (Rand[0] + Rand[1]*256) modulo pingPeriod
  m_pingSlotInfo.pingOffset = (rand[0] + rand[1]*256)  % m_pingSlotInfo.pingPeriod;
  // m_pingSlotInfo.pingOffset = m_pingSlotInfo.pingPeriod-1; //Use this one to test for the max pingOffset
  
  // For all the slotIndex = [0 ... PingNb-1] and schedule them on 
  // (BeaconReserved + (pingOffset+ slotIndex*pingPeriod)*slotLen)
  uint8_t slotIndex = 0;
 
  for (EventId& ping : m_pingSlotInfo.pendingPingSlotEvents)
   {
    //When switch back to class A is requested cancel all the non expired events 
    
     Time slotTime = (m_pingSlotInfo.pingOffset+ slotIndex*m_pingSlotInfo.pingPeriod)*m_pingSlotInfo.slotLen; 
     ping = Simulator::Schedule( slotTime, 
                                 &EndDeviceLoraMac::OpenPingSlotReceiveWindow,
                                 this,
                                 slotIndex);
     slotIndex++;
   }
}


void
EndDeviceLoraMac::BeaconMissed (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // Do nothing if beacon has not been locked yet
  if (m_beaconState == BEACON_SEARCH)
  {
    m_beaconState = BEACON_UNLOCKED;
    
    NS_LOG_INFO ("No beacon found!");

    // Notify the Application layer that beacon is not locked
    if (!m_beaconLostCallback.IsNull ())
      {
        m_beaconLostCallback ();
      }
    
    // increment missed beacons
    m_missedBeaconCount++;
    // fire trace-callback
    m_missedBeaconTracedCallback (GetMulticastDeviceAddress (), GetDeviceAddress (), m_missedBeaconCount);
    
    return;
  }
  
  
  // increment current beacon missed
  m_currentConsecutiveBeaconsMissed++;
  // Fire trace-callback on currentConsecutiveBeaconsMissed
  m_currentConsecutiveBeaconsMissedTracedCallback (GetMulticastDeviceAddress (),
                                                   GetDeviceAddress (),
                                                   m_currentConsecutiveBeaconsMissed);
  
  // increment missed beacons
  m_missedBeaconCount++;
  // fire trace-callback
  m_missedBeaconTracedCallback (GetMulticastDeviceAddress (), GetDeviceAddress (), m_missedBeaconCount);
  
  // If we exceed minimal beacon less operation time switch to class A and reset context
  // and invoke beacon lost call back
  if (((Simulator::Now () - m_beaconInfo.gwBcnTime) > m_beaconInfo.minimalBeaconLessOperationTime)
      && (m_beaconState == BEACONLESS))
  {
    m_deviceClass = CLASS_A;
    
    m_beaconState = BEACON_UNLOCKED;
    // Notify the Application layer that beacon is not locked
    if (!m_beaconLostCallback.IsNull ())
      {
        m_beaconLostCallback ();
      }
    
    NS_LOG_INFO ("Beacon lost! switching back to class A.");
    
    // Optional: update the bcnTimes to default
    m_beaconInfo.gwBcnTime = Seconds (0);
    m_beaconInfo.deviceBcnTime = Seconds (0);
    
    
    // Reset the consecutiveBeaconMissed state and callback
    
    // reset current beacon missed
    m_currentConsecutiveBeaconsMissed = 0;
    
    // Fire trace-callback on currentConsecutiveBeaconsMissed
    m_currentConsecutiveBeaconsMissedTracedCallback (GetMulticastDeviceAddress (),
                                                     GetDeviceAddress (),
                                                     m_currentConsecutiveBeaconsMissed);  
    return;
  }
  
  // Expand the beacon window if we missed a beacon before minimal beacon less operation time
  if (m_beaconState == BEACON_LOCKED || m_beaconState == BEACONLESS)
  { 
    m_beaconState = BEACONLESS;
        
    NS_LOG_INFO ("minimal beacon less operation mode");
    
    m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols *= m_classBReceiveWindowInfo.symbolToExpantionFactor;
    
    if (m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols > m_classBReceiveWindowInfo.maxBeaconReceiveWindowDurationInSymbols)
    {
      m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols = m_classBReceiveWindowInfo.maxBeaconReceiveWindowDurationInSymbols;
    }
    
    m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols  *= m_classBReceiveWindowInfo.symbolToExpantionFactor;
    
    if (m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols > m_classBReceiveWindowInfo.maxPingReceiveWindowDurationInSymbols)
    {
      m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols = m_classBReceiveWindowInfo.maxPingReceiveWindowDurationInSymbols;
    }

    NS_LOG_DEBUG ("Beacon expanded to " <<  (int)m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols <<
                 " and ping slot expanded to " << (int)m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols);
    
    // Advance the time stamp of the beacon payload stored by 128 second as the NS will not know whether the end device has lost the beacon or not.
    // This will enable a more precise calculation of pingOffset when beacon is not received.
    m_beaconInfo.deviceBcnTime += m_beaconInfo.beaconPeriod;
  }
  
  //\TODO Also include the slot movement together with the slot expansion
}

//\TODO the bcnPayload has to be replaced from the packet received, otherwise
// there will be a difference in the time calculated by the gateway and end-device
void
EndDeviceLoraMac::BeaconReceived(Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // If beacon is unlocked, then you should first do beacon search
  // So do nothing.
  // SwitchFromClassB can simply change m_beaconState to UNLOCKED to
  // stop receiving beacon.
  if (m_beaconState == BEACON_UNLOCKED)
  {
    return;
  }
  
  //Extract the beaconTime from the bcnPayload
    // Work on a copy of the packet
  Ptr<Packet> packetCopy = packet->Copy ();
  
  BcnPayload bcnPayload; 
  packetCopy->RemoveHeader (bcnPayload);
  
  //\TODO check CRC in the bcnPayload using bcnPayload.CrcCheck, which also need to be written
  
  // remove the time field of the bcnPayload and update the time
  m_beaconInfo.gwBcnTime = Seconds (bcnPayload.GetBcnTime ());
  // update the device beacon time
  m_beaconInfo.deviceBcnTime = m_beaconInfo.gwBcnTime;
  
  NS_LOG_DEBUG ("Last Beacon Received Time Updated to " << m_beaconInfo.deviceBcnTime);
  
  //Reset beacon and class b window to default
  m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols = EndDeviceLoraMac::ClassBReceiveWindowInfo().beaconReceiveWindowDurationInSymbols;
  m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols = EndDeviceLoraMac::ClassBReceiveWindowInfo().pingReceiveWindowDurationInSymbols;
  
  NS_LOG_DEBUG ("Beacon Receive Window Reset to " << (int)m_classBReceiveWindowInfo.beaconReceiveWindowDurationInSymbols);
  NS_LOG_DEBUG ("Ping Receive Window Reset to " << (int)m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols);
  
  // If beacon is locked for the first time
  if (m_beaconState == BEACON_SEARCH)
    { 
      // At the end of the beacon reserved the end device will be switched to 
      // Class B
     
      //Invoke the callback to notify the Application layer about the locked beacon
      if (!m_beaconLockedCallback.IsNull ())
        {  
          m_beaconLockedCallback ();
        }
      NS_LOG_INFO ("beacon locked!");
   }
  
  m_beaconState = BEACON_LOCKED;
  m_totalSuccessfulBeaconPackets++;
  
  //Fire traced callback
  m_totalSuccessfulBeaconPacketsTracedCallback (GetMulticastDeviceAddress (),
                                                GetDeviceAddress (),
                                                m_totalSuccessfulBeaconPackets);
}


void
EndDeviceLoraMac::OpenPingSlotReceiveWindow(uint8_t slotIndex)
{
  //Change Mac state to ping slot
  
  //\TODO: No actual solution in spec for dealing with of tx, rx1, rx2 overlap 
  // with ping. Different choices will lead to different network performance. 
  // The Tx knows about the ping slot and so can make this decision by referring 
  // the ResolveWithClassBAndGetTime () function for this.
  
  NS_LOG_FUNCTION (this << slotIndex);  

  // Check for receiver status: if it's locked on a packet or overlap with other
  // reception window refrain from transmission.
  if (m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::RX)
    {
      if (m_macState == MAC_RX1 || m_macState == MAC_RX2)
      {
        NS_LOG_INFO ("Collision with Rx1 and Rx2 window!");
      }
      NS_LOG_INFO ("Won't open ping window since we are in RX mode.");

      return;
    }
  
  if (!(m_beaconState == BEACONLESS || m_beaconState == BEACON_LOCKED))
    {
      NS_LOG_INFO ("Beacon has to be locked or device should be in minimal beaconless operation mode to open ping slots.");
    
      return;
    }
  
  if (!(m_deviceClass == CLASS_B))
    {
      NS_LOG_INFO ("Device is not in class B. Can't open ping slot!");
    
      return;
    }
  
  if (!(m_macState == MAC_IDLE))
    {
      NS_LOG_INFO ("Mac is busy!");
    
      return;
    }
  
  if (m_relayActivated && m_packetToRelay.size () > 0)
    {
      // Relay
      NS_LOG_DEBUG ("Relaying Packet!");
      
      Ptr<Packet> packetToRelay = m_packetToRelay.front ()->Copy ();
      m_packetToRelay.pop_front ();
    
      // Craft LoraTxParameters object
      LoraTxParameters params;
      params.sf = GetSfFromDataRate (m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate);
      params.headerDisabled = m_headerDisabled;
      params.codingRate = m_codingRate;
      params.bandwidthHz = GetBandwidthFromDataRate (m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate);
      params.nPreamble = m_nPreambleSymbols;
      params.crcEnabled = 1;
      params.lowDataRateOptimizationEnabled = 0;

      // Wake up PHY layer and directly send the packet

      NS_LOG_DEBUG ("Packet to relay: " << packetToRelay << "& UID :" << packetToRelay->GetUid ());
      m_phy->Send (packetToRelay, params, m_classBReceiveWindowInfo.pingSlotReceiveWindowFrequency, m_relayPower);


      //\TODO For future you need to register packet transmission for duty cycle
      // For now only one gateway is used. Therefore, if the gateway respects the duty cycle, the end-device will also respect.=
      // There will be a problem for multiple gateway case

      // Mac is now busy transmitting in the ping slot, 
      // The Tx finished will put it back to IDLE
      // Therefore only the purpose of the ping is changed the rest stays the same
      SetMacState (MAC_PING_SLOT);

      return;
    }
  
  SetMacState (MAC_PING_SLOT);

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  // Switch to appropriate channel and data rate
  NS_LOG_INFO ("Using parameters: " << m_classBReceiveWindowInfo.pingSlotReceiveWindowFrequency << "Hz, DR"
                                    << unsigned(m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate));

  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency
    (m_classBReceiveWindowInfo.pingSlotReceiveWindowFrequency);
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor (GetSfFromDataRate
                                                               (m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate));
  
  //Calculate the duration of a single symbol for the ping slot DR
  double tSym = pow (2, GetSfFromDataRate (m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate)) / GetBandwidthFromDataRate (m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate);
    
  // Schedule return to sleep after current beacon slot receive window duration
  m_closeSecondWindow = Simulator::Schedule (Seconds (m_classBReceiveWindowInfo.pingReceiveWindowDurationInSymbols *tSym),
                                             &EndDeviceLoraMac::ClosePingSlotRecieveWindow, this);
  
  //update slot index of the opened ping slot
  m_slotIndexLastOpened = slotIndex;  
}

void
EndDeviceLoraMac::ClosePingSlotRecieveWindow()
{  
  NS_LOG_FUNCTION_NOARGS ();
  
  NS_ASSERT_MSG ((m_macState == MAC_PING_SLOT || m_macState == MAC_PING_SLOT_BEACON_GUARD), 
                 "Mac should has stayed in MAC_PING_SLOT!");
  
  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // Check the Phy layer's state:
  // - TX -> Its an error, transmission during ping slot
  // - RX -> We have received a preamble.
  // - STANDBY -> Nothing was detected.
  switch (phy->GetState ())
    {
    case EndDeviceLoraPhy::TX:
      NS_LOG_ERROR ("TX can't happen while ping is opened! resolve conflict");
      break;
    case EndDeviceLoraPhy::SLEEP:
      NS_LOG_ERROR ("Device can't sleep before the duration of the preamble we opened finishes!");
      break;
    case EndDeviceLoraPhy::RX:
      // PHY is receiving: let it finish
      NS_LOG_DEBUG ("PHY is receiving: Receive will handle the result.");
      return;
    case EndDeviceLoraPhy::STANDBY:
      // Turn PHY layer to sleep
      phy->SwitchToSleep ();
      break;
    }
  
  // Update Mac state
  if (m_macState == MAC_PING_SLOT_BEACON_GUARD)
    {
    // If the Ping Slot Periodicity is 0 and the DR is bellow and equal to 
    // 3, then the last ping window will cross the beacon-guard boarder, as the 
    // last ping will start just 30ms before the beacon guard.
      NS_LOG_DEBUG ("Ping crossed the beacon guard boundary! Switching back to Beacon Guard");
      SetMacState (MAC_BEACON_GUARD);
    }
  else if (m_macState == MAC_PING_SLOT)
    {
      //Once the packet has been recieved via ping slot free the MAC
      NS_LOG_DEBUG ("Ping finished! Switching to IDLE");
      SetMacState (MAC_IDLE);
    }
  else
    {
      NS_LOG_ERROR ("Invalid MAC State at the End of receiving ping!");
    }
}

void
EndDeviceLoraMac::PingReceived(Ptr<Packet const> packet)
{
  
  NS_LOG_FUNCTION (this << packet);
  
  NS_ASSERT_MSG ((m_macState == MAC_PING_SLOT || m_macState == MAC_PING_SLOT_BEACON_GUARD), 
                 "Mac should has stayed in MAC_PING_SLOT!");

  //Working on the copy of the packet 
  Ptr<Packet> packetCopy = packet->Copy ();
  
  // Remove the Mac Header to get some information
  LoraMacHeader mHdr;
  packetCopy->RemoveHeader (mHdr);

  NS_LOG_DEBUG ("Mac Header: " << mHdr);

  // Only keep analyzing the packet if it's downlink
  if (mHdr.IsUplink ())
    {
      NS_LOG_DEBUG ("Uplink data received via the ping slot! Packet Dropped!");
    }
  else
    {
      NS_LOG_INFO ("Found a downlink packet.");
      
      // Remove the Frame Header
      LoraFrameHeader fHdr;
      fHdr.SetAsDownlink ();
      packetCopy->RemoveHeader (fHdr);
      
     NS_LOG_DEBUG ("Frame Header: " << fHdr);
     
     // Determine whether this packet is for us
     // Is it a unicasat message
     bool unicastMessage = (m_address == fHdr.GetAddress ());
     // Is it a multicast message
     bool multicastMessage = (m_mcAddress == fHdr.GetAddress ());
     
     if (unicastMessage)
       {
          NS_LOG_INFO ("Unicast Ping Message!");
          
          //\TODO Pass the packet up to the NetDevice
          
         // For now call the callback
         if (!m_classBDownlinkCallback.IsNull ())
            {
             m_classBDownlinkCallback (EndDeviceLoraMac::UNICAST,packetCopy, m_slotIndexLastOpened);
            }
          
          // Call the trace source 
          m_receivedPingPacket (0, m_address, packetCopy, m_slotIndexLastOpened);
       }
      else if (multicastMessage) 
        {
          NS_LOG_INFO ("Multicast Ping Message!");
          
         //\TODO pass the packet up to the NetDevice
          
         // For now call the callback
         if (m_enableMulticast)
           {
             HopCountTag hopCountTag;
             packetCopy->RemovePacketTag (hopCountTag);
             
              
             NS_ASSERT_MSG (hopCountTag.GetHopCount () > 0,"Hop count should be one or greater for a packet received!");
             
             NS_LOG_DEBUG ("Packet hop count is " << (int)hopCountTag.GetHopCount ());
             
             if (maxHop > hopCountTag.GetHopCount () && m_relayActivated)
               {
                  NS_ASSERT_MSG (m_packetToRelay.size () == 0, "Device should relay " << m_packetToRelay.size () << " packet before receiving!");
                  
                  NS_LOG_DEBUG ("Preparing to relay Packet!");
                  hopCountTag.IncreamentHopCount ();
                  NS_LOG_DEBUG ("Hop incremented to " << (int) hopCountTag.GetHopCount ());
                  Ptr<Packet> packetToRelay = packetCopy->Copy ();
                  packetToRelay->AddPacketTag (hopCountTag);
                  packetToRelay->AddHeader (fHdr);
                  packetToRelay->AddHeader (mHdr);
                  m_packetToRelay.push_back (packetToRelay);
               }
             
             if (!m_classBDownlinkCallback.IsNull ())
               {
                  m_classBDownlinkCallback (EndDeviceLoraMac::MULTICAST,packetCopy, m_slotIndexLastOpened);
               }
              // Call the trace source
              m_receivedPingPacket (m_mcAddress, m_address, packetCopy, m_slotIndexLastOpened);
            }
          else
            {
              NS_LOG_INFO ("MC packet received but device not MC enabled!");
            }
       }
    }
  
  // Update Mac state
  if (m_macState == MAC_PING_SLOT_BEACON_GUARD)
    {
      //If beacon guard started before end of the packet then switch back 
      //to the beacon reserved
      NS_LOG_DEBUG ("Ping Received! Switching back to beacon guard");
      SetMacState (MAC_BEACON_GUARD);
    }
  else if (m_macState == MAC_PING_SLOT)
    {
      //Once the packet has been recieved via ping slot free the MAC
      NS_LOG_DEBUG ("Ping Received! Switching to IDLE");
      SetMacState (MAC_IDLE);
    }
  else
    {
      NS_LOG_ERROR ("Invalid MAC State at the End of receiving ping!");
    }

}

///////////////////////////////////////////////////
//  LoRaWAN Class B Related Getters and Setters //
/////////////////////////////////////////////////

///////////////////////////////////
// Device Class and Mac State   //
/////////////////////////////////

bool
EndDeviceLoraMac::SetDeviceClass (DeviceClass deviceClass)
{
  NS_LOG_FUNCTION (this << deviceClass);
  
  if (deviceClass == CLASS_A)
    {
      if (m_deviceClass == CLASS_A)
        {
          NS_LOG_DEBUG ("Device already in class A!");
          return true;
        }
      else if (m_deviceClass == CLASS_B)
        {
          NS_LOG_DEBUG ("Switch device class from B to A");
          SwitchFromClassB ();
          return true;
        }
      else 
        {
          NS_LOG_DEBUG ("Device was on invalid Class! Only Class A and Class B are implemented currently");
          return false;
        }
    }
  else if (deviceClass == CLASS_B)
    {
      if (m_deviceClass == CLASS_A)
        {
          NS_LOG_DEBUG (" Use SwitchToClassB () instead as it will need to search and lock the beacon first");
          return false;
        }
      else if (m_deviceClass == CLASS_B)
        {
          NS_LOG_DEBUG ("Device Already in Class B!");
          return true;  
        }
      else 
        {
          NS_LOG_DEBUG ("Device was on invalid Class! Only Class A and Class B are implemented currently");
          return false;
        }
    
    }
  else if (deviceClass == CLASS_C)
    {
      NS_LOG_ERROR ("Device currently don't implement Class C");
      return false;
    }
  else
    {
  
      NS_LOG_ERROR (" Unknown Device Class" << deviceClass );
      return false;    
    }

}

EndDeviceLoraMac::DeviceClass
EndDeviceLoraMac::GetDeviceClass ()
{
  return m_deviceClass;
}

void
EndDeviceLoraMac::SetMacState (MacState macState)
{
  NS_LOG_FUNCTION (this << macState);
  //Check the logic before making a switch to other state
  //Mac State machine
  switch (m_macState)
  {
    case MAC_TX: 
      if (macState == MAC_IDLE)
        {
          m_macState = macState; 
        }
      else
        {
          NS_LOG_ERROR ("Only IDLE is a possible macState after Tx!");
        }
      break;
      
    case MAC_RX1:
      if (macState == MAC_IDLE)
        {
          m_macState = macState; 
        }
      else if (macState == MAC_RX_BEACON_GUARD )
        {
          m_macState = macState;
        }
      else
        {
          NS_LOG_ERROR ("Can not switch from " << m_macState << " to " << macState);
        }
      break;
      
    case MAC_RX2:
      if (macState == MAC_IDLE)
        {
          m_macState = macState; 
        }
      else if (macState == MAC_RX_BEACON_GUARD )
        {
          m_macState = macState;
        }
      else
        {
          NS_LOG_ERROR ("Can not switch from " << m_macState << " to " << macState);
        }
      break;
    
    case MAC_RX_BEACON_GUARD:
      if (macState == MAC_BEACON_GUARD)
        {
          m_macState = macState; 
        }
      else
        {
          NS_LOG_ERROR ("Only BEACON_GUARD is a possible macState after RX_BEACON_GUARD");
        }
      break; 
      
    case MAC_BEACON_GUARD:
      if (macState == MAC_BEACON_RESERVED)
        {
          m_macState = macState; 
        }
      else
        {
          NS_LOG_ERROR ("Only BEACON_RESERVED is a possible macState after BEACON_GUARD");
        }
      break;
      
    case MAC_BEACON_RESERVED:
      if (macState == MAC_IDLE)
        {
          m_macState = macState; 
        }
      else
        {
          NS_LOG_ERROR ("Only IDLE is a possible macState after BEACON_RESERVED");
        }
      break;
      
    case MAC_PING_SLOT:
      if (macState == MAC_IDLE)
        {
          m_macState = macState; 
        }
      else if (macState == MAC_PING_SLOT_BEACON_GUARD )
        {
          m_macState = macState;
        }
      else
        {
          NS_LOG_ERROR ("Can not switch from " << m_macState << " to " << macState);
        }
      break;
                 
    case MAC_PING_SLOT_BEACON_GUARD:
      if (macState == MAC_BEACON_GUARD)
        {
          m_macState = macState; 
        }
      else
        {
          NS_LOG_ERROR ("Only BEACON_GUARD is a possible macState after PING_SLOT_BEACON_GUARD");
        }
      break; 
                  
    case MAC_IDLE:
      if (macState == MAC_BEACON_RESERVED ||
          macState == MAC_PING_SLOT_BEACON_GUARD ||
          macState == MAC_RX_BEACON_GUARD)
        {
          NS_LOG_ERROR ("Can not switch from "<< m_macState << " to " << macState << "without an intermediate state");          
        }
      else
        {
          m_macState = macState;
        }
      break;
      
    default:
      NS_LOG_ERROR ("Unknown Mac State");
      break;
  }
  
}

EndDeviceLoraMac::MacState
EndDeviceLoraMac::GetMacState ()
{
  return m_macState;
}


//////////////
// Channel //
////////////

void
EndDeviceLoraMac::SetPingSlotReceiveWindowDataRate (uint8_t pingSlotDr)
{
  m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate = pingSlotDr;
}

uint8_t
EndDeviceLoraMac::GetPingSlotReceiveWindowDataRate ()
{
  return m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate;
}

void
EndDeviceLoraMac::SetPingSlotReceiveWindowFrequency (double frequency)
{
  m_classBReceiveWindowInfo.pingSlotReceiveWindowFrequency = frequency;
}

double
EndDeviceLoraMac::GetPingSlotRecieveWindowFrequency ()
{
  return m_classBReceiveWindowInfo.pingSlotReceiveWindowFrequency;
}

void
EndDeviceLoraMac::SetBeaconReceiveWindowDataRate (uint8_t beaconDr)
{
  m_classBReceiveWindowInfo.beaconReceiveWindowDataRate = beaconDr;
}

uint8_t
EndDeviceLoraMac::GetBeaconRecieveWindowDataRate ()
{
  return m_classBReceiveWindowInfo.pingSlotReceiveWindowDataRate;
}

void
EndDeviceLoraMac::SetBeaconReceiveWindowFrequency (double frequency)
{
  m_classBReceiveWindowInfo.beaconReceiveWindowFrequency = frequency;
}

double
EndDeviceLoraMac::GetBeaconRecieveWindowFrequency ()
{
  return m_classBReceiveWindowInfo.beaconReceiveWindowFrequency;
}

/////////////////////////
// Class B parameters //
///////////////////////

void
EndDeviceLoraMac::SetPingSlotPeriodicity (uint8_t periodicity)
{
  if (periodicity < 8 || periodicity >= 0)
    {
      m_pingSlotInfo.pingSlotPeriodicity = periodicity;
      m_pingSlotInfo.pingNb = std::pow (2, (7-periodicity));
      m_pingSlotInfo.pingPeriod = 4096/(m_pingSlotInfo.pingNb);
    }
  else
    {
      NS_LOG_ERROR ("Invalid Ping Slot periodicity");
    }

}

uint8_t
EndDeviceLoraMac::GetPingSlotPeriodicity ()
{
  return m_pingSlotInfo.pingSlotPeriodicity;
}

void
EndDeviceLoraMac::SetPingNb (uint8_t pingNb)
{
  if (pingNb <= 128 || pingNb >= 1)
    {
      m_pingSlotInfo.pingNb = pingNb;
      m_pingSlotInfo.pingSlotPeriodicity = 7 - std::log2 (pingNb);
      m_pingSlotInfo.pingPeriod = 4096/pingNb;
    }
  else
    {
      NS_LOG_ERROR ("Invalid PingNb");
    }   
}

uint8_t
EndDeviceLoraMac::GetPingNb ()
{
  return m_pingSlotInfo.pingNb;
}

void
EndDeviceLoraMac::SetPingPeriod (uint pingPeriod)
{
  if (pingPeriod <= 4096 || pingPeriod >= 32)
    {
      m_pingSlotInfo.pingPeriod = pingPeriod;
      m_pingSlotInfo.pingNb = 4096/pingPeriod;
      m_pingSlotInfo.pingSlotPeriodicity = 7 - std::log2 (m_pingSlotInfo.pingNb);
    }
  else
    {
      NS_LOG_ERROR ("Invalid pingPeriod");
    }   
}

uint
EndDeviceLoraMac::GetPingPeriod ()
{
  return m_pingSlotInfo.pingPeriod;
}


////////////////
//Callbacks  //
//////////////

void
EndDeviceLoraMac::SetBeaconLockedCallback (Callback<void> beaconLockedCallback)
{
  m_beaconLockedCallback = beaconLockedCallback;
}

void
EndDeviceLoraMac::SetBeaconLostCallback (Callback<void> beaconLostCallback)
{
  m_beaconLostCallback = beaconLostCallback;
}

void
EndDeviceLoraMac::SetClassBDownlinkCallback (ClassBDownlinkCallback classBDownlinkCallback)
{
  m_classBDownlinkCallback = classBDownlinkCallback;
}

///////////////////
// Multicasting //
/////////////////

void
EndDeviceLoraMac::EnableMulticast (void)
{
  if (m_mcAddress.Get () == 1)
    {
      NS_LOG_ERROR ("Set the multicast Address before enabling multicast!");
    }
  else 
    {
      m_enableMulticast = true;
    }
}

void
EndDeviceLoraMac::DisableMulticast ()
{
  m_enableMulticast = false;
}


bool
EndDeviceLoraMac::IsMulticastEnabled (void)
{
  return m_enableMulticast;
}



//////////////////////////////////////////////
// Related to coordinated relaying         //
////////////////////////////////////////////




void
EndDeviceLoraMac::EnableCoordinatedRelaying (uint32_t numberOfEndDeviceInMcGroup)
{
  NS_LOG_FUNCTION (this << numberOfEndDeviceInMcGroup);
  
  NS_ASSERT_MSG (numberOfEndDeviceInMcGroup > 1 ,"You can not activate coordinated relaying with only one node!");
  
  //\TODO Check power level and pending transmissions before enabling this
  
  // Divide the maximum power with the number of nodes in the multicast group
  m_relayPower = (m_maxBandTxPower+m_marginTxPower)/numberOfEndDeviceInMcGroup;
  
  // \TODO May be further decrease the m_relayPower according to device energy consumption
  
  m_relayActivated = true; 
}

}
}