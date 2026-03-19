#include "Config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace qos_sim {

// A simple manual parser for the specific JSON format expected to avoid external libraries
Config Config::load(const std::string& filepath) {
    Config config;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file " << filepath << " - using defaults.\n";
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove spaces and quotes for easier parsing
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        line.erase(std::remove(line.begin(), line.end(), '"'), line.end());

        if (line.empty() || line == "{" || line == "}") continue;
        
        // Remove trailing comma if exists
        if (line.back() == ',') {
            line.pop_back();
        }

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value_str = line.substr(colon_pos + 1);

            try {
                if (key == "simulation_time_sec") config.simulation_time_sec = std::stoi(value_str);
                else if (key == "packet_size_bytes") config.packet_size_bytes = std::stoi(value_str);
                else if (key == "bandwidth_mbps") config.bandwidth_mbps = std::stod(value_str);
                else if (key == "queue_size_packets") config.queue_size_packets = std::stoi(value_str);
                else if (key == "propagation_delay_ms") config.propagation_delay_ms = std::stod(value_str);
                else if (key == "realtime_rate_mbps") config.realtime_rate_mbps = std::stod(value_str);
                else if (key == "tcp_flows") config.tcp_flows = std::stoi(value_str);
                else if (key == "learning_rate") config.learning_rate = std::stod(value_str);
                else if (key == "epochs") config.epochs = std::stoi(value_str);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Error parsing key " << key << " from config. using default value.\n";
            }
        }
    }

    return config;
}

} // namespace qos_sim
