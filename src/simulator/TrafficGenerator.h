#pragma once

#include "../network/Packet.h"
#include <vector>
#include <memory>

namespace qos_sim {

class TrafficGenerator {
protected:
    int flow_id;
    int packet_size_bytes;
    int packet_id_counter;

public:
    TrafficGenerator(int f_id, int p_size) : flow_id(f_id), packet_size_bytes(p_size), packet_id_counter(0) {}
    virtual ~TrafficGenerator() = default;

    // Generates packets for the current ms
    virtual std::vector<std::shared_ptr<Packet>> generate(double current_time_ms) = 0;
    
    // Process an acknowledgement or loss event
    virtual void process_feedback(bool is_loss, double rtt_ms) = 0;
    
    int get_flow_id() const { return flow_id; }
};

} // namespace qos_sim
