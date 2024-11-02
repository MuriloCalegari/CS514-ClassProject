#include <string>
#include <array>

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

// static const std::array<CCAData, CCA_COUNT> ccaData = {
//     CCAData{0, "ns3::TcpBbr"},      // BBRv1
//     CCAData{0, "ns3::TcpBic"},       // BIC
//     CCAData{1, "ns3::TcpCubic"},    // CUBIC
//     CCAData{0, "ns3::TcpHtcp"},      // HTCP
//     CCAData{0, "ns3::TcpIllinois"},  // Illinois
//     CCAData{0, "ns3::TcpNewReno"},  // NewReno
//     CCAData{6, "ns3::TcpVegas"},     // Vegas
//     CCAData{1, "ns3::TcpVeno"}       // Veno
// };