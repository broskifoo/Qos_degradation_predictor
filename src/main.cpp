#include "utils/Config.h"
#include "utils/CSVReader.h"
#include "simulator/Simulator.h"
#include "ml/LogisticRegression.h"
#include "visualization/Dashboard.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstdlib>

int main() {
    std::cout << "Learning-Assisted QoS Degradation Prediction in Multi-Flow Packet-Switched Networks Simulator\n";
    std::cout << "===============================================================================================\n\n";

    // Build absolute paths relative to execution directory (assuming running from build/)
    std::string config_path = "../config/simulation_config.json";
    std::string dataset_path = "../data/qos_dataset.csv";
    std::string metrics_path = "../results/metrics.txt";
    std::string cm_path = "../results/confusion_matrix.txt";
    std::string weights_path = "../results/model_weights.txt";

    std::cout << "1. Loading configuration...\n";
    qos_sim::Config config = qos_sim::Config::load(config_path);

    std::cout << "2. Initializing Network Simulator...\n";
    qos_sim::Simulator simulator(config, dataset_path);

    std::cout << "3. Running Simulation & Generating Dataset...\n";
    simulator.run();

    std::cout << "\n4. Loading Generated Dataset...\n";
    std::vector<qos_sim::DataPoint> dataset = qos_sim::CSVReader::read_dataset(dataset_path);
    std::cout << "   Total samples loaded: " << dataset.size() << "\n";

    if (dataset.empty()) {
        std::cerr << "Dataset is empty. Exiting ML phase.\n";
        return 1;
    }

    // Shuffle dataset
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(dataset.begin(), dataset.end(), g);

    // 80-20 Train-Test split
    size_t split_idx = static_cast<size_t>(dataset.size() * 0.8);
    std::vector<qos_sim::DataPoint> train_data(dataset.begin(), dataset.begin() + split_idx);
    std::vector<qos_sim::DataPoint> test_data(dataset.begin() + split_idx, dataset.end());

    std::cout << "\n5. Initializing Logistic Regression Predictor...\n";
    qos_sim::LogisticRegression predictor(config.learning_rate, config.epochs);

    std::cout << "5.5 Performing Feature Correlation Analysis (Paper Section XVII.C)...\n";
    predictor.analyze_feature_correlation(dataset);

    std::cout << "6. Training Model...\n";
    predictor.train(train_data);

    std::cout << "\n7. Evaluating Model on Test Set...\n";
    qos_sim::EvaluationMetrics eval_metrics = predictor.evaluate(test_data);
    
    std::cout << "   Accuracy:  " << eval_metrics.accuracy * 100.0 << " %\n";
    std::cout << "   Precision: " << eval_metrics.precision * 100.0 << " %\n";
    std::cout << "   Recall:    " << eval_metrics.recall * 100.0 << " %\n";
    std::cout << "   F1 Score:  " << eval_metrics.f1_score * 100.0 << " %\n";

    std::cout << "\n8. Saving Results...\n";
    predictor.save_metrics(eval_metrics, metrics_path, cm_path);
    predictor.save_model_weights(weights_path);

    std::cout << "   (Automatically launching 'show_math.bat' to show mathematical calculations)\n";
    system("start \"\" \"..\\show_math.bat\"");

    std::vector<std::pair<double, double>> prob_curve;
    for (const auto& dp : test_data) {
        double prob = predictor.predict_prob(dp);
        // We'll plot Queue Occupancy vs Probability, since the feature correlation study says it's strong
        prob_curve.push_back({dp.queue_occupancy, prob});
    }
    std::sort(prob_curve.begin(), prob_curve.end());

    std::cout << "\n9. Presenting ML Results Dashboard...\n";
    {
        qos_sim::Dashboard results_dash(900, 500, "QoS Machine Learning Evaluation Results");
        while (!results_dash.should_close()) {
            results_dash.render_ml_results(
                eval_metrics.accuracy, eval_metrics.precision, 
                eval_metrics.recall, eval_metrics.f1_score, 
                eval_metrics.tp, eval_metrics.fp, 
                eval_metrics.tn, eval_metrics.fn,
                prob_curve
            );
        }
    }

    std::cout << "\nExecution successfully completed.\n";
    return 0;
}
