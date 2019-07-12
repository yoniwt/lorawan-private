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
 * Authors: Martina Capuzzo <capuzzom@dei.unipd.it>
 *          Davide Magrin <magrinda@dei.unipd.it>
 */

#ifndef NETWORK_STATUS_H
#define NETWORK_STATUS_H

#include "ns3/device-status.h"
#include "ns3/end-device-status.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-status.h"
#include "ns3/lora-device-address.h"
#include "ns3/network-scheduler.h"
#include "ns3/traced-value.h"

namespace ns3 {
namespace lorawan {

/**
 * This class represents the knowledge about the state of the network that is
 * available at the Network Server. It is essentially a collection of two maps:
 * one containing DeviceStatus objects, and the other containing GatewayStatus
 * objects.
 *
 * This class is meant to be queried by NetworkController components, which
 * can decide to take action based on the current status of the network.
 */
class NetworkStatus : public Object
{
public:
  static TypeId GetTypeId (void);

  NetworkStatus ();
  virtual ~NetworkStatus ();

  /**
   * Add a device to the ones that are tracked by this NetworkStatus object.
   */
  void AddNode (Ptr<EndDeviceLoraMac> edMac);

  /**
   * Add this gateway to the list of gateways connected to the network.
   *
   * Each GW is identified by its Address in the NS-GW network.
   */
  void AddGateway (Address& address, Ptr<GatewayStatus> gwStatus);

  /**
   * Update network status on the received packet.
   *
   * \param packet the received packet.
   * \param address the gateway this packet was received from.
   */
  void OnReceivedPacket (Ptr<const Packet> packet, const Address& gwaddress);

  /**
   * Return whether the specified device needs a reply.
   *
   * \param deviceAddress the address of the device we are interested in.
   */
  bool NeedsReply (LoraDeviceAddress deviceAddress);

  /**
   * Return whether we have a gateway that is available to send a reply to the
   * specified device.
   *
   * \param deviceAddress the address of the device we are interested in.
   */
  Address GetBestGatewayForDevice (LoraDeviceAddress deviceAddress);

  /**
   * Send a packet through a Gateway.
   *
   * This function assumes that the packet is already tagged with a LoraTag
   * that will inform the gateway of the parameters to use for the
   * transmission.
   */
  void SendThroughGateway (Ptr<Packet> packet, Address gwAddress);

  /**
   * Get the reply for the specified device address.
   */
  Ptr<Packet> GetReplyForDevice (LoraDeviceAddress edAddress, int windowNumber);

  /**
   * Get the EndDeviceStatus for the device that sent a packet.
   */
  Ptr<EndDeviceStatus> GetEndDeviceStatus (Ptr<Packet const> packet);

  /**
   * Get the EndDeviceStatus corresponding to a LoraDeviceAddress.
   */
  Ptr<EndDeviceStatus> GetEndDeviceStatus (LoraDeviceAddress address);
  
  /**
   * Broadcasts through all beacon enabled gateways
   * 
   * \return the time stamp included in the bcnPayload, 0 if beacon is not sent
   */
  uint32_t BroadcastBeacon (void);
  
  /**
   * Multicasts a packet to a multicast group using the geteways assigned to that group
   * 
   * \param packet the appPayload to be multicasted
   * \param mcAddress the multicast address for which to do the transmission
   * 
   * \return the number of gateways that successfully transmitted the multicast
   */
  uint8_t MulticastPacket (Ptr<Packet const> packet, LoraDeviceAddress mcAddress);

public:
  typedef std::map<LoraDeviceAddress, Ptr<EndDeviceStatus> > EndDeviceStatusMap;
  typedef std::map<LoraDeviceAddress, EndDeviceStatusMap > McEndDeviceStatusMap;
  
  EndDeviceStatusMap m_endDeviceStatuses;
  std::map<Address, Ptr<GatewayStatus> > m_gatewayStatuses;
  
  McEndDeviceStatusMap m_mcEndDeviceStatuses; ///< For corsponding the unicast and the multicat address

private:  
  uint8_t m_beaconDr; ///< beacon DR to be used, default is 3
  double m_beaconFrequency; ///< beacon Frequency to be used, default is 869.525

  /// Tracesources
  TracedValue<uint8_t> m_lastBeaconTransmittingGateways;
  TracedValue<uint8_t> m_lastMulticastTransmittingGateways; 
};

} /* namespace ns3 */

}
#endif /* NETWORK_STATUS_H */
