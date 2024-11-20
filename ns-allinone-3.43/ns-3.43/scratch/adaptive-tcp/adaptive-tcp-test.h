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

Ptr<TcpCongestionOps> cca_ops[] = {
    CreateObject<TcpBbr>(),
    CreateObject<TcpBic>(),
    CreateObject<TcpCubic>(),
    CreateObject<TcpHtcp>(),
    CreateObject<TcpIllinois>(),
    CreateObject<TcpNewReno>(),
    CreateObject<TcpVegas>(),
    CreateObject<TcpVeno>()
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

template<typename DataType>
struct DataPoint {
    double time;
    DataType value;

    DataPoint(double t, DataType v) : time(t), value(v) {}
};

struct FlowStats {
    std::string cca;
    std::vector<DataPoint<uint32_t>> rtts, throughputs, cwnds, lastRtts, srtts, rtos,
                           congestionStates, bytesInFlights;
    std::vector<DataPoint<DataRate>> pacingRates;
};

struct FlowData {
    Ptr<PacketSink> sink;  // Pointer to the PacketSink application
    std::string cca;       // Name of the CCA used
    Ptr<BulkSendApplication> app;
    FlowStats stats;
    uint64_t lastTotalRx = 0;  // Add this field to store the last total received bytes
};

void
setAdaptiveTcpCca(std::shared_ptr<FlowData> adaptiveTcpFlow, CCA new_cca);

void setPairGoingThroughLink(ns3::Ptr<ns3::Node> sender,
                             ns3::NodeContainer& bottleneck,
                             ns3::Ptr<ns3::Node> receiver,
                             double simulationTime,
                             int senderIndex,
                             ns3::TypeId tcpTypeId,
                             std::vector<std::shared_ptr<FlowData>>& flowData);

void saveFlowDataToJson(std::__1::vector<std::__1::shared_ptr<FlowData>>& flowData, std::string outputFileName);

static void
CwndTracer(FlowData* flow, uint32_t oldCwnd, uint32_t newCwnd);

static void
RttTracer(FlowData* flow, Time oldRtt, Time newRtt);

static void
LastRttTracer(FlowData* flow, Time oldLastRtt, Time newLastRtt);

static void
RtoTracer(FlowData* flow, Time oldRto, Time newRto);

static void
CongestionStateTracer(FlowData* flow, TcpSocketState::TcpCongState_t oldState, TcpSocketState::TcpCongState_t newState);

static void
BytesInFlightTracer(FlowData* flow, uint32_t oldBytesInFlight, uint32_t newBytesInFlight);

static void
PacingRateTracer(FlowData* flow, DataRate oldPacingRate, DataRate newPacingRate);

void
CalculateThroughput(FlowData* flow, double interval, double simulationTime);

void ConnectTraceSources(Ptr<Node> sender, FlowData* flow, int senderIndex);