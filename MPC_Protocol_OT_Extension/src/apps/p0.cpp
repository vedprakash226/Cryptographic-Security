#include "common.hpp"
#include "shares.hpp"
#include "mpc.hpp"
#include "utility.hpp"
#include "private_index_select.hpp"
#include <iostream>
#include <unordered_map>

#if !defined(ROLE_p0)
#error "ROLE must be defined as ROLE_p0"
#endif

awaitable<void> run_protocol(boost::asio::io_context& io_context, int k) {
    try {
        // Setup connections
        tcp::socket p1_sock = co_await setup_peer_connection(io_context);
        tcp::socket p2_sock = co_await setup_p2_connection(io_context);
        std::cout << "P0: Connections established." << std::endl;

        // Read user and item shares
        std::vector<Share> u_shares = read_vector("U0.txt", k);
        std::vector<Share> v_shares = read_vector("V0.txt", k);
        auto queries = read_queries("queries.txt");
        std::cout << "P0: Read initial data for " << queries.size() << " queries." << std::endl;

        MPCProtocol mpc(p1_sock, p2_sock);
        std::unordered_map<int, Share> final_reconstructed;

        for (const auto& query : queries) {
            int user_idx = query.first;
            int item_idx = query.second;

            // Use oblivious transfer to keep the query index confidential
            int private_index = co_await oblivious_transfer(p1_sock, item_idx);

            Share& u_b = u_shares[user_idx];
            const Share& v_b = v_shares[private_index];

            // Securely update the user share
            Share u_prime_b = co_await mpc.updateProtocol(u_b, v_b, k);
            u_shares[user_idx] = u_prime_b;

            // Reconstruct updated vector
            co_await send_vec(p1_sock, u_prime_b);
            Share u_prime_peer = co_await recv_vec(p1_sock, k);
            Share u_reconstructed = u_prime_b + u_prime_peer;

            // Sample new random share and send to P1
            Share new_p0(k);
            new_p0.randomizer();
            Share new_p1 = u_reconstructed - new_p0;

            u_shares[user_idx] = new_p0;
            co_await send_vec(p1_sock, new_p1);

            final_reconstructed[user_idx] = u_reconstructed;
        }

        // Write final results to file
        std::ofstream out("mpc_results.txt", std::ios::trunc);
        if (!out.is_open()) throw std::runtime_error("Could not open mpc_results.txt for writing");

        for (const auto& kv : final_reconstructed) {
            out << kv.first;
            for (auto v : kv.second.data) out << " " << v;
            out << "\n";
        }
        out.close();
        std::cout << "P0: Wrote mpc_results.txt" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "P0 caught exception: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <num_users> <num_items> <num_features> <num_queries>\n";
        return 1;
    }
    int k = std::stoi(argv[3]);

    boost::asio::io_context io_context(1);
    co_spawn(io_context, run_protocol(io_context, k), detached);
    io_context.run();
    return 0;
}