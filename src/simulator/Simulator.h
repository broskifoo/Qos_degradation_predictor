#pragma once

#include "../utils/Config.h"
#include "../network/Queue.h"
#include "../network/Link.h"
#include "../qos/Metrics.h"
#include "../qos/Logger.h"
#include "TrafficGenerator.h"
#include "../visualization/Dashboard.h"
#include <vector>
#include <memory>
#include <list>

namespace qos_sim {

class Simulator {
private:
    Config config;
    double current_time_ms;
    
    struct Path {
        std::unique_ptr<Queue> queue;
        std::unique_ptr<Link> link;
        double current_delay_estimate;
    };
    std::vector<Path> paths;
    
    // UI tracking variables
    int active_path_id = 0;
    int path_switching_events = 0;
    std::vector<double> per_path_delay;
    
    // Anomaly Detection UI Variables
    bool anomaly_flag = false;
    std::string anomaly_type = "NONE";
    double anomaly_severity = 0.0;
    
    // UI Comparison Toggle Mode
    bool is_enhanced_mode = true;
    
    MetricsTracker metrics;
    std::unique_ptr<Logger> logger;
    
    std::vector<std::unique_ptr<TrafficGenerator>> flows;
    
    // Simulating packet arrival queue at destination
    std::list<std::shared_ptr<Packet>> in_flight_packets;
    
    std::unique_ptr<Dashboard> dashboard; // New visual dashboard

public:
    Simulator(const Config& sim_config, const std::string& dataset_file);
    
    void run();

    // Returns a pointer to the existing dashboard window (for reuse after simulation ends)
    Dashboard* get_dashboard() { return dashboard.get(); }

private:
    void generate_traffic();
    void process_queue_and_link();
    void process_arrivals();
};

} // namespace qos_sim

