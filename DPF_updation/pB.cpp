#include "common.hpp"
#include "shares.hpp"
#include "mpc.hpp"
#include "utility.hpp"
#include <iostream>
#include <fstream>
#include <unordered_map>
using namespace std;

#if !defined(ROLE_p0) && !defined(ROLE_p1)
#error "ROLE must be defined as ROLE_p0 or ROLE_p1"
#endif

// setting up connection with P2
awaitable<tcp::socket> setup_p2_connection(boost::asio::io_context& io_context) {
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("p2", "9002");
    tcp::socket sock(io_context);
    co_await boost::asio::async_connect(sock, endpoints, use_awaitable);
    co_return sock;
}

// establishing peer connection (P0 connects to P1 and P1 accepts)
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

// main protocol execution ex
awaitable<void> run_protocol(boost::asio::io_context& io_context, int k) {
    const char* role =
    #ifdef ROLE_p0
        "P0";
    #else
        "P1";
    #endif
    try {
        tcp::socket p2_sock = co_await setup_p2_connection(io_context);
        tcp::socket peer_sock = co_await setup_peer_connection(io_context);
        cout << role << ": Connections established." << endl;

        // Identify to P2 (P0=0, P1=1)
        {
            uint8_t rid =
            #ifdef ROLE_p0
                0;
            #else
                1;
            #endif
            co_await boost::asio::async_write(p2_sock, boost::asio::buffer(&rid, 1), use_awaitable);
        }

        // reading the data 
        string u_file, v_file, s_file;
        #ifdef ROLE_p0
            u_file = "U0.txt"; v_file = "V0.txt"; s_file = "S0.txt";
        #else
            u_file = "U1.txt"; v_file = "V1.txt"; s_file = "S1.txt";
        #endif
        vector<Share> u_shares = read_vector(u_file, k);
        vector<Share> v_shares = read_vector(v_file, k); // n rows, k dims
        int n = static_cast<int>(v_shares.size());

        // Read selector shares: each line has n entries
        vector<Share> s_shares = read_vector(s_file, n);

        // Read user-only queries (one user index per line)
        auto users_only = read_users("queries_users.txt");
        if (static_cast<int>(users_only.size()) != static_cast<int>(s_shares.size())) {
            throw runtime_error("queries_users.txt and S.txt line count mismatch");
        }
        cout << role << ": Read data for " << users_only.size() << " queries (private item index)." << endl;
        cout << role << ": counts -> U=" << u_shares.size()
             << " V(n)=" << v_shares.size()
             << " k=" << k
             << " queries(users_only)=" << users_only.size()
             << " S-lines=" << s_shares.size() << endl;

        MPCProtocol mpc(peer_sock, p2_sock);

        // final reconstructed vector per user
        #ifdef ROLE_p0
        unordered_map<int, Share> final_reconstructed;
        #endif

        for (size_t q = 0; q < users_only.size(); ++q) {
            int user_idx = users_only[q];
            const Share& s_b = s_shares[q]; // length n selector share

            // Obliviously select v_j share without revealing j
            Share v_sel_b = co_await mpc.OT_select(s_b, v_shares, n, k);

            // securely updating the user share
            Share& u_b = u_shares[user_idx];
            Share u_prime_b = co_await mpc.updateProtocol(u_b, v_sel_b, k);
            u_shares[user_idx] = u_prime_b;

            // Reconstruct updated vector
            co_await send_vec(peer_sock, u_prime_b);
            Share u_prime_peer = co_await recv_vec(peer_sock, k);
            Share u_reconstructed = u_prime_b + u_prime_peer;

            // Re-share randomized
            #ifdef ROLE_p0
                Share new_p0(k);
                new_p0.randomizer();                 // fresh random share for P0
                Share new_p1 = u_reconstructed - new_p0; // complementary share for P1
                u_shares[user_idx] = new_p0;
                co_await send_vec(peer_sock, new_p1);
            #else
                Share new_p1 = co_await recv_vec(peer_sock, k);
                u_shares[user_idx] = new_p1;
            #endif

            #ifdef ROLE_p0
            final_reconstructed[user_idx] = u_reconstructed;
            #endif
        }

        // Signal end of protocol to P2
        #ifdef ROLE_p0
            co_await send_val(p2_sock, 0);
        #endif

        // writing final result and ok flag in file for verification 
        #ifdef ROLE_p0
        {
            ofstream out("mpc_results.txt", ios::trunc);
            if (!out.is_open()) throw runtime_error("Could not open mpc_results.txt for writing");
            std::cout << "P0: Writing " << final_reconstructed.size() << " users to mpc_results.txt\n";
            for (const auto& kv : final_reconstructed) {
                out << kv.first;
                for (auto v : kv.second.data) out << " " << v;
                out << "\n";
            }
            out.close();
            ofstream done("mpc_results.done", ios::trunc);
            done << "ok\n"; done.close();
            cout << "P0: Wrote mpc_results.txt and mpc_results.done" << endl;
        }
        #endif

    } catch (exception& e) {
        cerr << role << " caught exception: " << e.what() << endl;
    }
}


int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <num_users> <num_items> <num_features> <num_queries>\n";
        return 1;
    }
    int k = stoi(argv[3]);

    cout.setf(ios::unitbuf);
    boost::asio::io_context io_context(1);
    co_spawn(io_context, run_protocol(io_context, k), detached);
    io_context.run();
    return 0;
}
