#include "RealTimeFlow.h"

namespace qos_sim {

RealTimeFlow::RealTimeFlow(int f_id, int p_size, double r_mbps)
    : TrafficGenerator(f_id, p_size), rate_mbps(r_mbps), last_tx_time_ms(0) {}

std::vector<std::shared_ptr<Packet>> RealTimeFlow::generate(double current_time_ms) {
    std::vector<std::shared_ptr<Packet>> packets;
    
    // target packets per ms = (rate_mbps * 1e6 / 8) / size / 1000
    double pkts_per_sec = (rate_mbps * 1'000'000.0) / (packet_size_bytes * 8.0);
    double pkts_per_ms = pkts_per_sec / 1000.0;
    
    double elapsed = current_time_ms - last_tx_time_ms;
    double packets_to_send = pkts_per_ms * elapsed;
    
    if (packets_to_send >= 1.0) {
        int count = static_cast<int>(packets_to_send);
        for (int i = 0; i < count; i++) {
            packets.push_back(std::make_shared<Packet>(
                packet_id_counter++, flow_id, FlowType::REAL_TIME, packet_size_bytes, current_time_ms));
        }
        // Advance time by the exact fraction to avoid rounding drift
        last_tx_time_ms += (count / pkts_per_ms);
    }
    
    return packets;
}

void RealTimeFlow::process_feedback(bool is_loss, double rtt_ms) {
    // CBR doesn't adapt to feedback
    (void)is_loss;
    (void)rtt_ms;
}

} // namespace qos_sim
