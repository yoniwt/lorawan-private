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

#ifndef LORA_MAC_HELPER_H
#define LORA_MAC_HELPER_H

#include "ns3/net-device.h"
#include "ns3/lora-channel.h"
#include "ns3/lora-phy.h"
#include "ns3/lora-mac.h"
#include "ns3/lora-device-address-generator.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/node-container.h"

namespace ns3 {
namespace lorawan {

class LoraMacHelper
{
public:
  /**
   * Define the kind of device. Can be either GW (Gateway) or ED (End Device).
   */
  enum DeviceType
  {
    GW,
    ED
  };

  /**
   * Define the operational region.
   */
  enum Regions
  {
    EU,
    US,
    China,
    EU433MHz,
    Australia,
    CN,
    AS923MHz,
    SouthKorea
  };

  /**
   * Create a mac helper without any parameter set. The user must set
   * them all to be able to call Install later.
   */
  LoraMacHelper ();

  /**
   * Set an attribute of the underlying MAC object.
   *
   * \param name the name of the attribute to set.
   * \param v the value of the attribute.
   */
  void Set (std::string name, const AttributeValue &v);

  /**
   * Set the address generator to use for creation of these nodes.
   */
  void SetAddressGenerator (Ptr<LoraDeviceAddressGenerator> addrGen);

  /**
   * Set the kind of MAC this helper will create.
   *
   * \param dt the device type (either gateway or end device).
   */
  void SetDeviceType (enum DeviceType dt);

  /**
   * Set the region in which the device is to operate.
   */
  void SetRegion (enum Regions region);

  /**
   * Create the LoRaMac instance and connect it to a device
   *
   * \param node the node on which we wish to create a wifi MAC.
   * \param device the device within which this MAC will be created.
   * \returns a newly-created LoraMac object.
   */
  Ptr<LoraMac> Create (Ptr<Node> node, Ptr<NetDevice> device) const;

  /**
   * Set up the end device's data rates
   * This function assumes we are using the following convention:
   * SF7 -> DR5
   * SF8 -> DR4
   * SF9 -> DR3
   * SF10 -> DR2
   * SF11 -> DR1
   * SF12 -> DR0
   */
  static std::vector<int> SetSpreadingFactorsUp (NodeContainer endDevices,
                                                 NodeContainer gateways,
                                                 Ptr<LoraChannel> channel);
  
  //////////////////////////////
  // Class B related helpers //
  ////////////////////////////
  
  /**
   * Create multiple multicast group with the end-devices that are passed as a container
   * 
   * Creates mutlicast groups each having a numberofDevicesPerGroups members. A group can 
   * have less members if there are less endDevices to fit the whole group.
   * Example: 10 endDevices with 3 numberOfDevicesPerGroup will have 4 groups, where 
   * 3 of the groups contain 3 members each and the last group will contain one member only. 
   * If you instead of 10 endDevices you pass 2 endDevices, the number of groups created 
   * will be 1 with the 2 endDevices you supplied.
   * 
   * \param endDevices end device node container from which multiple multicast 
   * groups are going to be created
   * \param gateways the gateways that will serve the multicast groups created
   * \param numberOfDevicesPerGroups is the number of devices per one multicast group
   * \param enableCoordinatedRelaying to enable coordinated relaying for multicast ( Warning: Not Part of LoRaWAN Standard), default = false
   * 
   * \return the list of multicast group's LoraDeviceAddress created
   */
  std::vector<LoraDeviceAddress> CreateNMulticastGroup (NodeContainer endDevices,  NodeContainer gateways, uint32_t numberOfDevicesPerGroups, bool enableCoordinatedRelaying = false);
  
  /**
   * Create a multicast group with the end-devices that are passed as a container
   * 
   * Creates mutlicast groups each having a numberofDevicesPerGroups members. A group can 
   * have less members if there are less endDevices to fit the whole group.
   * Example: 10 endDevices with 3 numberOfDevicesPerGroup will have 4 groups, where 
   * 3 of the groups contain 3 members each and the last group will contain one member only. 
   * If you instead of 10 endDevices you pass 2 endDevices, the number of groups created 
   * will be 1 with the 2 endDevices you supplied.
   * 
   * \param endDevices end device node container from which multiple multicast 
   * groups are going to be created
   * \param gateways the gateways that will serve the multicast groups created
   * \param numberOfGroups is the number of multicast groups to be created
   * \param dr the DR the group is using
   * \param pingSlotPeriodicity the ping slot periodicity the group should use
   * \param enableCoordinatedRelaying to enable coordinated relaying for multicast ( Warning: Not Part of LoRaWAN Standard), default = false
   * \param channel the channel frequency the group is using, default is 869.525 MHz
   * 
   * \returnthe list of multicast group's LoraDeviceAddress created
   */
  std::vector<LoraDeviceAddress> CreateNMulticastGroup (NodeContainer endDevices,  NodeContainer gateways, uint32_t numberOfDevicesPerGroups, uint8_t dr, uint8_t pingSlotPeriodicity, bool enableCoordinatedRelaying = false, double channel=869.525);  
  
  
  /**
   * Create a multicast group with the end-devices that are passed as a container
   * 
   * \param endDevices end device node container that are planned to be in the
   * same multicast group
   * \param gateways the gateways that will serve the assigned multicast group
   * \param enableCoordinatedRelaying to enable coordinated relaying for multicast ( Warning: Not Part of LoRaWAN Standard), default = false
   * 
   * \return LoraDeviceAddress the multicast devAddr of the created group
   */
  LoraDeviceAddress CreateMulticastGroup (NodeContainer endDevices,  NodeContainer gateways, bool enableCoordinatedRelaying = false);
  
    /**
   * Create a multicast group with the end-devices that are passed as a container
   * 
   * \param endDevices end device node container that are planned to be in the
   * same multicast group
   * \param gateways the gateways that will serve the assigned multicast group
   * \param dr the DR the group is using
   * \param pingSlotPeriodicity the ping slot periodicity the group should use
   * \param enableCoordinatedRelaying to enable coordinated relaying for multicast ( Warning: Not Part of LoRaWAN Standard), default = false 
   * \param channel the channel frequency the group is using, default is 869.525 MHz
   *
   * \return LoraDeviceAddress the multicast devAddr of the created group
   */
  LoraDeviceAddress CreateMulticastGroup (NodeContainer endDevices,  NodeContainer gateways, uint8_t dr, uint8_t pingSlotPeriodicity, bool enableCoordinatedRelaying = false, double channel=869.525);
  
  /**
   * Add an end-device to a multicast group
   * 
   * \param endNode the node which have the end-device to add to a multicast group
   * \param mcDevAddr the LoraDeviceAddress of the multicast group to which you 
   * want the end-device to.
   * \param enableCoordinatedRelaying to enable coordinated relaying for multicast ( Warning: Not Part of LoRaWAN Standard), default = false
   * \param numberOfEndDevicesinMcGroup number of devices in a multicast group for enabling coordinated relaying ( Warning: Not Part of LoRaWAN Standard), default = 0
   * it must be greater than 1 for the coordinated relay to function   
   */
  void AddToMulticastGroup (Ptr<Node> endNode, LoraDeviceAddress mcDevAddr, bool enableCoordinatedRelaying = false, uint32_t numberOfEndDevicesinMcGroup = 0);
  
  /**
   * Add an end-device to a multicast group
   * 
   * Use the same dr, ping slot periodicity and channel that the group 
   * used
   * 
   * \param endNode the node which have the end-device to add to a multicast group
   * \param mcDevAddr the LoraDeviceAddress of the multicast group to which you 
   * want the end-device to.
   * \param \param dr the DR the group is using
   * \param pingSlotPeriodicity the ping slot periodicity the group should use
   * \param enableCoordinatedRelaying to enable coordinated relaying for multicast ( Warning: Not Part of LoRaWAN Standard), default = false
   * \param numberOfEndDevicesinMcGroup number of devices in a multicast group for enabling coordinated relaying ( Warning: Not Part of LoRaWAN Standard), default = 0
   * it must be greater than 1 for the coordinated relay to function
   * \param channel the channel frequency the group is using, default is 869.525 MHz
   * 
   */
  void AddToMulticastGroup (Ptr<Node> endNode, LoraDeviceAddress mcDevAddr, uint8_t dr, uint8_t pingSlotPeriodicity, bool enableCoordinatedRelaying = false, uint32_t numberOfEndDevicesinMcGroup = 0, double channel = 869.525);
  
  /**
   * Enabling gateways that transmit beacon
   * 
   * \param gateways A NodeContainer which have gateways on which you want to 
   * enable beacon transmission
   */
  void EnableBeaconTransmission (NodeContainer gateways);
  
  /**
   * Enabling gateways that can be used for Class B transmission
   * 
   * \param gateways a NodeContainer which have gateways on which you want to 
   * enable Class B downlink transmission
   */  
  void EnableClassBDownlinkTransmission (NodeContainer gateways);  

private:
  /**
   * Perform region-specific configurations for the 868 MHz EU band.
   */
  void ConfigureForEuRegion (Ptr<EndDeviceLoraMac> edMac) const;

  /**
   * Perform region-specific configurations for the 868 MHz EU band.
   */
  void ConfigureForEuRegion (Ptr<GatewayLoraMac> gwMac) const;

  /**
   * Apply configurations that are common both for the GatewayLoraMac and the
   * EndDeviceLoraMac classes.
   */
  void ApplyCommonEuConfigurations (Ptr<LoraMac> loraMac) const;

  ObjectFactory m_mac;
  Ptr<LoraDeviceAddressGenerator> m_addrGen; //!< Pointer to the address generator to use
  enum DeviceType m_deviceType; //!< The kind of device to install
  enum Regions m_region; //!< The region in which the device will operate
};

} //namespace ns3

}
#endif /* LORA_PHY_HELPER_H */
