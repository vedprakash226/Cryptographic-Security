#include "common.hpp"
#include "shares.hpp"
#include "mpc.hpp"
#include "utility.hpp"
#include <iostream>

#if !defined(ROLE_p0) && !defined(ROLE_p1)
#error "ROLE must be defined as ROLE_p0 or ROLE_p1"
#endif

// Setup connection to P2
awaitable<tcp::socket> setup_p2_connection(boost::asio::io_context& io_context) {
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("p2", "9002");
    tcp::socket sock(io_context);
    co_await boost::asio::async_connect(sock, endpoints, use_awaitable);
    co_return sock;
}

// Setup peer connection (P0 connects to P1, P1 accepts)
awaitable<tcp::socket> setup_peer_connection(boost::asio::io_context& io_context) {
    tcp::socket sock(io_context);
#ifdef ROLE_p0
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("p1", "9001");
    co_await boost::asio::async_connect(sock, endpoints, use_awaitable);
#else
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9001));
    sock = co_await acceptor.async_accept(use_awaitable);
#endif
    co_return sock;
}

// Main protocol execution
awaitable<void> run_protocol(boost::asio::io_context& io_context, int k) {
    const char* role = 
    #ifdef ROLE_p0
        "P0";
    #else
        "P1";
    #endif

    try {
        // Step 1: Setup connections
        tcp::socket p2_sock = co_await setup_p2_connection(io_context);
        tcp::socket peer_sock = co_await setup_peer_connection(io_context);
        std::cout << role << ": Connections established." << std::endl;

        // Step 2: Read initial data shares
        std::string u_file, v_file;
        #ifdef ROLE_p0
            u_file = "U0.txt"; v_file = "V0.txt";
        #else
            u_file = "U1.txt"; v_file = "V1.txt";
        #endif
        std::vector<Share> u_shares = read_vector(u_file, k);
        std::vector<Share> v_shares = read_vector(v_file, k);
        // to check the correctness, read the peer's shares as well
        std::vector<Share> u_shares_peer = read_vector(u_file == "U0.txt" ? "U1.txt" : "U0.txt", k);
        std::vector<Share> v_shares_peer = read_vector(v_file == "V0.txt" ? "V1.txt" : "V0.txt", k);

        auto queries = read_queries("queries.txt");
        std::cout << role << ": Read initial data for " << queries.size() << " queries." << std::endl;

        MPCProtocol mpc(peer_sock, p2_sock);

        // Step 3: Loop over queries and perform updates
        for (const auto& query : queries) {
            int user_idx = query.first;
            int item_idx = query.second;

            std::cout << "\n" << role << ": Processing query (user=" << user_idx << ", item=" << item_idx << ")." << std::endl;

            Share& u_b = u_shares[user_idx];
            const Share& v_b = v_shares[item_idx];

            Share& u_b_peer = u_shares_peer[user_idx];
            const Share& v_b_peer = v_shares_peer[item_idx];

            // Perform the secure update
            Share u_prime_b = co_await mpc.secure_update(u_b, v_b, k);
            u_shares[user_idx] = u_prime_b; // Update local share
            
            std::cout << role << ": Secure update complete." << std::endl;

            // For debugging: exchange final shares and reconstruct the updated vector
            co_await send_vec(peer_sock, u_prime_b);
            Share u_prime_peer = co_await recv_vec(peer_sock, k);
            Share u_reconstructed = u_prime_b + u_prime_peer;

            #ifdef ROLE_p0
                std::cout << "P0: Reconstructed u' for user " << user_idx << ": [";
                for(size_t i=0; i<u_reconstructed.size(); ++i) {
                    std::cout << u_reconstructed.data[i] << (i == u_reconstructed.size() - 1 ? "" : ", ");
                }
                std::cout << "]" << std::endl;
            #endif
        }

        // Signal end of protocol to P2
        #ifdef ROLE_p0
            co_await send_val(p2_sock, 0);
        #endif

    } catch (std::exception& e) {
        std::cerr << role << " caught exception: " << e.what() << std::endl;
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
