#include "TCPFlow.h"

namespace qos_sim {

TCPFlow::TCPFlow(int f_id, int p_size) 
    : TrafficGenerator(f_id, p_size), window_size(1.0), rtt_estimate_ms(100.0), 
      last_tx_time_ms(0), bytes_sent_in_rtt(0) {}

std::vector<std::shared_ptr<Packet>> TCPFlow::generate(double current_time_ms) {
    std::vector<std::shared_ptr<Packet>> packets;
    
    // R = W / RTT (Packets per ms)
    double sending_rate_pkts_per_ms = window_size / rtt_estimate_ms;
    
    double elapsed = current_time_ms - last_tx_time_ms;
    
    // Simple logic: we accumulate virtual packets and send integer amounts
    double packets_to_send = sending_rate_pkts_per_ms * elapsed;
    
    if (packets_to_send >= 1.0) {
        int count = static_cast<int>(packets_to_send);
        for (int i = 0; i < count; i++) {
            packets.push_back(std::make_shared<Packet>(
                packet_id_counter++, flow_id, FlowType::TCP, packet_size_bytes, current_time_ms));
            bytes_sent_in_rtt += packet_size_bytes;
        }
        last_tx_time_ms = current_time_ms;
    }
    
    return packets;
}

void TCPFlow::process_feedback(bool is_loss, double rtt_ms) {
    if (rtt_ms > 0) {
        // simple Exponential Moving Average for RTT
        rtt_estimate_ms = 0.8 * rtt_estimate_ms + 0.2 * rtt_ms;
    }
    
    if (is_loss) {
        // AIMD: Multiplicative Decrease
        window_size = 0.5 * window_size;
        if (window_size < 1.0) window_size = 1.0;
    } else {
        // AIMD: Additive Increase (1 packet per RTT)
        // This feedback is called per packet, so we increase W by 1/W
        window_size += 1.0 / window_size;
    }
}

} // namespace qos_sim
