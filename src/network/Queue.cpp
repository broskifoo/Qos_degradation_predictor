#include "Queue.h"

namespace qos_sim {

Queue::Queue(int max_size) : max_size_packets(max_size) {}

bool Queue::enqueue(std::shared_ptr<Packet> packet, double current_time) {
    if (is_full()) {
        packet->is_dropped = true;
        return false;
    }
    
    packet->enqueue_time = current_time;
    buffer.push(packet);
    return true;
}

std::shared_ptr<Packet> Queue::dequeue(double current_time) {
    if (is_empty()) return nullptr;
    
    auto packet = buffer.front();
    buffer.pop();
    packet->dequeue_time = current_time;
    return packet;
}

int Queue::get_occupancy() const {
    return static_cast<int>(buffer.size());
}

bool Queue::is_full() const {
    return static_cast<int>(buffer.size()) >= max_size_packets;
}

bool Queue::is_empty() const {
    return buffer.empty();
}

int Queue::get_max_size() const {
    return max_size_packets;
}

} // namespace qos_sim
