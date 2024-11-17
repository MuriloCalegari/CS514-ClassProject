/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

// This example shows how to create new simulations that are implemented in
// multiple files and headers. The structure of this simulation project
// is as follows:
//
// scratch/
// |  nested-subdir/
// |  |  - scratch-nested-subdir-executable.cc        // Main simulation file
// |  |  lib
// |  |  |  - scratch-nested-subdir-library-header.h  // Additional header
// |  |  |  - scratch-nested-subdir-library-source.cc // Additional header implementation
//
// This file contains the main() function, which calls an external function
// defined in the "scratch-nested-subdir-library-header.h" header file and
// implemented in "scratch-nested-subdir-library-source.cc".

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "adaptive-tcp-test.h"
// #include "ns3/mpi-interface.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdaptiveTcpTest");

int
main(int argc, char* argv[])
{
    // // Initialize MPI
    // MpiInterface::Enable(&argc, &argv);

    std::string linkBandwidth = "1000Mbps"; // Default to 1Gbps
    double simulationTime = 60.0; // Default to 1 minutes
    double senderCount = 10; // Default to 10 senders

    CommandLine cmd;
    cmd.AddValue("linkBandwidth", "Bandwidth of the middle link", linkBandwidth);
    cmd.AddValue("simulationTime", "Simulation runtime in seconds", simulationTime);
    cmd.AddValue("senderCount", "Number of senders", senderCount);
    cmd.Parse(argc, argv);

    // Create sender, receiver, and bottleneck nodes
    // Create nodes
    NodeContainer senders, bottleneck, receivers;
    senders.Create(senderCount);
    bottleneck.Create(2);  // bottleneck.Get(0) is left, bottleneck.Get(1) is right
    receivers.Create(senderCount);

    // Connect each sender to the bottleneck and each receiver to the bottleneck
    InternetStackHelper stack;
    stack.Install(senders);
    stack.Install(receivers);
    stack.Install(bottleneck);

    // Set up the throttled link between bottleneck nodes
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue(linkBandwidth));
    // TODO parameterize the delay
    bottleneckLink.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer bottleneckDevices = bottleneckLink.Install(bottleneck.Get(0), bottleneck.Get(1));

    // Assign IPs for the bottleneck link
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    
    Ipv4InterfaceContainer bottleneckInterfaces = address.Assign(bottleneckDevices);

    int senderIndex = 0; // Index to keep track of sender-receiver pairs
    
    // Install the competing congestion control algorithms    
    for (int i = 0; i < CCA_COUNT; i++) {
        int numSenders = ccaData[i].percentage / 100.0 * senderCount;

        if(numSenders == 0) continue;  // Skip if no senders for this CCA

        for (int j = 0; j < numSenders; j++) {

            if (senderIndex >= senderCount) break;  // Prevent going beyond available senders

            ns3::Ptr<ns3::Node> sender = senders.Get(senderIndex);
            ns3::Ptr<ns3::Node> receiver = receivers.Get(senderIndex);
            
            setPairGoingThroughLink(sender,
                                    bottleneck,
                                    receiver,
                                    simulationTime,
                                    senderIndex,
                                    TypeId::LookupByName(ccaData[i].tcpTypeId));

            senderIndex++;  // Move to the next sender-receiver pair
        }
    }

    // Add our custom CCA

    NS_LOG_INFO("Initialize Global Routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Enable NetAnim
    AnimationInterface anim("adaptive-tcp-test.xml");
    
    NS_LOG_INFO("Starting the simulation with runtime of " << simulationTime << "s...");
    Simulator::Stop(Seconds(simulationTime + 5));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Simulation completed.");

    // // Finalize MPI
    // MpiInterface::Disable();

    return 0;
}

void
setPairGoingThroughLink(ns3::Ptr<ns3::Node> sender,
                        ns3::NodeContainer& bottleneck,
                        ns3::Ptr<ns3::Node> receiver,
                        double simulationTime,
                        int senderIndex,
                        ns3::TypeId tcpTypeId)
{
    Ipv4AddressHelper address;
    
    // Connect each sender to bottleneck.Get(0)
    PointToPointHelper senderLink;
    senderLink.SetDeviceAttribute("DataRate",
                                  StringValue("10Gbps")); // High bandwidth for sender links
    senderLink.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer senderDevices =
        senderLink.Install(sender, bottleneck.Get(0));

    // Assign IPs for sender links
    address.SetBase(("10.2." + std::to_string(senderIndex) + ".0").c_str(), "255.255.255.0");
    Ipv4InterfaceContainer senderInterfaces = address.Assign(senderDevices);

    // Connect each receiver to bottleneck.Get(1)
    PointToPointHelper receiverLink;
    receiverLink.SetDeviceAttribute("DataRate",
                                    StringValue("10Gbps")); // High bandwidth for receiver links
    receiverLink.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer receiverDevices =
        receiverLink.Install(bottleneck.Get(1), receiver);

    // Assign IPs for receiver links
    address.SetBase(("10.3." + std::to_string(senderIndex) + ".0").c_str(), "255.255.255.0");
    Ipv4InterfaceContainer receiverInterfaces = address.Assign(receiverDevices);

    // Set TCP type for the specific sender based on ccaData
    // TypeId tcpTypeId = TypeId::LookupByName(ccaData[i].tcpTypeId);
    std::ostringstream path;
    // TODO is this right?
    path << "/NodeList/" << "ns3::TcpNewReno" << "/$ns3::TcpL4Protocol/SocketType";
    Config::Set(path.str(), TypeIdValue(tcpTypeId));

    // Set up applications
    uint16_t port = 9;

    // BulkSend application on sender side
    BulkSendHelper bulkSend("ns3::TcpSocketFactory",
                            InetSocketAddress(receiverInterfaces.GetAddress(1), port));
    bulkSend.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer senderApps = bulkSend.Install(sender);
    senderApps.Start(Seconds(1.0));
    senderApps.Stop(Seconds(simulationTime));

    // PacketSink application on receiver side to act as sink
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer receiverApps = sink.Install(receiver);
    receiverApps.Start(Seconds(1.0));
    receiverApps.Stop(Seconds(simulationTime));
}
