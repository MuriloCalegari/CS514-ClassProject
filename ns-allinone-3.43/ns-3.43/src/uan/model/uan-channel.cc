/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#include "uan-channel.h"

#include "uan-net-device.h"
#include "uan-noise-model-default.h"
#include "uan-phy.h"
#include "uan-prop-model-ideal.h"
#include "uan-prop-model.h"
#include "uan-transducer.h"
#include "uan-tx-mode.h"

#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UanChannel");

NS_OBJECT_ENSURE_REGISTERED(UanChannel);

TypeId
UanChannel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanChannel")
                            .SetParent<Channel>()
                            .SetGroupName("Uan")
                            .AddConstructor<UanChannel>()
                            .AddAttribute("PropagationModel",
                                          "A pointer to the propagation model.",
                                          StringValue("ns3::UanPropModelIdeal"),
                                          MakePointerAccessor(&UanChannel::m_prop),
                                          MakePointerChecker<UanPropModel>())
                            .AddAttribute("NoiseModel",
                                          "A pointer to the model of the channel ambient noise.",
                                          StringValue("ns3::UanNoiseModelDefault"),
                                          MakePointerAccessor(&UanChannel::m_noise),
                                          MakePointerChecker<UanNoiseModel>());

    return tid;
}

UanChannel::UanChannel()
    : Channel(),
      m_prop(nullptr),
      m_cleared(false)
{
}

UanChannel::~UanChannel()
{
}

void
UanChannel::Clear()
{
    if (m_cleared)
    {
        return;
    }
    m_cleared = true;
    auto it = m_devList.begin();
    for (; it != m_devList.end(); it++)
    {
        if (it->first)
        {
            it->first->Clear();
            it->first = nullptr;
        }
        if (it->second)
        {
            it->second->Clear();
            it->second = nullptr;
        }
    }
    m_devList.clear();
    if (m_prop)
    {
        m_prop->Clear();
        m_prop = nullptr;
    }
    if (m_noise)
    {
        m_noise->Clear();
        m_noise = nullptr;
    }
}

void
UanChannel::DoDispose()
{
    Clear();
    Channel::DoDispose();
}

void
UanChannel::SetPropagationModel(Ptr<UanPropModel> prop)
{
    NS_LOG_DEBUG("Set Prop Model " << this);
    m_prop = prop;
}

std::size_t
UanChannel::GetNDevices() const
{
    return m_devList.size();
}

Ptr<NetDevice>
UanChannel::GetDevice(std::size_t i) const
{
    return m_devList[i].first;
}

void
UanChannel::AddDevice(Ptr<UanNetDevice> dev, Ptr<UanTransducer> trans)
{
    NS_LOG_DEBUG("Adding dev/trans pair number " << m_devList.size());
    m_devList.emplace_back(dev, trans);
}

void
UanChannel::TxPacket(Ptr<UanTransducer> src, Ptr<Packet> packet, double txPowerDb, UanTxMode txMode)
{
    Ptr<MobilityModel> senderMobility = nullptr;

    NS_LOG_DEBUG("Channel scheduling");
    for (auto i = m_devList.begin(); i != m_devList.end(); i++)
    {
        if (src == i->second)
        {
            senderMobility = i->first->GetNode()->GetObject<MobilityModel>();
            break;
        }
    }
    NS_ASSERT(senderMobility);
    uint32_t j = 0;
    auto i = m_devList.begin();
    for (; i != m_devList.end(); i++)
    {
        if (src != i->second)
        {
            NS_LOG_DEBUG("Scheduling " << i->first->GetMac()->GetAddress());
            Ptr<MobilityModel> rcvrMobility = i->first->GetNode()->GetObject<MobilityModel>();
            Time delay = m_prop->GetDelay(senderMobility, rcvrMobility, txMode);
            UanPdp pdp = m_prop->GetPdp(senderMobility, rcvrMobility, txMode);
            double rxPowerDb =
                txPowerDb - m_prop->GetPathLossDb(senderMobility, rcvrMobility, txMode);

            NS_LOG_DEBUG("txPowerDb="
                         << txPowerDb << "dB, rxPowerDb=" << rxPowerDb << "dB, distance="
                         << senderMobility->GetDistanceFrom(rcvrMobility) << "m, delay=" << delay);

            uint32_t dstNodeId = i->first->GetNode()->GetId();
            Ptr<Packet> copy = packet->Copy();
            Simulator::ScheduleWithContext(dstNodeId,
                                           delay,
                                           &UanChannel::SendUp,
                                           this,
                                           j,
                                           copy,
                                           rxPowerDb,
                                           txMode,
                                           pdp);
        }
        j++;
    }
}

void
UanChannel::SetNoiseModel(Ptr<UanNoiseModel> noise)
{
    NS_ASSERT(noise);
    m_noise = noise;
}

void
UanChannel::SendUp(uint32_t i, Ptr<Packet> packet, double rxPowerDb, UanTxMode txMode, UanPdp pdp)
{
    NS_LOG_DEBUG("Channel:  In sendup");
    m_devList[i].second->Receive(packet, rxPowerDb, txMode, pdp);
}

double
UanChannel::GetNoiseDbHz(double fKhz)
{
    NS_ASSERT(m_noise);
    double noise = m_noise->GetNoiseDbHz(fKhz);
    return noise;
}

} // namespace ns3
