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

#ifndef NETWORK_SERVER_HELPER_H
#define NETWORK_SERVER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/network-server.h"
#include <stdint.h>
#include <string>

namespace ns3 {
namespace lorawan {

/**
 * This class can install Network Server applications on multiple nodes at once.
 */
class NetworkServerHelper
{
public:
  NetworkServerHelper ();

  ~NetworkServerHelper ();

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c);

  ApplicationContainer Install (Ptr<Node> node);

  /**
   * Set which gateways will need to be connected to this NS.
   */
  void SetGateways (NodeContainer gateways);

  /**
   * Set which end devices will be managed by this NS.
   */
  void SetEndDevices (NodeContainer endDevices);
  
  /**
   * To Enable the servers for the class B downlink transmission
   * 
   * \param enable true to enable class B downlink transmission and false to 
   * disable
   */
  void EnableClassBDownlink (bool enable);
  
  /**
   * To enable the servers for beacon transmission
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

private:
  void InstallComponents (Ptr<NetworkServer> netServer);
  Ptr<Application> InstallPriv (Ptr<Node> node);

  ObjectFactory m_factory;

  NodeContainer m_gateways;   //!< Set of gateways to connect to this NS

  NodeContainer m_endDevices;   //!< Set of endDevices to connect to this NS

  PointToPointHelper p2pHelper; //!< Helper to create PointToPoint links
  
  /**
   * This is enabled when a class B enabled gateway is added to the server
   */
  bool m_classBEnabled;
  
  /**
   * This is enabled when a beacon transmitting gateway is added to the server
   */
  bool m_beaconEnabled;
  
  /**
   * To enable sequenced packet generation
   */
  bool m_enableSequencedPacketGeneration;
  
  /**
   * ping downlink packet size to be used for all multicast groups
   */
  uint8_t m_pingDownlinkPacketSize;
};

} // namespace ns3

}
#endif /* NETWORK_SERVER_HELPER_H */
