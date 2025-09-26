#include "common.hpp"
#include "shares.hpp"
#include "mpc.hpp"
#include "utility.hpp"
#include "private_index_select.hpp"
#include <iostream>
#include <unordered_map>

#if !defined(ROLE_p1)
#error "ROLE must be defined as ROLE_p1"
#endif

awaitable<void> run_protocol(boost::asio::io_context& io_context, int k) {
    try {
        // Setup connection with P0
        tcp::socket peer_sock = co_await setup_peer_connection(io_context);
        std::cout << "P1: Connection established with P0." << std::endl;

        // Read the data
        std::vector<Share> v_shares = read_vector("V1.txt", k);
        auto queries = read_queries("queries.txt");
        std::cout << "P1: Read initial data for " << queries.size() << " queries." << std::endl;

        MPCProtocol mpc(peer_sock, p2_sock);

        for (const auto& query : queries) {
            int user_idx = query.first;
            int item_idx = query.second;

            // Use oblivious transfer to keep the query index confidential
            int private_index = co_await oblivious_transfer(peer_sock, item_idx);

            Share& v_b = v_shares[private_index];

            // Securely updating the user share
            Share u_prime_b = co_await mpc.updateProtocol(u_b, v_b, k);

            // Send updated share back to P0
            co_await send_vec(peer_sock, u_prime_b);
        }

    } catch (const std::exception& e) {
        std::cerr << "P1 caught exception: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <num_users> <num_items> <num_features> <num_queries>\n";
        return 1;
    }
    int k = std::stoi(argv[3]);

    std::cout.setf(std::ios::unitbuf);
    boost::asio::io_context io_context(1);
    co_spawn(io_context, run_protocol(io_context, k), detached);
    io_context.run();
    return 0;
}