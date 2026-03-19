#pragma once

namespace qos_sim {

enum class FlowType {
    TCP,
    REAL_TIME
};

struct Packet {
    int id;
    int flow_id;
    FlowType type;
    int size_bytes;
    
    // Timestamps for delay calculations (in ms)
    double generation_time;
    double enqueue_time;
    double dequeue_time;
    double arrival_time;
    
    bool is_dropped;
    
    Packet(int cur_id, int f_id, FlowType f_type, int size, double gen_time);
};

} // namespace qos_sim
