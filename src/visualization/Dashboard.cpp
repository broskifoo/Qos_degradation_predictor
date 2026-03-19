#include "Dashboard.h"
#include <raylib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <random>

namespace qos_sim {

Dashboard::Dashboard(int window_width, int window_height, const std::string& window_title)
    : width(window_width), height(window_height), title(window_title) {
    
    // Setting config flags to allow window resizing and to prevent Raylib from taking output
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING); // Suppress Raylib info spam in console
    
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
}

Dashboard::~Dashboard() {
    CloseWindow();
}

bool Dashboard::should_close() const {
    return WindowShouldClose();
}

void Dashboard::draw_text_centered(const std::string& text, int x, int y, int fontSize, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    Color c = {r, g, b, a};
    int textWidth = MeasureText(text.c_str(), fontSize);
    DrawText(text.c_str(), x - textWidth / 2, y, fontSize, c);
}

bool Dashboard::render(double current_time_ms, const MetricsTracker& metrics, 
                       double delay_thresh, double jitter_thresh, double loss_thresh, int active_flows, const UIStateData& ui_state, bool is_enhanced_mode) {
    bool toggle_clicked = false;
    if (should_close()) return false;

    // Track delay history for plotting with added visual randomness
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> noise(-2.5f, 2.5f);
    
    double current_delay = metrics.get_avg_rtt();
    if (current_delay > 0) {
        current_delay += noise(gen); // Apply visual jitter
        if (current_delay < 0) current_delay = 0;
    }

    if (delay_history.size() >= max_history_size) {
        delay_history.pop_front();
    }
    delay_history.push_back((float)current_delay);

    BeginDrawing();
    
    // Modern Dark Theme Colors
    Color bgDark = { 15, 23, 42, 255 };      // #0f172a (Deep Slate)
    Color panelDark = { 30, 41, 59, 255 };   // #1e293b (Slate 800)
    Color textLight = { 241, 245, 249, 255 };// #f1f5f9 (Slate 100)
    Color textMute = { 148, 163, 184, 255 }; // #94a3b8 (Slate 400)
    
    // Status colors
    Color colorGood = { 34, 197, 94, 255 };  // #22c55e (Green 500)
    Color colorWarn = { 245, 158, 11, 255 }; // #f59e0b (Amber 500)
    Color colorBad = { 239, 68, 68, 255 };   // #ef4444 (Red 500)
    
    // Component colors
    Color colorSource = { 56, 189, 248, 255 }; // Light Blue
    Color colorDest = { 168, 85, 247, 255 };   // Purple
    Color colorLink = { 71, 85, 105, 255 };    // Gray Link

    ClearBackground(bgDark);

    int margin = 20;
    
    // Calculate current width and height in case window was resized
    width = GetScreenWidth();
    height = GetScreenHeight();

    // -------------------------------------------------------------
    // LAYOUT DIMENSIONS
    // -------------------------------------------------------------
    int side_panel_w = 280;
    Rectangle sidePanel = { (float)(width - side_panel_w - margin), (float)margin, (float)side_panel_w, (float)(height - 2*margin) };
    
    int main_w = width - side_panel_w - 3*margin;
    int main_h = height - 2*margin;
    
    int topo_h = (main_h - margin) * 0.55f;
    int plot_h = (main_h - margin) * 0.45f;
    
    Rectangle topoPanel = { (float)margin, (float)margin, (float)main_w, (float)topo_h };
    Rectangle plotPanel = { (float)margin, (float)(margin + topo_h + margin), (float)main_w, (float)plot_h };

    // -------------------------------------------------------------
    // RIGHT SIDE PANEL: METRICS & STATUS
    // -------------------------------------------------------------
    DrawRectangleRounded({ sidePanel.x, sidePanel.y + 5, sidePanel.width, sidePanel.height }, 0.05f, 10, {0,0,0, 60}); // Shadow
    DrawRectangleRounded(sidePanel, 0.05f, 10, panelDark);
    
    DrawText("QoS Monitor", sidePanel.x + 20, sidePanel.y + 20, 24, textLight);
    
    std::stringstream time_ss;
    time_ss << std::fixed << std::setprecision(1) << (current_time_ms / 1000.0) << "s";
    DrawText(("Time: " + time_ss.str()).c_str(), sidePanel.x + 20, sidePanel.y + 60, 20, textMute);

    // QoS Status Bubble
    int qos_label = metrics.calculate_qos_label(delay_thresh, jitter_thresh, loss_thresh);
    Color statusColor = (qos_label == 0) ? colorGood : colorBad;
    std::string statusText = (qos_label == 0) ? "STATUS: OPTIMAL" : "STATUS: DEGRADED";
    
    Rectangle statusRect = { sidePanel.x + 20, sidePanel.y + 100, sidePanel.width - 40, 40 };
    DrawRectangleRounded({ statusRect.x, statusRect.y + 4, statusRect.width, statusRect.height }, 0.5f, 10, {0,0,0, 50});
    DrawRectangleRounded(statusRect, 0.5f, 10, statusColor);
    draw_text_centered(statusText, statusRect.x + statusRect.width/2, statusRect.y + 11, 16, 255, 255, 255, 255);

    // Setup values for cards
    double current_delay_val = metrics.get_avg_rtt();
    double jitter = metrics.get_jitter();
    double loss = metrics.get_packet_loss() * 100.0; // percentage
    double throughput = metrics.get_throughput_bps(1.0) / 1000000.0; // Mbps

    // Stat Cards (Vertical Stack) scaled for smaller cards
    int stats_start_y = statusRect.y + statusRect.height + 20;
    int card_h = 60; // Shrink to fit more modules
    int card_gap = 10;
    
    auto draw_side_stat_card = [&](int index, const char* label, double val, const char* unit, bool alert) {
        float x = sidePanel.x + 20;
        float y = stats_start_y + index * (card_h + card_gap);
        float w = sidePanel.width - 40;
        
        Rectangle card = { x, y, w, (float)card_h };
        DrawRectangleRounded({ card.x, card.y + 4, card.width, card.height }, 0.15f, 10, {0,0,0, 40});
        Color cardBg = alert ? Color{127, 29, 29, 255} : bgDark; 
        DrawRectangleRounded(card, 0.15f, 10, cardBg);
        
        Color labelColor = alert ? Color{252, 165, 165, 255} : textMute;
        DrawText(label, x + 10, y + 8, 14, labelColor);
        
        std::stringstream val_ss;
        val_ss << std::fixed << std::setprecision(2) << val;
        Color valColor = alert ? textLight : colorSource; 
        DrawText(val_ss.str().c_str(), x + 10, y + 25, 22, valColor);
        
        int valWidth = MeasureText(val_ss.str().c_str(), 22);
        DrawText(unit, x + 10 + valWidth + 5, y + 33, 12, labelColor);
    };

    if (!is_enhanced_mode) {
        draw_side_stat_card(0, "Avg RTT (Delay)", current_delay_val, "ms", current_delay_val > delay_thresh);
        draw_side_stat_card(1, "Packet Jitter", jitter, "ms", jitter > jitter_thresh);
        draw_side_stat_card(2, "Traffic Loss", loss, "%", loss > (loss_thresh * 100.0));
        draw_side_stat_card(3, "Agg. Throughput", throughput, "Mbps", false);
    } else {
        // --- ADVANCED MODULES PANELS ---
        // 1. AQM UI PANEL
        draw_side_stat_card(0, "AQM Avg Queue (RED)", ui_state.avg_queue_size, "pkts", ui_state.avg_queue_size > 400);
        draw_side_stat_card(1, "AQM Drop Prob", ui_state.current_drop_prob * 100.0, "%", ui_state.is_aqm_aggressive);
        
        // 2. MULTI-PATH ROUTING PANEL
        draw_side_stat_card(2, "Active Topology Path", ui_state.active_path, "ID", false);
        draw_side_stat_card(3, "Path Swap Latency Adjusts", ui_state.path_switches, "events", ui_state.path_switches > 100);

        // 3. ANOMALY DETECTION PANEL
        draw_side_stat_card(4, "Anomaly Threat Score", ui_state.anomaly_severity, "Z", ui_state.anomaly_detected);

        // 4. CROSS-LAYER TCP OPTIMIZATION PANEL
        bool is_tcp_throttled = (ui_state.control_mode == "Aggressive");
        draw_side_stat_card(5, "Cross-Layer TCP CWND", ui_state.current_cwnd, "MSS", is_tcp_throttled);
        
        // Dynamic Status Overlays indicating live model states natively onto the cards
        DrawText((ui_state.is_aqm_aggressive ? "AGGR" : "NORM"), sidePanel.x + 200, stats_start_y + 1 * (card_h + card_gap) + 25, 16, ui_state.is_aqm_aggressive ? colorBad : colorGood);
        DrawText((ui_state.anomaly_detected ? ui_state.anomaly_type.c_str() : "SECURE"), sidePanel.x + 140, stats_start_y + 4 * (card_h + card_gap) + 25, 14, ui_state.anomaly_detected ? colorBad : colorGood);
        DrawText(ui_state.control_mode.c_str(), sidePanel.x + 180, stats_start_y + 5 * (card_h + card_gap) + 25, 16, (ui_state.control_mode == "Stable") ? colorSource : (is_tcp_throttled ? colorBad : colorWarn));
    }
    
    // --- TOGGLE BUTTON (Baseline vs Enhanced) ---
    Rectangle toggleBtn = { sidePanel.x + 20, sidePanel.y + sidePanel.height - 60, sidePanel.width - 40, 40 };
    DrawRectangleRounded(toggleBtn, 0.2f, 10, is_enhanced_mode ? colorSource : textMute);
    draw_text_centered(is_enhanced_mode ? "MODE: ENHANCED (IEEE)" : "MODE: BASELINE (FIFO)", toggleBtn.x + toggleBtn.width/2, toggleBtn.y + 12, 16, 255, 255, 255, 255);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(GetMousePosition(), toggleBtn)) {
            toggle_clicked = true;
        }
    }

    // -------------------------------------------------------------
    // TOP MAIN SECTION: 3D TOPOLOGY
    // -------------------------------------------------------------
    DrawRectangleRounded({ topoPanel.x, topoPanel.y + 5, topoPanel.width, topoPanel.height }, 0.1f, 10, {0,0,0, 60}); // Shadow
    DrawRectangleRounded(topoPanel, 0.1f, 10, panelDark);

    // Setup 3D Camera looking at the origin
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 8.0f, 16.0f }; // Pulled further back and higher up to encompass all Z-depth blocks
    camera.target = (Vector3){ 0.0f, -1.0f, 0.0f };    // Pointing lower than objects forces projection higher up on the screen
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f; // Standard FOV to reduce edge distortion
    camera.projection = CAMERA_PERSPECTIVE;

    // Use scissor mode to perfectly restrict 3D drawing overflow into other panels
    BeginScissorMode((int)topoPanel.x, (int)topoPanel.y, (int)topoPanel.width, (int)topoPanel.height);
    BeginMode3D(camera);

    float worldY = 3.5f; // Lift all physical 3D objects a full 3.5 units higher globally
    Vector3 qPos   = {  0.0f, worldY, 0.0f };
    Vector3 dstPos = {  3.5f, worldY, 0.0f };

    Color srcColors[] = {
        { 56, 189, 248, 255 }, // Light Blue
        { 168, 85, 247, 255 }, // Purple
        { 34, 197, 94, 255 },  // Green
        { 245, 158, 11, 255 }, // Amber
        { 236, 72, 153, 255 }, // Pink
        { 239, 68, 68, 255 }   // Red
    };

    // Link Queue to Dest
    DrawCube({  1.75f, worldY, 0.0f }, 3.5f, 0.1f, 0.1f, colorLink);

    int num_sources = std::max(1, active_flows);
    float anim_time = current_time_ms / 500.0f;

    // --- ENHANCED MODE: Draw 3 distinct routing paths to a shared router node ---
    if (is_enhanced_mode && !ui_state.path_delays.empty()) {
        // Router midpoint
        Vector3 routerPos = { 0.0f, worldY, 0.0f };
        DrawCube(routerPos, 0.9f, 0.9f, 0.9f, { 56, 189, 248, 200 }); // Cyan router cube
        DrawCubeWires(routerPos, 0.9f, 0.9f, 0.9f, { 255,255,255,180 });

        // 3 Path links from router to destination with staggered Z offsets
        float pathZ[] = { -2.5f, 0.0f, 2.5f };
        Color pathColors[] = { colorGood, colorWarn, colorSource };
        for (int p = 0; p < 3; p++) {
            Vector3 midPos = { 2.0f, worldY, pathZ[p] };
            Color pc = (p == ui_state.active_path) ? pathColors[p] : Color{80, 100, 120, 180};
            DrawLine3D(routerPos, midPos, pc);
            DrawLine3D(midPos, dstPos, pc);
            DrawCube(midPos, 0.4f, 0.4f, 0.4f, pc);

            // Animate packets on active path only
            if (p == ui_state.active_path) {
                for (int i = 0; i < 3; i++) {
                    float t = fmod(anim_time + i * 0.333f, 1.0f);
                    Vector3 pPt = {
                        midPos.x + t * (dstPos.x - midPos.x),
                        midPos.y,
                        midPos.z + t * (dstPos.z - midPos.z)
                    };
                    DrawSphere(pPt, 0.13f, pc);
                }
            }
        }
    }

    for (int s = 0; s < num_sources; ++s) {
        float zOffset = (s - (num_sources - 1) / 2.0f) * 2.5f;
        Vector3 srcPos = { -3.5f, worldY, zOffset };
        Color flowColor = srcColors[s % 6];
        DrawLine3D(srcPos, qPos, flowColor);
        DrawCube(srcPos, 0.8f, 0.8f, 0.8f, flowColor);
        DrawCubeWires(srcPos, 0.8f, 0.8f, 0.8f, {255, 255, 255, 100});
        for(int i = 0; i < 3; i++) {
            float t = fmod(anim_time + (i * 0.333f) + (s * 0.2f), 1.0f);
            Vector3 pktPos = {
                srcPos.x + t * (qPos.x - srcPos.x),
                srcPos.y + t * (qPos.y - srcPos.y) + 0.3f,
                srcPos.z + t * (qPos.z - srcPos.z)
            };
            DrawSphere(pktPos, 0.15f, flowColor);
        }
    }

    // Flowing packets from Queue/Router to Dest (baseline mode only)
    if (!is_enhanced_mode) {
        for(int i = 0; i < 3; i++) {
            float t = fmod(anim_time + (i * 0.333f), 1.0f);
            float xPos2 = 0.0f + (t * 3.5f);
            DrawSphere({ xPos2, worldY + 0.3f, 0.0f }, 0.15f, colorSource);
        }
    }

    // Destination Node
    DrawCube(dstPos, 1.0f, 1.0f, 1.0f, colorDest);
    DrawCubeWires(dstPos, 1.0f, 1.0f, 1.0f, {255, 255, 255, 100});

    // Queue Node (Bottleneck)
    double occupancy = metrics.get_queue_occupancy();
    float fill_percentage = std::min(1.0f, std::max(0.0f, (float)(occupancy / 100.0))); 
    float fill_width = 1.6f * fill_percentage;
    // In Enhanced mode: if AQM is aggressive, flash the cube RED to indicate protective dropping
    Color qColor = (is_enhanced_mode && ui_state.is_aqm_aggressive) ? colorBad
                 : (fill_percentage > 0.8f) ? colorBad
                 : (fill_percentage > 0.5f) ? colorWarn : colorGood;
    
    // Transparent box for buffer capacity
    DrawCube(qPos, 1.8f, 1.0f, 1.0f, { 20, 25, 40, 200 }); // Dark glass
    DrawCubeWires(qPos, 1.8f, 1.0f, 1.0f, textMute);
    
    // Filled packet portion
    if (fill_width > 0.01f) {
        DrawCube({ qPos.x - 0.9f + (fill_width / 2.0f), worldY, 0.0f }, fill_width, 0.9f, 0.9f, qColor);
    }

    EndMode3D();
    EndScissorMode();

    // 2D Overlay text strictly tracking the 3D objects using GetWorldToScreen projection mapping
    Vector2 srcLabel = GetWorldToScreen({ -3.5f, worldY + 0.8f, 0.0f }, camera);
    draw_text_centered("Flows", srcLabel.x, srcLabel.y, 16, 255, 255, 255, 255);

    Vector2 dstLabel = GetWorldToScreen({ dstPos.x, worldY + 0.8f, 0.0f }, camera);
    draw_text_centered("Dest", dstLabel.x, dstLabel.y, 16, 255, 255, 255, 255);

    Vector2 qLabel = GetWorldToScreen({ qPos.x, worldY - 1.2f, 0.0f }, camera); // placed subtly below the queue cube
    std::stringstream q_ss;
    q_ss << "Buffer: " << (int)occupancy << " pkts";
    draw_text_centered(q_ss.str(), qLabel.x, qLabel.y, 16, textMute.r, textMute.g, textMute.b, textMute.a);

    // -------------------------------------------------------------
    // BOTTOM MAIN SECTION: LIVE QOS PLOT
    // -------------------------------------------------------------
    DrawRectangleRounded({ plotPanel.x, plotPanel.y + 5, plotPanel.width, plotPanel.height }, 0.1f, 10, {0,0,0, 60});
    DrawRectangleRounded(plotPanel, 0.1f, 10, panelDark);
    
    if (is_enhanced_mode && !ui_state.path_delays.empty()) {
        // --- Per-Path Delay Bar Chart ---
        DrawText("Multi-Path Delay Comparison (ms)", plotPanel.x + 20, plotPanel.y + 15, 18, textLight);
        float barW = 60, barGap = 30;
        float barMaxH = plotPanel.height - 70;
        float totalBarW = ui_state.path_delays.size() * (barW + barGap);
        float startX = plotPanel.x + (plotPanel.width - totalBarW) / 2;
        
        double maxD = 1.0;
        for (double d : ui_state.path_delays) if (d > maxD) maxD = d;
        maxD = std::max(maxD, 30.0);
        
        for (size_t p = 0; p < ui_state.path_delays.size(); p++) {
            double d = ui_state.path_delays[p];
            float barH = (float)(d / maxD) * (barMaxH - 20);
            float bx = startX + p * (barW + barGap);
            float by = plotPanel.y + 45 + (barMaxH - barH - 20);
            bool isActive = ((int)p == ui_state.active_path);
            Color bc = isActive ? colorGood : Color{71, 85, 105, 200};
            
            DrawRectangle(bx, plotPanel.y + 45, barW, barMaxH - 20, {0,0,0,60});
            DrawRectangle(bx, by, barW, barH, bc);
            if (isActive) DrawRectangleLinesEx({bx - 2, by - 2, barW + 4, barH + 4}, 2, colorGood);
            
            std::stringstream ds; ds << std::fixed << std::setprecision(1) << d << "ms";
            draw_text_centered(ds.str(), bx + barW/2, by - 20, 13, bc.r, bc.g, bc.b, 255);
            draw_text_centered(isActive ? "PATH " + std::to_string(p) + " *" : "PATH " + std::to_string(p),
                               bx + barW/2, plotPanel.y + 45 + barMaxH - 12, 13, 255, 255, 255, 200);
        }
    } else {
        DrawText("Real-Time QoS Delay Plot", plotPanel.x + 20, plotPanel.y + 20, 20, textLight);
    
    if (delay_history.size() > 1) {
        float max_val = 1.0f;
        for (float v : delay_history) if (v > max_val) max_val = v;
        max_val = std::max((float)delay_thresh * 1.5f, max_val); // Keep scale relative to threshold
        
        float plotX = plotPanel.x + 60; // Extra left margin for labels
        float plotY = plotPanel.y + 60;
        float plotW = plotPanel.width - 80;
        float plotH = plotPanel.height - 90;
        
        // Draw axes
        DrawLineV({plotX, plotY}, {plotX, plotY + plotH}, textMute);
        DrawLineV({plotX, plotY + plotH}, {plotX + plotW, plotY + plotH}, textMute);
        
        // Draw threshold line
        float threshY = plotY + plotH - (delay_thresh / max_val) * plotH;
        DrawLineV({plotX, threshY}, {plotX + plotW, threshY}, {239, 68, 68, 150}); // Red line
        DrawText("Thresh", plotX + plotW - 45, threshY - 15, 12, colorBad);
        
        // Draw Y Axis Labels
        std::stringstream max_ss; max_ss << std::fixed << std::setprecision(1) << max_val << "ms";
        DrawText(max_ss.str().c_str(), plotX - MeasureText(max_ss.str().c_str(), 10) - 5, plotY, 10, textMute);
        DrawText("0ms", plotX - MeasureText("0ms", 10) - 5, plotY + plotH - 10, 10, textMute);
        
        // Draw lines
        float stepX = plotW / (float)(max_history_size - 1);
        for (size_t i = 1; i < delay_history.size(); i++) {
            float x1 = plotX + (i - 1) * stepX;
            float y1 = plotY + plotH - (delay_history[i - 1] / max_val) * plotH;
            float x2 = plotX + i * stepX;
            float y2 = plotY + plotH - (delay_history[i] / max_val) * plotH;
            
            Color segmentColor = (delay_history[i] > delay_thresh) ? colorBad : colorSource;
            DrawLineEx({x1, y1}, {x2, y2}, 2.0f, segmentColor);
        }
    }

    } // end enhanced/baseline toggle for bottom plot

    EndDrawing();
    return toggle_clicked;
}

void Dashboard::render_ml_results(double accuracy, double precision, double recall, double f1_score, int tp, int fp, int tn, int fn, const std::vector<std::pair<double, double>>& prediction_curve) {
    if (should_close()) return;

    BeginDrawing();
    
    // Modern Dark Theme Colors
    Color bgDark = { 15, 23, 42, 255 };      // Slate 900
    Color panelDark = { 30, 41, 59, 255 };   // Slate 800
    Color textLight = { 248, 250, 252, 255 };// Slate 50
    Color textMute = { 148, 163, 184, 255 }; // Slate 400
    Color colorGood = { 34, 197, 94, 255 };  // Emerald 500
    Color colorWarn = { 245, 158, 11, 255 }; // Amber 500
    Color colorBad = { 239, 68, 68, 255 };   // Red 500
    Color colorSource = { 56, 189, 248, 255 }; // Sky 400
    
    ClearBackground(bgDark);
    
    width = GetScreenWidth();
    height = GetScreenHeight();
    int margin = 30;

    // TITLE
    DrawText("Machine Learning Prediction Results", margin, margin, 32, textLight);
    DrawText("Logistic Regression Model Evaluation", margin, margin + 40, 20, colorSource);

    // 3 Panels: Scores & Confusion Matrix & Prediction Graph
    int panel_y = margin + 90;
    int panel_h = height - panel_y - margin;
    int panel_w = (width - 4 * margin) / 3;

    Rectangle scoresPanel = { (float)margin, (float)panel_y, (float)panel_w, (float)panel_h };
    Rectangle cmPanel = { (float)(margin * 2 + panel_w), (float)panel_y, (float)panel_w, (float)panel_h };
    Rectangle graphPanel = { (float)(margin * 3 + panel_w * 2), (float)panel_y, (float)panel_w, (float)panel_h };

    // Draw Panels
    DrawRectangleRounded({ scoresPanel.x, scoresPanel.y + 5, scoresPanel.width, scoresPanel.height }, 0.05f, 10, {0,0,0, 60});
    DrawRectangleRounded(scoresPanel, 0.05f, 10, panelDark);

    DrawRectangleRounded({ cmPanel.x, cmPanel.y + 5, cmPanel.width, cmPanel.height }, 0.05f, 10, {0,0,0, 60});
    DrawRectangleRounded(cmPanel, 0.05f, 10, panelDark);

    DrawRectangleRounded({ graphPanel.x, graphPanel.y + 5, graphPanel.width, graphPanel.height }, 0.05f, 10, {0,0,0, 60});
    DrawRectangleRounded(graphPanel, 0.05f, 10, panelDark);

    // Score Board
    DrawText("Performance Metrics", scoresPanel.x + 20, scoresPanel.y + 20, 24, textLight);

    auto draw_metric_bar = [&](int idx, const char* label, double val, Color c) {
        float y = scoresPanel.y + 80 + idx * 80;
        DrawText(label, scoresPanel.x + 20, y, 18, textMute);
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << (val * 100.0) << "%";
        
        // Value Text
        DrawText(ss.str().c_str(), scoresPanel.x + scoresPanel.width - MeasureText(ss.str().c_str(), 20) - 20, y, 20, c);

        // Bar
        Rectangle barBg = { scoresPanel.x + 20, y + 30, scoresPanel.width - 40, 10 };
        DrawRectangleRec(barBg, {0,0,0, 100});
        Rectangle barFg = { scoresPanel.x + 20, y + 30, (scoresPanel.width - 40) * (float)val, 10 };
        DrawRectangleRec(barFg, c);
    };

    draw_metric_bar(0, "Accuracy", accuracy, colorGood);
    draw_metric_bar(1, "Precision", precision, colorSource);
    draw_metric_bar(2, "Recall", recall, colorWarn);
    draw_metric_bar(3, "F1 Score", f1_score, {168, 85, 247, 255}); // Purple

    // Confusion Matrix
    DrawText("Confusion Matrix", cmPanel.x + 20, cmPanel.y + 20, 24, textLight);
    
    // Auto scale grid cells
    int cell_w = (cmPanel.width - 40) / 2 - 20;
    int cell_h = (cmPanel.height - 150) / 2;
    int grid_x = cmPanel.x + 80;
    int grid_y = cmPanel.y + 100;

    // Headers
    draw_text_centered("Actual Degraded", grid_x + cell_w/2, grid_y - 20, 16, textMute.r, textMute.g, textMute.b, 255);
    draw_text_centered("Actual Optimal", grid_x + cell_w + cell_w/2, grid_y - 20, 16, textMute.r, textMute.g, textMute.b, 255);

    // Side Labels
    draw_text_centered("Pred\nDegraded", grid_x - 50, grid_y + cell_h/2 - 10, 16, textMute.r, textMute.g, textMute.b, 255);
    draw_text_centered("Pred\nOptimal", grid_x - 50, grid_y + cell_h + cell_h/2 - 10, 16, textMute.r, textMute.g, textMute.b, 255);

    // Cells
    auto draw_cell = [&](int r, int c, int val, Color bgColor, const char* label) {
        Rectangle rect = { (float)(grid_x + c * cell_w), (float)(grid_y + r * cell_h), (float)cell_w, (float)cell_h };
        DrawRectangleRec(rect, bgColor);
        DrawRectangleLinesEx(rect, 2.0f, bgDark);
        
        std::stringstream ss; ss << val;
        draw_text_centered(ss.str(), rect.x + rect.width/2, rect.y + rect.height/2 - 15, 32, textLight.r, textLight.g, textLight.b, 255);
        draw_text_centered(label, rect.x + rect.width/2, rect.y + rect.height/2 + 20, 14, textLight.r, textLight.g, textLight.b, 200);
    };

    draw_cell(0, 0, tp, {185, 28, 28, 200}, "True Positive");   // predicted 1, actual 1
    draw_cell(0, 1, fp, {245, 158, 11, 200}, "False Positive"); // predicted 1, actual 0
    draw_cell(1, 0, fn, {21, 128, 61, 200}, "False Negative");  // predicted 0, actual 1
    draw_cell(1, 1, tn, {3, 105, 161, 200}, "True Negative");   // predicted 0, actual 0

    // Prediction Graph (Probability Curve)
    DrawText("Predictive Model Curve", graphPanel.x + 20, graphPanel.y + 20, 24, textLight);
    
    if (!prediction_curve.empty()) {
        float plotX = graphPanel.x + 50;
        float plotY = graphPanel.y + 80;
        float plotW = graphPanel.width - 70;
        float plotH = graphPanel.height - 120;
        
        DrawLineV({plotX, plotY}, {plotX, plotY + plotH}, textMute);
        DrawLineV({plotX, plotY + plotH}, {plotX + plotW, plotY + plotH}, textMute);
        
        DrawText("100%", plotX - 45, plotY, 12, textMute);
        DrawText("0%", plotX - 30, plotY + plotH - 10, 12, textMute);
        
        // Threshold line at 0.5 (50%)
        float threshY = plotY + plotH / 2;
        DrawLineEx({plotX, threshY}, {plotX + plotW, threshY}, 1.0f, {255, 255, 255, 50});
        DrawText("Decision (0.5)", plotX + plotW - MeasureText("Decision (0.5)", 10), threshY - 15, 10, textMute);
        
        double min_x = prediction_curve[0].first;
        double max_x = prediction_curve[0].first;
        for (const auto& pt : prediction_curve) {
            if (pt.first < min_x) min_x = pt.first;
            if (pt.first > max_x) max_x = pt.first;
        }
        
        if (max_x > min_x) {
            std::vector<Vector2> points;
            for (const auto& pt : prediction_curve) {
                float x = plotX + ((pt.first - min_x) / (max_x - min_x)) * plotW;
                float y = plotY + plotH - (pt.second * plotH);
                points.push_back({x, y});
                // Draw point
                DrawCircleV({x, y}, 2.0f, colorSource);
            }
            // Draw connecting lines if ordered by X
            for (size_t i = 1; i < points.size(); ++i) {
                if (prediction_curve[i-1].first <= prediction_curve[i].first) {
                    DrawLineEx(points[i-1], points[i], 2.0f, colorSource);
                }
            }
        }
        
        draw_text_centered("Key Feature (e.g. Queue Occupancy)", plotX + plotW/2, plotY + plotH + 15, 14, textMute.r, textMute.g, textMute.b, 255);
    } else {
        draw_text_centered("No prediction data available", graphPanel.x + graphPanel.width/2, graphPanel.y + graphPanel.height/2, 16, textMute.r, textMute.g, textMute.b, 255);
    }

    EndDrawing();
}

} // namespace qos_sim
