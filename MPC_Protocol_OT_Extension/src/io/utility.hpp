#pragma once

#include <string>
#include <vector>
#include <utility>
#include "shares.hpp"

// Function to read a vector of shares from a file
std::vector<Share> read_vector(const std::string& filename, int k);

// Function to write a vector of shares to a file
void write(const std::string& filename, const Share& vec);

// Function to read queries from a file
std::vector<std::pair<int, int>> read_queries(const std::string& filename);