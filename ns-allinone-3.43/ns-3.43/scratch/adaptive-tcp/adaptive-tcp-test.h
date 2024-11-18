#include <string>
#include <array>
#include "ns3/node-container.h"
#include "ns3/ipv4-address-helper.h"

using namespace ns3;

// Enum for CCA constants
enum CCA {
    BBRv1,
    BIC,
    CUBIC,
    HTCP,
    Illinois,
    NewReno,
    Vegas,
    Veno,
    CCA_COUNT
};

struct CCAData {
    int percentage;
    std::string tcpTypeId;
};

static const std::array<CCAData, CCA_COUNT> ccaData = {
    CCAData{17, "ns3::TcpBbr"},      // BBRv1
    CCAData{4, "ns3::TcpBic"},       // BIC
    CCAData{52, "ns3::TcpCubic"},    // CUBIC
    CCAData{4, "ns3::TcpHtcp"},      // HTCP
    CCAData{5, "ns3::TcpIllinois"},  // Illinois
    CCAData{12, "ns3::TcpNewReno"},  // NewReno
    CCAData{6, "ns3::TcpVegas"},     // Vegas
    CCAData{1, "ns3::TcpVeno"}       // Veno
};

struct FlowStats {
    std::string cca;
    std::vector<double> times;
    std::vector<double> throughputs;
    std::vector<uint32_t> cwnds;
    std::vector<double> rtts;
};

struct FlowData {
    Ptr<PacketSink> sink;  // Pointer to the PacketSink application
    std::string cca;       // Name of the CCA used
    Ptr<BulkSendApplication> app;
    FlowStats stats;
};

void setPairGoingThroughLink(ns3::Ptr<ns3::Node> sender,
                             ns3::NodeContainer& bottleneck,
                             ns3::Ptr<ns3::Node> receiver,
                             double simulationTime,
                             int senderIndex,
                             ns3::TypeId tcpTypeId,
                             std::vector<std::shared_ptr<FlowData>>& flowData);

static void
CwndTracer(FlowData* flow, uint32_t oldCwnd, uint32_t newCwnd);

static void
RttTracer(FlowData* flow, Time oldRtt, Time newRtt);

void
CalculateThroughput(FlowData* flow, double interval, double simulationTime);

void ConnectTraceSources(Ptr<Node> sender, FlowData* flow, int senderIndex);