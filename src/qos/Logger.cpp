#include "Logger.h"
#include <iostream>

namespace qos_sim {

Logger::Logger(const std::string& filepath) : dataset_filepath(filepath) {
    file.open(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open dataset log file " << filepath << "\n";
    } else {
        file << "avg_rtt,rtt_variance,queue_occupancy,packet_loss,throughput,jitter,qos_label\n";
    }
}

Logger::~Logger() {
    if (file.is_open()) {
        file.close();
    }
}

void Logger::log_metrics(const MetricsTracker& metrics, double window_sec, 
                         double delay_thresh, double jitter_thresh, double loss_thresh) {
    if (!file.is_open()) return;

    double avg_rtt = metrics.get_avg_rtt();
    double rtt_variance = metrics.get_rtt_variance();
    double queue_occupancy = metrics.get_queue_occupancy();
    double packet_loss = metrics.get_packet_loss();
    double throughput = metrics.get_throughput_bps(window_sec);
    double jitter = metrics.get_jitter();
    int label = metrics.calculate_qos_label(delay_thresh, jitter_thresh, loss_thresh);
    
    file << avg_rtt << ","
         << rtt_variance << ","
         << queue_occupancy << ","
         << packet_loss << ","
         << throughput << ","
         << jitter << ","
         << label << "\n";
}

} // namespace qos_sim
