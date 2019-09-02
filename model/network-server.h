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

#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "ns3/object.h"
#include "ns3/application.h"
#include "ns3/net-device.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/packet.h"
#include "ns3/lora-device-address.h"
#include "ns3/device-status.h"
#include "ns3/gateway-status.h"
#include "ns3/network-status.h"
#include "ns3/network-scheduler.h"
#include "ns3/network-controller.h"
#include "ns3/node-container.h"
#include "ns3/log.h"
#include "ns3/end-device-lora-mac.h"

namespace ns3 {
namespace lorawan {

/**
 * The NetworkServer is an application standing on top of a node equipped with
 * links that connect it with the gateways.
 *
 * This version of the NetworkServer attempts to closely mimic an actual
 * Network Server, by providing as much functionality as possible.
 */
class NetworkServer : public Application
{
public:
  static TypeId GetTypeId (void);

  NetworkServer ();
  virtual ~NetworkServer ();

  /**
   * Start the NS application.
   */
  void StartApplication (void);

  /**
   * Stop the NS application.
   */
  void StopApplication (void);

  /**
   * Inform the NetworkServer that these nodes are connected to the network.
   *
   * This method will create a DeviceStatus object for each new node, and add
   * it to the list.
   */
  void AddNodes (NodeContainer nodes);

  /**
   * Inform the NetworkServer that this node is connected to the network.
   * This method will create a DeviceStatus object for the new node (if it
   * doesn't already exist).
   */
  void AddNode (Ptr<Node> node);

  /**
   * Add this gateway to the list of gateways connected to this NS.
   * Each GW is identified by its Address in the NS-GWs network.
   */
  void AddGateway (Ptr<Node> gateway, Ptr<NetDevice> netDevice);

  /**
   * A NetworkControllerComponent to this NetworkServer instance.
   */
  void AddComponent (Ptr<NetworkControllerComponent> component);

  /**
   * Receive a packet from a gateway.
   * \param packet the received packet
   */
  bool Receive (Ptr<NetDevice> device, Ptr<const Packet> packet,
                uint16_t protocol, const Address& address);

  Ptr<NetworkStatus> GetNetworkStatus (void);
  
  Ptr<NetworkScheduler> GetNetworkScheduler (void); 

  ///////////////////////////////////////////////////////
  // Functions to modify network-scheduler attributes //
  /////////////////////////////////////////////////////
  
  /**
   * To Enable the server for the class B downlink transmission
   * 
   * \param enable true to enable class B downlink transmission and false to 
   * disable
   */
  void EnableClassBDownlink (bool enable);
  
  /**
   * To Enable the server for beacon transmission
   * 
   * \param enable true to enable beacon transmission and false to disable
   */
  void EnableBeaconTransmission (bool enable);
  
  /**
   * Enable Fragmented Data Generation which leads to sequenced packet generation
   * 
   * \param enable if true it enables the sequenced packet generation
   */
  void EnableSequencedPacketGeneration (bool enable);
  
  /**
   * Set the packet size for the ping downlink
   * 
   * \param [in] downlinkPacketSize downlink packet size to be used for downlink.
   * If it is 0 randomSize will be selected and if it is above the maximum packet
   * size the data-rate support to 255 the corresponding maximum packet size will
   * be used
   */
  void SetPingDownlinkPacketSize (uint8_t pingDownlinkPacketSize);
  
  /**
   * Get the packet size set for the ping downlinks
   * 
   * \return downlink packet size used for the ping downlink. If it is 0 
   * randomSize will be selected and if it is above the maximum packet size the 
   * data-rate support to 255 the corresponding maximum packet size will be used
   */
  uint8_t GetPingDownlinkPacketSize (void) const;
  
protected:
  Ptr<NetworkStatus> m_status;
  Ptr<NetworkController> m_controller;
  Ptr<NetworkScheduler> m_scheduler;

  TracedCallback<Ptr<const Packet> > m_receivedPacket;
  
  /**
   * This is enabled when a class B enabled gateway is added to the server
   */
  bool m_classBEnabled;
  
  /**
   * This is enabled when a beacon transmitting gateway is added to the server
   */
  bool m_beaconEnabled;
  
};

} /* namespace ns3 */

}
#endif /* NETWORK_SERVER_H */
