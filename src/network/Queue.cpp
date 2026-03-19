#include "Queue.h"

namespace qos_sim {

Queue::Queue(int max_size) 
    : max_size_packets(max_size),
      avg_queue_size(0.0), ewma_weight(0.002), 
      min_th(max_size * 0.2), max_th(max_size * 0.8), max_p(0.1),
      target_delay_ms(5.0), interval_ms(100.0), first_above_time(-1.0),
      current_drop_probability(0.0), is_aggressive_mode(false),
      rng(std::random_device{}()), dist(0.0, 1.0) {}

bool Queue::enqueue(std::shared_ptr<Packet> packet, double current_time, double qos_degradation_probability) {
    if (is_full()) {
        packet->is_dropped = true;
        current_drop_probability = 1.0;
        return false;
    }
    
    // 3. Evaluate ML Prediction
    is_aggressive_mode = (qos_degradation_probability > 0.7);
    
    // 1. RED (Random Early Detection) Phase
    avg_queue_size = (1.0 - ewma_weight) * avg_queue_size + ewma_weight * buffer.size();
    
    double active_max_th = is_aggressive_mode ? max_th * 0.75 : max_th;
    double active_min_th = is_aggressive_mode ? min_th * 0.5 : min_th;
    double active_max_p  = is_aggressive_mode ? 0.35 : max_p;
    
    if (avg_queue_size > active_max_th) {
        packet->is_dropped = true;
        current_drop_probability = 1.0;
        return false; // RED Max Drop
    }
    
    if (avg_queue_size > active_min_th) {
        current_drop_probability = active_max_p * (avg_queue_size - active_min_th) / (active_max_th - active_min_th);
        if (dist(rng) < current_drop_probability) {
            packet->is_dropped = true;
            return false; // RED Probabilistic Drop
        }
    } else {
        current_drop_probability = 0.0;
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

double Queue::get_current_drop_probability() const {
    return current_drop_probability;
}

double Queue::get_avg_queue_size() const {
    return avg_queue_size;
}

bool Queue::get_aqm_status() const {
    return is_aggressive_mode;
}

} // namespace qos_sim
