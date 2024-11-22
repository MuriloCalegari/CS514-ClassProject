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
#include "lib/json.h"
// #include "ns3/mpi-interface.h"

#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdaptiveTcpTest");

// void delayChanger(PointToPointHelper *bottleneckLink) {
//     bottleneckLink->SetDeviceAttribute("DataRate", StringValue("10Mbps"));
//     bottleneckLink->SetChannelAttribute("Delay", StringValue("2ms"));

//     DataRateValue dataRateValue;
//     bottleneckLink->GetDeviceAttribute("DataRate", dataRateValue);

//     // Convert to DataRate object
//     DataRate dataRate = dataRateValue.Get();

//     // Print the bandwidth
//     std::cout << "BOTTLENECK LINK BANDWIDTH: " << dataRate.GetBitRate() << " bps" << std::endl;
// }

int
main(int argc, char* argv[])
{
    // // Initialize MPI
    // MpiInterface::Enable(&argc, &argv);

    // LogComponentEnable("TcpCubic", LOG_LEVEL_DEBUG);

    LogComponentEnable("AdaptiveTcpTest", LOG_LEVEL_DEBUG);
    // LogComponentEnable("AdaptiveTcp", LOG_LEVEL_INFO);
    LogComponentEnable("AdaptiveTcp", LOG_LEVEL_WARN);

    std::string linkBandwidth = "1000Mbps"; // Default to 1Gbps
    double simulationTime = 60.0; // Default to 1 minutes
    double senderCount = 7; // Default to 8 senders
    std::string bottleneckDelay = "2ms"; // Default to 2ms
    std::string buffer = "50p"; // Default to 50 packets
    std::string outputFilename = "";

    CommandLine cmd;
    cmd.AddValue("linkBandwidth", "Bandwidth of the middle link", linkBandwidth);
    cmd.AddValue("simulationTime", "Simulation runtime in seconds", simulationTime);
    cmd.AddValue("senderCount", "Number of senders", senderCount);
    cmd.AddValue("delay", "Delay time of bottleneck link", bottleneckDelay);
    cmd.AddValue("buffer", "Buffer size in packets", buffer);
    cmd.AddValue("output", "Output file name", outputFilename);
    cmd.Parse(argc, argv);


    // If we didn't specify an output filename, stitch together our own
    if (outputFilename == "") {
        outputFilename = linkBandwidth + "-" + bottleneckDelay + "-" + buffer;
    }

    // Create sender, receiver, and bottleneck nodes
    // Create nodes
    NS_LOG_INFO("Creating nodes.");
    NodeContainer senders, bottleneck, receivers;
    senders.Create(senderCount + 1);
    bottleneck.Create(2);  // bottleneck.Get(0) is left, bottleneck.Get(1) is right
    receivers.Create(senderCount + 1);

    // Connect each sender to the bottleneck and each receiver to the bottleneck
    InternetStackHelper stack;
    stack.Install(senders);
    stack.Install(receivers);
    stack.Install(bottleneck);

    // Set up the throttled link between bottleneck nodes
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue(linkBandwidth));
    // TODO parameterize the delay
    bottleneckLink.SetChannelAttribute("Delay", StringValue(bottleneckDelay));
    bottleneckLink.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(buffer)); // Example: 50 packets

    NetDeviceContainer bottleneckDevices = bottleneckLink.Install(bottleneck.Get(0), bottleneck.Get(1));

    // Assign IPs for the bottleneck link
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    
    Ipv4InterfaceContainer bottleneckInterfaces = address.Assign(bottleneckDevices);

    int senderIndex = 0; // Index to keep track of sender-receiver pairs
    
    std::vector<std::shared_ptr<FlowData>> flowData;
    
    // Install the competing congestion control algorithms    
    for (int i = 0; i < CCA_COUNT; i++) {
        // int numSenders = ccaData[i].percentage / 100.0 * senderCount;
        int numSenders = 1;

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
                                    TypeId::LookupByName(ccaData[i].tcpTypeId),
                                    flowData,
                                    false);

            senderIndex++;  // Move to the next sender-receiver pair
        }
    }

    // Add our custom CCA, AdaptiveTCP
    setPairGoingThroughLink(senders.Get(senderIndex),
                            bottleneck,
                            receivers.Get(senderIndex),
                            simulationTime,
                            senderIndex,
                            TypeId::LookupByName("ns3::TcpCubic"),
                            flowData, true);

    // Store our AdaptiveTCPs flow data
    auto adaptiveTcpFlow = flowData.back();

    NS_LOG_INFO("Initialize Global Routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Enable NetAnim
    // AnimationInterface anim("adaptive-tcp-test.xml");
    
    NS_LOG_INFO("Starting the simulation with runtime of " << simulationTime << "s...");
    Simulator::Stop(Seconds(simulationTime + 5));

    ShowProgress progress (Seconds (5), std::cerr);

    // Simulator::Schedule(
    //     Seconds(15),
    //     &setAdaptiveTcpCca,
    //     adaptiveTcpFlow,
    //     CCA::Vegas
    // );

    // let's change the delay after 15s
    // Simulator::Schedule(
    //     Seconds(5),
    //     &delayChanger,
    //     &bottleneckLink
    // );

    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Simulation completed.");

    // Compute throughput for each flow
    // for (const auto& fd : flowData) {
    //     double bytesReceived = fd->sink->GetTotalRx();
    //     double throughput = (bytesReceived * 8) / ((simulationTime - 1) * 1048576); // Throughput in Mbps
    //     NS_LOG_INFO("CCA: " << fd->cca << ", Throughput: " << throughput << " Mbps");
    // }

    for (const auto& fd : flowData) {
        double bytesReceived = fd->sink->GetTotalRx();
        unsigned long long sum = 0;
        for (const auto &val : fd->stats.throughputs) {
            sum += val.value;
        }

        // Throughput in Mbps
        long double throughput = sum / (int) fd->stats.throughputs.size() / 1e3;

        // double throughput = (bytesReceived * 8) / (simulationTime * 1e6); // Throughput in Mbps
        NS_LOG_INFO("CCA: " << fd->cca << ", Throughput: " << throughput << " Mbps");
    }

    saveFlowDataToJson(flowData, outputFilename);

    // // Finalize MPI
    // MpiInterface::Disable();

    return 0;
}

// Change the AdaptiveTcp's CCA algorithm to an input one
void
setAdaptiveTcpCca(std::shared_ptr<FlowData> adaptiveTcpFlow, CCA new_cca) {
    Ptr<Socket> tcpSocket = adaptiveTcpFlow->app->GetSocket();
    NS_ASSERT_MSG(tcpSocket, "TcpSocket not found");

    // Cast the Socket to TcpSocketBase
    Ptr<TcpSocketBase> tcpSocketBase = DynamicCast<TcpSocketBase>(tcpSocket);
    NS_ASSERT_MSG(tcpSocketBase, "TcpSocketBase not found");

    // Set the congestion control algorithm on the socket
    tcpSocketBase->SetCongestionControlAlgorithm(cca_ops[new_cca]);
}

void
saveFlowDataToJson(std::vector<std::shared_ptr<FlowData>>& flowData, std::string outputFileName)
{
    // Serialize the information in flowdata to a json file using the nlohmann json library
    nlohmann::json j;
    for (const auto& fd : flowData)
    {
        nlohmann::json flow;
        flow["cca"] = fd->cca;

        for (const auto& dp : fd->stats.throughputs)
        {
            flow["throughputs"].push_back({dp.time, dp.value});
        }

        for (const auto& dp : fd->stats.cwnds)
        {
            flow["cwnds"].push_back({dp.time, dp.value});
        }

        for (const auto& dp : fd->stats.rtts)
        {
            flow["rtts"].push_back({dp.time, dp.value});
        }

        for (const auto& dp : fd->stats.lastRtts)
        {
            flow["lastRtts"].push_back({dp.time, dp.value});
        }

        for (const auto& dp : fd->stats.rtos)
        {
            flow["rtos"].push_back({dp.time, dp.value});
        }

        for (const auto& dp : fd->stats.congestionStates)
        {
            flow["congestionStates"].push_back({dp.time, dp.value});
        }

        for (const auto& dp : fd->stats.bytesInFlights)
        {
            flow["bytesInFlights"].push_back({dp.time, dp.value});
        }

        for (const auto& dp : fd->stats.pacingRates)
        {
            flow["pacingRates"].push_back({dp.time, dp.value.GetBitRate()});
        }

        j.push_back(flow);
    }

    std::string final_out = outputFileName + ".json";

    std::ofstream o(final_out);
    o << std::setw(4) << j << std::endl;
}

// Every 5 seconds, sample the throughput
void sampling_switcher() {
    
}

void
setPairGoingThroughLink(ns3::Ptr<ns3::Node> sender,
                        ns3::NodeContainer& bottleneck,
                        ns3::Ptr<ns3::Node> receiver,
                        double simulationTime,
                        int senderIndex,
                        ns3::TypeId tcpTypeId,
                        std::vector<std::shared_ptr<FlowData>>& flowData,
                        bool isAdaptiveTcp)
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
    Config::Set("/NodeList/" + std::to_string(sender->GetId()) + "/$ns3::TcpL4Protocol/SocketType",
                TypeIdValue(tcpTypeId));

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

    // Get a pointer to the PacketSink application
    Ptr<PacketSink> pktSink = DynamicCast<PacketSink>(receiverApps.Get(0));

    // Store flow data
    auto flow = std::make_shared<FlowData>();
    flow->sink = pktSink;
    flow->cca = (isAdaptiveTcp ? "ns3::TcpCubic" : tcpTypeId.GetName());
    flow->app = DynamicCast<BulkSendApplication>(senderApps.Get(0));
    NS_ASSERT_MSG(flow->app, "BulkSendApplication not found");

    // Schedule throughput calculation
    double interval = 1; // Interval in seconds
    Simulator::Schedule(Seconds(0), &CalculateThroughput, flow.get(), interval, simulationTime);

    // **Connect the cwnd and RTT trace sources**
    // ConnectTraceSources(sender, &flow, senderIndex);
    Simulator::Schedule(Seconds(0), &ConnectTraceSources, sender, flow.get(), senderIndex);

    // Store the flow data
    flowData.push_back(flow);
}

void ConnectTraceSources(Ptr<Node> sender, FlowData* flow, int senderIndex)
{
    // Construct the path to the socket used by the BulkSendApplication
    NS_LOG_DEBUG("Connecting trace sources for flow " << senderIndex);
    Ptr<Socket> socket = flow->app->GetSocket();

    // If socket is null, reschedule for one second later
    if (socket == nullptr) {
        Simulator::Schedule(Seconds(1.0), &ConnectTraceSources, sender, flow, senderIndex);
        return;
    }

    std::string path = "/NodeList/" + std::to_string(sender->GetId()) +
                       "/$ns3::TcpL4Protocol/SocketList/0";

    // Connect to the CongestionWindow and RTT trace sources
    Config::ConnectWithoutContext(path + "/CongestionWindow",
                                  MakeBoundCallback(&CwndTracer, flow));
    Config::ConnectWithoutContext(path + "/RTT",
                                  MakeBoundCallback(&RttTracer, flow));
    Config::ConnectWithoutContext(path + "/LastRTT",
                                  MakeBoundCallback(&LastRttTracer, flow));
    Config::ConnectWithoutContext(path + "/RTO",
                                  MakeBoundCallback(&RtoTracer, flow));
    Config::ConnectWithoutContext(path + "/CongState",
                                  MakeBoundCallback(&CongestionStateTracer, flow));
    Config::ConnectWithoutContext(path + "/BytesInFlight",
                                  MakeBoundCallback(&BytesInFlightTracer, flow));
    Config::ConnectWithoutContext(path + "/PacingRate",
                                    MakeBoundCallback(&PacingRateTracer, flow));
}

static void
CwndTracer(FlowData* flow, uint32_t oldCwnd, uint32_t newCwnd)
{
    Time now = Simulator::Now();
    flow->stats.cwnds.push_back({now.GetSeconds(), newCwnd});
}

void
RttTracer(FlowData* flow, Time oldRtt, Time newRtt)
{
    Time now = Simulator::Now();
    flow->stats.rtts.push_back({now.GetSeconds(), static_cast<uint32_t>(newRtt.GetMilliSeconds())});
}

void
LastRttTracer(FlowData* flow, Time oldLastRtt, Time newLastRtt)
{
    Time now = Simulator::Now();
    flow->stats.lastRtts.push_back({now.GetSeconds(), static_cast<uint32_t>(newLastRtt.GetMilliSeconds())});
}

void
RtoTracer(FlowData* flow, Time oldRto, Time newRto)
{
    Time now = Simulator::Now();
    flow->stats.rtos.push_back({now.GetSeconds(), static_cast<uint32_t>(newRto.GetMilliSeconds())});
}

void
CongestionStateTracer(FlowData* flow, TcpSocketState::TcpCongState_t oldState, TcpSocketState::TcpCongState_t newState)
{
    Time now = Simulator::Now();
    flow->stats.congestionStates.push_back({now.GetSeconds(), static_cast<uint32_t>(newState)});
}

void
BytesInFlightTracer(FlowData* flow, uint32_t oldBytesInFlight, uint32_t newBytesInFlight)
{
    Time now = Simulator::Now();
    flow->stats.bytesInFlights.push_back({now.GetSeconds(), newBytesInFlight});
}

void
PacingRateTracer(FlowData* flow, DataRate oldPacingRate, DataRate newPacingRate)
{
    Time now = Simulator::Now();
    flow->stats.pacingRates.push_back({now.GetSeconds(), newPacingRate});
}

void
CalculateThroughput(FlowData* flow, double interval, double simulationTime)
{
    Time now = Simulator::Now();
    uint64_t totalBytes = flow->sink->GetTotalRx();
    double throughput = (totalBytes - flow->lastTotalRx) * 8 / (interval * 1e3); // Kbps

    // Collect data
    flow->stats.throughputs.push_back({now.GetSeconds(), static_cast<uint32_t>(throughput)});

    // std::cout << static_cast<uint32_t>(throughput) << "\n";

    flow->lastTotalRx = totalBytes;

    // Schedule next throughput calculation
    if (now.GetSeconds() < simulationTime) {
        Simulator::Schedule(Seconds(interval), &CalculateThroughput, flow, interval, simulationTime);
    }
}