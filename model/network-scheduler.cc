#include "network-scheduler.h"
#include "src/core/model/log-macros-enabled.h"
#include "ns3/aes.h"
#include "ns3/hop-count-tag.h"
#include "src/core/model/assert.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("NetworkScheduler");

NS_OBJECT_ENSURE_REGISTERED (NetworkScheduler);

TypeId
NetworkScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetworkScheduler")
    .SetParent<Object> ()
    .AddConstructor<NetworkScheduler> ()
    .AddTraceSource ("ReceiveWindowOpened",
                     "Trace source that is fired when a receive window opportunity happens.",
                     MakeTraceSourceAccessor (&NetworkScheduler::m_receiveWindowOpened),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TotalBeaconsBroadcasted",
                     "The number of beacons broadcasted at least by one gateways",
                     MakeTraceSourceAccessor
                       (&NetworkScheduler::m_totalBeaconsBroadcasted),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("TotalBeaconsBlocked",
                     "The number of beacons that are not broadcasted by all gateways at all",
                     MakeTraceSourceAccessor
                       (&NetworkScheduler::m_totalBeaconsBlocked),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("McPingSent",
                     "The last multicast sent via ping slot",
                     MakeTraceSourceAccessor
                       (&NetworkScheduler::m_mcPingSent),
                     "ns3::NetworkScheduler::McPingSentCallback")  
    .AddTraceSource ("UcPingSent",
                     "The last unicast sent via ping slot",
                     MakeTraceSourceAccessor
                       (&NetworkScheduler::m_ucPingSent),
                     "ns3::NetworkScheduler::UcPingSentCallback")  
    .AddTraceSource ("TotalBytesSent",
                     "The number of bytes sent from the network server",
                     MakeTraceSourceAccessor
                       (&NetworkScheduler::m_totalByteSent),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("BeaconStatusCallback",
                     "Shows the continuity of the missed or sent beacons", 
                     MakeTraceSourceAccessor 
                       (&NetworkScheduler::m_beaconStatusCallback),
                     "ns3::NetworkScheduler::BeaconStatusCallback")
    .AddAttribute ("PingDownlinkPacketSize",
                   "The packet size for the ping downlink. If 0, a random size"
                    "will be used and if greater than what is supported by the" 
                    "data rate to 255 the maximum data rate will be used ",
                    UintegerValue(255),
                    MakeIntegerAccessor(&NetworkScheduler::m_pingDownlinkPacketSize),
                    MakeUintegerChecker<uint8_t>()                  
                  )
    .SetGroupName ("lorawan");
  return tid;
}

NetworkScheduler::NetworkScheduler () :
  m_beaconBroadcastEnabled (false),
  m_totalBeaconsBroadcasted (0),
  m_totalBeaconsBlocked (0),
  m_maxAppPayloadForDataRate {51,51,51,115,222,222,222,222},  //Max MacPayload for EU863-870, taking FOpt to be empty
  m_enableSequencedPacketGeneration (false),
  m_totalByteSent (0),
  m_beaconStatus (NetworkScheduler::BeaconStatus()),
  m_beaconRelatedConstants (NetworkScheduler::BeaconRelatedConstants())
{
  m_randomPacketSize = CreateObject<UniformRandomVariable> ();
}

NetworkScheduler::NetworkScheduler (Ptr<NetworkStatus> status,
                                    Ptr<NetworkController> controller) :
  m_status (status),
  m_controller (controller),
  m_beaconBroadcastEnabled (false),
  m_totalBeaconsBroadcasted (0),
  m_totalBeaconsBlocked (0),
  m_maxAppPayloadForDataRate {51,51,51,115,222,222,222,222}, //Max AppPayload for EU863-870, taking FOpt to be empty
  m_enableSequencedPacketGeneration (false),
  m_totalByteSent (0),
  m_beaconStatus (NetworkScheduler::BeaconStatus()),
  m_beaconRelatedConstants (NetworkScheduler::BeaconRelatedConstants())    
{
  m_randomPacketSize = CreateObject<UniformRandomVariable> ();
}

NetworkScheduler::~NetworkScheduler ()
{
}

void
NetworkScheduler::OnReceivedPacket (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (packet);

  // Create a copy of the packet
  Ptr<Packet> myPacket = packet->Copy ();

  // TODO Check if this packet is a duplicate:
  // It's possible that we already received the same packet from another
  // gateway.
  // - Extract the address
  LoraMacHeader macHeader;
  LoraFrameHeader frameHeader;
  myPacket->RemoveHeader (macHeader);
  myPacket->RemoveHeader (frameHeader);
  LoraDeviceAddress deviceAddress = frameHeader.GetAddress ();

  // Schedule OnReceiveWindowOpportunity event
  Simulator::Schedule (Seconds (1),
                       &NetworkScheduler::OnReceiveWindowOpportunity,
                       this,
                       deviceAddress,
                       1);     // This will be the first receive window
}

void
NetworkScheduler::OnReceiveWindowOpportunity (LoraDeviceAddress deviceAddress, int window)
{
  NS_LOG_FUNCTION (deviceAddress);

  NS_LOG_DEBUG ("Opening receive window number " << window << " for device "
                                                 << deviceAddress);

  // Check whether we can send a reply to the device, again by using
  // NetworkStatus
  Address gwAddress = m_status->GetBestGatewayForDevice (deviceAddress);

  NS_LOG_DEBUG ("Found available gateway with address: " << gwAddress);

  if (gwAddress == Address () && window == 1)
    {
      // No suitable GW was found
      // Schedule OnReceiveWindowOpportunity event
      Simulator::Schedule (Seconds (1),
                           &NetworkScheduler::OnReceiveWindowOpportunity,
                           this,
                           deviceAddress,
                           2);     // This will be the second receive window
    }
  else if (gwAddress == Address () && window == 2)
    {
      // No suitable GW was found
      // Simply give up.
      NS_LOG_INFO ("Giving up on reply: no suitable gateway was found " <<
                   "on the second receive window");

      // Reset the reply
      // XXX Should we reset it here or keep it for the next opportunity?
      m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
    }
  else
    {
      // A gateway was found
      m_controller->BeforeSendingReply (m_status->GetEndDeviceStatus
                                          (deviceAddress));

      // Check whether this device needs a response by querying m_status
      bool needsReply = m_status->NeedsReply (deviceAddress);

      if (needsReply)
        {
          NS_LOG_INFO ("A reply is needed");

          // Send the reply through that gateway
          m_status->SendThroughGateway (m_status->GetReplyForDevice
                                          (deviceAddress, window),
                                        gwAddress);

          // Reset the reply
          m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
        }
    }
}

void
NetworkScheduler::BroadcastBeacon (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  //Time minimalBeaconLessOperationTime = Minutes (128); ///< minimal beacon less operation time, default 128 minutes 

  Time beaconReserved = Seconds (2.12); ///< Beacon_reserved, default 2.12 Seconds
  //Time beaconGuard = Seconds (3.0); ///< Beacon_guard, default 3.0 Seconds
  //Time beaconWindow = Seconds (122.88); ///< Beacon_window, default 122.88 Seconds  
  
  // k is the smallest integer for which k * 128 > T, where 
  // T = seconds since 00:00:00, Sunday 5th of January 1980 (start of the GPS time)
  // which is the Simulator::Now in this ns3 simulation */
  if (m_beaconBroadcastEnabled == false && enable == true)
    {
      m_beaconBroadcastEnabled = true;
      
      Time tBeaconDelay = Seconds (0.015); ///< TBeaconDelay = 0.015 Seconds, not including +/-1uSec jitter
        
      int k = 0; 
  
      while (Simulator::Now () >=  Seconds (k*128))
      {
        k++;
      }
      
      Time bT = Seconds (k*128) + tBeaconDelay; 
      
      Simulator::Schedule (bT, 
                           &NetworkScheduler::BroadcastBeacon, 
                           this,
                           true);
    }
  else if (m_beaconBroadcastEnabled == true && enable == true)
    {
     
      NS_LOG_DEBUG ("BroadcastBeacon at " << Simulator::Now ().GetSeconds ());
      
      uint32_t bcnTime = m_status->BroadcastBeacon ();
     
      //bcnTime is zero if no gateway transmitted beacon and it will contain 
      //the time stamp if at least one gateway transmitted beacon
      if (bcnTime == 0) 
        {
        
          NS_LOG_DEBUG ("Broadcast failed!");
          
          m_lastBeaconTime += 128;
          m_totalBeaconsBlocked++;
          
          //If the NS previously sent a beacon 
          //fire trace source and update the state of beacon
          //Otherwise increment the number of beacons skipped continuously 
          if (m_beaconStatus.isSent) 
            {
              NS_LOG_DEBUG ("Start of continuous beacon skipping");
              
              m_beaconStatusCallback (m_beaconStatus.isSent, m_beaconStatus.continuousCount);
              m_beaconStatus.isSent = false; 
              m_beaconStatus.continuousCount = 1;
            }
          else
            {
              m_beaconStatus.continuousCount++;
              
              //Stop LoRaWAN Class B operation as we have skipped beacons for 
              //more than minimal beaconless operation mode
              if ((m_beaconStatus.continuousCount+1) > 
                   m_beaconRelatedConstants.minimalBeaconLessOperationMode.GetSeconds ()/
                   m_beaconRelatedConstants.beaconWindow.GetSeconds ())
                {
                  //Fire tracesource before exiting
                  m_beaconStatusCallback (m_beaconStatus.isSent, m_beaconStatus.continuousCount);
                  //Call this function with disabling class B so that we can immediately stop downlink
                }
            }
        }
      else 
        {
          m_lastBeaconTime = bcnTime;
          m_totalBeaconsBroadcasted++;
         
          //If the NS previously was not sent
          //fire trace source and update the state of beacon
          //Otherwise increment the number of beacons transmitted continuously           
          if (!m_beaconStatus.isSent)
            {
              NS_LOG_DEBUG ("Start of a continuous beacon sending");
              
              m_beaconStatusCallback (m_beaconStatus.isSent, m_beaconStatus.continuousCount);
              m_beaconStatus.isSent = true;
              m_beaconStatus.continuousCount = 1;
            }
          else
            {
              m_beaconStatus.continuousCount++;
            }
        }
      
      Time beaconPeriod = Seconds (128); ///< Beacon_period, default 128 Seconds
      
      Simulator::Schedule (beaconPeriod, 
                           &NetworkScheduler::BroadcastBeacon, 
                           this,
                           true);

      // Start schedule downlink at the end of beacon reserved
      // Only schedule if you have not skipped sending beacon beyond
      // beaconless operation mode
      if (m_beaconStatus.isSent ||((m_beaconStatus.continuousCount+1) < 
           m_beaconRelatedConstants.minimalBeaconLessOperationMode.GetSeconds ()/
           m_beaconRelatedConstants.beaconWindow.GetSeconds ()))
        {
          Simulator::Schedule (beaconReserved, 
                       &NetworkScheduler::ScheduleClassBDownlink,
                       this,
                       m_lastBeaconTime);       
        }
      
    }
  else if (enable == false)
    {
      m_beaconBroadcastEnabled = false;
    }
  
  // The time when beacon is sent
  

  //Here also we need to make a decision to schedule beacon if class A has been 
  // until now taking priority
}

void
NetworkScheduler::ScheduleClassBDownlink (uint32_t bcnTime)
{
  NS_LOG_FUNCTION (this << bcnTime);
  
  //Fire tracesource at the beginning beacon period along whether the immediate beacon is sent or not
  
  
  //Schedule the first ping of the beacon period 
  
  // For now Send packet as soon as always there is an opportunity, so that we can 
  // measure maximum throughput (limited by duty cycle)
  
  // Duration of a slot from the LoRaWAN Specification.
  Time slotLen = Seconds (0.03);
    
  for (auto it = m_status->m_mcEndDeviceStatuses.begin (); it != m_status->m_mcEndDeviceStatuses.end (); ++it)
    {
      LoraDeviceAddress address = it->first;
      uint pingPeriod = (it)->second.begin ()->second->GetMac ()->GetPingPeriod ();
      uint pingNb = (it)->second.begin ()->second->GetMac ()->GetPingNb ();
      uint8_t dataRate = (it)->second.begin ()->second->GetMac ()->GetPingSlotReceiveWindowDataRate ();
      
      //We assume all the multicast devices are configured with the same parameter prior to enabling them 
      //as multicast devices.
      //Therefore, we can take the parameter of one of the devices in the multicast for transmission 
      //to the whole multicast
      uint64_t offset = GetPingOffset (bcnTime, address, pingPeriod); 
      
      //\TODO Vary number of byte per packet
      //check also whether payload size varying could be helpful and what is the optimum. 
      //check also with different number of transmission length (fragmented data length). 
      //check also whether periodic downlink helps
      //Ptr<Packet> downlinkPacket = Create<Packet> (variablePacketLength); 
      
      //Create a downlink packet that corresponds to the maximum packetSize for now
      //Create a sequenced downlink for now
      
      if (m_downlinkPacket.find (address) == m_downlinkPacket.end ())
        {
        
          uint8_t sizeOfAppPayload; 
          uint8_t maxAppPayloadForDataRate = m_maxAppPayloadForDataRate.at (dataRate);
          
          if (m_pingDownlinkPacketSize == 0)
            {
              // Generate a random packet size from 1 to the maximum size possible
              sizeOfAppPayload = m_randomPacketSize->GetInteger (1, (uint32_t)maxAppPayloadForDataRate);
            }
          else if (m_pingDownlinkPacketSize <= 255 && m_pingDownlinkPacketSize > maxAppPayloadForDataRate)
            {
              //Set to maximum size
              sizeOfAppPayload = m_maxAppPayloadForDataRate.at (dataRate);
            }
          else if (m_pingDownlinkPacketSize > 0 && m_pingDownlinkPacketSize < maxAppPayloadForDataRate)
            {
              //Use the packet size defined as the packet size
              sizeOfAppPayload = m_pingDownlinkPacketSize;
            }
          else 
            {
              NS_ASSERT_MSG (false, "Invalid pingDownlinkPacketSize");
            }
          
          NS_LOG_DEBUG ("Ping Downlink Packet Size to be used for multicast group address " << address << " is " << (int)sizeOfAppPayload);
            
          enum DownlinkType downlinkType = m_enableSequencedPacketGeneration ? DownlinkType::SEQUENCED : DownlinkType::EMPTY;
          Ptr<DownlinkPacketGenerator> downlinkPacket = Create<DownlinkPacketGenerator> (downlinkType, 
                                                                                         sizeOfAppPayload, 
                                                                                         0); //Start from sequence 0
          //No packet generator yet for the device address so add
          m_downlinkPacket.insert (std::pair<LoraDeviceAddress, Ptr<DownlinkPacketGenerator> > (address, downlinkPacket));
        }
      //downlinkPacket generator already is included if the address is found
      
      Simulator::Schedule (offset*slotLen,
                           &NetworkScheduler::SendPingDownlink,
                           this, 
                           address,
                           true,
                           pingPeriod,
                           pingNb, //Pass the pingNb = 4096/pingPeriod in order to avoid multiple division in each slot
                           0); //The first slotIndex as this is exactly after beacon reserved
      
      // Reschedule for the next periods without offset until next period 
      // To do this may be have another function that could be invoked periodicaly for each multicast address
    }
  
}

uint64_t
NetworkScheduler::GetPingOffset (uint32_t bcnTime, LoraDeviceAddress address, uint pingPeriod)
{
  NS_LOG_FUNCTION_NOARGS (); 
  
  //Key = 16 times 0x00 (4x4 block)
  uint8_t key[16] = {0}; //  AES encryption with a fixed key of all zeros is used to randomize
  
  //Rand is also a 4x4 block (16 byte)
  uint8_t rand[16] = {0};
   
  //Serialize the beaconPayload
  uint32_t bcnPayload = bcnTime;
  uint8_t *beaconTime = (uint8_t *)&bcnPayload;
  rand[0] = beaconTime[0];
  rand[1] = beaconTime[1];
  rand[2] = beaconTime[2];
  rand[3] = beaconTime[3];
  
  // Serialize the devAddr
  // Serialize method doesn't have any side effect so could be directly used
  uint8_t devAddr[4] = {0};
  address.Serialize (devAddr);
    
  rand[4] = devAddr[0];
  rand[5] = devAddr[1];
  rand[6] = devAddr[2];
  rand[7] = devAddr[3];

  //Rand = aes128_encrypt (key, beaconTime(4byte)|DevAddr(4byte)|Pad16)
  AES aes;
  aes.SetKey(key, 16);
  aes.Encrypt(rand, 16);
  
  // pingOffset = (Rand[0] + Rand[1]*256) modulo pingPeriod
  uint64_t pingOffset = (rand[0] + rand[1]*256)  % pingPeriod;
  // m_pingSlotInfo.pingOffset = m_pingSlotInfo.pingPeriod-1; //Use this one to test for the max pingOffset
  
  return pingOffset;
}

void
NetworkScheduler::SendPingDownlink (LoraDeviceAddress address, bool isMulticast, uint pingPeriod, uint8_t pingNb, uint8_t slotIndex)
{
  NS_LOG_FUNCTION (this << address << isMulticast << pingPeriod << pingNb << slotIndex);
  
  NS_ASSERT_MSG (m_downlinkPacket.find (address) != m_downlinkPacket.end (), "DownlinkPacketGenerator is not included for this devAddress");
  
  //\TODO If there is conflict with class A donwlink or reply, resolve here (Give priority for class A)  
  
  // Resends on the next slot as far as there is gateway remaining and on ping periodicity
  Ptr<Packet> downlinkPacket = m_downlinkPacket.find (address)-> second->GetPacket ();
  
  //If coordinatedRelaying is enabled
  HopCountTag hopCountTag;
  hopCountTag.IncreamentHopCount ();
  downlinkPacket->AddPacketTag (hopCountTag);
  
  if (isMulticast)
    {
      //\TODO fire traces for the number of successful transmission. If one gateway
      // is involved in the tranmission it will still tell you by receiving them with sink  
      uint8_t successfulGateways = m_status->MulticastPacket (downlinkPacket, address);
      
      NS_LOG_DEBUG ("Multicast Packet sent on " << (int)successfulGateways << " Gateways");
      NS_LOG_DEBUG ("Multicast Packet sent to " << address.Print ());
      
      //To generate next packet, condition for now is: 
      //If at list one gateway sent the donwlink we don't need to repeat the transmission
      if (successfulGateways > 0)
        { 
          Time now = Simulator::Now ();
         
          //Information on the downlink packet sent
          bool isSequencialPacket = (m_downlinkPacket.find (address)->second->m_downlinkType == DownlinkType::SEQUENCED);
          uint32_t packetSequenceNumber = isSequencialPacket ? m_downlinkPacket.find (address)->second->m_sequence : 0;
          
          //Update the packet generator
          NS_LOG_DEBUG ("Packet Sequence Sent " << m_downlinkPacket.find (address)->second->m_sequence);
          m_downlinkPacket.find (address)->second->PacketSent (true);
          
          //Fire tracesource of the sent multicast packet
          m_mcPingSent(address, successfulGateways, pingNb, slotIndex, now, downlinkPacket, isSequencialPacket, packetSequenceNumber);
          
          //std::cerr << "Time now " << now.GetMilliSeconds () << std::endl;
          //std::cerr << "Address: " <<  address.Print () << std::endl;
        }
    }
  else
    {
    //\TODO Revise this part and move most of this functionalities to network-status
    //\TODO If uplink has never been made, you should use the gateway that is already preassigned for a gateway
    //\TODO assignement of gw to end devices could be done by taking the shortest gateway to each end device, in the LoRaHelper with same way the 
    //spreading factors are assigned. 
    
      // Send the reply through that gateway
    
      Address gwAddress = m_status->GetBestGatewayForDevice (address);
      
      NS_ASSERT_MSG (m_status->m_gatewayStatuses.find (gwAddress) != m_status->m_gatewayStatuses.end (), "Best Gateway Selected Not found!");
      
      //Check if the gateway is available for transmission
      Ptr<GatewayStatus> gwStatus = m_status->m_gatewayStatuses.at (gwAddress);
     
      Ptr<GatewayLoraMac> gwLoraMac =  gwStatus->GetGatewayMac ();
      
      //Check if the gateway is class B enabled and available for transmission
      if (gwLoraMac->IsClassBTransmissionEnabled () && 
          gwStatus->IsAvailableForTransmission ( m_status-> GetEndDeviceStatus (address)-> GetMac ()->GetPingSlotRecieveWindowFrequency ()))
        {
          //Reserve the Gateway for transmission
          gwStatus->SetNextTransmissionTime (Simulator::Now ());
          
          //\TODO Packet header has to be added here as the following method do no do that
          //Prepare header and send packet
          LoraFrameHeader frameHeader;
          frameHeader.SetAsDownlink ();
          //frameHeader.SetAck ()
          frameHeader.SetAddress (address);
          //frameHeader.SetFPending ()
          //frameHeader.SetFCnt ()
          //frameHeader.SetFPort () //this has specific port for fragmented data downlink
            
           LoraMacHeader macHeader;
           macHeader.SetMType (LoraMacHeader::UNCONFIRMED_DATA_DOWN); //\TODO allow also a confirmed downlink //\TODO modifiy also the EndDeviceLoraMac to respond to confirmed ones in the ping
           
           // copy the packet before adding MAC headers
           Ptr<Packet> macPacket = downlinkPacket->Copy ();
           
           macPacket->AddHeader (frameHeader);
           macPacket->AddHeader (macHeader);
           
           // Apply the appropriate tag
           LoraTag tag;
           tag.SetFrequency (m_status->GetEndDeviceStatus (address)->GetMac ()->GetPingSlotRecieveWindowFrequency ());
           tag.SetDataRate (m_status->GetEndDeviceStatus (address)->GetMac ()->GetPingSlotReceiveWindowDataRate ());
           
           macPacket->AddPacketTag (tag);
           m_status->SendThroughGateway (macPacket,
                                         gwAddress);
           
           NS_LOG_DEBUG ("Unicast Packet Sent to " << address);
           
           // Information on the downlink packet sent
           Time now = Simulator::Now ();
           bool isSequencialPacket = (m_downlinkPacket.find (address)->second->m_downlinkType == DownlinkType::SEQUENCED);
           uint32_t packetSequenceNumber = isSequencialPacket ? m_downlinkPacket.find (address)->second->m_sequence : 0; 
           
           //If packet is successfully sent then update the packet generator
           NS_LOG_DEBUG ("Packet Sequence Sent " << m_downlinkPacket.find (address)->second->m_sequence);
           m_downlinkPacket.find (address)->second->PacketSent (true);
                    
           //Fire tracesource of the sent unicast packet with out the mac header
           m_ucPingSent(address, pingNb, slotIndex, now, downlinkPacket, isSequencialPacket, packetSequenceNumber);
        }
      else 
        {
          NS_LOG_DEBUG ("Unicast Packet Not Sent to " << address);
        }
    }
  
  //Schedule another ping if there is still another slot remaining in the beacon period
  if (++slotIndex < pingNb) 
    {
      Time slotLen = Seconds (0.03);
      Simulator::Schedule (pingPeriod*slotLen,
                           &NetworkScheduler::SendPingDownlink,
                           this, 
                           address,
                           isMulticast,
                           pingPeriod,
                           pingNb, //Pass the pingNb = 4096/pingPeriod in order to avoid multiple division in each slot
                           slotIndex); //The first slotIndex as this is exactly after beacon reserved      
    }
  
}

void
NetworkScheduler::SetMaxAppPayloadForDataRate (std::vector<uint32_t> maxAppPayloadForDataRate)
{
  m_maxAppPayloadForDataRate = maxAppPayloadForDataRate;
}

void
NetworkScheduler::EnableSequencedPacketGeneration (bool enable)
{
  m_enableSequencedPacketGeneration = enable;
}

void
NetworkScheduler::SetPingDownlinkPacketSize (uint8_t pingDownlinkPacketSize)
{
  NS_LOG_FUNCTION (this << pingDownlinkPacketSize);
  
  m_pingDownlinkPacketSize = pingDownlinkPacketSize;
}

uint8_t
NetworkScheduler::GetPingDownlinkPacketSize () const
{
  NS_LOG_FUNCTION (this);
  
  return m_pingDownlinkPacketSize;
}

}
}
