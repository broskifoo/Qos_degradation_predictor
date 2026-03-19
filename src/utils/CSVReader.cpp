#include "CSVReader.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace qos_sim {

std::vector<DataPoint> CSVReader::read_dataset(const std::string& filepath) {
    std::vector<DataPoint> dataset;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open dataset file " << filepath << "\n";
        return dataset;
    }

    std::string line;
    // Read header and ignore
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        DataPoint dp;

        try {
            std::getline(ss, cell, ','); dp.avg_rtt = std::stod(cell);
            std::getline(ss, cell, ','); dp.rtt_variance = std::stod(cell);
            std::getline(ss, cell, ','); dp.queue_occupancy = std::stod(cell);
            std::getline(ss, cell, ','); dp.packet_loss = std::stod(cell);
            std::getline(ss, cell, ','); dp.throughput = std::stod(cell);
            std::getline(ss, cell, ','); dp.jitter = std::stod(cell);
            std::getline(ss, cell, ','); dp.qos_label = std::stoi(cell);
            dataset.push_back(dp);
        } catch (...) {
            std::cerr << "Warning: Error parsing a row in CSV. Skipping...\n";
        }
    }

    return dataset;
}

} // namespace qos_sim
