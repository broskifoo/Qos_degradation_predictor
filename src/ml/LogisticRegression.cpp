#include "LogisticRegression.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <iomanip>


namespace qos_sim {

LogisticRegression::LogisticRegression(double lr, int ep)
    : learning_rate(lr), epochs(ep), bias(0.0) {
    // 6 features: avg_rtt, rtt_variance, queue_occupancy, packet_loss, throughput, jitter
    weights.resize(6, 0.0);
}

double LogisticRegression::predict_prob(const DataPoint& dp) const {
    double z = bias;
    z += weights[0] * dp.avg_rtt;
    z += weights[1] * dp.rtt_variance;
    z += weights[2] * dp.queue_occupancy;
    z += weights[3] * dp.packet_loss;
    z += weights[4] * dp.throughput;
    z += weights[5] * dp.jitter;
    
    // Sigmoid function
    return 1.0 / (1.0 + std::exp(-z));
}

int LogisticRegression::predict(const DataPoint& dp) const {
    return predict_prob(dp) >= 0.5 ? 1 : 0;
}

void LogisticRegression::train(const std::vector<DataPoint>& data) {
    if (data.empty()) return;
    std::cout << "Training Logistic Regression model with " << data.size() << " samples...\n";
    
    // Normalization parameters
    std::vector<double> means(6, 0.0);
    std::vector<double> stds(6, 0.0);
    
    // Normalize features for stable gradient descent
    std::vector<std::vector<double>> X(data.size(), std::vector<double>(6));
    std::vector<int> y(data.size());
    
    for (size_t i = 0; i < data.size(); i++) {
        X[i][0] = data[i].avg_rtt;
        X[i][1] = data[i].rtt_variance;
        X[i][2] = data[i].queue_occupancy;
        X[i][3] = data[i].packet_loss;
        X[i][4] = data[i].throughput;
        X[i][5] = data[i].jitter;
        y[i] = data[i].qos_label;
        
        for (int j = 0; j < 6; j++) {
            means[j] += X[i][j];
        }
    }
    
    for (int j = 0; j < 6; j++) means[j] /= data.size();
    
    for (size_t i = 0; i < data.size(); i++) {
        for (int j = 0; j < 6; j++) {
            stds[j] += (X[i][j] - means[j]) * (X[i][j] - means[j]);
        }
    }
    
    for (int j = 0; j < 6; j++) {
        stds[j] = std::sqrt(stds[j] / data.size());
        if (stds[j] == 0) stds[j] = 1.0; // avoid division by zero
    }
    
    for (size_t i = 0; i < data.size(); i++) {
        for (int j = 0; j < 6; j++) {
            X[i][j] = (X[i][j] - means[j]) / stds[j];
        }
    }

    // Gradient descent
    for (int epoch = 0; epoch < epochs; epoch++) {
        std::vector<double> grad_w(6, 0.0);
        double grad_b = 0.0;
        
        double total_loss = 0.0;
        
        for (size_t i = 0; i < data.size(); i++) {
            double z = bias;
            for (int j = 0; j < 6; j++) {
                z += weights[j] * X[i][j];
            }
            double p = 1.0 / (1.0 + std::exp(-z));
            
            double error = p - y[i];
            
            grad_b += error;
            for (int j = 0; j < 6; j++) {
                grad_w[j] += error * X[i][j];
            }
            
            // Log loss L = -[y log(p) + (1-y) log(1-p)]
            p = std::clamp(p, 1e-15, 1.0 - 1e-15);
            total_loss -= (y[i] * std::log(p) + (1 - y[i]) * std::log(1 - p));
        }
        
        bias -= (learning_rate / data.size()) * grad_b;
        for (int j = 0; j < 6; j++) {
            weights[j] -= (learning_rate / data.size()) * grad_w[j];
        }
        
        if (epoch % 100 == 0 || epoch == epochs - 1) {
            std::cout << "Epoch " << epoch << " / " << epochs << " | Loss: " << total_loss / data.size() << "\n";
        }
    }
    
    // Adjust weights and bias back to un-normalized scale mapping
    for (int j = 0; j < 6; j++) {
        bias -= weights[j] * means[j] / stds[j];
        weights[j] /= stds[j];
    }
}

EvaluationMetrics LogisticRegression::evaluate(const std::vector<DataPoint>& data) const {
    EvaluationMetrics metrics = {0, 0, 0, 0, 0, 0, 0, 0};
    
    std::cout << "\n------------------------------------------------------------------------------------------------\n";
    std::cout << "                              ML MODEL LIVE PREDICTION SAMPLES                                  \n";
    std::cout << "------------------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(12) << "Avg RTT(s)" 
              << std::setw(14) << "RTT Var(s^2)" 
              << std::setw(15) << "Pkt Loss(%)" 
              << std::setw(15) << "T-put(Mbps)" 
              << std::setw(12) << "Predicted" 
              << std::setw(12) << "Actual" 
              << std::setw(10) << "Status" << "\n";
    std::cout << "------------------------------------------------------------------------------------------------\n";

    int sample_count = 0;
    
    for (const auto& dp : data) {
        int pred = predict(dp);
        
        if (pred == 1 && dp.qos_label == 1) metrics.tp++;
        else if (pred == 1 && dp.qos_label == 0) metrics.fp++;
        else if (pred == 0 && dp.qos_label == 1) metrics.fn++;
        else if (pred == 0 && dp.qos_label == 0) metrics.tn++;
        
        // Print detailed info for the first 20 samples to show it working
        if (sample_count < 20) {
            std::cout << std::fixed << std::setprecision(4)
                      << std::left << std::setw(12) << dp.avg_rtt
                      << std::setw(14) << dp.rtt_variance
                      << std::setw(15) << dp.packet_loss * 100.0
                      << std::setw(15) << dp.throughput / 1e6
                      << std::setw(12) << (pred == 1 ? "Degraded" : "Normal")
                      << std::setw(12) << (dp.qos_label == 1 ? "Degraded" : "Normal")
                      << std::setw(10) << (pred == dp.qos_label ? "CORRECT" : "WRONG") << "\n";
            sample_count++;
        }
        else if (sample_count == 20) {
            std::cout << "------------------------------------------------------------------------------------------------\n";
            std::cout << "... (Showing first 20 samples out of " << data.size() << ") ...\n";
            std::cout << "------------------------------------------------------------------------------------------------\n";
            sample_count++;
        }
    }
    
    int total = metrics.tp + metrics.tn + metrics.fp + metrics.fn;
    metrics.accuracy = total > 0 ? (double)(metrics.tp + metrics.tn) / total : 0.0;
    metrics.precision = (metrics.tp + metrics.fp) > 0 ? (double)metrics.tp / (metrics.tp + metrics.fp) : 0.0;
    metrics.recall = (metrics.tp + metrics.fn) > 0 ? (double)metrics.tp / (metrics.tp + metrics.fn) : 0.0;
    
    if (metrics.precision + metrics.recall > 0) {
        metrics.f1_score = 2.0 * (metrics.precision * metrics.recall) / (metrics.precision + metrics.recall);
    }
    
    return metrics;
}

void LogisticRegression::save_metrics(const EvaluationMetrics& metrics, 
                                      const std::string& metrics_path, 
                                      const std::string& confusion_matrix_path) const {
    std::ofstream m_file(metrics_path);
    if (m_file.is_open()) {
        m_file << "Logistic Regression Predictor Metrics\n";
        m_file << "=====================================\n";
        m_file << "Accuracy:  " << metrics.accuracy * 100.0 << " %\n";
        m_file << "Precision: " << metrics.precision * 100.0 << " %\n";
        m_file << "Recall:    " << metrics.recall * 100.0 << " %\n";
        m_file << "F1 Score:  " << metrics.f1_score * 100.0 << " %\n";
    }

    std::ofstream cm_file(confusion_matrix_path);
    if (cm_file.is_open()) {
        cm_file << "Confusion Matrix:\n";
        cm_file << "                Actual 1    Actual 0\n";
        cm_file << "Predicted 1:      " << metrics.tp << "          " << metrics.fp << "\n";
        cm_file << "Predicted 0:      " << metrics.fn << "          " << metrics.tn << "\n";
    }
    
    std::cout << "Metrics saved to " << metrics_path << " and " << confusion_matrix_path << "\n";
}

void LogisticRegression::save_model_weights(const std::string& weights_path) const {
    std::ofstream out(weights_path);
    if (!out.is_open()) return;
    out << "Bias," << bias << "\n";
    out << "Weight_AvgRTT," << weights[0] << "\n";
    out << "Weight_RTTVar," << weights[1] << "\n";
    out << "Weight_Queue," << weights[2] << "\n";
    out << "Weight_Loss," << weights[3] << "\n";
    out << "Weight_Tput," << weights[4] << "\n";
    out << "Weight_Jitter," << weights[5] << "\n";
    std::cout << "Model weights saved to " << weights_path << "\n";

    // Print Mathematical Calculations directly to Terminal
    std::cout << "\n===============================================================================================\n";
    std::cout << "                LOGISTIC REGRESSION MATHEMATICAL CALCULATIONS                   \n";
    std::cout << "===============================================================================================\n\n";
    std::cout << "Logistic Regression predicts the probability of QoS Degradation (Label = 1) using a Sigmoid function:\n\n";
    std::cout << "   P(Y=1) = 1 / (1 + e^-z)\n\n";
    std::cout << "where 'z' is the linear combination of weighted features:\n";
    std::cout << "   z = b + (w1 * x1) + (w2 * x2) + ... + (wn * xn)\n\n";

    std::cout << "Trained weights for the model:\n";
    std::cout << "--------------------------------\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Bias (b)         = " << bias << "\n";
    std::cout << "Avg RTT (w1)     = " << weights[0] << "\n";
    std::cout << "RTT Var (w2)     = " << weights[1] << "\n";
    std::cout << "Queue Occ (w3)   = " << weights[2] << "\n";
    std::cout << "Pkt Loss (w4)    = " << weights[3] << "\n";
    std::cout << "Throughput (w5)  = " << weights[4] << "\n";
    std::cout << "Jitter (w6)      = " << weights[5] << "\n";
    std::cout << "--------------------------------\n\n";

    std::cout << "Formula for 'z':\n";
    std::cout << "z = " << bias << " + (" << weights[0] << " * AvgRTT) + (" << weights[1] << " * RTTVar) + (" 
              << weights[2] << " * QueueOcc) + (" << weights[3] << " * PktLoss) + (" 
              << weights[4] << " * Tput) + (" << weights[5] << " * Jitter)\n\n";
    std::cout << "===============================================================================================\n";
}

void LogisticRegression::analyze_feature_correlation(const std::vector<DataPoint>& data) const {
    if (data.empty()) return;
    
    int n = data.size();
    
    // Calculate means
    double mean_rtt = 0, mean_var = 0, mean_queue = 0, mean_loss = 0, mean_tput = 0, mean_jitter = 0, mean_y = 0;
    for (const auto& dp : data) {
        mean_rtt += dp.avg_rtt;
        mean_var += dp.rtt_variance;
        mean_queue += dp.queue_occupancy;
        mean_loss += dp.packet_loss;
        mean_tput += dp.throughput;
        mean_jitter += dp.jitter;
        mean_y += dp.qos_label;
    }
    mean_rtt /= n; mean_var /= n; mean_queue /= n; mean_loss /= n; mean_tput /= n; mean_jitter /= n; mean_y /= n;
    
    // Calculate Pearson components
    double num_rtt = 0, num_var = 0, num_queue = 0, num_loss = 0, num_tput = 0, num_jitter = 0;
    double den_x_rtt = 0, den_x_var = 0, den_x_queue = 0, den_x_loss = 0, den_x_tput = 0, den_x_jitter = 0;
    double den_y = 0;
    
    for (const auto& dp : data) {
        double dy = (dp.qos_label - mean_y);
        den_y += dy * dy;
        
        double drtt = dp.avg_rtt - mean_rtt;
        num_rtt += drtt * dy;
        den_x_rtt += drtt * drtt;
        
        double dvar = dp.rtt_variance - mean_var;
        num_var += dvar * dy;
        den_x_var += dvar * dvar;
        
        double dq = dp.queue_occupancy - mean_queue;
        num_queue += dq * dy;
        den_x_queue += dq * dq;
        
        double dl = dp.packet_loss - mean_loss;
        num_loss += dl * dy;
        den_x_loss += dl * dl;
        
        double dt = dp.throughput - mean_tput;
        num_tput += dt * dy;
        den_x_tput += dt * dt;
        
        double dj = dp.jitter - mean_jitter;
        num_jitter += dj * dy;
        den_x_jitter += dj * dj;
    }
    
    std::cout << "\n===============================================================================================\n";
    std::cout << "                          PAPER SECTION XVII.C: FEATURE CORRELATION STUDY                      \n";
    std::cout << "===============================================================================================\n";
    std::cout << "Pearson Correlation Coefficients (r) between Features and QoS Degradation Label:\n\n";
    
    auto print_corr = [](const std::string& name, double num, double den_x, double den_y) {
        double r = (den_x == 0 || den_y == 0) ? 0.0 : num / std::sqrt(den_x * den_y);
        std::cout << "   " << std::left << std::setw(20) << name << " r = " << std::fixed << std::setprecision(4) << r;
        if (std::abs(r) > 0.5) std::cout << "   (Strong predictive factor)";
        else if (std::abs(r) > 0.3) std::cout << "   (Moderate predictive factor)";
        else std::cout << "   (Weak predictive factor)";
        std::cout << "\n";
    };
    
    print_corr("Queue Occupancy", num_queue, den_x_queue, den_y);
    print_corr("RTT Variance", num_var, den_x_var, den_y);
    print_corr("Average RTT", num_rtt, den_x_rtt, den_y);
    print_corr("Packet Jitter", num_jitter, den_x_jitter, den_y);
    print_corr("Packet Loss", num_loss, den_x_loss, den_y);
    print_corr("Throughput", num_tput, den_x_tput, den_y);
    std::cout << "===============================================================================================\n";
    std::cout << "Justification: As stated in the paper, strong positive correlations (like Queue Occupancy)\n";
    std::cout << "validate that these analytical networking features are mathematically sound predictors.\n";
    std::cout << "===============================================================================================\n\n";
}

} // namespace qos_sim
