#pragma once

#include <vector>
#include <map>

namespace qos_sim {

struct FlowMetrics {
    int total_sent = 0;
    int total_dropped = 0;
    double bytes_delivered = 0;
    
    double sum_rtt = 0;
    std::vector<double> rtt_history;
    
    double last_delay = 0;
    double sum_jitter = 0;
    int jitter_samples = 0;
};

class MetricsTracker {
private:
    std::map<int, FlowMetrics> flow_stats;
    int current_queue_occupancy = 0;
    
public:
    void record_sent(int flow_id);
    void record_drop(int flow_id);
    void record_delivery(int flow_id, int size_bytes, double rtt, double delay);
    void update_queue_occupancy(int size);

    // Global periodic metrics for the whole network over a window
    double get_avg_rtt() const;
    double get_rtt_variance() const;
    double get_queue_occupancy() const;
    double get_packet_loss() const;
    double get_throughput_bps(double window_sec) const;
    double get_jitter() const;
    
    void reset_window();
    
    // Helper to calculate QoS label (1 if degraded)
    int calculate_qos_label(double delay_thresh, double jitter_thresh, double loss_thresh) const;
};

} // namespace qos_sim
