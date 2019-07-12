/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
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
 */

#ifndef NETWORK_SCHEDULER_H
#define NETWORK_SCHEDULER_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/core-module.h"
#include "ns3/lora-device-address.h"
#include "ns3/lora-mac-header.h"
#include "ns3/lora-frame-header.h"
#include "ns3/network-controller.h"
#include "ns3/network-status.h"

namespace ns3 {
namespace lorawan {

class NetworkStatus;     // Forward declaration
class NetworkController;     // Forward declaration

class NetworkScheduler : public Object
{
public:
  static TypeId GetTypeId (void);

  NetworkScheduler ();
  NetworkScheduler (Ptr<NetworkStatus> status,
                    Ptr<NetworkController> controller);
  virtual ~NetworkScheduler ();

  /**
   * Method called by NetworkServer to inform the Scheduler of a newly arrived
   * uplink packet. This function schedules the OnReceiveWindowOpportunity
   * events 1 and 2 seconds later.
   */
  void OnReceivedPacket (Ptr<const Packet> packet);

  /**
   * Method that is scheduled after packet arrivals in order to act on
   * receive windows 1 and 2 seconds later receptions.
   */
  void OnReceiveWindowOpportunity (LoraDeviceAddress deviceAddress, int window);
  
  /**
   * Sends beacon through gateways that are class B enabled
   * 
   * It first checks pending downlinks and it either sends immediately or if 
   * all gateways are busy or no duty cycle is available it discards it
   *
   * \brief enable either to enable or disable beacon broadcast
   */
  void BroadcastBeacon (bool enable); 
  
  /**
   * It sends downlinks to unicast and multicast devices via ping slot
   * 
   * \param bcnTime beacon time used for beacon that correspond to the current
   * beacon period.
   */
  void ScheduleClassBDownlink (uint32_t bcnTime);

  /**
   * Set the maximum App layer payload for a set DataRate.
   *
   * This overrides the default [59,59,59,123,230,230,230,230] which corresponds
   * to [DR0, DR1, DR2, DR3, DR4, DR5, DR6, DR7] 
   * 
   * \param maxAppPayloadForDataRate A vector that contains at position i the
   * maximum Application layer payload that should correspond to DR i in this
   * MAC's region.
   */  
  void SetMaxAppPayloadForDataRate (std::vector<uint32_t> maxAppPayloadForDataRate);
  
  /**
   * Generate for all devAddr that are class B a downlink is sequenced, that is 
   * fragmented data
   * 
   * \param enable if true the SequencedPacketGeneration is enabled.
   */
  void EnableSequencedPacketGeneration (bool enable);
  
  
  /****************************
   * TracedCallback Signatures
   ****************************/
  
  /**
   * The trace source fired when class B multicast downlink is sent at list on 
   * one of the gateways. Fired after sending the packet.
   * 
   * \param mcAddress the multicast address to which we are sending the downlink
   * \param numberOfGateways the number of gateways that sent the packet successfully
   * \param pingNb the number of pings per beacon period which is 2**(7-PingSlotPeriodicity)
   * \param slotIndex the slot index on which the downlink is sent. SlotIndex = [0 - pingNb)
   * \param time the time at which the packet is sent
   * \param packet the packet transmitted
   * \param isSequentialPacket  if the packet is sequential or not
   * \param sequenceNumber the sequence number of the packet if the packet is sequential
   * 
   */
   typedef void (* McPingSentCallback) 
                (LoraDeviceAddress mcAddress, uint8_t numberOfGateways, uint8_t pingSlotPeriodicity, 
                 uint8_t slotIndex, Time time, Ptr<Packet const> packet, bool isSequentialPacket, uint32_t sequenceNumber);
   
  /**
   * The trace source fired when class B multicast downlink is sent at list on 
   * one of the gateways. Fired after sending the packet.
   * 
   * \param ucAddress the unicast address to which we are sending the downlink
   * \param pingNb the number of pings per beacon period which is 2**(7-PingSlotPeriodicity)
   * \param slotIndex the slot index on which the downlink is sent. SlotIndex = [0 - pingNb)
   * \param time the time at which the packet is sent
   * \param packet the packet transmitted
   * \param isSequentialPacket  if the packet is sequential or not
   * \param sequenceNumber the sequence number of the packet if the packet is sequential
   * 
   */
   typedef void (* UcPingSentCallback) 
                (LoraDeviceAddress ucAddress, uint8_t pingSlotPeriodicity, uint8_t slotIndex, 
                 Time time, Ptr<Packet const> packet, bool isSequentialPacket, uint32_t sequenceNumber);
   
  /**
   * The trace source fired when beacon status changes from continuously transmitting beacon 
   * to continuously skipping or vise versa
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
   */  
   typedef void (* BeaconStatusCallback) (bool isSent, uint32_t continuousCount);

private:
    /**
   * Get a ping-offset for a device-address and ping-period for a given beacon time
   * 
   * The Network Server can use this function to calculate the ping-offset for a multicast
   * and unicast devices to calculate the offset from the beacon reserved to open
   * the ping slots.
   * 
   * \param bcnTime the beacon time of the immediately preceeding beacon.  
   * \param address the devAddress for which you want to calculate the offset
   * \param pingPeriod the ping-period between two consecutive ping slots in number of slots.
   * 
   * \return the offset in number of slots from the beacon. while the slotLen is 30ms.  
   */
  uint64_t GetPingOffset (uint32_t bcnTime, LoraDeviceAddress address, uint pingPeriod);
  
  /**
   * Sends multicast and unicast packets to the gateways via the gateways status
   * 
   * The packet can be generated from 'm_downlinkPacket' that corresponds the 
   * the particular address
   * 
   * \param address unicast address for if isMulticast is false otherwise multicast
   * \param isUnicast whether the transmission is unicast or multicast
   * \param pingPeriod the period between two pings in number of slots
   * \param pingNb the number of pings per beacon period 4096/pingPeriod
   * \param slotIndex the index of the ping slot for which to send downlink for a particular devAddress. 
   * It goes from [0...N] where N = pingNb-1 and pingNb is the number of ping per beacon period.
   */
  void SendPingDownlink (LoraDeviceAddress address, bool isMulticast, uint pingPeriod, uint8_t pingNb, uint8_t slotIndex);
  
  enum DownlinkType
  {
    SEQUENCED, ///< For unicast and multicast fragemented data block 
    EMPTY      ///< For creaing an empty packet with what ever size you want 
  };
  
  /// Structure for Generating Downlink Packet
  struct DownlinkPacketGenerator : public SimpleRefCount<DownlinkPacketGenerator> //For use of smart pointer
  {
    /**
     * Initializer
     */
    DownlinkPacketGenerator () 
      : m_downlinkType (EMPTY),
        m_packetSize (51), 
        m_sequence (0)
    {
    }
    
    /**
     * Constructor for DownlinkPacket
     * 
     * This structure will generate the kind of packet required for downlink
     * 
     * \param downlinkType either DownlinkType::Empty or DownlinkType::Sequenced
     * for sequenced and 
     * \param packetSize the size of the packet to be generated =
     * \param sequence the sequence number to start with, default 0
     */
    DownlinkPacketGenerator(enum DownlinkType downlinkType, uint8_t packetSize, uint sequence = 0)
      : m_downlinkType (downlinkType),
        m_packetSize (packetSize), //\TODO  Add a random size packet generation as well
        m_sequence (sequence)
    {
    }
    
    ~DownlinkPacketGenerator()
    {
      m_downlinkType = EMPTY;
      m_sequence = 0;
    }
    
    /**
     * Generate packet to send for downlink
     * 
     * \return the packet to sent  
     */
    Ptr<Packet> GetPacket ()
    {
      if (m_downlinkType == SEQUENCED)
      {
        //If packet is sequenced generate a packet with a data corresponding to 
        //to the current sequence of a packet
        uint8_t *buff = new uint8_t[m_packetSize];
        uint32_t sequence = m_sequence;
        //divide sequence to byte stream backward side
        //Should be reconstructed in the application side by redoing the process
        for (int i = 0; i < m_packetSize; i++)
        {
          buff[i] = sequence%10;
          sequence = sequence/10;
        }
        Ptr<Packet> packet =  Create<Packet> (buff, m_packetSize);
        //remove buff as it is already copied to the packet
        delete [] buff;
        
        return packet;
      }
      else
      {
        Ptr<Packet> packet = Create<Packet> (m_packetSize);
        return packet;
      }
    }
    
    /**
     * This has to be called after the packet is generated after getting 
     * the feedback whether it is sent or not
     * 
     * This function is useful for sequenced data generation.
     * 
     * \param isSent true if the generated packet is sent otherwise false
     */
    void PacketSent (bool isSent)
    {
      if (isSent)
      {
        //std::cerr << std::endl << std::endl << "Generating Sequence : " << (int)m_sequence << std::endl;
        m_sequence += 1;
      }
      //If packet is not sent keep the state
    }  
    
    enum DownlinkType m_downlinkType; ///< type of packet created for downlinks
    uint8_t m_packetSize; ///< Size of a packet for which we are going to generate sequenced downlink
    uint32_t m_sequence; ///< For SEQUENCED, the current packets sequence number to generate
  };
  
  /**
   * Stores the corresponding downlink packet generator corresponding to a devAddr (both unicast and multicast) 
   */
  std::map<LoraDeviceAddress, Ptr<DownlinkPacketGenerator> > m_downlinkPacket;
  
  TracedCallback<Ptr<const Packet> > m_receiveWindowOpened;
  Ptr<NetworkStatus> m_status;
  Ptr<NetworkController> m_controller;
  
  bool m_beaconBroadcastEnabled;
  TracedValue<uint32_t> m_totalBeaconsBroadcasted;
  TracedValue<uint32_t> m_totalBeaconsBlocked;
  uint32_t m_lastBeaconTime; ///< time stamp included in the bcnPacket, if bcn is not sent it is advanced by 128 second;
  
  /**
   * A vector holding the maximum app payload size that corresponds to a
   * certain DataRate.
   */
  std::vector<uint32_t> m_maxAppPayloadForDataRate;
    
  /**
   * This is true when the Sequenced Packet Generation is enabled
   */
  bool m_enableSequencedPacketGeneration;
  
  /**
   * Other TraceSources
   */
  
  /**
   * The trace source fired when class B multicast downlink is sent at list on 
   * one of the gateways. Fired after sending the packet.
   * 
   * \see ns3::NetworkScheduler::McPingSentCallback
   */
  TracedCallback<LoraDeviceAddress, uint8_t, uint8_t, uint8_t, Time, Ptr<Packet const>, bool, uint32_t> m_mcPingSent; 
  
   
  /**
   * The trace source fired when class B multicast downlink is sent at list on 
   * one of the gateways. Fired after sending the packet.
   * 
   * \see ns3::NetworkScheduler::UcPingSentCallback
   */
  TracedCallback<LoraDeviceAddress, uint8_t, uint8_t, Time, Ptr<Packet const>, bool , uint32_t> m_ucPingSent;
  
  /**
   * The trace source fired whenever a packet is sent successfully to indicate
   * the total number of byte sent from the moment simulation started.
   */
  TracedValue<uint32_t> m_totalByteSent;
  
  /**
   * The trace source fired when beacon status changes from continuously transmitting beacon 
   * to continuously skipping or vise versa
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
   */
  TracedCallback<bool, uint32_t> m_beaconStatusCallback;
  
  struct BeaconStatus
  {
    uint32_t continuousCount = 0; ///< For beacons that are sent if isSent = true, otherwise its for missed beacons 
    bool isSent = false; ///< true if the state is on beacon sent otherwise false;
  };
  struct BeaconStatus m_beaconStatus;
  
  struct BeaconRelatedConstants
  {
    Time minimalBeaconLessOperationMode = Minutes(120) ; 
    Time beaconWindow = Seconds(128);
  };
  
  struct BeaconRelatedConstants m_beaconRelatedConstants; 
  

  
};

} /* namespace ns3 */

}
#endif /* NETWORK_SCHEDULER_H */
