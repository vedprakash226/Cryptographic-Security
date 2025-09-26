#pragma once

#include "shares.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
typedef long long int ll;

// write a vector to a file by appending it
inline void write(const string& filename, const Share& vec) {
    ofstream file(filename, ios_base::app);
    if (!file.is_open()) {
        throw runtime_error("Could not open file for writing: " + filename);
    }
    for (size_t i = 0; i < vec.size(); i++)
        file << vec.data[i] << (i == vec.size() - 1 ? "" : " ");
    file << "\n";
}

// reading of all the vector from a file
inline vector<Share> read_vector(const string& filename, int k) {
    vector<Share> all_vectors;
    ifstream input(filename);
    if (!input.is_open()) {
        throw runtime_error("Could not open file for reading: " + filename);
    }
    string line;
    while (getline(input, line)) {
        stringstream ss(line);
        vector<ll> vec_data;
        ll val;
        while (ss >> val) {
            vec_data.push_back(val);
        }
        if (vec_data.size() != k) {
             throw runtime_error("Malformed data in " + filename + ": unexpected vector size.");
        }
        all_vectors.emplace_back(vec_data);
    }
    return all_vectors;
}

// reads the queries from a file
inline vector<pair<int, int>> read_queries(const string& filename) {
    vector<pair<int, int>> queries;
    ifstream input(filename);
    if (!input.is_open()) {
        throw runtime_error("Could not open queries file: " + filename);
    }
    int u, i;
    while (input >> u >> i) {
        queries.push_back({u, i});
    }
    return queries;
}
