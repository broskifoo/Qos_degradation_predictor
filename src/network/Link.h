#pragma once

#include "Packet.h"
#include <memory>

namespace qos_sim {

class Link {
private:
    double bandwidth_bps; // bits per second
    double propagation_delay_ms;
    double next_available_time_ms; // When the link finishes transmitting the current packet

public:
    Link(double bandwidth_mbps, double prop_delay_ms);

    // Determines if link can start transmitting a new packet at current_time_ms
    bool can_transmit(double current_time_ms) const;
    
    // Transmit a packet, updates next_available_time_ms and returns the time the packet will arrive
    double transmit(std::shared_ptr<Packet> packet, double current_time_ms);
    
    double get_transmission_delay_ms(int size_bytes) const;
    double get_bandwidth_bps() const { return bandwidth_bps; }
    double get_propagation_delay_ms() const { return propagation_delay_ms; }
};

} // namespace qos_sim
