/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
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
 * Author: Mathieu Lacage, <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/object-factory.h"
#include "yans-wifi-channel.h"
#include "yans-wifi-phy.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"

// 3rd party headers
#include "Modules/Toolbox/FileManager.h"

// 3rd party namespaces
using namespace Toolbox;
using namespace SdnLab;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("YansWifiChannel");

NS_OBJECT_ENSURE_REGISTERED (YansWifiChannel);

TypeId
YansWifiChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::YansWifiChannel")
    .SetParent<WifiChannel> ()
    .SetGroupName ("Wifi")
    .AddConstructor<YansWifiChannel> ()
    .AddAttribute ("PropagationLossModel", "A pointer to the propagation loss model attached to this channel.",
                   PointerValue (),
                   MakePointerAccessor (&YansWifiChannel::m_loss),
                   MakePointerChecker<PropagationLossModel> ())
    .AddAttribute ("PropagationDelayModel", "A pointer to the propagation delay model attached to this channel.",
                   PointerValue (),
                   MakePointerAccessor (&YansWifiChannel::m_delay),
                   MakePointerChecker<PropagationDelayModel> ())
	.AddTraceSource("Transmission", "Fired when something is transmitted on the channel",
				   MakeTraceSourceAccessor(&YansWifiChannel::m_channelTransmission), "ns3::YansWifiChannel::TransmissionCallback")
  ;
  return tid;
}

YansWifiChannel::YansWifiChannel ()
{
}

YansWifiChannel::~YansWifiChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_phyList.clear ();
}

void
YansWifiChannel::SetPropagationLossModel (Ptr<PropagationLossModel> loss)
{
  m_loss = loss;
}

void
YansWifiChannel::SetPropagationDelayModel (Ptr<PropagationDelayModel> delay)
{
  m_delay = delay;
}

void YansWifiChannel::Send (Ptr<YansWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm, WifiTxVector txVector, WifiPreamble preamble, uint8_t packetType, Time duration, PacketContext context) const{
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT (senderMobility != 0);
  uint32_t j = 0;

  m_channelTransmission(sender->GetDevice(), packet->Copy());

  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender != (*i))
        {
          //For now don't account for inter channel interference
          if ((*i)->GetChannelNumber () != sender->GetChannelNumber ())
            {
              continue;
            }

          Ptr<MobilityModel> receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
          Time delay = m_delay->GetDelay (senderMobility, receiverMobility);
          double rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility);
          NS_LOG_DEBUG ("propagation: txPower=" << txPowerDbm << "dbm, rxPower=" << rxPowerDbm << "dbm, " <<
                        "distance=" << senderMobility->GetDistanceFrom (receiverMobility) << "m, delay=" << delay);
          Ptr<Packet> copy = packet->Copy ();
          Ptr<Object> dstNetDevice = m_phyList[j]->GetDevice ();
          uint32_t dstNode;
          if (dstNetDevice == 0)
            {
              dstNode = 0xffffffff;
            }
          else
            {
              dstNode = dstNetDevice->GetObject<NetDevice> ()->GetNode ()->GetId ();
            }

          double *atts = new double[3];
          *atts = rxPowerDbm;
          *(atts + 1) = packetType;
          *(atts + 2) = duration.GetNanoSeconds ();
          // send to Rx
          if(context.IsEmpty()){
            void (YansWifiChannel::*callback)(uint32_t, Ptr<Packet>, double *, WifiTxVector, WifiPreamble) const = NULL;
            callback = &YansWifiChannel::Receive;
            Simulator::ScheduleWithContext (dstNode, delay, callback, this, j, copy, atts, txVector, preamble);
          }else{
            // record the node index into the context and send
            context.SetNodeIndex(j);
            void (YansWifiChannel::*callback)(PacketContext, Ptr<Packet>, double *, WifiTxVector, WifiPreamble) const = NULL;
            callback = &YansWifiChannel::Receive;
            Simulator::ScheduleWithContext (dstNode, delay, callback, this, context, copy, atts, txVector, preamble);
          }
        }
    }
}

void YansWifiChannel::Receive (uint32_t i, Ptr<Packet> packet, double *atts, WifiTxVector txVector, WifiPreamble preamble) const{
  m_phyList[i]->StartReceivePreambleAndHeader (packet, *atts, txVector, preamble, *(atts + 1), NanoSeconds (*(atts + 2)));
  delete[] atts;
}
void YansWifiChannel::Receive (SdnLab::PacketContext context, Ptr<Packet> packet, double *atts, WifiTxVector txVector, WifiPreamble preamble) const{
  m_phyList[context.GetNodeIndex()]->StartReceivePreambleAndHeader (packet, *atts, txVector, preamble, *(atts + 1), NanoSeconds (*(atts + 2)), context);
  delete[] atts;
}

uint32_t
YansWifiChannel::GetNDevices (void) const
{
  return m_phyList.size ();
}

Ptr<NetDevice>
YansWifiChannel::GetDevice (uint32_t i) const
{
  return m_phyList[i]->GetDevice ()->GetObject<NetDevice> ();
}

void
YansWifiChannel::Add (Ptr<YansWifiPhy> phy)
{
  m_phyList.push_back (phy);
}

int64_t
YansWifiChannel::AssignStreams (int64_t stream)
{
  int64_t currentStream = stream;
  currentStream += m_loss->AssignStreams (stream);
  return (currentStream - stream);
}

} //namespace ns3
