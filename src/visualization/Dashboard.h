#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "qos/Metrics.h"
#include <string>
#include <deque>

namespace qos_sim {

// New GUI Payload Struct containing all advanced module statistics
struct UIStateData {
    double current_drop_prob;
    double avg_queue_size;
    bool is_aqm_aggressive;
    int active_path;
    std::vector<double> path_delays;
    int path_switches;
    bool anomaly_detected;
    std::string anomaly_type;
    double anomaly_severity;
    double current_cwnd;
    std::string control_mode;
};

class Dashboard {
public:
    Dashboard(int window_width = 800, int window_height = 600, const std::string& title = "QoS Network Simulator");
    ~Dashboard();

    // Renders the dashboard frame with current metrics. Returns true if the Baseline/Enhanced toggle was pressed.
    bool render(double current_time_ms, const MetricsTracker& metrics, double delay_thresh, double jitter_thresh, double loss_thresh, int active_flows, const UIStateData& ui_state, bool is_enhanced_mode);

    // Renders the ML post-simulation evaluation results
    void render_ml_results(double accuracy, double precision, double recall, double f1_score, int tp, int fp, int tn, int fn, const std::vector<std::pair<double, double>>& prediction_curve = {});

    // Check if the user tried to close the window
    bool should_close() const;

private:
    int width;
    int height;
    std::string title;
    
    std::deque<float> delay_history;
    const size_t max_history_size = 200;
    
    // Helper to draw text centered (raylib Color is usually defined as struct Color, using void* or forward decl if needed, but since it's a cpp file we can just use raylib types by including it or using auto. We'll use an int for RGBA or a generic type)
    // Actually, to avoid raylib.h leaking, let's just make it take a generic uint32_t color hex, or just implement it locally in cpp. 
    // Wait, the header isn't including raylib.h, so we can't use Color directly. We will pass r,g,b,a instead.
    void draw_text_centered(const std::string& text, int x, int y, int fontSize, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
};

} // namespace qos_sim

#endif // DASHBOARD_H
