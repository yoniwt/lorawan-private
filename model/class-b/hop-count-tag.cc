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

#include "ns3/hop-count-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {
namespace lorawan {

NS_OBJECT_ENSURE_REGISTERED (HopCountTag);

TypeId
HopCountTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HopCountTag")
    .SetParent<Tag> ()
    .SetGroupName ("lorawan")
    .AddConstructor<HopCountTag> ()
  ;
  return tid;
}

TypeId
HopCountTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

HopCountTag::HopCountTag () :
  m_hopCount (0)
{
}

HopCountTag::~HopCountTag ()
{
}

uint32_t
HopCountTag::GetSerializedSize (void) const
{
  return 1;
}

void
HopCountTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_hopCount);
}

void
HopCountTag::Deserialize (TagBuffer i)
{
  m_hopCount = i.ReadU8 ();
}

void
HopCountTag::Print (std::ostream &os) const
{
  os << m_hopCount;
}

uint8_t
HopCountTag::GetHopCount () const
{
  return m_hopCount;
}

uint8_t 
HopCountTag::IncreamentHopCount ()
{
  m_hopCount++;
  return m_hopCount;
}
}
} // namespace ns3
