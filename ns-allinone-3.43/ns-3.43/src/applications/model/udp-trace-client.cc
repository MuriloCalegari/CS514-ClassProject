/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */
#include "udp-trace-client.h"

#include "seq-ts-header.h"

#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UdpTraceClient");

NS_OBJECT_ENSURE_REGISTERED(UdpTraceClient);

/**
 * \brief Default trace to send
 */
UdpTraceClient::TraceEntry UdpTraceClient::g_defaultEntries[] = {
    {0, 534, 'I'},
    {40, 1542, 'P'},
    {120, 134, 'B'},
    {80, 390, 'B'},
    {240, 765, 'P'},
    {160, 407, 'B'},
    {200, 504, 'B'},
    {360, 903, 'P'},
    {280, 421, 'B'},
    {320, 587, 'B'},
};

TypeId
UdpTraceClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UdpTraceClient")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<UdpTraceClient>()
            .AddAttribute("RemoteAddress",
                          "The destination Address of the outbound packets",
                          AddressValue(),
                          MakeAddressAccessor(&UdpTraceClient::m_peerAddress),
                          MakeAddressChecker())
            .AddAttribute("RemotePort",
                          "The destination port of the outbound packets",
                          UintegerValue(100),
                          MakeUintegerAccessor(&UdpTraceClient::m_peerPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Tos",
                          "The Type of Service used to send IPv4 packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&UdpTraceClient::m_tos),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("MaxPacketSize",
                          "The maximum size of a packet (including the SeqTsHeader, 12 bytes).",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&UdpTraceClient::m_maxPacketSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("TraceFilename",
                          "Name of file to load a trace from. By default, uses a hardcoded trace.",
                          StringValue(""),
                          MakeStringAccessor(&UdpTraceClient::SetTraceFile),
                          MakeStringChecker())
            .AddAttribute("TraceLoop",
                          "Loops through the trace file, starting again once it is over.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&UdpTraceClient::SetTraceLoop),
                          MakeBooleanChecker())

        ;
    return tid;
}

UdpTraceClient::UdpTraceClient()
{
    NS_LOG_FUNCTION(this);
    m_sent = 0;
    m_socket = nullptr;
    m_sendEvent = EventId();
    m_maxPacketSize = 1400;
}

UdpTraceClient::UdpTraceClient(Ipv4Address ip, uint16_t port, char* traceFile)
{
    NS_LOG_FUNCTION(this);
    m_sent = 0;
    m_socket = nullptr;
    m_sendEvent = EventId();
    m_peerAddress = ip;
    m_peerPort = port;
    m_currentEntry = 0;
    m_maxPacketSize = 1400;
    if (traceFile != nullptr)
    {
        SetTraceFile(traceFile);
    }
}

UdpTraceClient::~UdpTraceClient()
{
    NS_LOG_FUNCTION(this);
    m_entries.clear();
}

void
UdpTraceClient::SetRemote(Address ip, uint16_t port)
{
    NS_LOG_FUNCTION(this << ip << port);
    m_entries.clear();
    m_peerAddress = ip;
    m_peerPort = port;
}

void
UdpTraceClient::SetRemote(Address addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_entries.clear();
    m_peerAddress = addr;
}

void
UdpTraceClient::SetTraceFile(std::string traceFile)
{
    NS_LOG_FUNCTION(this << traceFile);
    if (traceFile.empty())
    {
        LoadDefaultTrace();
    }
    else
    {
        LoadTrace(traceFile);
    }
}

void
UdpTraceClient::SetMaxPacketSize(uint16_t maxPacketSize)
{
    NS_LOG_FUNCTION(this << maxPacketSize);
    m_maxPacketSize = maxPacketSize;
}

uint16_t
UdpTraceClient::GetMaxPacketSize()
{
    NS_LOG_FUNCTION(this);
    return m_maxPacketSize;
}

void
UdpTraceClient::LoadTrace(std::string filename)
{
    NS_LOG_FUNCTION(this << filename);
    uint32_t time = 0;
    uint32_t index = 0;
    uint32_t oldIndex = 0;
    uint32_t size = 0;
    uint32_t prevTime = 0;
    char frameType;
    TraceEntry entry;
    std::ifstream ifTraceFile;
    ifTraceFile.open(filename, std::ifstream::in);
    m_entries.clear();
    if (!ifTraceFile.good())
    {
        LoadDefaultTrace();
    }
    while (ifTraceFile.good())
    {
        ifTraceFile >> index >> frameType >> time >> size;
        if (index == oldIndex)
        {
            continue;
        }
        if (frameType == 'B')
        {
            entry.timeToSend = 0;
        }
        else
        {
            entry.timeToSend = time - prevTime;
            prevTime = time;
        }
        entry.packetSize = size;
        entry.frameType = frameType;
        m_entries.push_back(entry);
        oldIndex = index;
    }
    ifTraceFile.close();
    NS_ASSERT_MSG(prevTime != 0, "A trace file can not contain B frames only.");
    m_currentEntry = 0;
}

void
UdpTraceClient::LoadDefaultTrace()
{
    NS_LOG_FUNCTION(this);
    uint32_t prevTime = 0;
    for (uint32_t i = 0; i < (sizeof(g_defaultEntries) / sizeof(TraceEntry)); i++)
    {
        TraceEntry entry = g_defaultEntries[i];
        if (entry.frameType == 'B')
        {
            entry.timeToSend = 0;
        }
        else
        {
            uint32_t tmp = entry.timeToSend;
            entry.timeToSend -= prevTime;
            prevTime = tmp;
        }
        m_entries.push_back(entry);
    }
    m_currentEntry = 0;
}

void
UdpTraceClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        NS_ABORT_MSG_IF(m_peerAddress.IsInvalid(), "'RemoteAddress' attribute not properly set");
        if (Ipv4Address::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
            m_socket->Connect(
                InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
        else if (Ipv6Address::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind6() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(
                Inet6SocketAddress(Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
        else if (InetSocketAddress::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
            m_socket->Connect(m_peerAddress);
        }
        else if (Inet6SocketAddress::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind6() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(m_peerAddress);
        }
        else
        {
            NS_ASSERT_MSG(false, "Incompatible address type: " << m_peerAddress);
        }
    }
    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    m_socket->SetAllowBroadcast(true);
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &UdpTraceClient::Send, this);
}

void
UdpTraceClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_sendEvent);
}

void
UdpTraceClient::SendPacket(uint32_t size)
{
    NS_LOG_FUNCTION(this << size);
    Ptr<Packet> p;
    uint32_t packetSize;
    if (size > 12)
    {
        packetSize = size - 12; // 12 is the size of the SeqTsHeader
    }
    else
    {
        packetSize = 0;
    }
    p = Create<Packet>(packetSize);
    SeqTsHeader seqTs;
    seqTs.SetSeq(m_sent);
    p->AddHeader(seqTs);

    std::stringstream addressString;
    if (Ipv4Address::IsMatchingType(m_peerAddress))
    {
        addressString << Ipv4Address::ConvertFrom(m_peerAddress);
    }
    else if (Ipv6Address::IsMatchingType(m_peerAddress))
    {
        addressString << Ipv6Address::ConvertFrom(m_peerAddress);
    }
    else
    {
        addressString << m_peerAddress;
    }

    if ((m_socket->Send(p)) >= 0)
    {
        ++m_sent;
        NS_LOG_INFO("Sent " << size << " bytes to " << addressString.str());
    }
    else
    {
        NS_LOG_INFO("Error while sending " << size << " bytes to " << addressString.str());
    }
}

void
UdpTraceClient::Send()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_sendEvent.IsExpired());

    bool cycled = false;
    Ptr<Packet> p;
    TraceEntry* entry = &m_entries[m_currentEntry];
    do
    {
        for (uint32_t i = 0; i < entry->packetSize / m_maxPacketSize; i++)
        {
            SendPacket(m_maxPacketSize);
        }

        uint16_t sizetosend = entry->packetSize % m_maxPacketSize;
        SendPacket(sizetosend);

        m_currentEntry++;
        if (m_currentEntry >= m_entries.size())
        {
            m_currentEntry = 0;
            cycled = true;
        }
        entry = &m_entries[m_currentEntry];
    } while (entry->timeToSend == 0);

    if (!cycled || m_traceLoop)
    {
        m_sendEvent =
            Simulator::Schedule(MilliSeconds(entry->timeToSend), &UdpTraceClient::Send, this);
    }
}

void
UdpTraceClient::SetTraceLoop(bool traceLoop)
{
    m_traceLoop = traceLoop;
}

} // Namespace ns3
