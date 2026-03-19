#pragma once

#include "TrafficGenerator.h"

namespace qos_sim {

class TCPFlow : public TrafficGenerator {
private:
    double window_size;
    double rtt_estimate_ms;
    double last_tx_time_ms;
    
    // bytes sent in the current window interval
    double bytes_sent_in_rtt;

public:
    TCPFlow(int f_id, int p_size);

    std::vector<std::shared_ptr<Packet>> generate(double current_time_ms) override;
    void process_feedback(bool is_loss, double rtt_ms) override;
};

} // namespace qos_sim
