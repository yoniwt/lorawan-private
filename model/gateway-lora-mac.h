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

#ifndef GATEWAY_LORA_MAC_H
#define GATEWAY_LORA_MAC_H

#include "ns3/lora-mac.h"
#include "ns3/lora-tag.h"

#include "ns3/lora-device-address.h"

namespace ns3 {
namespace lorawan {

class GatewayLoraMac : public LoraMac
{
public:
  static TypeId GetTypeId (void);

  GatewayLoraMac ();
  virtual ~GatewayLoraMac ();

  // Implementation of the LoraMac interface
  virtual void Send (Ptr<Packet> packet);

  // Implementation of the LoraMac interface
  bool IsTransmitting (void);

  // Implementation of the LoraMac interface
  virtual void Receive (Ptr<Packet const> packet);

  // Implementation of the LoraMac interface
  virtual void FailedReception (Ptr<Packet const> packet);

  // Implementation of the LoraMac interface
  virtual void TxFinished (Ptr<Packet const> packet);

  /**
   * Return the next time at which we will be able to transmit.
   *
   * \return The next transmission time.
   */
  Time GetWaitingTime (double frequency);
  
  //////////////////////////////
  // LoRaWAN Class B related //
  ////////////////////////////
    
  /**
   * \brief Enable this Gateway for sending donwlinks via ping slots
   */
  void EnableClassBTransmission (void);
  
  /**
   * \brief Disable the Gateway from sending downlinks via ping slots
   */
  void DisableClassBTransmission (void);
  
  /**
   * \brief Whether the gateway is enabled to send downlinks via ping slots
   * 
   * \return true if the gateway is enabled to send downlinks via ping slots
   */
  bool IsClassBTransmissionEnabled (void);
  
  /**
   * \brief Enable this Gateway for beacon transmission
   */
  void EnableBeaconTransmission (void);
  
  /**
   * \brief Disable this Gateway from beacon transmission
   */
  void DisableBeaconTransmission (void);
  
  /**
   * \brief Whether the gateway is enabled for beacon transmission
   * 
   * \return true if the gateway is enabled to transmit beacons 
   */
  bool IsBeaconTransmissionEnabled (void);
  
  //////////////////////////////////////////////////////////////
  // For managing multicast groups to be served by a gateway //
  ////////////////////////////////////////////////////////////
  
  /**
   * Add a multicast address to be served by a gateway
   * 
   * \param mcAddress the multicast address to be served by this gateway
   */
  void AddMulticastGroup (LoraDeviceAddress mcAddress);
  
  /**
   * Get the list of multicast address this gateway is serving
   * 
   * \return a list of LoraDeviceAddress that this gateway serves
   */
  std::list<LoraDeviceAddress> GetMulticastGroups (void);
  
  /**
   * Check if a multicast address is in the group of multicast address that this 
   * gateway serves
   * 
   * \param mcAddress the address to check whether it is in the list of multicast
   * address that this gateway is serving
   * \return true if the multicast address is in the list or false otherwise. 
   */
  bool CheckMulticastGroup (LoraDeviceAddress mcAddress);
  
private:
  /**
   * Whether this gateway can do beacon transmission
   */
  bool m_beaconTransmission;
  
  /**
   * Whether this gateway can send class B downlinks via pingslots
   */
  bool m_classBTransmission;
  
  /**
   * Multicast address for which the gateway is responsible for transmitting
   * 
   * To be used by the NetworkStatus in order to select suitable gateway 
   */
  std::list<LoraDeviceAddress> m_mcAddressList;
  
protected:
};

} /* namespace ns3 */

}
#endif /* GATEWAY_LORA_MAC_H */
