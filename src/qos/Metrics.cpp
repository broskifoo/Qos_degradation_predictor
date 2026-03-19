#include "Metrics.h"
#include <cmath>
#include <numeric>

namespace qos_sim {

void MetricsTracker::record_sent(int flow_id) {
    flow_stats[flow_id].total_sent++;
}

void MetricsTracker::record_drop(int flow_id) {
    flow_stats[flow_id].total_dropped++;
}

void MetricsTracker::record_delivery(int flow_id, int size_bytes, double rtt, double delay) {
    auto& stats = flow_stats[flow_id];
    stats.bytes_delivered += size_bytes;
    stats.sum_rtt += rtt;
    stats.rtt_history.push_back(rtt);
    
    // Jitter J = |D_i - D_{i-1}|
    if (stats.last_delay > 0) {
        stats.sum_jitter += std::abs(delay - stats.last_delay);
        stats.jitter_samples++;
    }
    stats.last_delay = delay;
}

void MetricsTracker::update_queue_occupancy(int size) {
    current_queue_occupancy = size;
}

double MetricsTracker::get_avg_rtt() const {
    double total_sum = 0;
    int total_count = 0;
    for (const auto& [id, stats] : flow_stats) {
        total_sum += stats.sum_rtt;
        total_count += stats.rtt_history.size();
    }
    return total_count > 0 ? total_sum / total_count : 0.0;
}

double MetricsTracker::get_rtt_variance() const {
    double avg = get_avg_rtt();
    double sum_sq_diff = 0;
    int total_count = 0;
    
    for (const auto& [id, stats] : flow_stats) {
        for (double rtt : stats.rtt_history) {
            double diff = rtt - avg;
            sum_sq_diff += diff * diff;
            total_count++;
        }
    }
    return total_count > 1 ? sum_sq_diff / (total_count - 1) : 0.0;
}

double MetricsTracker::get_queue_occupancy() const {
    return current_queue_occupancy;
}

double MetricsTracker::get_packet_loss() const {
    int total_sent = 0;
    int total_dropped = 0;
    for (const auto& [id, stats] : flow_stats) {
        total_sent += stats.total_sent;
        total_dropped += stats.total_dropped;
    }
    return total_sent > 0 ? static_cast<double>(total_dropped) / total_sent : 0.0;
}

double MetricsTracker::get_throughput_bps(double window_sec) const {
    double total_bytes = 0;
    for (const auto& [id, stats] : flow_stats) {
        total_bytes += stats.bytes_delivered;
    }
    return window_sec > 0 ? (total_bytes * 8.0) / window_sec : 0.0;
}

double MetricsTracker::get_jitter() const {
    double total_sum = 0;
    int total_samples = 0;
    for (const auto& [id, stats] : flow_stats) {
        total_sum += stats.sum_jitter;
        total_samples += stats.jitter_samples;
    }
    return total_samples > 0 ? total_sum / total_samples : 0.0;
}

void MetricsTracker::reset_window() {
    for (auto& [id, stats] : flow_stats) {
        stats.total_sent = 0;
        stats.total_dropped = 0;
        stats.bytes_delivered = 0;
        stats.sum_rtt = 0;
        stats.rtt_history.clear();
        stats.sum_jitter = 0;
        stats.jitter_samples = 0;
        // Don't reset last_delay to preserve jitter calculation continuity
    }
}

int MetricsTracker::calculate_qos_label(double delay_thresh, double jitter_thresh, double loss_thresh) const {
    bool degraded = (get_avg_rtt() > delay_thresh) || 
                    (get_jitter() > jitter_thresh) || 
                    (get_packet_loss() > loss_thresh);
    return degraded ? 1 : 0;
}

} // namespace qos_sim
