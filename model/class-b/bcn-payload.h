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

#ifndef BCN_PAYLOAD_H
#define BCN_PAYLOAD_H

#include "ns3/header.h"

namespace ns3 {
namespace lorawan {

/**
 * This class represents the Mac header of a LoRaWAN packet.
 */
class BcnPayload : public Header
{
public:

  static TypeId GetTypeId (void);

  BcnPayload ();
  ~BcnPayload ();

  // Pure virtual methods from Header that need to be implemented by this class
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * Serialize the header.
   *
   * See Page 15 of LoRaWAN specification for a representation of fields.
   *
   * \param start A pointer to the buffer that will be filled with the
   * serialization.
   */
  virtual void Serialize (Buffer::Iterator start) const;

  /**
   * Deserialize the header.
   *
   * \param start A pointer to the buffer we need to deserialize.
   * \return The number of consumed bytes.
   */
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Print the header in a human readable format.
   *
   * \param os The std::ostream on which to print the header.
   */
  virtual void Print (std::ostream &os) const;

  /**
   * Set the time the beacon is sent from the gateway
   *
   * \param bcnTime The the beacon is sent
   */
  void SetBcnTime (uint32_t bcnTime);

  /**
   * Get the time the beacon was sent from the gateway.
   *
   * \return The uint32_t corresponding to the time the beacon was sent
   */
  uint32_t GetBcnTime (void) const;

  /**
   * Set the InfoDesc (Information Descriptor) which describes how the information
   * field Info shall be interpreted. 
   *
   * \param infoDesc if 0 it means the first antenna location, if 1 the second 
   * antenna location, and if 2 it means the third antenna location.
   * Number from 3-127 are RFU and number from 128-255 are used for custom 
   * network specific broadcasts. 
   */
  void SetInfoDesc (uint8_t infoDesc);

  /**
   * Get the InfoDesc (Information Descriptor) which describes how the information
   * field Info shall be interpreted. 
   *
   * \return uint8_t if 0 it means the first antenna location, if 1 the second 
   * antenna location, and if 2 it means the third antenna location.
   * Number from 3-127 are RFU and number from 128-255 are used for custom 
   * network specific broadcasts. 
   */
  uint8_t GetInfoDesc (void) const;

  /**
   * Set the latitude of the Antenna if InfoDesc is [0..2]
   *
   * \param latitude Latitude of the Antenna indicated in InfoDesc
   */
  void SetLatitude (uint32_t latitude);

  /**
   * Get the latitude of the Antenna if InfoDesc is [0..2]
   *
   * \return uint32_t Latitude of the Antenna indicated in InfoDesc
   */
  uint32_t GetLatitude (void) const;
  
  /**
   * Set the longitude of the Antenna if InfoDesc is [0..2]
   *
   * \param longitude Longitude of the Antenna indicated in InfoDesc
   */
  void SetLongitude (uint32_t longitude);

  /**
   * Get the longitude of the Antenna if InfoDesc is [0..2]
   *
   * \return uint32_t Longitude of the Antenna indicated in InfoDesc
   */
  uint32_t GetLongitude (void) const;
  
  /**
   * Set the info for InfoDesc that lies in between [128..255]
   *
   * \param info Information that matches the InfoDesc
   */
  void SetInfo (uint64_t info);

  /**
   * Get the info for InfoDesc that lies in between [128..255]
   *
   * \return uint32_t Information that matches the InfoDesc
   */
  uint64_t GetInfo (void) const;

private:
  /**
   * For CRC calculation
   * 
   * Taken form the lr-wpan-mac-trailer in ns3
   * 
   *\param data the stream of data you want generate CRC for 
   *\param length how many bytes is the data stream you will generate CRC for
   *\return the computed CRC
   */
  uint16_t GenerateCrc16 (uint8_t *data, int length) const;
  
  /**
   * the bcnTime part of the bcnPayload
   */
  uint32_t m_bcnTime;
    
  /**
   * the InfoDesc part of the GwSpecific info
   */
  uint8_t m_infoDesc;
  
  /**
   * the latitude to be included in the payload if InfoDesc is [0..2].
   * (Only 3 bytes will be used from the 4 byte Set)
   */
  uint32_t m_latitude;
    
  /**
   * the longitude to be included in the payload if InfoDesc is [0..2].
   * (Only 3 bytes will be used from the 4 byte Set)
   */
  uint32_t m_longitude;
    
  /**
   * the information to be included in the payload if InfoDesc is [128..255].
   * (Only 6 bytes will be used from the 8 byte Set)
   */  
  uint64_t m_info;  
};
}

}
#endif
