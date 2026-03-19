#include "Simulator.h"
#include "TCPFlow.h"
#include "RealTimeFlow.h"
#include <iostream>

namespace qos_sim {

Simulator::Simulator(const Config& sim_config, const std::string& dataset_file)
    : config(sim_config), current_time_ms(0.0) {
    
    // Initialize Multi-Path Topology (Self-Healing Routing)
    // Create 3 pathways, splitting total bandwidth but adding staggered propagation latency
    for (int i = 0; i < 3; i++) {
        Path p;
        p.queue = std::make_unique<Queue>(config.queue_size_packets / 3);
        p.link = std::make_unique<Link>(config.bandwidth_mbps / 3.0, config.propagation_delay_ms + (i * 4.0));
        p.current_delay_estimate = config.propagation_delay_ms + (i * 4.0);
        paths.push_back(std::move(p));
        per_path_delay.push_back(p.current_delay_estimate);
    }
    
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
        
        int total_occupancy = 0;
        for (const auto& path : paths) {
            total_occupancy += path.queue->get_occupancy();
        }
        metrics.update_queue_occupancy(total_occupancy);
        
        // Build dynamic GUI payload package
        UIStateData ui_state;
        auto* tcp = dynamic_cast<TCPFlow*>(flows[0].get());
        if (tcp) {
            ui_state.current_cwnd = tcp->get_cwnd();
            ui_state.control_mode = tcp->get_control_mode();
        }
        ui_state.active_path = active_path_id;
        ui_state.path_switches = path_switching_events;
        ui_state.path_delays = per_path_delay;
        ui_state.anomaly_detected = anomaly_flag;
        ui_state.anomaly_type = anomaly_type;
        ui_state.anomaly_severity = anomaly_severity;
        if (!paths.empty()) {
            ui_state.current_drop_prob = paths[active_path_id].queue->get_current_drop_probability();
            ui_state.avg_queue_size = paths[active_path_id].queue->get_avg_queue_size();
            ui_state.is_aqm_aggressive = paths[active_path_id].queue->get_aqm_status();
        }

        // Update the visual dashboard periodically
        if (current_time_ms - last_render_time_ms >= render_interval_ms) {
            bool toggle = dashboard->render(current_time_ms, metrics, 50.0, 10.0, 0.05, flows.size(), ui_state, is_enhanced_mode);
            if (toggle) {
                is_enhanced_mode = !is_enhanced_mode; // Interactive UI Physics Overhaul Swap
                std::cout << "\n[SYSTEM] Swapped physics model -> " << (is_enhanced_mode ? "ENHANCED IEEE" : "LEGACY BASELINE") << "\n";
            }
            last_render_time_ms = current_time_ms;
        }
        
        if (current_time_ms - last_log_time_ms >= log_interval_ms) {
            // -----------------------------------------------------
            // NETWORK ANOMALY DETECTION & CROSS-LAYER OPTIMIZATION
            // -----------------------------------------------------
            static std::vector<double> rtt_hist, q_hist, rate_hist;
            double cur_rtt = metrics.get_avg_rtt();
            double cur_q = metrics.get_queue_occupancy();
            double cur_rate = metrics.get_throughput_bps(1.0) / (8.0 * config.packet_size_bytes);
            
            // Reset state
            anomaly_flag = false;
            anomaly_type = "NONE";
            anomaly_severity = 0.0;

            if (cur_rtt > 0) {
                rtt_hist.push_back(cur_rtt);
                q_hist.push_back(cur_q);
                rate_hist.push_back(cur_rate);

                if (rtt_hist.size() > 10) {
                    auto calc_stats = [](const std::vector<double>& h, double& m, double& sd) {
                        m = 0; for(double v : h) m += v; m /= h.size();
                        double sq = 0; for(double v : h) sq += (v - m)*(v - m);
                        sd = std::sqrt(sq / h.size());
                        if (sd < 0.001) sd = 0.001; // Avoid div/0
                    };
                    
                    double m_rtt, sd_rtt, m_q, sd_q, m_rate, sd_rate;
                    calc_stats(rtt_hist, m_rtt, sd_rtt);
                    calc_stats(q_hist, m_q, sd_q);
                    calc_stats(rate_hist, m_rate, sd_rate);
                    
                    // Z-Score thresholding for anomaly detection (Mean + 3 * StdDev)
                    if (is_enhanced_mode) {
                        if (cur_rtt > m_rtt + 3.0 * sd_rtt && sd_rtt > 2.0) {
                            anomaly_flag = true;
                            anomaly_type = "CONGESTION_SPIKE";
                            anomaly_severity = (cur_rtt - m_rtt) / sd_rtt;
                        } else if (cur_rate > m_rate + 3.0 * sd_rate && sd_rate > 5.0) {
                            anomaly_flag = true;
                            anomaly_type = "BURST_TRAFFIC";
                            anomaly_severity = (cur_rate - m_rate) / sd_rate;
                        }

                        if (anomaly_flag) {
                            std::cout << "\n[!] " << anomaly_type << " DETECTED at " << current_time_ms/1000.0 
                                      << "s | Severity: " << anomaly_severity << " Z-Score\n";
                            
                            // Action 1: Cross-layer signaling to proactively throttle TCP
                            for(auto& flow : flows) {
                                if (auto* tcp = dynamic_cast<TCPFlow*>(flow.get())) {
                                    tcp->apply_cross_layer_feedback(metrics.get_jitter(), sd_rtt, cur_q);
                                } else {
                                    flow->process_feedback(true, 0); 
                                }
                            }
                            
                            // Action 2: Trigger aggressive localized rerouting
                            for(auto& path : paths) {
                                path.current_delay_estimate += 100.0; // Penalty enforces route recalculation
                            }
                        }
                    }
                    
                    // Maintain rolling sliding window
                    if(rtt_hist.size() > 50) {
                        rtt_hist.erase(rtt_hist.begin());
                        q_hist.erase(q_hist.begin());
                        rate_hist.erase(rate_hist.begin());
                    }
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
            
            // SELF-HEALING MULTI-PATH ROUTING: Least-Delay path selection
            int best_path_idx = 0;
            double min_delay = 999999.0;
            
            if (is_enhanced_mode) {
                for (size_t i = 0; i < paths.size(); i++) {
                    double queue_delay_ms = 0;
                    if (paths[i].link->get_bandwidth_bps() > 0) {
                        queue_delay_ms = (paths[i].queue->get_occupancy() * pkt->size_bytes * 8.0) / (paths[i].link->get_bandwidth_bps() / 1000.0);
                    }
                    double path_delay = paths[i].link->get_propagation_delay_ms() + queue_delay_ms;
                    paths[i].current_delay_estimate = path_delay;
                    if (i < per_path_delay.size()) per_path_delay[i] = path_delay; // Update UI Array
                    
                    if (path_delay < min_delay) {
                        min_delay = path_delay;
                        best_path_idx = i;
                    }
                }

                // Track Self-Healing Path Switching Event
                if (active_path_id != best_path_idx) {
                    path_switching_events++;
                    active_path_id = best_path_idx;
                }
            } else {
                // LEGACY BASELINE MODE: Force everything aggressively down isolated Bottleneck 0 
                best_path_idx = 0; 
                active_path_id = 0;
            }

            // Note: Bypassing RED AQM math when disabled requires modifying Queue::enqueue or just turning ML off
            double aqm_score = is_enhanced_mode ? (anomaly_flag ? 1.0 : 0.0) : -1.0; 
            if (!paths[best_path_idx].queue->enqueue(pkt, current_time_ms, aqm_score)) {
                metrics.record_drop(pkt->flow_id);
                flow->process_feedback(true, 0); // notify loss
            }
        }
    }
}

void Simulator::process_queue_and_link() {
    for (auto& path : paths) {
        if (path.link->can_transmit(current_time_ms)) {
            if (!path.queue->is_empty()) {
                auto packet = path.queue->dequeue(current_time_ms);
                if (packet != nullptr) {
                    path.link->transmit(packet, current_time_ms);
                    in_flight_packets.push_back(packet);
                }
            }
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
