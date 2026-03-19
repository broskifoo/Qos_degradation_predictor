#pragma once

#include <string>

namespace qos_sim {

struct Config {
    int simulation_time_sec = 300;
    int packet_size_bytes = 1024;
    double bandwidth_mbps = 10.0;
    int queue_size_packets = 500;
    double propagation_delay_ms = 10.0;
    double realtime_rate_mbps = 1.0;
    int tcp_flows = 3;
    double learning_rate = 0.01;
    int epochs = 1000;

    static Config load(const std::string& filepath);
};

} // namespace qos_sim
