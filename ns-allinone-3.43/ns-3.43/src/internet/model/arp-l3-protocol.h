/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef ARP_L3_PROTOCOL_H
#define ARP_L3_PROTOCOL_H

#include "ipv4-header.h"

#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traced-callback.h"

#include <list>

namespace ns3
{

class ArpCache;
class NetDevice;
class Node;
class Packet;
class Ipv4Interface;
class TrafficControlLayer;

/**
 * \ingroup ipv4
 * \defgroup arp ARP protocol.
 *
 * The ARP protocol and its associated tables are responsible
 * for the IPv4 - MAC address translation.
 * Each NetDevice has its own ARP table.
 */

/**
 * \ingroup arp
 * \brief An implementation of the ARP protocol.
 */
class ArpL3Protocol : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    static const uint16_t PROT_NUMBER; //!< ARP protocol number (0x0806)

    ArpL3Protocol();
    ~ArpL3Protocol() override;

    // Delete copy constructor and assignment operator to avoid misuse
    ArpL3Protocol(const ArpL3Protocol&) = delete;
    ArpL3Protocol& operator=(const ArpL3Protocol&) = delete;

    /**
     * \brief Set the node the ARP L3 protocol is associated with
     * \param node the node
     */
    void SetNode(Ptr<Node> node);

    /**
     * \brief Set the TrafficControlLayer.
     * \param tc TrafficControlLayer object
     */
    void SetTrafficControl(Ptr<TrafficControlLayer> tc);

    /**
     * \brief Create an ARP cache for the device/interface
     * \param device the NetDevice
     * \param interface the Ipv4Interface
     * \returns a smart pointer to the ARP cache
     */
    Ptr<ArpCache> CreateCache(Ptr<NetDevice> device, Ptr<Ipv4Interface> interface);

    /**
     * \brief Receive a packet
     * \param device the source NetDevice
     * \param p the packet
     * \param protocol the protocol
     * \param from the source address
     * \param to the destination address
     * \param packetType type of packet (i.e., unicast, multicast, etc.)
     */
    void Receive(Ptr<NetDevice> device,
                 Ptr<const Packet> p,
                 uint16_t protocol,
                 const Address& from,
                 const Address& to,
                 NetDevice::PacketType packetType);
    /**
     * \brief Perform an ARP lookup
     * \param p the packet
     * \param ipHeader the IPv4 header
     * \param destination destination IP address
     * \param device outgoing device
     * \param cache ARP cache
     * \param hardwareDestination filled with the destination MAC address (if the entry exists)
     * \return true if there is a matching ARP Entry
     */
    bool Lookup(Ptr<Packet> p,
                const Ipv4Header& ipHeader,
                Ipv4Address destination,
                Ptr<NetDevice> device,
                Ptr<ArpCache> cache,
                Address* hardwareDestination);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    void DoDispose() override;
    /*
     * This function will notify other components connected to the node that a new stack member is
     * now connected This will be used to notify Layer 3 protocol of layer 4 protocol stack to
     * connect them together.
     */
    void NotifyNewAggregate() override;

  private:
    typedef std::list<Ptr<ArpCache>> CacheList; //!< container of the ARP caches

    /**
     * \brief Finds the cache associated with a NetDevice
     * \param device the NetDevice
     * \returns the ARP cache, or null if no cache is found
     */
    Ptr<ArpCache> FindCache(Ptr<NetDevice> device);

    /**
     * \brief Send an ARP request to an host
     * \param cache the ARP cache to use
     * \param to the destination IP
     */
    void SendArpRequest(Ptr<const ArpCache> cache, Ipv4Address to);
    /**
     * \brief Send an ARP reply to an host
     * \param cache the ARP cache to use
     * \param myIp the source IP address
     * \param toIp the destination IP
     * \param toMac the destination MAC address
     */
    void SendArpReply(Ptr<const ArpCache> cache, Ipv4Address myIp, Ipv4Address toIp, Address toMac);

    CacheList m_cacheList;                         //!< ARP cache container
    Ptr<Node> m_node;                              //!< node the ARP L3 protocol is associated with
    TracedCallback<Ptr<const Packet>> m_dropTrace; //!< trace for packets dropped by ARP
    Ptr<RandomVariableStream> m_requestJitter;     //!< jitter to de-sync ARP requests
    Ptr<TrafficControlLayer> m_tc;                 //!< The associated TrafficControlLayer
};

} // namespace ns3

#endif /* ARP_L3_PROTOCOL_H */
