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

#ifndef PERIODIC_SENDER_HELPER_H
#define PERIODIC_SENDER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/end-device-class-b-app.h"
#include <stdint.h>
#include <string>

namespace ns3 {
namespace lorawan {

/**
 * This class can be used to install PeriodicSender applications on a wide
 * range of nodes.
 */
class EndDeviceClassBAppHelper
{
public:
  EndDeviceClassBAppHelper ();

  ~EndDeviceClassBAppHelper ();

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c) const;

  ApplicationContainer Install (Ptr<Node> node) const;
  
  /**
   * Set the number of attempt used by the applications to switch to class B if 
   * we are an successful to lock the beacon
   * 
   * \param nAttempt number of attempts to switch to class B. If zero it means
   * there is no limit to the number of attempts
   */
  void SetNumberOfAttempt (uint8_t nAttempt);
  
    /**
   * Set for the applications the initial delay before attempting to switch to 
     * class B and intermediate delay after losing a beacon before the next attempt
     * 
     * If Seconds (0) is used or if left as default, a period of delay of 
     * 1 minute will be used
     * 
   * \param delay The initial delay and intermediate delay before switching to 
   * class B  
   */
  void SetSwitchToClassBDelay (Time delay);

  /**
   * Enabling Sending of Uplinks after beacon is locked in the applications
   */
  void PeriodicUplinks (bool enable);
  
  /**
   * Set the period to be used by the applications created by this helper.
   *
   * A value of Seconds (0) results in randomly generated periods according to
   * the model contained in the TR 45.820 document.
   *
   * \param period The period to set
   */
  void SetSendingPeriod (Time period);

  void SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv);

  void SetPacketSize (uint8_t size);
  
  /**
   * To Enable decoding fragmented data reception
   * 
   * \param first the first sequence number of the fragments
   * \param last the last sequence number of the fragments, where the default is zero.
   * if zero or less than first the last fragment is max value of uint32_t which is 2^32
   *  
   */
  void EnableFragmentedDataReception (uint32_t first, uint32_t last=0);

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory;

  Ptr<UniformRandomVariable> m_sendingInitialDelay; //!< Both for switching to class B and for 
                                     //sending uplinks after switching

  Ptr<UniformRandomVariable> m_sendingIntervalProb;

  Time m_sendingPeriod; //!< The period with which the application will be set to send
                 // messages

  Ptr<RandomVariableStream> m_pktSizeRV; // whether or not a random component is added to the packet size

  uint8_t m_pktSize; // the packet size.
  
  uint8_t m_nAttempt; // Number of attempt to switch to Class B
  
  bool m_uplinkEnabled; //Whether uplinks are enabled in the class B end devices
  
  Time m_classBDelay; //!< Intial delay and a delay after after each lost beacon 
                  // before attempting to switch again to class B
  
  /**
   * For configuring fragmented data when it is enabled
   */
  bool m_fragmentEnable; ///< Whether it is enabled or not
  uint32_t m_fragment_first; ///< The first fragment sequence number
  uint32_t m_fragment_last; ///< The last fragment sequence number

};

} // namespace ns3

}
#endif /* PERIODIC_SENDER_HELPER_H */
