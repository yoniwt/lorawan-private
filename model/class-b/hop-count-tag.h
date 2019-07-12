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

#ifndef HOP_COUNT_TAG_H
#define HOP_COUNT_TAG_H

#include "ns3/tag.h"

namespace ns3 {
namespace lorawan {

/**
 * Tag used to save various data about a packet, like its Spreading Factor and
 * data about interference.
 */
class HopCountTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  HopCountTag ();
  
  virtual ~HopCountTag ();

  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;

  /**
   * Read the number of hops for this packet 
   *
   * If it is zero, it means this tag is generated here and 
   * therefore did not have any hop yet.
   *
   * \return number of hops the packet has traversed
   */
  uint8_t GetHopCount () const;
  
  /**
   * Increament the hop count of the packet
   *
   * hop is increamented before transmitting so that when 
   * the receiver receives it gets the hop count just by reading
   * GetHopCout (). 
   *
   * \return the increamented packet hope count
   */
  uint8_t IncreamentHopCount ();
  
private:
  uint8_t m_hopCount; //!< How many hopes has the packet traversed
};
} // namespace ns3
}
#endif
