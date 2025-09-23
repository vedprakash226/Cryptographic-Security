#pragma once

#include "shares.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// write a vector to a file by appending it
inline void write_vector(const std::string& filename, const Share& vec) {
    std::ofstream file(filename, std::ios_base::app);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    for (size_t i = 0; i < vec.size(); ++i)
        file << vec.data[i] << (i == vec.size() - 1 ? "" : " ");
    file << "\n";
}

// reading of all the vector from a file
inline std::vector<Share> read_vector(const std::string& filename, int k) {
    std::vector<Share> all_vectors;
    std::ifstream input(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Could not open file for reading: " + filename);
    }
    std::string line;
    while (std::getline(input, line)) {
        std::stringstream ss(line);
        std::vector<uint32_t> vec_data;
        uint32_t val;
        while (ss >> val) {
            vec_data.push_back(val);
        }
        if (vec_data.size() != k) {
             throw std::runtime_error("Malformed data in " + filename + ": unexpected vector size.");
        }
        all_vectors.emplace_back(vec_data);
    }
    return all_vectors;
}

// reads the queries from a file
inline std::vector<std::pair<int, int>> read_queries(const std::string& filename) {
    std::vector<std::pair<int, int>> queries;
    std::ifstream input(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Could not open queries file: " + filename);
    }
    int u, i;
    while (input >> u >> i) {
        queries.push_back({u, i});
    }
    return queries;
}
