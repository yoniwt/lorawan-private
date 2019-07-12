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

#include "ns3/end-device-class-b-app-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "src/core/model/log-macros-enabled.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("EndDeviceClassBAppHelper");

EndDeviceClassBAppHelper::EndDeviceClassBAppHelper ()
{
  m_factory.SetTypeId ("ns3::EndDeviceClassBApp");

  // m_factory.Set ("PacketSizeRandomVariable", StringValue
  //                  ("ns3::ParetoRandomVariable[Bound=10|Shape=2.5]"));

  m_sendingInitialDelay = CreateObject<UniformRandomVariable> ();
  m_sendingInitialDelay->SetAttribute ("Min", DoubleValue (0));

  m_sendingIntervalProb = CreateObject<UniformRandomVariable> ();
  m_sendingIntervalProb->SetAttribute ("Min", DoubleValue (0));
  m_sendingIntervalProb->SetAttribute ("Max", DoubleValue (1));

  m_pktSize = 10;
  m_pktSizeRV = 0;
  m_nAttempt = 0;
  m_uplinkEnabled = false;
  m_classBDelay = Seconds (0);
  
  m_fragmentEnable = false;
  m_fragment_first = 0;
  m_fragment_last = 0;
}

EndDeviceClassBAppHelper::~EndDeviceClassBAppHelper ()
{
}

void
EndDeviceClassBAppHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
EndDeviceClassBAppHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EndDeviceClassBAppHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
EndDeviceClassBAppHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this << node);
  
  Ptr<EndDeviceClassBApp> app = m_factory.Create<EndDeviceClassBApp> ();
  NS_ASSERT (app != 0);
  app->SetNumberOfAttempt (m_nAttempt);
  
  if (m_classBDelay != Seconds (0))
    {
      app->SetSwitchToClassBDelay (m_classBDelay);
    }
  
  if (m_fragmentEnable)
    {
      app->EnableFragmentedDataReception (m_fragment_first, m_fragment_last);
    }
  
  
  if (m_uplinkEnabled)
    {
      Time interval;
      if (m_sendingPeriod == Seconds (0))
       {
          double intervalProb = m_sendingIntervalProb->GetValue ();
         NS_LOG_DEBUG ("IntervalProb = " << intervalProb);
         
          // Based on TR 45.820
          if (intervalProb < 0.4)
            {
              interval = Days (1);
            }
          else if (0.4 <= intervalProb  && intervalProb < 0.8)
            {
             interval = Hours (2);
            }
          else if (0.8 <= intervalProb  && intervalProb < 0.95)
           {
             interval = Hours (1);
           }
         else
           {
             interval = Minutes (30);
           }
       }
     else
       {
         interval = m_sendingPeriod;
       }
     
     app->EnablePeriodicUplinks (); 
     app->SetSendingInterval (interval);
     NS_LOG_DEBUG ("Created an application with interval = " <<
                    interval.GetHours () << " hours");

     app->SetSendingInitialDelay (Seconds (m_sendingInitialDelay->GetValue (0, interval.GetSeconds ())));
     app->SetPacketSize (m_pktSize);
     if (m_pktSizeRV)
       {
         app->SetPacketSizeRandomVariable (m_pktSizeRV);
       }    
    }


  app->SetNode (node);
  node->AddApplication (app);

  return app;
}

void
EndDeviceClassBAppHelper::SetNumberOfAttempt (uint8_t nAttempt)
{
  m_nAttempt = nAttempt;
}

void
EndDeviceClassBAppHelper::SetSwitchToClassBDelay (Time delay)
{
  m_classBDelay = delay;
}


void
EndDeviceClassBAppHelper::PeriodicUplinks (bool enable)
{
  m_uplinkEnabled = enable;
}


void
EndDeviceClassBAppHelper::SetSendingPeriod (Time period)
{
  m_sendingPeriod = period;
}

void
EndDeviceClassBAppHelper::SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv)
{
  m_pktSizeRV = rv;
}

void
EndDeviceClassBAppHelper::SetPacketSize (uint8_t size)
{
  m_pktSize = size;
}

void
EndDeviceClassBAppHelper::EnableFragmentedDataReception (uint32_t first, uint32_t last)
{
  m_fragmentEnable = true;
  m_fragment_first = first;
  m_fragment_last = last;
}


}
} // namespace ns3
