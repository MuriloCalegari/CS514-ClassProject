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

struct FlowData {
    Ptr<PacketSink> sink;  // Pointer to the PacketSink application
    std::string cca;       // Name of the CCA used
};

void setPairGoingThroughLink(ns3::Ptr<ns3::Node> sender,
                             ns3::NodeContainer& bottleneck,
                             ns3::Ptr<ns3::Node> receiver,
                             double simulationTime,
                             int senderIndex,
                             ns3::TypeId tcpTypeId,
                             std::vector<FlowData>& flowData);