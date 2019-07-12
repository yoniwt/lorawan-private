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

#ifndef PERIODIC_SENDER_H
#define PERIODIC_SENDER_H

#include "ns3/application.h"
#include "ns3/nstime.h"
#include "ns3/lora-mac.h"
#include "ns3/attribute.h"
#include "ns3/end-device-lora-mac.h"

namespace ns3 {
namespace lorawan {

class EndDeviceClassBApp : public Application
{
public:
  EndDeviceClassBApp ();
  ~EndDeviceClassBApp ();

  static TypeId GetTypeId (void);
  
  /**
   * Callback invoked by the EndDeviceLoraMac when beacon is locked
   */
  void BeaconLockedCallback (void);
  
  /**
   * Callback invoked by the EndDeviceLoraMac when beacon is lost or when the 
   * quest to search beacon fails.
   */
  void BeaconLostCallback (void);
  
  /**
   * Callback invoked when packet is received via ping slot
   */
  void ClassBDownlinkCallback (EndDeviceLoraMac::ServiceType serviceType, Ptr<const Packet> packet, uint8_t pingIndex);
  
  /**
   * Set the Initial delay before attempting to switch to class B and intermediate delay
   * after losing a beacon before the next attempt
   * \param delay The initial delay and intermediate delay before switching to 
   * class B  
   */
  void SetSwitchToClassBDelay (Time delay);
  
  /**
   * Set the number of attempt to switch to class B if we are an successful 
   * to lock the beacon
   * 
   * \param nAttempt number of attempts to switch to class B. If zero it means
   * there is no limit to the number of attemps
   */
  void SetNumberOfAttempt (uint8_t nAttempt);
  
  /**
   * Initiates a switch to Class B
   */
  void SwitchToClassB (void);
  
  
  //More function for writing to file and locking
  
  //////////////////////////////////
  // For Sending periodic uplink //
  ////////////////////////////////
  
  /**
   * Enabling Sending of Uplinks after beacon is locked
   */
  void EnablePeriodicUplinks (void);
  
  /**
   * Disabling Sending of Uplinks after beacon is locked
   */
  void DisablePeriodicUplinks (void);
  
  /**
   * Set the sending interval
   * \param interval the interval between two packet sendings
   */
  void SetSendingInterval (Time interval);

  /**
   * Get the sending inteval
   * \returns the interval between two packet sends
   */
  Time GetSendingInterval (void) const;

  /**
   * Set the initial delay of this application
   */
  void SetSendingInitialDelay (Time delay);
  
  /**
   * Set packet size
   */
  void SetPacketSize (uint8_t size);
  
  /**
   * Enable to keep track of the received packets content
   * 
   * When this is enabled the application keeps track of the enabled
   * packets and then fire a tracesource for the missed and restored 
   * packets
   * 
   * \param first the index of the first fragment we expect to receive
   * \param last the index of the last fragment we expect to receive, the default
   * is zero which will continue to receive as far as the simulator is running
   */
  void EnableFragmentedDataReception (uint32_t first, uint32_t last=0);

  /**
   * Set if using randomness in the packet size
   */
  void SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv);
  
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
   * Send a packet using the LoraNetDevice's Send method
   */
  void SendPacket (void);

  /**
   * Start the application by scheduling the first SendPacket event
   */
  void StartApplication (void);

  /**
   * Stop the application
   */
  void StopApplication (void);
  
  /**
   * Tracedcallback when fragments are missed
   * 
   * \param mcAddress multicast address of this device
   * \param ucAddress unicast address of this device
   * \param currentNumberOfFragementsMissed the number of fragments that are identified to be missed
   * \param totalNumberOfFragmentsMissed the total number of fragments missed up until now
   */
  void (*FragmentsMissed) (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t currentNumberOfFragmentsMissed, uint32_t totalNumberOfFragmentsMissed);

private:
  /**
   * The interval between to consecutive send events
   */
  Time m_sendingInterval;

  /**
   * The initial delay of this application
   */
  Time m_initialSendingDelay;

  /**
   * The sending event scheduled as next
   */
  EventId m_sendEvent;

  /**
   * The MAC layer of this node
   */
  Ptr<LoraMac> m_mac;
  
  /**
   * The EndDeviceLoraMac Layer of this node
   */
  Ptr<EndDeviceLoraMac> m_endDeviceLoraMac;

  /**
   * The packet size.
   */
  uint8_t m_basePktSize;


  /**
   * The random variable that adds bytes to the packet size
   */
  Ptr<RandomVariableStream> m_pktSizeRV;
  
  /**
   * Initial delay to switch to class B
   */
  Time m_classBDelay;
  
  /**
   * Number of attempt to switch to class B
   */
  uint8_t m_nAttempt; 
  
  /**
   * Count of the number of attempt to switch to Class B until now before
   * locking a beacon    
   */
  uint64_t m_countAttempt;
  
  /**
   * Whether to send an uplink transmission after locking beacon or not
   */
  bool m_uplinkEnabled;
  
  /**
   * Switch to Class B Event that is called when the maximum time out to wait 
   * for feedback from the EndDeviceLoraMac is reached
   */
  EventId m_switchToClassBTimeOutEvent;
  
  /**
   * Structure for decoding multicastPackets and checking lost fragments
   */
  
  struct FragmentedPacketDecoder// : public SimpleRefCount<FragmentedPacketDecoder>
  {
    FragmentedPacketDecoder ()
    : startingFragment (0),
      finalFragment (0),
      expectedFragment (0),
      maxSize (255),
      lastNumberOfFragmentMissed (0)
    { 
    }
    
    FragmentedPacketDecoder (uint32_t min, uint8_t max, uint32_t size)
    : startingFragment (min),
      finalFragment (max),
      expectedFragment (min),
      maxSize (size),
      lastNumberOfFragmentMissed (0)
    {
    }
    
    ~FragmentedPacketDecoder ()
    {
      startingFragment = 0;
      finalFragment = 0;
      expectedFragment = 0;
      maxSize = 0;
      lastNumberOfFragmentMissed = 0;
    }
    
    /**
     * Decode packet and update the state of the fragment
     * 
     * \param [in] packet the packet received
     * \param [out] the fragment number of the received packet
     * \return the number of missed fragment until now
     */
    uint FragmentReceived (Ptr<Packet const> packet, uint32_t& fragmentReceived)
    {
      //maxSize could also be the size of the final fragement
      uint8_t *buff = new uint8_t[maxSize] ();
      
      Ptr<Packet> copyPacket = packet->Copy ();
      //Work on copy of the packet
      copyPacket->CopyData (buff, maxSize);
      //If packet is sequenced generate a packet with a data corresponding to 
      //to the current sequence of a packet
      uint32_t sequence = 0;
      //divide sequence to byte stream backward side
       //Should be reconstructed in the application side by redoing the process
       for (uint8_t i = 0; i < maxSize; i++)
        {
          uint8_t seq = buff[i];
          sequence += (uint)seq*(std::pow (10, i));
          //\TODO Terminate the loop if it is too much zero or reach the final fragment equivalent number
        }
       
      fragmentReceived = sequence;
       //std::cerr << "ExpectedFragment = " << expectedFragment << std::endl;
       //std::cerr << "FragmentReceived = " << sequence << std::endl;
      
       if (sequence - expectedFragment < 0)
        { 
          //\TODO We might need to raise error here 
          expectedFragment = sequence;
        } 
      
      if (sequence > expectedFragment )
      {
        //There is a missed packet therefore find how many
        lastNumberOfFragmentMissed = 0;
      }
      //Store all the missed fragements and adjust fragments
      for ( ; (int32_t)sequence - (int32_t)expectedFragment > 0; expectedFragment++ )
      {
        missedFragments.push_back (expectedFragment);
        lastNumberOfFragmentMissed++;
      }
      //adjust next expected fragment
      expectedFragment++;
      
      if (expectedFragment > (std::numeric_limits<uint32_t>::max () - 20))
      {
        std::cout << "Limit is approaching  " << expectedFragment << std::endl; 
      }
      
      delete [] buff;
      
      return missedFragments.size();
    }
    
    //Missed packets
    std::list<uint32_t> missedFragments;
    //First fragment 
    uint32_t startingFragment; 
    //Final fragment
    uint32_t finalFragment;
    //To keep track of what the next packet fragment is
    uint32_t expectedFragment;
    //Size of the packet
    uint8_t maxSize;
    
    uint32_t lastNumberOfFragmentMissed;
  };
  
  FragmentedPacketDecoder m_fragmentedPacketDecoder;
  
  /**
   * A vector holding the maximum app payload size that corresponds to a
   * certain DataRate.
   */
  std::vector<uint32_t> m_maxAppPayloadForDataRate;
  

  /**
   * Tracedcallback when fragments are missed
   * 
   * \param mcAddress multicast address of this device
   * \param ucAddress unicast address of this device
   * \param currentNumberOfFragementsMissed the number of fragments that are identified to be missed
   * \param totalNumberOfFragmentsMissed the total number of fragments missed up until now
   */
  TracedCallback <LoraDeviceAddress, LoraDeviceAddress, uint32_t, uint32_t> m_fragmentsMissed;
  
  //Enabling and Disabling the fragmentedPacketDecoder and its start and end time
  struct EnableFragmentedPacketDecoder
  {
    bool enable = false; ///< enable or disable packet fragment decoder
    uint32_t first = 0; ///< the sequence number of the first packet
    uint32_t last = 0; ///< the sequence number of the last packet
  };
  
  EnableFragmentedPacketDecoder m_enableFragmentedPacketDecoder;


};

} //namespace ns3

}
#endif /* SENDER_APPLICATION */
