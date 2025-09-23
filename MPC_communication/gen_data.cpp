#include "shares.hpp"
#include "utility.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdlib>

void generate_shares_for_matrix(const std::string& prefix, int num_rows, int k) {
    std::ofstream(prefix + "0.txt").close(); // Clear files
    std::ofstream(prefix + "1.txt").close();

    for (int i = 0; i < num_rows; ++i) {
        Share v(k);
        v.randomizer(); // The "real" vector

        Share v0(k);
        v0.randomizer(); // Share for P0

        Share v1 = v - v0; // Share for P1

        write_vector(prefix + "0.txt", v0);
        write_vector(prefix + "1.txt", v1);
    }
    std::cout << "Generated shares for " << prefix << " matrix." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <num_users> <num_items> <num_features> <num_queries>\n";
        return 1;
    }

    try {
        int m = std::stoi(argv[1]);
        int n = std::stoi(argv[2]);
        int k = std::stoi(argv[3]);
        int num_queries = std::stoi(argv[4]);

        // Generate shares for U (user matrix)
        generate_shares_for_matrix("U", m, k);
        
        // Generate shares for V (item matrix)
        generate_shares_for_matrix("V", n, k);

        // Generate random queries
        std::ofstream queries_file("queries.txt");
        if (!queries_file.is_open()) {
            throw std::runtime_error("Could not open queries.txt for writing.");
        }
        for (int i = 0; i < num_queries; ++i) {
            queries_file << random_uint32() % m << " " << random_uint32() % n << "\n";
        }
        queries_file.close();
        std::cout << "Generated " << num_queries << " queries in queries.txt." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
