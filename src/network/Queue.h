#pragma once

#include "Packet.h"
#include <queue>
#include <memory>

namespace qos_sim {

class Queue {
private:
    int max_size_packets;
    std::queue<std::shared_ptr<Packet>> buffer;

public:
    explicit Queue(int max_size);

    bool enqueue(std::shared_ptr<Packet> packet, double current_time);
    std::shared_ptr<Packet> dequeue(double current_time);
    
    int get_occupancy() const;
    bool is_full() const;
    bool is_empty() const;
    int get_max_size() const;
};

} // namespace qos_sim
