#include "Link.h"
#include <algorithm>

namespace qos_sim {

Link::Link(double bandwidth_mbps, double prop_delay_ms)
    : bandwidth_bps(bandwidth_mbps * 1'000'000.0),
      propagation_delay_ms(prop_delay_ms),
      next_available_time_ms(0.0) {}

bool Link::can_transmit(double current_time_ms) const {
    return current_time_ms >= next_available_time_ms;
}

double Link::get_transmission_delay_ms(int size_bytes) const {
    // delay = size / bandwidth. bits / (bits/sec) = sec. * 1000 = ms
    return (size_bytes * 8.0 / bandwidth_bps) * 1000.0;
}

double Link::transmit(std::shared_ptr<Packet> packet, double current_time_ms) {
    double tx_delay_ms = get_transmission_delay_ms(packet->size_bytes);
    
    // The link might be idle, so transmission starts at max(current_time_ms, next_available_time_ms)
    double start_tx_time = std::max(current_time_ms, next_available_time_ms);
    
    // Update when link will be available again
    next_available_time_ms = start_tx_time + tx_delay_ms;
    
    // Packet arrival time = start_tx_time + tx_delay_ms + prop_delay_ms
    double arrival_time_ms = next_available_time_ms + propagation_delay_ms;
    packet->arrival_time = arrival_time_ms;
    
    return arrival_time_ms;
}

} // namespace qos_sim
