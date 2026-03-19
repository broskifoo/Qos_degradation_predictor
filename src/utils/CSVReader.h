#pragma once

#include <string>
#include <vector>

namespace qos_sim {

struct DataPoint {
    double avg_rtt;
    double rtt_variance;
    double queue_occupancy;
    double packet_loss;
    double throughput;
    double jitter;
    int qos_label;
};

class CSVReader {
public:
    static std::vector<DataPoint> read_dataset(const std::string& filepath);
};

} // namespace qos_sim
