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
#include "adaptive-tcp-test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdaptiveTcpTest");

int
main(int argc, char* argv[])
{
    std::string linkBandwidth = "1000Mbps"; // Default to 1Gbps
    double simulationTime = 300.0; // Default to 5 minutes

    CommandLine cmd;
    cmd.AddValue("linkBandwidth", "Bandwidth of the middle link", linkBandwidth);
    cmd.AddValue("simulationTime", "Simulation runtime in seconds", simulationTime);
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(linkBandwidth));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    for (int i = 0; i < CCA_COUNT; i++) {
        int numSenders = ccaData[i].percentage;
        for (int j = 0; j < numSenders; j++) {
            // Create sender and receiver nodes
            Ptr<Node> sender = CreateObject<Node>();
            Ptr<Node> receiver = CreateObject<Node>();

            // Install internet stack on sender and receiver
            stack.Install(sender);
            stack.Install(receiver);

            // Create point-to-point link between sender and receiver
            NetDeviceContainer senderReceiverDevices;
            senderReceiverDevices = pointToPoint.Install(sender, receiver);

            // Assign IP addresses
            Ipv4InterfaceContainer senderReceiverInterfaces;
            senderReceiverInterfaces = address.Assign(senderReceiverDevices);

            // Set TCP type
            TypeId tcpTypeId = TypeId::LookupByName(ccaData[i].tcpTypeId);
            Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tcpTypeId));

            // Create and install applications
            uint16_t port = 9;
            BulkSendHelper bulkSend("ns3::TcpSocketFactory", InetSocketAddress(senderReceiverInterfaces.GetAddress(1), port));
            bulkSend.SetAttribute("MaxBytes", UintegerValue(0)); // Unlimited data
            ApplicationContainer apps = bulkSend.Install(sender);
            apps.Start(Seconds(1.0));
            apps.Stop(Seconds(simulationTime)); // Use the simulationTime parameter

            PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
            apps = sink.Install(receiver);
            apps.Start(Seconds(1.0));
            apps.Stop(Seconds(simulationTime)); // Use the simulationTime parameter
        }
    }

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
