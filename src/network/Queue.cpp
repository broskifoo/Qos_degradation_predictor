#include "Queue.h"

namespace qos_sim {

Queue::Queue(int max_size) 
    : max_size_packets(max_size),
      avg_queue_size(0.0), ewma_weight(0.002), 
      min_th(max_size * 0.2), max_th(max_size * 0.8), max_p(0.1),
      target_delay_ms(5.0), interval_ms(100.0), first_above_time(-1.0),
      rng(std::random_device{}()), dist(0.0, 1.0) {}

bool Queue::enqueue(std::shared_ptr<Packet> packet, double current_time, bool ml_predicts_degraded) {
    if (is_full()) {
        packet->is_dropped = true;
        return false;
    }
    
    // RED (Random Early Detection) Phase
    avg_queue_size = (1.0 - ewma_weight) * avg_queue_size + ewma_weight * buffer.size();
    
    double active_max_th = ml_predicts_degraded ? max_th * 0.75 : max_th;
    double active_max_p  = ml_predicts_degraded ? 0.3 : max_p;
    
    if (avg_queue_size > active_max_th) {
        packet->is_dropped = true;
        return false; // RED Max Drop
    }
    
    if (avg_queue_size > min_th) {
        double pb = active_max_p * (avg_queue_size - min_th) / (active_max_th - min_th);
        if (dist(rng) < pb) {
            packet->is_dropped = true;
            return false; // RED Probabilistic Drop
        }
    }

    packet->enqueue_time = current_time;
    buffer.push(packet);
    return true;
}

std::shared_ptr<Packet> Queue::dequeue(double current_time) {
    while (!buffer.empty()) {
        auto packet = buffer.front();
        buffer.pop();
        
        // CoDel (Controlled Delay) Phase
        double sojourn_time = current_time - packet->enqueue_time;
        if (sojourn_time > target_delay_ms) {
            if (first_above_time < 0) {
                first_above_time = current_time;
            } else if (current_time - first_above_time > interval_ms) {
                // CoDel Drop: packet took too long in queue, drop and find next
                packet->is_dropped = true;
                continue; 
            }
        } else {
            first_above_time = -1.0; 
        }
        
        packet->dequeue_time = current_time;
        return packet;
    }
    return nullptr;
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
