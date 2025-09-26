#include "shares.hpp"
#include "utility.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <utility>

// Function to generate user and item shares
void generator(const std::string& prefix, int row, int k) {
    std::ofstream(prefix + "0.txt").close(); 
    std::ofstream(prefix + "1.txt").close();

    for (int i = 0; i < row; i++) {
        Share v(k);
        v.randomizer();

        // Creating share for P0
        Share v0(k);
        v0.randomizer();

        // Creating share for P1
        Share v1 = v - v0;

        write(prefix + "0.txt", v0);
        write(prefix + "1.txt", v1);
    }
    std::cout << "Generated shares for " << prefix << " matrix." << std::endl;
}

// Function to generate random queries
void generate_queries(int queries, int m, int n) {
    std::ofstream queries_file("queries.txt");
    if (!queries_file.is_open()) {
        throw std::runtime_error("Could not open queries.txt for writing.");
    }
    for (int i = 0; i < queries; i++) {
        queries_file << random_uint32() % m << " " << random_uint32() % n << std::endl;
    }
    queries_file.close();
    std::cout << "Generated " << queries << " queries in queries.txt." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <num_users> <num_items> <num_features> <queries>\n";
        return 1;
    }

    try {
        int m = std::stoi(argv[1]);
        int n = std::stoi(argv[2]);
        int k = std::stoi(argv[3]);
        int queries = std::stoi(argv[4]);

        // User matrix creation
        generator("U", m, k);
        
        // Item matrix creation
        generator("V", n, k);

        // Generate random queries
        generate_queries(queries, m, n);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}