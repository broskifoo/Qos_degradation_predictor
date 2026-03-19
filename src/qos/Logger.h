#pragma once

#include "Metrics.h"
#include <string>
#include <fstream>

namespace qos_sim {

class Logger {
private:
    std::string dataset_filepath;
    std::ofstream file;

public:
    Logger(const std::string& filepath);
    ~Logger();
    
    void log_metrics(const MetricsTracker& metrics, double window_sec, 
                     double delay_thresh, double jitter_thresh, double loss_thresh);
};

} // namespace qos_sim
