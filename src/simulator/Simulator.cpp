#include "Simulator.h"
#include "TCPFlow.h"
#include "RealTimeFlow.h"
#include <iostream>

namespace qos_sim {

Simulator::Simulator(const Config& sim_config, const std::string& dataset_file)
    : config(sim_config), current_time_ms(0.0) {
    
    queue = std::make_unique<Queue>(config.queue_size_packets);
    link = std::make_unique<Link>(config.bandwidth_mbps, config.propagation_delay_ms);
    logger = std::make_unique<Logger>(dataset_file);
    
    // Initialize Dashboard
    dashboard = std::make_unique<Dashboard>(800, 600, "QoS Network Simulator");
    
    int flow_id_counter = 1;
    
    // Add Real-time flow
    flows.push_back(std::make_unique<RealTimeFlow>(
        flow_id_counter++, config.packet_size_bytes, config.realtime_rate_mbps));
        
    // Add TCP flows
    for (int i = 0; i < config.tcp_flows; i++) {
        flows.push_back(std::make_unique<TCPFlow>(
            flow_id_counter++, config.packet_size_bytes));
    }
}

void Simulator::run() {
    std::cout << "Starting simulation for " << config.simulation_time_sec << " seconds...\n";
    
    double dt_ms = 1.0; // 1 ms timesteps
    double end_time_ms = config.simulation_time_sec * 1000.0;
    
    double last_log_time_ms = 0;
    double log_interval_ms = 1000.0; // Log every 1 second
    
    double last_render_time_ms = 0;
    double render_interval_ms = 16.0; // Target ~60FPS rendering
    
    while (current_time_ms < end_time_ms) {
        
        // Check if user closed the dashboard window
        if (dashboard->should_close()) {
            std::cout << "Simulation aborted by user (window closed).\n";
            break;
        }

        generate_traffic();
        process_queue_and_link();
        process_arrivals();
        
        metrics.update_queue_occupancy(queue->get_occupancy());
        
        // Update the visual dashboard periodically
        if (current_time_ms - last_render_time_ms >= render_interval_ms) {
            dashboard->render(current_time_ms, metrics, 50.0, 10.0, 0.05, flows.size());
            last_render_time_ms = current_time_ms;
        }
        
        if (current_time_ms - last_log_time_ms >= log_interval_ms) {
            // -----------------------------------------------------
            // NETWORK ANOMALY DETECTION & CROSS-LAYER OPTIMIZATION
            // -----------------------------------------------------
            static std::vector<double> rtt_history;
            double current_rtt = metrics.get_avg_rtt();
            if (current_rtt > 0) {
                rtt_history.push_back(current_rtt);
                if (rtt_history.size() > 10) {
                    double mean = 0;
                    for(double r : rtt_history) mean += r;
                    mean /= rtt_history.size();
                    
                    double sq_sum = 0;
                    for(double r : rtt_history) sq_sum += (r - mean) * (r - mean);
                    double std_dev = std::sqrt(sq_sum / rtt_history.size());
                    
                    // Z-Score thresholding for anomaly detection
                    if (current_rtt > mean + 3.0 * std_dev && std_dev > 2.0) {
                        std::cout << "\n[!] ANOMALY DETECTED at " << current_time_ms/1000.0 << "s | RTT: " << current_rtt << "ms (Mean: " << mean << ")\n";
                        std::cout << "--> Deploying Cross-Layer Optimization: Halving TCP cwnd proactively.\n";
                        // Cross-layer signaling to throttle TCP
                        for(auto& flow : flows) {
                            flow->process_feedback(true, 0); // Packet loss signal throttles AIMD TCP
                        }
                    }
                    if(rtt_history.size() > 50) rtt_history.erase(rtt_history.begin()); // sliding window
                }
            }

            // Log snapshot
            // For QoS degradation label threshold, setting typical values:
            // Delay > 50ms, Jitter > 10ms, Loss > 0.05 (5%)
            logger->log_metrics(metrics, 1.0, 50.0, 10.0, 0.05);
            metrics.reset_window();
            last_log_time_ms = current_time_ms;
        }
        
        current_time_ms += dt_ms;
    }
    
    std::cout << "Simulation completed.\n";
}

void Simulator::generate_traffic() {
    for (auto& flow : flows) {
        auto packets = flow->generate(current_time_ms);
        for (auto& pkt : packets) {
            metrics.record_sent(pkt->flow_id);
            if (!queue->enqueue(pkt, current_time_ms)) {
                // Buffer full
                metrics.record_drop(pkt->flow_id);
                flow->process_feedback(true, 0); // notify loss
            }
        }
    }
}

void Simulator::process_queue_and_link() {
    // If link is idle, process up to the capacity, but since discrete events:
    // Link transmit returns the time the packet finishes transmission.
    if (link->can_transmit(current_time_ms)) {
        if (!queue->is_empty()) {
            auto packet = queue->dequeue(current_time_ms);
            
            // This sets packet->arrival_time
            link->transmit(packet, current_time_ms);
            
            in_flight_packets.push_back(packet);
        }
    }
}

void Simulator::process_arrivals() {
    for (auto it = in_flight_packets.begin(); it != in_flight_packets.end(); ) {
        if ((*it)->arrival_time <= current_time_ms) {
            auto packet = *it;
            // E2E delay (simplification: generation_time to current arrival time)
            double rtt = (current_time_ms - packet->generation_time) * 2.0; // Assume symetric delay for RTT
            double delay = current_time_ms - packet->generation_time;
            
            metrics.record_delivery(packet->flow_id, packet->size_bytes, rtt, delay);
            
            // Notify flow of successful delivery (ACK)
            flows[packet->flow_id - 1]->process_feedback(false, rtt);
            
            it = in_flight_packets.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace qos_sim
