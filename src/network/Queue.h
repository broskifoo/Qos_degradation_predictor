#pragma once

#include "Packet.h"
#include <queue>
#include <memory>
#include <random>

namespace qos_sim {

class Queue {
private:
    int max_size_packets;
    std::queue<std::shared_ptr<Packet>> buffer;
    
    // RED (Random Early Detection) variables
    double avg_queue_size;
    double ewma_weight;
    double min_th;
    double max_th;
    double max_p;
    
    // CoDel (Controlled Delay) variables
    double target_delay_ms;
    double interval_ms;
    double first_above_time;
    
    // UI Logging Variables
    double current_drop_probability;
    bool is_aggressive_mode;
    
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

public:
    explicit Queue(int max_size);

    bool enqueue(std::shared_ptr<Packet> packet, double current_time, double qos_degradation_probability = 0.0);
    std::shared_ptr<Packet> dequeue(double current_time);
    
    int get_occupancy() const;
    bool is_full() const;
    bool is_empty() const;
    int get_max_size() const;
    
    // UI Exposing Getters
    double get_current_drop_probability() const;
    double get_avg_queue_size() const;
    bool get_aqm_status() const;
};

} // namespace qos_sim
