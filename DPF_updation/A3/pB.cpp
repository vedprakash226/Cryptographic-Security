#include "common.hpp"
#include "shares.hpp"
#include "mpc.hpp"
#include "utility.hpp"
#include "DPF.hpp"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <chrono>
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
        string u_file, v_file;
        #ifdef ROLE_p0
            u_file = "U0.txt"; v_file = "V0.txt";
        #else
            u_file = "U1.txt"; v_file = "V1.txt";
        #endif
        vector<Share> u_shares = read_vector(u_file, k);
        vector<Share> v_shares = read_vector(v_file, k); // n rows, k dims
        int n = static_cast<int>(v_shares.size());

        // Read user-only queries (one user index per line)
        auto users_only = read_users("queries_users.txt");
        
        cout << role << ": Read data for " << users_only.size() << " queries (private item index)." << endl;
        cout << role << ": counts -> U=" << u_shares.size()
             << " V(n)=" << v_shares.size()
             << " k=" << k
             << " queries(users_only)=" << users_only.size() << endl;

        // New: read DPF keys and negate hints
        vector<DPFKey> dpf_keys;
        {
            ifstream kif(
            #ifdef ROLE_p0
                "DPF0.txt"
            #else
                "DPF1.txt"
            #endif
            );
            if (!kif.is_open()) throw runtime_error("Could not open DPF0.txt/DPF1.txt");
            while (kif.peek() != EOF) {
                streampos pos = kif.tellg();
                try {
                    DPFKey K = readKey(kif);
                    dpf_keys.push_back(K);
                } catch (...) {
                    kif.clear();
                    kif.seekg(pos);
                    string dummy;
                    if (!(kif >> dummy)) break;
                }
            }
        }
        vector<int> negateBits;
        {
            ifstream nf("DPF_NEG.txt");
            if (!nf.is_open()) throw runtime_error("Could not open DPF_NEG.txt");
            int b; while (nf >> b) negateBits.push_back(b);
        }
        if ((int)dpf_keys.size() != (int)users_only.size() ||
            (int)negateBits.size() != (int)users_only.size()) {
            throw runtime_error("DPF files count mismatch with queries");
        }

        MPCProtocol mpc(peer_sock, p2_sock);

        // No user reconstruction anymore; only item update via DPF
        // For verification we will also reconstruct updated users on P0.
        #ifdef ROLE_p0
        unordered_map<int, Share> final_reconstructed;
        std::vector<long long> item_us, user_us;
        #endif

        const ll inv2 = (mod + 1) / 2; // since mod is prime

        for (size_t q = 0; q < users_only.size(); ++q) {
            int user_idx = users_only[q];

            auto t_item_start =
            #ifdef ROLE_p0
                chrono::steady_clock::now();
            #else
                chrono::steady_clock::now();
            #endif

            // DPF-based selection
            const DPFKey& myKey = dpf_keys[q];
            bool negateThisParty =
            #ifdef ROLE_p0
                (negateBits[q] == 1);
            #else
                (negateBits[q] == 0);
            #endif
            Share v_sel_b = co_await mpc.DPF_select_item(myKey, negateThisParty, v_shares, n, k);

            // Item update share
            Share& u_b = u_shares[user_idx];
            Share M_b = co_await mpc.itemUpdateShare(u_b, v_sel_b, k);

            ll fcw_b = DPF_getFinalCW(myKey);
            Share masked(k);
            for (int d = 0; d < k; ++d) masked.data[d] = subm(M_b.data[d], fcw_b);
            co_await send_vec(peer_sock, masked);
            Share peer_masked = co_await recv_vec(peer_sock, k);
            Share FCWm = masked + peer_masked;

            vector<int8_t> signs = evalSigns(myKey, (u64)n, negateThisParty);
            for (int idx = 0; idx < n; ++idx) {
                ll s_mod = (signs[idx] == 1) ? 1 : (mod - 1);
                ll coeff = mulm(s_mod, inv2);
                for (int d = 0; d < k; ++d) {
                    ll add = mulm(coeff, FCWm.data[d]);
                    v_shares[idx].data[d] = addm(v_shares[idx].data[d], add);
                }
            }

            auto t_item_end = chrono::steady_clock::now();

            auto t_user_start = chrono::steady_clock::now();
            // User update
            Share u_prime_b = co_await mpc.updateProtocol(u_b, v_sel_b, k);
            u_shares[user_idx] = u_prime_b;

            co_await send_vec(peer_sock, u_prime_b);
            Share u_prime_peer = co_await recv_vec(peer_sock, k);
            Share u_reconstructed = u_prime_b + u_prime_peer;
            #ifdef ROLE_p0
                final_reconstructed[user_idx] = u_reconstructed;
                Share new_p0(k); new_p0.randomizer();
                Share new_p1 = u_reconstructed - new_p0;
                u_shares[user_idx] = new_p0;
                co_await send_vec(peer_sock, new_p1);
            #else
                Share new_p1 = co_await recv_vec(peer_sock, k);
                u_shares[user_idx] = new_p1;
            #endif
            auto t_user_end = chrono::steady_clock::now();

            #ifdef ROLE_p0
                item_us.push_back(chrono::duration_cast<chrono::microseconds>(t_item_end - t_item_start).count());
                user_us.push_back(chrono::duration_cast<chrono::microseconds>(t_user_end - t_user_start).count());
            #endif
        }

        // Signal end of protocol to P2
        #ifdef ROLE_p0
            co_await send_val(p2_sock, 0);
        #endif

        // New: reconstruct final V at P0 (for verification only)
        #ifdef ROLE_p0
        {
            // request P1 to dump its final V shares
            co_await send_val(peer_sock, (ll)-1);
            ofstream vout("mpc_V_results.txt", ios::trunc);
            if (!vout.is_open()) throw runtime_error("Could not open mpc_V_results.txt for writing");
            for (int idx = 0; idx < n; ++idx) {
                Share peer_row = co_await recv_vec(peer_sock, k);
                Share recon = v_shares[idx] + peer_row;
                vout << idx;
                for (auto v : recon.data) vout << " " << v;
                vout << "\n";
            }
            vout.close();
            cout << "P0: Wrote mpc_V_results.txt" << endl;
        }
        #else
        {
            ll tag = co_await recv_val(peer_sock);
            if (tag != -1) throw runtime_error("Unexpected tag while dumping V shares");
            for (int idx = 0; idx < n; ++idx) {
                co_await send_vec(peer_sock, v_shares[idx]);
            }
        }
        #endif

        // Write final user reconstructions and completion flag
        #ifdef ROLE_p0
        {
            ofstream out("mpc_results.txt", ios::trunc);
            if (!out.is_open()) throw runtime_error("Could not open mpc_results.txt for writing");
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

        #ifdef ROLE_p0
        {
            // timings file
            ofstream tf("timings.txt", ios::trunc);
            tf << "query,item_us,user_us\n";
            for (size_t i = 0; i < item_us.size(); ++i)
                tf << i << "," << item_us[i] << "," << user_us[i] << "\n";
            tf.close();
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
