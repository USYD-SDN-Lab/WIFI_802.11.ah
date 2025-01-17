/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#pragma once
#ifndef REGULAR_WIFI_MAC_H
#define REGULAR_WIFI_MAC_H

#include "ns3/wifi-mac.h"
#include "dca-txop.h"
#include "edca-txop-n.h"
#include "wifi-remote-station-manager.h"
#include "ssid.h"
#include "qos-utils.h"
#include <map>
#include "drop-reason.h"
#include "ns3/traced-callback.h"

// self-defined headers
#include "Components/PacketContext.h"

namespace ns3 {

class Dcf;
class MacLow;
class MacRxMiddle;
class MacTxMiddle;
class DcfManager;

/**
 * \brief base class for all MAC-level wifi objects.
 * \ingroup wifi
 *
 * This class encapsulates all the low-level MAC functionality
 * DCA, EDCA, etc) and all the high-level MAC functionality
 * (association/disassociation state machines).
 *
 */
class RegularWifiMac : public WifiMac
{
public:
  static TypeId GetTypeId (void);
  typedef void (* PacketDroppedCallback)
                    (Ptr<const Packet> packet, DropReason reason);

  typedef void (* CollisionCallback)(uint32_t nrOfSlotsToBackOff);
  typedef void (* TransmissionWillCrossRAWBoundaryCallback)(Time txDuration, Time remainingRawTime);
  RegularWifiMac ();
  virtual ~RegularWifiMac ();

  /**
   * \param slotTime the slot duration
   */
  void SetSlot (Time slotTime);
  /**
   * \param sifs the sifs duration
   */
  void SetSifs (Time sifs);
  /**
   * \param eifsNoDifs the duration of an EIFS minus DIFS.
   *
   * This value is used to calculate the EIFS depending
   * on AIFSN.
   */
  void SetEifsNoDifs (Time eifsNoDifs);
  /**
   * \param pifs the pifs duration.
   */
  void SetPifs (Time pifs);
  /**
   * \param rifs the rifs duration.
   */
  void SetRifs (Time rifs);
  /**
   * \param ctsTimeout the duration of a CTS timeout.
   */
  void SetCtsTimeout (Time ctsTimeout);
  /**
   * \param ackTimeout the duration of an ACK timeout.
   */
  void SetAckTimeout (Time ackTimeout);

  Time GetRifs (void) const;
  /**
   * \return the current PIFS duration.
   */
  Time GetPifs (void) const;
  /**
   * \return the current SIFS duration.
   */
  Time GetSifs (void) const;
  /**
   * \return the current slot duration.
   */
  Time GetSlot (void) const;
  /**
   * \return the current EIFS minus DIFS duration
   */
  Time GetEifsNoDifs (void) const;
  /**
   * \return the current CTS timeout duration.
   */
  Time GetCtsTimeout (void) const;
  /**
   * \return the current ACK timeout duration.
   */
  Time GetAckTimeout (void) const;

  /**
   * Enable or disable CTS-to-self feature.
   *
   * \param enable true if CTS-to-self is to be supported,
   *               false otherwise
   */
  void SetCtsToSelfSupported (bool enable);

  /**
   * Return whether the device supports CTS-to-self
   * capability.
   *
   * \return true if CTS-to-self is supported,
   *         false otherwise.
   */
  bool GetCtsToSelfSupported () const;
  /**
   * \return the MAC address associated to this MAC layer.
   */
  virtual Mac48Address GetAddress (void) const;
  /**
   * \return the ssid which this MAC layer is going to try to stay in.
   */
  virtual Ssid GetSsid (void) const;
  /**
   * \param address the current address of this MAC layer.
   */
  virtual void SetAddress (Mac48Address address);
  /**
   * \param ssid the current ssid of this MAC layer.
   */
  virtual void SetSsid (Ssid ssid);
  /**
   * \param bssid the BSSID of the network that this device belongs to.
   */
  virtual void SetBssid (Mac48Address bssid);
  /**
   * \return the bssid of the network this device belongs to.
   */
  virtual Mac48Address GetBssid (void) const;
  /**
   * \brief Sets the interface in promiscuous mode.
   *
   * Enables promiscuous mode on the interface. Note that any further
   * filtering on the incoming frame path may affect the overall
   * behavior.
   */
  virtual void SetPromisc (void);

  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   * \param from the address from which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.  The extra parameter "from" allows
   * this device to operate in a bridged mode, forwarding received
   * frames without altering the source address.
   */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from);

  virtual bool SupportsSendFrom (void) const;

  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.
   */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to) = 0;
  /**
   * \param phy the physical layer attached to this MAC.
   */
  virtual void SetWifiPhy (Ptr<WifiPhy> phy);
  /**
   * \return the physical layer attached to this MAC.
   */
  virtual Ptr<WifiPhy> GetWifiPhy (void) const;
  /**
   * removes attached WifiPhy device from this MAC.
   */
  virtual void ResetWifiPhy (void);
  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager);
  /**
   * \return the station manager attached to this MAC.
   */
  virtual Ptr<WifiRemoteStationManager> GetWifiRemoteStationManager (void) const;

  /**
   * This type defines the callback of a higher layer that a
   * WifiMac(-derived) object invokes to pass a packet up the stack.
   *
   * \param packet the packet that has been received.
   * \param from the MAC address of the device that sent the packet.
   * \param to the MAC address ot the device that the packet is destined for.
   */
  typedef Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> ForwardUpCallback;
  /**
   * \param upCallback the callback to invoke when a packet must be
   * forwarded up the stack.
   */
  virtual void SetForwardUpCallback (ForwardUpCallback upCallback);
  /**
   * \param linkUp the callback to invoke when the link becomes up.
   */
  virtual void SetLinkUpCallback (Callback<void> linkUp);
  /**
   * \param linkDown the callback to invoke when the link becomes down.
   */
  virtual void SetLinkDownCallback (Callback<void> linkDown);

  /* Next functions are not pure virtual so non Qos WifiMacs are not
   * forced to implement them.
   */
  virtual void SetBasicBlockAckTimeout (Time blockAckTimeout);
  virtual Time GetBasicBlockAckTimeout (void) const;
  virtual void SetCompressedBlockAckTimeout (Time blockAckTimeout);
  virtual Time GetCompressedBlockAckTimeout (void) const;


protected:
  virtual void DoInitialize ();
  virtual void DoDispose ();

  MacRxMiddle *m_rxMiddle;  //!< RX middle (de-fragmentation etc.)
  MacTxMiddle *m_txMiddle;  //!< TX middle (aggregation etc.)
  Ptr<MacLow> m_low;        //!< MacLow (RTS, CTS, DATA, ACK etc.)
  DcfManager *m_dcfManager; //!< DCF manager (access to channel)
  Ptr<WifiPhy> m_phy;       //!< Wifi PHY

  Ptr<WifiRemoteStationManager> m_stationManager; //!< Remote station manager (rate control, RTS/CTS/fragmentation thresholds etc.)

  ForwardUpCallback m_forwardUp; //!< Callback to forward packet up the stack
  Callback<void> m_linkUp;       //!< Callback when a link is up
  Callback<void> m_linkDown;     //!< Callback when a link is down

  Ssid m_ssid; //!< Service Set ID (SSID)

  /** This holds a pointer to the DCF instance for this WifiMac - used
  for transmission of frames to non-QoS peers. */
  Ptr<DcaTxop> m_dca;

  /** This type defines a mapping between an Access Category index,
  and a pointer to the corresponding channel access function */
  typedef std::map<AcIndex, Ptr<EdcaTxopN> > EdcaQueues;

  /** This is a map from Access Category index to the corresponding
  channel access function */
  EdcaQueues m_edca;

  virtual void OnQueuePacketDropped(std::string context, Ptr<const Packet> packet, DropReason reason);

  virtual void OnCollision(std::string context, uint32_t nrOfBackOffSlots);

  virtual void OnTransmissionWillCrossRAWBoundary(std::string context, Time txDuration, Time remainingTimeInRAWSlot);

  /**
   * Accessor for the DCF object
   *
   * \return a smart pointer to DcaTxop
   */
  Ptr<DcaTxop> GetDcaTxop (void) const;

  /**
   * Accessor for the AC_VO channel access function
   *
   * \return a smart pointer to EdcaTxopN
   */
  Ptr<EdcaTxopN> GetVOQueue (void) const;
  /**
   * Accessor for the AC_VI channel access function
   *
   * \return a smart pointer to EdcaTxopN
   */
  Ptr<EdcaTxopN> GetVIQueue (void) const;
  /**
   * Accessor for the AC_BE channel access function
   *
   * \return a smart pointer to EdcaTxopN
   */
  Ptr<EdcaTxopN> GetBEQueue (void) const;
  /**
   * Accessor for the AC_BK channel access function
   *
   * \return a smart pointer to EdcaTxopN
   */
  Ptr<EdcaTxopN> GetBKQueue (void) const;

  /**
   * \param standard the phy standard to be used
   *
   * This method is called by ns3::WifiMac::ConfigureStandard to
   * complete the configuration process for a requested phy standard.
   *
   * This method may be overriden by a derived class (e.g., in order
   * to apply DCF or EDCA parameters specific to the usage model it is
   * dealing with), in which case the reimplementation may choose to
   * deal with certain values in the WifiPhyStandard enumeration, and
   * chain up to this implementation to deal with the remainder.
   */
  virtual void FinishConfigureStandard (enum WifiPhyStandard standard);

  /**
   * This method is invoked by a subclass to specify what type of
   * station it is implementing. This is something that the channel
   * access functions (instantiated within this class as EdcaTxopN's)
   * need to know.
   *
   * \param type the type of station.
   */
  void SetTypeOfStation (TypeOfStation type);

  /**
   * This method acts as the MacRxMiddle receive callback and is
   * invoked to notify us that a frame has been received. The
   * implementation is intended to capture logic that is going to be
   * common to all (or most) derived classes. Specifically, handling
   * of Block Ack managment frames is dealt with here.
   *
   * This method will need, however, to be overriden by derived
   * classes so that they can perform their data handling before
   * invoking the base version.
   *
   * \param packet the packet that has been received.
   * \param hdr a pointer to the MAC header of the received frame.
   * \param packetContext the context of this packet
   */
  virtual void Receive (Ptr<Packet> packet, const WifiMacHeader *hdr, SdnLab::PacketContext context = SdnLab::PacketContext());
  /**
   * The packet we sent was successfully received by the receiver
   * (i.e. we received an ACK from the receiver).
   *
   * \param hdr the header of the packet that we successfully sent
   */
  virtual void TxOk (const WifiMacHeader &hdr);
  /**
   * The packet we sent was successfully received by the receiver
   * (i.e. we did not receive an ACK from the receiver).
   *
   * \param hdr the header of the packet that we failed to sent
   */
  virtual void TxFailed (const WifiMacHeader &hdr);

  /**
   * Forward the packet up to the device.
   *
   * \param packet the packet that we are forwarding up to the device
   * \param from the address of the source
   * \param to the address of the destination
   */
  void ForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to);

  /**
   * This method can be called to de-aggregate an A-MSDU and forward
   * the constituent packets up the stack.
   *
   * \param aggregatedPacket the Packet containing the A-MSDU.
   * \param hdr a pointer to the MAC header for \c aggregatedPacket.
   */
  virtual void DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket,
                                           const WifiMacHeader *hdr);

  /**
   * This method can be called to accept a received ADDBA Request. An
   * ADDBA Response will be constructed and queued for transmission.
   *
   * \param reqHdr a pointer to the received ADDBA Request header.
   * \param originator the MAC address of the originator.
   */
  virtual void SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr,
                                  Mac48Address originator);

  /**
   * This Boolean is set \c true iff this WifiMac is to model
   * 802.11e/WMM style Quality of Service. It is exposed through the
   * attribute system.
   *
   * At the moment, this flag is the sole selection between QoS and
   * non-QoS operation for the STA (whether IBSS, AP, or
   * non-AP). Ultimately, we will want a QoS-enabled STA to be able to
   * fall back to non-QoS operation with a non-QoS peer. This'll
   * require further intelligence - i.e., per-association QoS
   * state. Having a big switch seems like a good intermediate stage,
   * however.
   */
  bool m_qosSupported;

  /**
   * Enable or disable QoS support for the device.
   *
   * \param enable whether QoS is supported
   */
  void SetQosSupported (bool enable);
  /**
   * Return whether the device supports QoS.
   *
   * \return true if QoS is supported, false otherwise
   */
  bool GetQosSupported () const;

  /**
    * This Boolean is set \c true iff this WifiMac is to model
    * 802.11n. It is exposed through the
    * attribute system.
    *
    * At the moment, this flag is the sole selection between HT and
    * non-HT operation for the STA (whether IBSS, AP, or
    * non-AP). Ultimately, we will want a HT-enabled STA to be able to
    * fall back to non-HT operation with a non-HT peer. This'll
    * require further intelligence - i.e., per-association HT
    * state. Having a big switch seems like a good intermediate stage,
    * however.
    */
  bool m_htSupported;
  bool m_s1gSupported;
 
  uint8_t m_s1gStaType;
  /**
   * Enable or disable HT support for the device.
   *
   * \param enable whether HT is supported
   */
  void SetHtSupported (bool enable);
  /**
   * Return whether the device supports QoS.
   *
   * \return true if HT is supported, false otherwise
   */
  bool GetHtSupported () const;
    
  /**
   * Enable or disable S1G support for the device.
   *
   * \param enable whether S1G is supported
   */
  void SetS1gSupported (bool enable);
  /**
   * Return whether the device supports QoS.
   *
   * \return true if S1G is supported, false otherwise
   */
  bool GetS1gSupported () const;
    
  void SetS1gStaType (uint8_t type);
  uint8_t GetS1gStaType (void) const;

  TracedCallback<Ptr<const Packet>, DropReason> m_packetdropped;
  TracedCallback<uint32_t> m_collisionTrace;
  TracedCallback<Time,Time> m_transmissionWillCrossRAWBoundary;

private:
  RegularWifiMac (const RegularWifiMac &);
  RegularWifiMac & operator= (const RegularWifiMac &);

  /**
   * This method is a private utility invoked to configure the channel
   * access function for the specified Access Category.
   *
   * \param ac the Access Category index of the queue to initialise.
   */
  void SetupEdcaQueue (enum AcIndex ac);

  TracedCallback<const WifiMacHeader &> m_txOkCallback;
  TracedCallback<const WifiMacHeader &> m_txErrCallback;
};

} //namespace ns3

#endif /* REGULAR_WIFI_MAC_H */
