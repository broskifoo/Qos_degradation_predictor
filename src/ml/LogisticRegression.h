#pragma once

#include "../utils/CSVReader.h"
#include <vector>
#include <string>

namespace qos_sim {

struct EvaluationMetrics {
    double accuracy;
    double precision;
    double recall;
    double f1_score;
    int tp, fp, tn, fn;
};

class LogisticRegression {
private:
    std::vector<double> weights;
    double bias;
    double learning_rate;
    int epochs;

public:
    LogisticRegression(double lr, int ep);

    void train(const std::vector<DataPoint>& data);
    int predict(const DataPoint& dp) const;
    double predict_prob(const DataPoint& dp) const;
    
    EvaluationMetrics evaluate(const std::vector<DataPoint>& data) const;
    void save_metrics(const EvaluationMetrics& metrics, const std::string& metrics_path, const std::string& confusion_matrix_path) const;
    void save_model_weights(const std::string& weights_path) const;
    void analyze_feature_correlation(const std::vector<DataPoint>& data) const;
};

} // namespace qos_sim
