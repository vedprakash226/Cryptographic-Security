#include "shares.hpp"
#include "utility.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <chrono>
using namespace std;

static bool fileExist(const string& path) {
    ifstream f(path);
    return f.good();
}

static unordered_map<int, Share> readMPC_result(const string& path, int k) {
    unordered_map<int, Share> res;
    ifstream in(path);
    if (!in.is_open()) throw runtime_error("Could not open. Error occurred at " + path);
    string line;
    while (getline(in, line)) {
        stringstream ss(line);
        int idx; ss >> idx;
        vector<ll> vec(k);
        for (int i = 0; i < k; ++i) ss >> vec[i];
        res.emplace(idx, Share{vec});
    }
    return res;
}

// Direct update: u_i' = u_i + v_j * (1 - <u_i, v_j>) (mirror MPC math mod p)
static void directProtocol(Share& ui, const Share& vj) {
    ll dot = 0;
    for (int i = 0; i < (int)ui.size(); i++)
        dot = addm(dot, mulm(ui.data[i], vj.data[i]));
    ll delta = subm(1, dot);
    for (int i = 0; i < (int)ui.size(); ++i)
        ui.data[i] = addm(ui.data[i], mulm(vj.data[i], delta));
}

static bool fileWait(const string& path, int max_seconds) {
    for (int s = 0; s < max_seconds; ++s) {
        if (fileExist(path)) return true;
        this_thread::sleep_for(chrono::seconds(1));
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <num_users> <num_items> <num_features> <num_queries>\n";
        return 1;
    }
    try {
        int m = stoi(argv[1]);
        int n = stoi(argv[2]);
        int k = stoi(argv[3]);
        (void)m; (void)n;

        // waiting for the P0 to finish its protocol computation
        if (!fileWait("mpc_results.done", /*max_seconds=*/3600)) {
            cerr << "Timeout waiting for mpc_results.done. Exiting.\n";
            return 2;
        }
        // sleep to ensure file flusing
        this_thread::sleep_for(chrono::milliseconds(200));

        // Now read mpc_results.txt
        if (!fileExist("mpc_results.txt")) {
            cerr << "mpc_results.txt not found after done flag. Exiting.\n";
            return 2;
        }

        // Read initial shares and reconstruct original U, V
        auto U0 = read_vector("U0.txt", k);
        auto U1 = read_vector("U1.txt", k);
        auto V0 = read_vector("V0.txt", k);
        auto V1 = read_vector("V1.txt", k);
        if (U0.size() != U1.size() || V0.size() != V1.size()) {
            throw runtime_error("U0/U1 or V0/V1 size mismatch");
            return 1;
        }

        vector<Share> U(U0.size(), Share(k));
        vector<Share> V(V0.size(), Share(k));
        for (int i = 0; i < U.size(); i++) U[i] = U0[i] + U1[i];
        for (int j = 0; j < V.size(); j++) V[j] = V0[j] + V1[j];

        // Replay queries directly
        auto queries = read_queries("queries.txt");
        for (const auto& q : queries) {
            int ui = q.first;
            int vj = q.second;
            directProtocol(U[ui], V[vj]);
        }

        // Read MPC results (only for updated users)
        auto mpc_res = readMPC_result("mpc_results.txt", k);

        // Compare and print
        cout << "Verifying MPC results against direct update for updated users" << "\n";
        bool all_match = true;
        for (const auto& kv : mpc_res) {
            int idx = kv.first;
            const Share& mpc_u = kv.second;
            const Share& dir_u = U[idx];

            bool match = true;
            if (mpc_u.size() != dir_u.size()){
                throw runtime_error("MPC and direct update size mismatch");
                return 0;
            }
            else {
                for (int i = 0; i < mpc_u.size(); ++i) {
                    if (mpc_u.data[i] != dir_u.data[i]){
                        match = false;
                        all_match = false;
                        break;  
                    }
                }
            }

            cout << "User " << idx << ": "<< (match ? "Matched" : "Not matched") << "\n";
        }

        if (all_match) {
            cout << "Successful match between all MPC and direct update.\n";
        }
        return all_match ? 0 : 3;

    } catch (const exception& e) {
        cerr << "verify error: " << e.what() << "\n";
        return 1;
    }
}