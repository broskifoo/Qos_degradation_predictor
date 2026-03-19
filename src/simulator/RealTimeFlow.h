#pragma once

#include "TrafficGenerator.h"

namespace qos_sim {

class RealTimeFlow : public TrafficGenerator {
private:
    double rate_mbps;
    double last_tx_time_ms;

public:
    RealTimeFlow(int f_id, int p_size, double r_mbps);

    std::vector<std::shared_ptr<Packet>> generate(double current_time_ms) override;
    void process_feedback(bool is_loss, double rtt_ms) override;
};

} // namespace qos_sim
