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

#include "ns3/bcn-payload.h"
#include "ns3/log.h"
#include "src/network/model/buffer.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include <bitset>

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("BcnPayload");

BcnPayload::BcnPayload () : m_bcnTime (0),
                            m_infoDesc (0),
                            m_latitude (0),
                            m_longitude (0),
                            m_info (0)
{
}

BcnPayload::~BcnPayload ()
{
}

TypeId
BcnPayload::GetTypeId (void)
{
  static TypeId tid = TypeId ("BcnPayload")
    .SetParent<Header> ()
    .AddConstructor<BcnPayload> ()
  ;
  return tid;
}

TypeId
BcnPayload::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
BcnPayload::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  
    //bcnTime is set 
  if (m_bcnTime == 0)
   {
     NS_LOG_DEBUG ("No beacon header attached to the packet");
     //Number of bytes consumed
     return 0;
   }

  return 17;       // BcnPayload is 17 byte for EU region
}

void
BcnPayload::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  if (m_bcnTime == 0)
    {
      NS_ASSERT_MSG(false,"bcnTime has to be set before serializing a beacon payload.");
      return;
    }
  
  //Beacon payload is made up of two parts each checked by crc
  //Part1 = RFU(2 byte) + Time(4 byte) + CRC (2 byte)
  //Part2 = GwSpecific+ CRC
  
  //Part 1 without crc
  uint8_t* part1 = new uint8_t[6];
  //RFU should be initialized to zero
  part1[0] = 0;
  part1[1] = 0;
  
  start.WriteU8 (part1[0]);
  start.WriteU8 (part1[1]);
  
  //Now serialize and write the bcnTime
  part1[2] = static_cast<uint8_t>((m_bcnTime & 0xff000000) >> 24);
  part1[3] = static_cast<uint8_t>((m_bcnTime & 0x00ff0000) >> 16); 
  part1[4] = static_cast<uint8_t>((m_bcnTime & 0x0000ff00) >> 8); 
  part1[5] = static_cast<uint8_t>(m_bcnTime & 0x000000ff); 
  
  start.WriteU8 (part1[2]);
  start.WriteU8 (part1[3]);
  start.WriteU8 (part1[4]);
  start.WriteU8 (part1[5]);
  
  //Calculate the crc of part1 and Write
  uint16_t crc1 = GenerateCrc16 (part1, 6);
  start.WriteU16 (crc1);
  
  NS_LOG_DEBUG ("Serialization of BcnTime = " << m_bcnTime << " and CRC1 = " << crc1 << " complete");
  
  //De-Allocate Part1
  delete [] part1;
  
  //Part2 InfoDesc+latitude+longitude+CRC2 ( or ) GwSpecific+CRC2
  //\TODO calculate CRC2
  
  // 1 byte InfoDesc
  uint8_t infoDesc = m_infoDesc;
  //\TODO to be used for the future 
  uint16_t crc2 = 0;
  
  start.WriteU8 (infoDesc);
  
  // the Info that goes with the InfoDesc and the 2nd CRC (crc2)
  uint64_t info = 0;
  
  if (infoDesc < 3)
  {
    // We need to move the 32bit types to 64 bit type in-order to do shift operation 
    uint64_t latitude = m_latitude;
    uint64_t longitude = m_longitude;
    
    info |= (latitude << (5*8)) & 0xffffff0000000000; 
    info |= (longitude << (2*8)) & 0x000000ffffff0000;
    info |= crc2;
    
    NS_LOG_DEBUG ("Serialization of Info for InfoDesc " << infoDesc << ": info = " << std::hex << info << " (in Hex)");
  }
  else if (infoDesc > 127)
  {
    info |= m_info << (2*8);
    //\TODO include CRC on the last 2 bytes
    info |= crc2;
    
    NS_LOG_DEBUG ("Serialization of Info for InfoDesc " << infoDesc << ": info = " << std::hex << info << " (in Hex)");
  }
  
  start.WriteU64 (info);
}

uint32_t
BcnPayload::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  //Beacon payload is made up of two parts each checked by CRC
  //Part1 = RFU(2 byte) + Time(4 byte) + CRC (2 byte)
  //Part2 = GwSpecific+ CRC
  
  //Part 1 : Read without CRC 
  uint8_t* part1 = new uint8_t[6];
  
  //RFU : Read
  part1[0] = start.ReadU8 ();
  part1[1] = start.ReadU8 ();
  
  //bcnTime : Read
  part1[2] = start.ReadU8 (); 
  part1[3] = start.ReadU8 ();
  part1[4] = start.ReadU8 ();
  part1[5] = start.ReadU8 ();
  
  //CRC1 : Compute
  uint16_t crc1_comp = GenerateCrc16 (part1, 6);  
  //CRC1: Read
  uint16_t crc1_read = start.ReadU16 ();
  
  if (crc1_comp != crc1_read)
    {
      NS_LOG_DEBUG ("Packet is either non-beacon or corrupted!");
      return 0;
    }
  
  // bcnTime: Reconstruct 
  m_bcnTime = static_cast<uint32_t>( 
                                     ((part1[2] << (8*3)) & 0xff000000) | 
                                     ((part1[3] << (8*2)) & 0x00ff0000) |
                                     ((part1[4] << (8*1)) & 0x0000ff00) |
                                     ((part1[5] << (8*0)) & 0x000000ff)  
                                   );
  
  NS_LOG_DEBUG ("BcnTime = " << Seconds (m_bcnTime));//.GetSeconds () << " Seconds"); 
  
  //De-Allocate Part1
  delete [] part1;
  
  if (Simulator::Now ().GetSeconds () < m_bcnTime)
    {
      //The CRC just accidentally matched while the packet does not actually contain a beacon Time stamp
      NS_LOG_DEBUG ("Beacon received can't exceed simulator time!");
      NS_LOG_DEBUG ("The time stamp for the beacon is invalid, the crc just matched accidentally. Discard packet!");
      m_bcnTime = 0;
      return 0;
    }
  
  // Part2 InfoDesc+latitude+longitude+CRC2 ( or ) GwSpecific+CRC2
  // \TODO calculate CRC2 and if it fails discard m_bcnTime also to be sure 
  // that you are getting the right packet for you. If another packet has 6 bytes 
  // with CRC in the initial you would otherwise take it as if it is a beacon packet
  // This will result in wrong calculation of ping slots.
  
  // 1 byte InfoDesc
  m_infoDesc = start.ReadU8 ();
  
  NS_LOG_DEBUG ("InfoDesc = " << (int) m_infoDesc);
  
  // Info plus CRC2
  m_info = start.ReadU64 ();
   
  if (m_infoDesc < 3)
  {
    m_latitude = (m_info >> (5*8)) & 0x00ffffff; 
    m_longitude = (m_info >> (2*8)) & 0x00ffffff;    
    
    NS_LOG_DEBUG ("Latitude = " << m_latitude);
    NS_LOG_DEBUG ("Longitude = " << m_longitude);
  }
  
  //\TODO define m_crc2 |= m_info and check the CRC
  
  //extract the Info part
  m_info = (m_info >> (2*8))& 0x0000ffffffffffff;
    
  NS_LOG_DEBUG ("Info = " << m_info );

  return 17;       // the number of bytes consumed. 17 byte for EU region
}

void
BcnPayload::Print (std::ostream &os) const
{
  os << "BcnTime = " << m_bcnTime << std::endl;
  os << "InfoDesc = " << m_infoDesc << std::endl;
  
  if (m_infoDesc < 3)
  {
    os << "Latitude = " << m_latitude << std::endl;
    os << "Longitude = " << m_longitude << std::endl;
  }
  else if (m_infoDesc > 127)
  {
    os << "Info = " << m_info << std::endl;
  }
}

void
BcnPayload::SetBcnTime(uint32_t bcnTime)
{
  NS_LOG_FUNCTION (this << bcnTime);
  
  m_bcnTime = bcnTime;
}

uint32_t
BcnPayload::GetBcnTime() const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  return m_bcnTime;
}

void
BcnPayload::SetInfo(uint64_t info)
{
  NS_LOG_FUNCTION (this << info);
  
  NS_ASSERT_MSG (info < 281474976710656, "Info field of GwSpecific in bcnPayload can't exceed 6 bytes");
  
  m_info = info;
  
  if (m_infoDesc < 3)
  {
    m_latitude = (m_info >> (3*8)) & 0x00ffffff; 
    m_longitude = m_info & 0x00ffffff; 
  }
  else if (m_infoDesc < 128 && m_infoDesc > 2)
  {
    NS_LOG_ERROR ("Invalid Info Desc!");
  }
  
}

uint64_t
BcnPayload::GetInfo() const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  return m_info;
}

void
BcnPayload::SetInfoDesc(uint8_t infoDesc)
{
  NS_LOG_FUNCTION (this << infoDesc);
  
  if (m_infoDesc < 128 && m_infoDesc > 2)
  {
    NS_LOG_ERROR ("Invalid Info Desc!  InfoDesc can't be between 3 and 127 inclusive ");
  }
  
  m_infoDesc = infoDesc;
}

uint8_t
BcnPayload::GetInfoDesc() const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  return m_infoDesc;
}

void
BcnPayload::SetLatitude(uint32_t latitude)
{
  NS_LOG_FUNCTION (this << latitude);
  
  NS_ASSERT_MSG (latitude < 16777216, "latitude can't exceed 3 bytes!");
  
  m_latitude = latitude;
  
  if  (m_infoDesc < 127)
  {
    m_info &= 0x000000ffffff;
    m_info |= (latitude << (3*8)); 
  }
  else if (m_infoDesc < 128 && m_infoDesc > 2)
  {
    NS_LOG_ERROR ("Invalid Info Desc!");
  }
}

uint32_t
BcnPayload::GetLatitude() const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  return m_latitude;
}

void
BcnPayload::SetLongitude (uint32_t longitude)
{
  NS_LOG_FUNCTION (this << longitude);
  
  NS_ASSERT_MSG (longitude < 16777216, "latitude can't exceed 3 bytes!");
  
  m_longitude = longitude;
  
  if  (m_infoDesc < 127)
  {
    m_info &= 0xffffff000000;
    m_info |= longitude; 
  }
  else if (m_infoDesc < 128 && m_infoDesc > 2)
  {
    NS_LOG_ERROR ("Invalid Info Desc!");
  }
}

uint32_t
BcnPayload::GetLongitude (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  return m_longitude;
}

uint16_t
BcnPayload::GenerateCrc16 (uint8_t* data, int length) const
{
  int i;
  uint16_t accumulator = 0;
   for (i = 0; i < length; ++i)
    {
      accumulator ^= *data;
      accumulator  = (accumulator >> 8) | (accumulator << 8);
      accumulator ^= (accumulator & 0xff00) << 4;
      accumulator ^= (accumulator >> 8) >> 4;
      accumulator ^= (accumulator & 0xff00) >> 5;
      ++data;
    }
  return accumulator;  
}

}
}
