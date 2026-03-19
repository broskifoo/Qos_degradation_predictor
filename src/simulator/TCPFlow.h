#pragma once

#include "TrafficGenerator.h"
#include <string>

namespace qos_sim {

class TCPFlow : public TrafficGenerator {
private:
    double window_size;
    double rtt_estimate_ms;
    double last_tx_time_ms;
    
    // bytes sent in the current window interval
    double bytes_sent_in_rtt;

    // UI & Logging Variables
    double current_cwnd;
    std::string control_mode;

public:
    TCPFlow(int f_id, int p_size);

    std::vector<std::shared_ptr<Packet>> generate(double current_time_ms) override;
    void process_feedback(bool is_loss, double rtt_ms) override;
    
    // Cross-Layer Feedback Intercept
    double get_cwnd() const { return current_cwnd; }
    std::string get_control_mode() const { return control_mode; }
    void apply_cross_layer_feedback(double jitter, double rtt_ms_var, double queue_occupancy);
};

} // namespace qos_sim
