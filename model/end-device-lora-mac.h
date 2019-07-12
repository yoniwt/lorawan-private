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

#ifndef END_DEVICE_LORA_MAC_H
#define END_DEVICE_LORA_MAC_H

#include "ns3/lora-mac.h"
#include "ns3/lora-mac-header.h"
#include "ns3/lora-frame-header.h"
#include "ns3/random-variable-stream.h"
#include "ns3/lora-device-address.h"
#include "ns3/traced-value.h"
#include "ns3/aes.h"

namespace ns3 {
namespace lorawan {

/**
 * Class representing the MAC layer of a LoRaWAN device.
 */
class EndDeviceLoraMac : public LoraMac
{
public:
  static TypeId GetTypeId (void);

  EndDeviceLoraMac ();
  virtual ~EndDeviceLoraMac ();

  /////////////////////
  // Sending methods //
  /////////////////////

  /**
   * Send a packet.
   *
   * The MAC layer of the ED will take care of using the right parameters.
   *
   * \param packet the packet to send
   */
  virtual void Send (Ptr<Packet> packet);

  /**
   * Checking if we are performing the transmission of a new packet or a retransmission, and call SendToPhy function.
   *
   * \param packet the packet to send
   */
  virtual void DoSend (Ptr<Packet> packet);

  /**
  * Add headers and send a packet with the sending function of the physical layer.
  *
  * \param packet the packet to send
  */
  virtual void SendToPhy (Ptr<Packet> packet);

  /**
   * Postpone transmission to the specified time and delete previously scheduled transmissions if present.
   *
   * \param nextTxDelay Delay at which the transmission will be performed.
   */
  virtual void postponeTransmission (Time nextTxDelay, Ptr<Packet>);


  ///////////////////////
  // Receiving methods //
  ///////////////////////

  /**
   * Receive a packet.
   *
   * This method is typically registered as a callback in the underlying PHY
   * layer so that it's called when a packet is going up the stack.
   *
   * \param packet the received packet.
   */
  virtual void Receive (Ptr<Packet const> packet);

  virtual void FailedReception (Ptr<Packet const> packet);

  /**
   * Perform the actions that are required after a packet send.
   *
   * This function handles opening of the first receive window.
   */
  void TxFinished (Ptr<const Packet> packet);

  /**
   * Perform operations needed to open the first receive window.
   */
  void OpenFirstReceiveWindow (void);

  /**
   * Perform operations needed to open the second receive window.
   */
  void OpenSecondReceiveWindow (void);

  /**
   * Perform operations needed to close the first receive window.
   */
  void CloseFirstReceiveWindow (void);

  /**
   * Perform operations needed to close the second receive window.
   */
  void CloseSecondReceiveWindow (void);

  /////////////////////////
  // Getters and Setters //
  /////////////////////////

  /**
  * Reset retransmission parameters contained in the structure LoraRetxParams
  */
  virtual void resetRetransmissionParameters ();

  /**
   * Enable data rate adaptation in the retransmitting procedure.
   *
   * \param adapt If the data rate adaptation is enabled or not.
   */
  void SetDataRateAdaptation (bool adapt);

  /**
   * Get if data rate adaptation is enabled or not.
   */
  bool GetDataRateAdaptation (void);

  /**
   * Set the maximum number of transmissions allowed.
   *
   * \param maxNumbTx The maximum number of transmissions allowed
   */
  void SetMaxNumberOfTransmissions (uint8_t maxNumbTx);

  /**
   * Set the maximum number of transmissions allowed.
   */
  uint8_t GetMaxNumberOfTransmissions (void);

  /**
   * Set the data rate this end device will use when transmitting. For End
   * Devices, this value is assumed to be fixed, and can be modified via MAC
   * commands issued by the GW.
   *
   * \param dataRate The dataRate to use when transmitting.
   */
  void SetDataRate (uint8_t dataRate);

  /**
   * Get the data rate this end device is set to use.
   *
   * \return The data rate this device uses when transmitting.
   */
  uint8_t GetDataRate (void);

  /**
   * Set the network address of this device.
   *
   * \param address The address to set.
   */
  void SetDeviceAddress (LoraDeviceAddress address);

  /**
   * Get the network address of this device.
   *
   * \return This device's address.
   */
  LoraDeviceAddress GetDeviceAddress (void);

  /**
   * Set the multicast network address of this device.
   * 
   * This has to be set and different from `1` before calling ``EnableMulticast``.
   *
   * \param address The multicast address to set.
   */
  void SetMulticastDeviceAddress (LoraDeviceAddress address);

  /**
   * Get the multicast network address of this device.
   *
   * \return This device's multicast address.
   */
  LoraDeviceAddress GetMulticastDeviceAddress (void);
  
  /**
   * Set the Data Rate to be used in the second receive window.
   *
   * \param dataRate The Data Rate.
   */
  void SetSecondReceiveWindowDataRate (uint8_t dataRate);

  /**
   * Get the Data Rate that will be used in the first receive window.
   *
   * \return The Data Rate
   */
  uint8_t GetFirstReceiveWindowDataRate (void);

  /**
   * Get the Data Rate that will be used in the second receive window.
   *
   * \return The Data Rate
   */
  uint8_t GetSecondReceiveWindowDataRate (void);

  /**
   * Set the frequency that will be used for the second receive window.
   *
   * \param frequencyMHz the Frequency.
   */
  void SetSecondReceiveWindowFrequency (double frequencyMHz);

  /**
   * Get the frequency that is used for the second receive window.
   *
   * @return The frequency, in MHz
   */
  double GetSecondReceiveWindowFrequency (void);

  /**
   * Set a value for the RX1DROffset parameter.
   *
   * This value decides the offset to use when deciding the DataRate of the
   * downlink transmission during the first receive window from the
   * replyDataRateMatrix.
   *
   * \param rx1DrOffset The value to set for the offset.
   */
  // void SetRx1DrOffset (uint8_t rx1DrOffset);

  /**
   * Get the value of the RX1DROffset parameter.
   *
   * \return The value of the RX1DROffset parameter.
   */
  // uint8_t GetRx1DrOffset (void);

  /**
   * Get the aggregated duty cycle.
   *
   * \return A time instance containing the aggregated duty cycle in fractional
   * form.
   */
  double GetAggregatedDutyCycle (void);

  /////////////////////////
  // MAC command methods //
  /////////////////////////

  /**
   * Add the necessary options and MAC commands to the LoraFrameHeader.
   *
   * \param frameHeader The frame header on which to apply the options.
   */
  void ApplyNecessaryOptions (LoraFrameHeader &frameHeader);

  /**
   * Add the necessary options and MAC commands to the LoraMacHeader.
   *
   * \param macHeader The mac header on which to apply the options.
   */
  void ApplyNecessaryOptions (LoraMacHeader &macHeader);

  /**
   * Set the message type to send when the Send method is called.
   */
  void SetMType (LoraMacHeader::MType mType);

  /**
 * Get the message type to send when the Send method is called.
 */
  LoraMacHeader::MType GetMType (void);

  /**
   * Parse and take action on the commands contained on this FrameHeader.
   */
  void ParseCommands (LoraFrameHeader frameHeader);

  /**
   * Perform the actions that need to be taken when receiving a LinkCheckAns command.
   *
   * \param margin The margin value of the command.
   * \param gwCnt The gateway count value of the command.
   */
  void OnLinkCheckAns (uint8_t margin, uint8_t gwCnt);

  /**
   * Perform the actions that need to be taken when receiving a LinkAdrReq command.
   *
   * \param dataRate The data rate value of the command.
   * \param txPower The transmission power value of the command.
   * \param enabledChannels A list of the enabled channels.
   * \param repetitions The number of repetitions prescribed by the command.
   */
  void OnLinkAdrReq (uint8_t dataRate, uint8_t txPower,
                     std::list<int> enabledChannels, int repetitions);

  /**
   * Perform the actions that need to be taken when receiving a DutyCycleReq command.
   *
   * \param dutyCycle The aggregate duty cycle prescribed by the command, in
   * fraction form.
   */
  void OnDutyCycleReq (double dutyCycle);

  /**
   * Perform the actions that need to be taken when receiving a RxParamSetupReq command.
   *
   * \param rx1DrOffset The offset to set.
   * \param rx2DataRate The data rate to use for the second receive window.
   * \param frequency The frequency to use for the second receive window.
   */
  void OnRxParamSetupReq (uint8_t rx1DrOffset, uint8_t rx2DataRate, double frequency);

  /**
   * Perform the actions that need to be taken when receiving a DevStatusReq command.
   */
  void OnDevStatusReq (void);

  /**
   * Perform the actions that need to be taken when receiving a NewChannelReq command.
   */
  void OnNewChannelReq (uint8_t chIndex, double frequency, uint8_t minDataRate,
                        uint8_t maxDataRate);

  ////////////////////////////////////
  // Logical channel administration //
  ////////////////////////////////////

  /**
   * Add a logical channel to the helper.
   *
   * \param frequency The channel's center frequency.
   */
  void AddLogicalChannel (double frequency);

  /**
   * Set a new logical channel in the helper.
   *
   * \param chIndex The channel's new index.
   * \param frequency The channel's center frequency.
   * \param minDataRate The minimum data rate allowed on the channel.
   * \param maxDataRate The maximum data rate allowed on the channel.
   */
  void SetLogicalChannel (uint8_t chIndex, double frequency,
                          uint8_t minDataRate, uint8_t maxDataRate);

  /**
   * Add a logical channel to the helper.
   *
   * \param frequency The channel's center frequency.
   */
  void AddLogicalChannel (Ptr<LogicalLoraChannel> logicalChannel);

  /**
   * Add a subband to the logical channel helper.
   *
   * \param startFrequency The SubBand's lowest frequency.
   * \param endFrequency The SubBand's highest frequency.
   * \param dutyCycle The SubBand's duty cycle, in fraction form.
   * \param maxTxPowerDbm The maximum transmission power allowed on the SubBand.
   */
  void AddSubBand (double startFrequency, double endFrequency, double dutyCycle,
                   double maxTxPowerDbm);

  /**
   * Add a MAC command to the list of those that will be sent out in the next
   * packet.
   */
  void AddMacCommand (Ptr<MacCommand> macCommand);

  uint8_t GetTransmissionPower (void);
  
  //////////////////////////////////////////////////
  //  LoRaWAN Class B Related enums and typdefs  //
  ////////////////////////////////////////////////
  
  /// The Mac state the end device is currently at
  enum MacState
  {
    MAC_TX, ///< Device Mac is transmitting 
    MAC_RX1, ///< Device Mac has currently opened the Rx1 window
    MAC_RX2, ///< Device Mac has currently opened the Rx2 window
    MAC_BEACON_GUARD, ///< Device Mac is in Beacon_guard state
    MAC_RX_BEACON_GUARD, ///< Device Mac is entering beacon guard during Rx1/Rx2 reception
    MAC_PING_SLOT_BEACON_GUARD, ///< Device Mac is entering beacon guard during ping slot reception
    MAC_BEACON_RESERVED, ///< Device Mac is in the Beacon_reserved state
    MAC_PING_SLOT,  ///< Device Mac has opened the ping slot receiver window
    MAC_IDLE, ///< Device Mac is free. You can transmit only in this state.
  };
  
  /// The LoRaWAN Device Class that the end device is operating on
  enum DeviceClass
  {
    CLASS_A, ///< device currently operating on Class A
    CLASS_B, ///< device currently operating on Class B
    CLASS_C  ///< device currently operating on Class C. NOT IMPLIMENTED YET.
  };
  
  /// The state of the device with respect to receiving beacon
  enum BeaconState
  {
    BEACON_UNLOCKED, ///< Beacon is not locked
    BEACON_SEARCH, ///< Mac is searching for beacon
    BEACON_LOCKED, ///< Beacon is locked
    BEACONLESS, ///< Beacon not received but device is still in the beaconless operation mode
  };
  
  /// Kind of service for which we are receiving a downlink in the ping slots
  enum ServiceType
  {
    UNICAST, ///< receives messages in the unicast address
    MULTICAST ///< receives messages in the multicast address
  };
  
  /**
   * \param the kind of service that the packet is intended for based on whether
   * the packet is received on multicast or unicast address
   * \param the received packet
   * \param the ping number in which the packet is received
   * \param the slot index through which we received the downlink
   */
  typedef Callback<void, enum ServiceType, Ptr<const Packet>, uint8_t > ClassBDownlinkCallback;
  
  //////////////////////////////////////////////////////
  // Conflict resolution Between Class A and Class B //
  ////////////////////////////////////////////////////
  
  /**
   * \brief Uses defined algorithm to give the next time by when you can transmit 
   * this packet 
   * 
   * Tx requests come from the application layer asynchronously. Therefore,
   * conditions arise where either the Tx it self or its corresponding Rx 
   * window conflict with the beacon guard, beacon reserved, or ping slots.
   * We can change this algorithm so as to try different techniques and compare
   * performance. 
   * \TODO Therefore additional API could be provided for selecting from 
   * available algorithms.   
   * 
   * \param packet The packet which you want to transmit. This can be used to 
   * calculate air-time and find out whether it fits or not. 
   * \return The next time by which you can attempt to transmit.  
   */
  Time ResolveWithClassBAndGetTime ( Ptr<const Packet> packet );
  
  ////////////////////////////////////////
  //  LoRaWAN Class B Beacon Receive   //
  //////////////////////////////////////
  
  /**
   * \brief It begins the beacon search process to switch to class B
   * 
   * The end-device application requests the LoRaWAN layer to switch to Class B mode.
   * The LoRaWAN layer in the end-device searches for a beacon and invokes the
   * m_beaconLockedCallback of the application if beacon was found and locked or 
   * a m_beaconLostCallback (to inform beacon is not locked). 
   */
  void SwitchToClassB (void);
  
  /**
   * \brief It cancels all pending ping slots and beacon guard and switches back
   * to Class A
   */
  void SwitchFromClassB (void); 
  
  /**
   * \brief Reserves the beacon guard time so that no transmission is during this
   * time
   * 
   */
  void StartBeaconGuard (void);
  
  /**
   * \brief Releases the beacon guard and schedules the beacon reserved.
   * 
   */
  void EndBeaconGuard (void);
  
  /**
   * \brief Perform operations needed to open the beacon receive window and 
   * also reserves the beacon reserved time for beacon only
   * 
   */
  void StartBeaconReserved (void);
  
  /**
   * \brief Releases the beacon reserved duration and performs operation to be
   * done at the end of the beacon reserved.
   * 
   * It schedules the coming ping slots, it schedules the next StartBeaconGuard
   * and it calls the beaconLockedCallback if m_beaconInfo.bcnTime is still 
   * 0 seconds. 
   * 
   */
  void EndBeaconReserved (void);
  
  /**
   * \brief Perform operations needed to close the beacon receive window
   * 
   * If the m_beaconInfo.bcnTime has a difference more than 128 minute 
   * it will call the  m_beaconLostCallback and do operations to switch the 
   * device back to Class A.
   */
  void CloseBeaconReceiveWindow (void);
  
  /**
   * \brief Do procedure that need to be done when you miss a beacon
   * 
   * If no beacon is locked yet it doesn't do anything.If the minimal beacon less 
   * operation mode is exceeded it switches back to class A.
   */
  void BeaconMissed (void);
  
  /**
   * \brief When beacon is received we reset the beacon paramters back to default
   * and  update beacon related information
   * 
   * This function should be called from the Receive function when a beacon 
   * payload is received (which is during the m_beaconState == BEACON_RESERVED).
   * 
   * \param packet the packet that contains the beacon payload
   */
  void BeaconReceived (Ptr<Packet const> packet);
  
  
  
  ////////////////////////////////////////
  // LoRaWAN Class B PingSlot Receive  //
  //////////////////////////////////////
  
  /**
   * \brief Perform operations needed to open the ping slot receive window 
   * 
   * \param slotIndex the slot index (N) to which this ping slot corresponds to
   * which ranges from 0 to pingNb-1 
   * 
   */
  void OpenPingSlotReceiveWindow (uint8_t slotIndex);
  
  /**
   * \brief Perform operations needed to close the ping slot receive window
   */
  void ClosePingSlotRecieveWindow (void);
  
  /**
   * \brief Process a ping payload and fire traces and invoke the classBDownlinkCallback
   * callback with the slot index it is received at.
   * 
   * This is called by the Receive method when a packet is received while the 
   * EndDeviceLoraMac is in the PINGSLOT context.
   * 
   * \param packet the packet that is received in the ping slot
   */
  void PingReceived (Ptr<Packet const> packet);

  
  ///////////////////////////////////////////////////
  //  LoRaWAN Class B Related Getters and Setters //
  /////////////////////////////////////////////////
  
  /**
   * \brief Set the device class that the end device is operating on
   *
   * This method switches the device state from one class to another. 
   * To switch to Class B, it checks the current DeviceClass and BeaconState.
   *  
   * \param deviceClass the device class to change to
   * 
   * \return true when devices class is successfully set  
   */
  bool SetDeviceClass (enum DeviceClass deviceClass);
  
  /**
   * \brief Get the device class that the end device is operating on
   * 
   * \return device class that the end device is operating on will be returned
   */
  enum DeviceClass GetDeviceClass (void);
  
  /**
   * \brief Set the data rate corresponding to the ping slot windows
   * 
   * \param pingSlotDr the data rate to be used for the ping slots
   */
  void SetPingSlotReceiveWindowDataRate (uint8_t pingSlotDr);
  
  /**
   * \brief Get the data rate which the ping slots are operating on
   * 
   * \return the data rate the ping slots are operating on
   */
  uint8_t GetPingSlotReceiveWindowDataRate (void);

  /**
   * \brief Set the frequency that correspond to the ping slot windows
   * 
   * The ping slots by default operates on 869.525MHz for EU Region. 
   * 
   * \param frequency the frequency in MHz that corresponds to the ping slots
   */
  void SetPingSlotReceiveWindowFrequency (double frequency); 
  
  /**
   * \brief Get the frequency that the ping slots are operating on
   * 
   * \return the frequency in MHz on which the ping slots are operating on
   */
  double GetPingSlotRecieveWindowFrequency (void);
  
  /**
   * \brief Set the frequency that we expect to receive beacons
   * 
   * The beacon by default operates on 869.525MHz for EU Region. This is method
   * is provided just for experiment and the default should be left as it is for
   * EU Region
   * 
   * \param frequency the frequency in MHz to be used by the end devices to
   * listen to beacons. 
   */
  void SetBeaconReceiveWindowFrequency (double frequency); 
  
  /**
   * \brief Get the frequency that we expect to receive the beacons
   * 
   * \return the frequency in MHz on which we receive beacons
   */
  double GetBeaconRecieveWindowFrequency (void);
 
    /**
   * \brief Set the DR that we expect to receive beacons
   * 
   * The beacon by default uses DR3 for EU Region. This is method
   * is provided just for experiment and the default should be left as it is for
   * EU Region
   * 
   * \param beaconDr the DR used by the end devices to listen to beacons. 
   */
  void SetBeaconReceiveWindowDataRate (uint8_t beaconDr); 
  
  /**
   * \brief Get the DR that we expect to receive the beacons
   * 
   * \return the DR used to receive beacons
   */
  uint8_t GetBeaconRecieveWindowDataRate (void);
  
  /**
   * \brief Set the ping-slot-periodicity for which to open the ping slots 
   * 
   * This will set the ping-slot-periodicity for opening ping slots. It will   
   * additionally drive and set the PingNb and the PingPeriod. Periodicity 
   * should be between 0 to 7. 
   * It resizes and void m_pingSlotInfo.pendingPingSlotEvents.
   * 
   * \param periodicity the ping-slot-periodicity for opening the ping slots 
   * per beacon period.
   */
  void SetPingSlotPeriodicity (uint8_t periodicity);
  
  /**
   * \brief Get the ping-slot-periodicity which the end-device is opening the
   * ping slots
   * 
   * \return the ping-slot-periodicity which the end-device is opening its ping
   * slots 
   */
  uint8_t GetPingSlotPeriodicity (void);
  
  /**
   * \brief Get the number of ping slots to open in a beacon period
   *
   * This will set the pingNb which is the number of ping slots per beacon period. 
   * It will also drive and set the PingSlotPeriodicity and the PingPeriod. It 
   * will give an error for invalid PingNb.
   * It resizes and void m_pingSlotInfo.pendingPingSlotEvents.
   * 
   * \param pingNb the number of ping slots per beacon period 
   */
  void SetPingNb (uint8_t pingNb);
  
  /**
   * \brief Get the number of ping slots the device is opening per beacon period.
   * 
   * \return the number of ping slots per beacon period
   */
  uint8_t GetPingNb (void);
  
  /**
   * \brief Set the ping period between ping slots
   * 
   * This will set the ping period between ping slots. Additionally, it will 
   * drive and set the PingNb and the PingPeriod. It will give an error for
   * invalid ping periods.
   * It resizes and void m_pingSlotInfo.pendingPingSlotEvents.
   * 
   * \param pingPeriod the ping period between ping slots
   */
  void SetPingPeriod (uint pingPeriod); 
  
  /**
   * \brief Get the ping period between ping slots
   * 
   * \return the ping period between ping slots
   */
  uint GetPingPeriod (void);

  /**
   * \brief Used to set a callback for which to receive the classB downlinks.
   * 
   * \param classBDownlinkCallback a callback to be invoked when receiving
   * downlinks in the ping slots. 
   */
  void SetClassBDownlinkCallback (ClassBDownlinkCallback classBDownlinkCallback);
  
  /**
   * \brief Used to set a callback to be invoked when beacon is locked after 
   * the BeaconSearch method is called.
   * 
   * \param beaconLockedCallback the callback to be invoked when beacon is locked.
   */
  void SetBeaconLockedCallback (Callback<void> beaconLockedCallback);
  
  /**
   * \brief Used to set a callback to be invoked when beacon is lost after
   * the minimal beaconless operation mode, or after invoking the function
   * BeaconSearch.
   * 
   * \param beaconLostCallback the callback to be invoked when beacon is not locked. 
   */
  void SetBeaconLostCallback (Callback<void> beaconLostCallback);
  
  
  /**
   * \brief Get the mac state that end device is currently at
   * 
   * The SetMacState should be private as the state should be only modified 
   * internally when conditions are meet.
   * 
   * \return the m_macState that the end device is currently at
   */
  enum MacState GetMacState (void);  
  
  ///////////////////////////////////////////
  // Enabling and Disabling Multicasting  //
  /////////////////////////////////////////
  
  /**
   * \brief Enabling Class B multicast reception
   * 
   * The multicast address has to be set in-order to enable multicast. If it is 
   * enabled, Switching to Class B will automatically use the class B parameters
   * for multicasting. Furthermore, you will now receive in the application 
   * layer callback the multicast packets.
   * 
   * //\NOTE coexistence of multicast and unicast with different parameter is not 
   * implemented in this module. Furthermore, there can only be one multicast 
   * service per device in the current implementation.
   *  
   */
  void EnableMulticast (void);
  
  /**
   * \brief Disable Class B multicast reception
   * 
   * If disabled although the packet will still be received, it won't be passed
   * to the application layer.
   * 
   */
  void DisableMulticast (void);
  
  /**
   * \brief Check whether the end device is enabled for multicast service.
   *
   * \return true if the end device is enabled for multicast transmission.
   */
  bool IsMulticastEnabled  (void);
  
  ////////////////////////////
  // Tracesource callbacks //
  //////////////////////////
  
  /**
   * Tracecallback for receiving a packet that has been received via ping slot
   * 
   * \param mcAddress the multicast address of this device, 0 if it is a unicast device
   * \param ucAddress the unicast address of this device
   * \param packet the packet received
   * \param slotIndex from 0 to N-1 where N is the PingNb which is the number of 
   *  ping slots in a beacon period
   */
  typedef void (*ReceivedPingPacket) (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, Ptr<const Packet> packet, uint8_t slotIndex);
  
  
  /**
   * Custom traced value that will include the unicast and the multicast address along
   * with the new traced value
   * 
   * \param mcAddress the multicast address of this device, 0 if it is a unicast device
   * \param ucAddress the unicast address of this device
   * \param beaconMissedCount number of beacons missed
   */
  typedef void (*CustomTracedValue) (LoraDeviceAddress mcAddress, LoraDeviceAddress ucAddress, uint32_t newValue);
  
  
  //////////////////////////////////////////////////////////////
  // MAC Layer modification to enabling cooprative relaying 
  // 
  // Note: This modification is just to relay the packet received
  // on the next ping slot if a received packet has a hop count of
  // 0. The transmission power that is used in-order to relay 
  // the packet is the 27db which is the maximum power allowed in
  // 869.525 band + margin power (like 10 db) devided by the 
  // number of devices in the multicast group
  //
  ////////////////////////////////////////////////////////////
  
  void EnableCoordinatedRelaying (uint32_t numberOfEndDeviceInMcGroup); 
  
  
private:
  /**
   * Structure representing the parameters that will be used in the
   * retransmission procedure.
   */
  struct LoraRetxParameters
  {
    Time firstAttempt;
    Ptr<Packet> packet = 0;
    bool waitingAck = false;
    uint8_t retxLeft;
  };
  
  /**
   * Structure for storing beacon related informations matching the LoRaWAN-v1.03
   * and greater specification
   */
  struct BeaconInfo
  { 
    EventId endBeaconGuardEvent; ///< Progress of a scheduled beacon guard
    EventId endBeaconReservedEvent; ///< Progress of a shceduled beacn reserved
    EventId nextBeaconGuardEvent;  ///< Scheduled upcoming beacon guard event
    Time gwBcnTime = Seconds (0); ///< time stamp of last beacon payload recieved from gateway, 0 if none
    Time deviceBcnTime = Seconds (0); ///< Equal with the time stamp of the last beacon payload received from gateway and gets adjusted during minimal beacon less opration time
    Time minimalBeaconLessOperationTime = Minutes (128); ///< minimal beacon less operation time, default 128 minutes 
    Time beaconPeriod = Seconds (128); ///< Beacon_period, default 128 Seconds
    Time beaconReserved = Seconds (2.12); ///< Beacon_reserved, default 2.12 Seconds
    Time beaconGuard = Seconds (3.0); ///< Beacon_guard, default 3.0 Seconds
    Time beaconWindow = Seconds (122.88); ///< Beacon_window, default 122.88 Seconds  
    Time tBeaconDelay = Seconds (0.015); ///< TBeaconDelay = 0.015 Seconds, not including +/-1uSec jitter
  };
  
  /**
   * Structure for storing ping slot related informations
   */
  struct PingSlotInfo
  {
    std::vector<EventId> pendingPingSlotEvents; ///< EventId ping slot events that are scheduled per beacon window
    uint8_t pingSlotPeriodicity = 0; ///< PingSlotPeriodicity, default is 0
    uint8_t pingNb = 128; ///< PingNb (number of pings in beacon window), default is 128
    uint pingPeriod = 32; ///< pingPeriod (in number of slots), default is 32
    Time slotLen = Seconds (0.03); ///< slotLen length of a slot is 30mSec
    uint64_t pingOffset = 31; ///< the latest pingOffset, default is 31 (the maximum)
  };
  
  /**
   * Structure for storing informations related to class B receive windows.
   * 
   * The beacon and ping slot receiver channel and ping slot are stored here. 
   * On top of this, receive window duration timeout both for the beacon and 
   * the ping slots which are expanded by symbolToExpantionFactor on each missed 
   * beacon are stored here.   
   */
  struct ClassBReceiveWindowInfo
  {
    double beaconReceiveWindowFrequency = 869.525;
    uint8_t beaconReceiveWindowDataRate = 3;
    double pingSlotReceiveWindowFrequency = 869.525;
    uint8_t pingSlotReceiveWindowDataRate = 3;  
    uint16_t beaconReceiveWindowDurationInSymbols = 8;
    uint8_t pingReceiveWindowDurationInSymbols = 8;
    uint8_t maxBeaconReceiveWindowDurationInSymbols = 255;
    uint8_t maxPingReceiveWindowDurationInSymbols = 30;
    uint8_t symbolToExpantionFactor = 2;
  };
  
  /**
   * \brief Set method to change the MacState.
   * The SetMacState should be private as the state should be only modified 
   * internally when conditions are meet.
   * 
   * \param macState the state to change the MacStateTo
   */
  void SetMacState (enum MacState macState);
  
  /**
   * \brief Schedules all ping slots in the current beacon period
   * 
   * This will calculate the ping slots with their corresponding ping offsets, which
   * again depends on the device Address. The device address depends on whether
   * the device is in MULTICAST or UNICAST.
   */
  void SchedulePingSlots (void);
  
  /**
   * Enable Data Rate adaptation during the retransmission procedure.
   */
  bool m_enableDRAdapt;

  /**
   * Maximum number of transmission allowed.
   */
  uint8_t m_maxNumbTx;

  /**
   * Randomly shuffle a Ptr<LogicalLoraChannel> vector.
   *
   * Used to pick a random channel on which to send the packet.
   */
  std::vector<Ptr<LogicalLoraChannel> > Shuffle (std::vector<Ptr<LogicalLoraChannel> > vector);

  /**
    * Find the minimum waiting time before the next possible transmission.
    */
  Time GetNextTransmissionDelay (void);


  /**
   * Find a suitable channel for transmission. The channel is chosen among the
   * ones that are available in the ED's LogicalLoraChannel, based on their duty
   * cycle limitations.
   */
  Ptr<LogicalLoraChannel> GetChannelForTx (void);

  /**
   * An uniform random variable, used by the Shuffle method to randomly reorder
   * the channel list.
   */
  Ptr<UniformRandomVariable> m_uniformRV;


/**
   * The total number of transmissions required.
   */
/*
TracedValue<uint8_t> m_requiredTx;
*/

  /**
   * The DataRate this device is using to transmit.
   */
  TracedValue<uint8_t> m_dataRate;

  /**
   * The transmission power this device is using to transmit.
   */
  TracedValue<double> m_txPower;

  /**
   * The coding rate used by this device.
   */
  uint8_t m_codingRate;

  /**
   * Whether or not the header is disabled for communications by this device.
   */
  bool m_headerDisabled;

  /**
   * The interval between when a packet is done sending and when the first
   * receive window is opened.
   */
  Time m_receiveDelay1;

  /**
   * The interval between when a packet is done sending and when the second
   * receive window is opened.
   */
  Time m_receiveDelay2;

  /**
   * The duration of a receive window in number of symbols. This should be 
   * converted to time based or the reception parameter used.
   * 
   * The downlink preamble transmitted by the gateways contains 8 symbols. 
   * The receiver requires 5 symbols to detect the preamble and synchronize. 
   * Therefore there must be a 5 symbols overlap between the receive window 
   * and the transmitted preamble. 
   * (Ref: Recommended SX1272/76 Settings for EU868 LoRaWAN Network Operation )
   */
  uint8_t m_receiveWindowDurationInSymbols;

  /**
   * The event of the closing the first receive window.
   *
   * This Event will be canceled if there's a successful reception of a packet.
   */
  EventId m_closeFirstWindow;

  /**
   * The event of the closing the second receive window.
   *
   * This Event will be canceled if there's a successful reception of a packet.
   */
  EventId m_closeSecondWindow;

  /**
   * The event of the second receive window opening.
   *
   * This Event is used to cancel the second window in case the first one is
   * successful.
   */
  EventId m_secondReceiveWindow;

  /**
   * The event of retransmitting a packet in a consecutive moment if an ACK is not received.
   *
   * This Event is used to cancel the retransmission if the ACK is found in ParseCommand function and
   * if a newer packet is delivered from the application to be sent.
   */
  EventId m_nextTx;

  /**
   * The event of transmitting a packet in a consecutive moment, when the duty cycle let us transmit.
   *
   * This Event is used to cancel the transmission of this packet if a newer packet is delivered from the application to be sent.
   */
  EventId m_nextRetx;
  /**
   * The address of this device.
   */
  LoraDeviceAddress m_address;
  
  /**
   * The multicast address of this device
   * \todo there could be more than one multicast address in LoRaWAN
   */
  LoraDeviceAddress m_mcAddress;

  /**
   * The frequency to listen on for the second receive window.
   */
  double m_secondReceiveWindowFrequency;

  /**
   * The Data Rate to listen for during the second downlink transmission.
   */
  uint8_t m_secondReceiveWindowDataRate;

  /**
   * The RX1DROffset parameter value
   */
  uint8_t m_rx1DrOffset;

  /**
   * The last known link margin.
   *
   * This value is obtained (and updated) when a LinkCheckAns Mac command is
   * received.
   */
  TracedValue<double> m_lastKnownLinkMargin;

  /**
   * The last known gateway count (i.e., gateways that are in communication
   * range with this end device)
   *
   * This value is obtained (and updated) when a LinkCheckAns Mac command is
   * received.
   */
  TracedValue<int> m_lastKnownGatewayCount;

  /**
   * List of the MAC commands that need to be applied to the next UL packet.
   */
  std::list<Ptr<MacCommand> > m_macCommandList;

  /**
   * The aggregated duty cycle this device needs to respect across all sub-bands.
   */
  TracedValue<double> m_aggregatedDutyCycle;

  /**
   * The message type to apply to packets sent with the Send method.
   */
  LoraMacHeader::MType m_mType;

  /* Structure containing the retransmission parameters
   * for this device.
   */
  struct LoraRetxParameters m_retxParams;

  uint8_t m_currentFCnt;
  
  ///////////////////////
  // End Device State //
  /////////////////////
  
  /**
   * The Mac state the end device is currently at
   * 
   * \todo Make this a traced member 
   */
  TracedValue <MacState> m_macState;
  
  /**
   * The Device Class that the end-device is currently operating on
   */
  TracedValue <DeviceClass> m_deviceClass;
  
  /**
   * The state of the device with respect to receiving beacon
   */
  TracedValue <BeaconState> m_beaconState;
  
  ///////////////////////////////////////////
  //  LoRaWAN Class B related Information //
  /////////////////////////////////////////
  
  /**
   * Structure for storing and retrieving beacon related informations 
   */
  struct BeaconInfo m_beaconInfo;
  
  /**
   * Structure for storing and retrieving ping slot related informations
   */
  struct PingSlotInfo m_pingSlotInfo;
  
  /**
   * Structure for storing and retrieving ping slot and beacon related informations
   */
  struct ClassBReceiveWindowInfo m_classBReceiveWindowInfo;
  
  /**
   * Latest slot index that is opened, to keep track when ping is received
   * 
   * if it is 255 it means nothing was received
   */
  uint8_t m_slotIndexLastOpened;
  
  /////////////////
  //  Callbacks  //
  /////////////////

  /**
   * The trace source fired when the transmission procedure is finished.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<uint8_t, bool, Time, Ptr<Packet> > m_requiredTxCallback;
  
  ///////////////////////////////
  // ClassB related Callbacks //
  /////////////////////////////

  /**
   * Beacon-lost callback (invoked after the minimal beaconless operation mode)
   */
  Callback<void> m_beaconLostCallback;
  
  /**
   * Beacon-locked callback (invoked when beacon is locked)
   */
  Callback<void> m_beaconLockedCallback;
  
  /**
   * Callback to invoke when you receive downlink via the ping slots
   */
  ClassBDownlinkCallback m_classBDownlinkCallback;
  
  /**
   * The trace source fired when receiving ping packet
   */
  TracedCallback<LoraDeviceAddress, LoraDeviceAddress, Ptr<const Packet>, uint8_t> m_receivedPingPacket;
  
  /**
   * Number of packets failed while receiving in the ping slot
   */
  TracedValue<uint32_t> m_failedPings;
  
  /**
   * Number of beacon packets received
   */
  TracedValue<uint32_t> m_totalSuccessfulBeaconPackets;
  
  /**
   * Number of packets received traced-callback
   */
  TracedCallback<LoraDeviceAddress, LoraDeviceAddress, uint32_t> m_totalSuccessfulBeaconPacketsTracedCallback;
  
  /**
   * Number of packets failed while receiving in the ping slot
   */
  TracedValue<uint32_t> m_missedBeaconCount;
  
  /**
   * Missed beacon traced-callback
   */
   TracedCallback<LoraDeviceAddress, LoraDeviceAddress, uint32_t> m_missedBeaconTracedCallback;
  
  /**
   * Number of packets failed while receiving in the ping slot
   */
  TracedValue<uint8_t> m_maximumConsecutiveBeaconsMissed;
  
  /**
   * Number of consecutive beacons missed in the current minimal beaconless operation mode
   * 
   * If it is not in minimal beaconless operation mode it will be zero
   */
  TracedValue<uint8_t> m_currentConsecutiveBeaconsMissed;  
  
  /**
   * Number of consecutive beacons missed in the current minimal beaconless operation mode traced callback
   */
   TracedCallback<LoraDeviceAddress, LoraDeviceAddress, uint8_t> m_currentConsecutiveBeaconsMissedTracedCallback;
  
  /**
   * Number of failed attempts to switch to class B
   */
  TracedValue<uint32_t> m_attemptToClassB;
  
  /**
   * Total number of downlink bytes received
   */
  TracedValue<uint32_t> m_totalBytesReceived;
  
  ////////////////////////////////
  // Related to multicasting   //
  //////////////////////////////
  
  bool m_enableMulticast;
  
  //////////////////////////////
  // Related to coordinated-relaying     //
  ////////////////////////////
  bool m_relayActivated; 
  
  bool m_relayPending;
  
  double m_maxBandTxPower; ///< Max transmission power allowed in the band
  
  double m_marginTxPower; ///< Margin Tx power to be added to the m_maxBandTxPower before calculating m_relayPower
  
  double m_relayPower; ///< Actual power that is going to be used for relay transmission
  
  uint8_t maxHop; ///< The maximum amount of time the packet hops
  
  std::list<Ptr<Packet>> m_packetToRelay; ///< Packet to be relayed to the next
  
};


} /* namespace ns3 */

}
#endif /* END_DEVICE_LORA_MAC_H */
