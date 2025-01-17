/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "wifi-mac-queue.h"
#include "qos-blocked-destinations.h"

// 3rd party namespaces
using namespace SdnLab;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WifiMacQueue);

WifiMacQueue::Item::Item (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tstamp){
  this->packet = packet;
  this->hdr = hdr;
  this->tstamp = tstamp;
}
WifiMacQueue::Item::Item (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tstamp, PacketContext context){
  this->packet = packet;
  this->hdr = hdr;
  this->tstamp = tstamp;
  this->context = context;
}

TypeId WifiMacQueue::GetTypeId (void){
  static TypeId tid = TypeId ("ns3::WifiMacQueue")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiMacQueue> ()
    .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are already this number of packets, it is dropped.",
                   UintegerValue (400),
                   MakeUintegerAccessor (&WifiMacQueue::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (MilliSeconds (500.0)),
                   MakeTimeAccessor (&WifiMacQueue::m_maxDelay),
                   MakeTimeChecker ())
	.AddTraceSource ("PacketDropped",
					 "Trace source indicating a packet has been dropped from the queue",
					 MakeTraceSourceAccessor (&WifiMacQueue::m_packetdropped),
					 "ns3::WifiMacQueue::PacketDroppedCallback")
  ;
  return tid;
}

WifiMacQueue::WifiMacQueue ()
  : m_size (0)
{
}

WifiMacQueue::~WifiMacQueue ()
{
  Flush ();
}

void
WifiMacQueue::SetMaxSize (uint32_t maxSize)
{
  m_maxSize = maxSize;
}

void
WifiMacQueue::SetMaxDelay (Time delay)
{
  m_maxDelay = delay;
}

uint32_t
WifiMacQueue::GetMaxSize (void) const
{
  return m_maxSize;
}

Time
WifiMacQueue::GetMaxDelay (void) const
{
  return m_maxDelay;
}


void WifiMacQueue::Enqueue (Ptr<const Packet> packet, const WifiMacHeader &hdr, PacketContext context){
  Cleanup ();
  if (m_size == m_maxSize){
    //std::cout << "DROPPING PACKET FROM WIFI MAC QUEUE " << std::endl;
    m_packetdropped(packet->Copy(), DropReason::MacQueueSizeExceeded);
    return;
  }
  Time now = Simulator::Now ();
  m_queue.push_back (Item (packet, hdr, now, context));
  m_size++;
}
void WifiMacQueue::Enqueue (Ptr<const Packet> packet, const WifiMacHeader &hdr){
  Enqueue(packet, hdr, PacketContext());
}

void
WifiMacQueue::Cleanup (void)
{
  if (m_queue.empty ())
    {
      return;
    }

  Time now = Simulator::Now ();
  uint32_t n = 0;
  for (PacketQueueI i = m_queue.begin (); i != m_queue.end (); )
    {
      if (i->tstamp + m_maxDelay > now)
        {
          i++;
        }
      else
        {
    	  m_packetdropped(i->packet->Copy(), DropReason::MacQueueDelayExceeded);
          i = m_queue.erase (i);
          n++;
        }
    }
  m_size -= n;
}

Ptr<const Packet> WifiMacQueue::Dequeue (WifiMacHeader *hdr){
  Cleanup ();
  if (!m_queue.empty ()){
    Item i = m_queue.front ();
    m_queue.pop_front ();
    m_size--;
    *hdr = i.hdr;
    return i.packet;
  }
  return 0;
}
Ptr<const Packet> WifiMacQueue::Dequeue (WifiMacHeader *hdr, PacketContext &context){
  Cleanup ();
  if (!m_queue.empty ()){
    Item i = m_queue.front ();
    m_queue.pop_front ();
    m_size--;
    *hdr = i.hdr;
    context = i.context;
    return i.packet;
  }
  return 0;
}

Ptr<const Packet>
WifiMacQueue::Peek (WifiMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

Ptr<const Packet>
WifiMacQueue::DequeueByTidAndAddress (WifiMacHeader *hdr, uint8_t tid,
                                      WifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  Ptr<const Packet> packet = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  packet = it->packet;
                  *hdr = it->hdr;
                  m_queue.erase (it);
                  m_size--;
                  break;
                }
            }
        }
    }
  return packet;
}

Ptr<const Packet>
WifiMacQueue::PeekByTidAndAddress (WifiMacHeader *hdr, uint8_t tid,
                                   WifiMacHeader::AddressType type, Mac48Address dest, Time *timestamp)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  *hdr = it->hdr;
                  *timestamp = it->tstamp;
                  return it->packet;
                }
            }
        }
    }
  return 0;
}

Ptr<const Packet>
WifiMacQueue::PeekByAddress (WifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
              if (GetAddressForPacket (type, it) == dest)
                {
                  return it->packet;
                }
        }
    }
  return 0;
}

bool
WifiMacQueue::IsEmpty (void)
{
  Cleanup ();
  return m_queue.empty ();
}

uint32_t
WifiMacQueue::GetSize (void)
{
  return m_size;
}

void
WifiMacQueue::Flush (void)
{
  m_queue.erase (m_queue.begin (), m_queue.end ());
  m_size = 0;
}

Mac48Address
WifiMacQueue::GetAddressForPacket (enum WifiMacHeader::AddressType type, PacketQueueI it)
{
  if (type == WifiMacHeader::ADDR1)
    {
      return it->hdr.GetAddr1 ();
    }
  if (type == WifiMacHeader::ADDR2)
    {
      return it->hdr.GetAddr2 ();
    }
  if (type == WifiMacHeader::ADDR3)
    {
      return it->hdr.GetAddr3 ();
    }
  return 0;
}

bool
WifiMacQueue::Remove (Ptr<const Packet> packet)
{
  PacketQueueI it = m_queue.begin ();
  for (; it != m_queue.end (); it++)
    {
      if (it->packet == packet)
        {
          m_queue.erase (it);
          m_size--;
          return true;
        }
    }
  return false;
}

void WifiMacQueue::PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  Cleanup ();
  if (m_size == m_maxSize)
    {
	  m_packetdropped(packet->Copy(), DropReason::MacQueueSizeExceeded);
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_front (Item (packet, hdr, now));
  m_size++;
}

uint32_t
WifiMacQueue::GetNPacketsByTidAndAddress (uint8_t tid, WifiMacHeader::AddressType type,
                                          Mac48Address addr)
{
  Cleanup ();
  uint32_t nPackets = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); it++)
        {
          if (GetAddressForPacket (type, it) == addr)
            {
              if (it->hdr.IsQosData () && it->hdr.GetQosTid () == tid)
                {
                  nPackets++;
                }
            }
        }
    }
  return nPackets;
}

Ptr<const Packet>
WifiMacQueue::DequeueFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                     const QosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  Ptr<const Packet> packet = 0;
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          packet = it->packet;
          m_queue.erase (it);
          m_size--;
          return packet;
        }
    }
  return packet;
}

Ptr<const Packet>
WifiMacQueue::PeekFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                  const QosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          return it->packet;
        }
    }
  return 0;
}


Ptr<const Packet>
WifiMacQueue::PeekAvailable (WifiMacHeader *hdr, Time &timestamp,
                                  const QosBlockedDestinations *blockedPackets, uint16_t k)
{
  Cleanup ();
  uint16_t count;
  count = 0;
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (count < k)
        {
          count++;
          continue;
        }
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          return it->packet;
        }
    }
  return 0;
}

} //namespace ns3
