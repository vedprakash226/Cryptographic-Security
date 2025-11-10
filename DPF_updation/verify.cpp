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

// Direct step: apply both updates using the same pre-step values
static void directStep(vector<Share>& U, vector<Share>& V, int ui, int vj) {
    Share u_old = U[ui];
    Share v_old = V[vj];

    ll dot = 0;
    for (int t = 0; t < (int)u_old.size(); ++t)
        dot = addm(dot, mulm(u_old.data[t], v_old.data[t]));
    ll delta = subm(1, dot);

    // v_j' = v_j + u_i * delta
    for (int t = 0; t < (int)v_old.size(); ++t)
        V[vj].data[t] = addm(V[vj].data[t], mulm(u_old.data[t], delta));

    // u_i' = u_i + v_j * delta
    for (int t = 0; t < (int)u_old.size(); ++t)
        U[ui].data[t] = addm(U[ui].data[t], mulm(v_old.data[t], delta));
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

        // wait for completion flag from P0
        if (!fileWait("mpc_results.done", /*max_seconds=*/3600)) {
            cerr << "Timeout waiting for mpc_results.done. Exiting.\n";
            return 2;
        }
        this_thread::sleep_for(chrono::milliseconds(200));

        // Items results will be in mpc_V_results.txt

        // reconstruct original U, V from shares
        auto U0 = read_vector("U0.txt", k);
        auto U1 = read_vector("U1.txt", k);
        auto V0 = read_vector("V0.txt", k);
        auto V1 = read_vector("V1.txt", k);

        vector<Share> U(U0.size(), Share(k));
        vector<Share> V(V0.size(), Share(k));
        for (int i = 0; i < (int)U.size(); i++) U[i] = U0[i] + U1[i];
        for (int j = 0; j < (int)V.size(); j++) V[j] = V0[j] + V1[j];

        // Replay queries directly: item-only update
        auto queries = read_queries("queries.txt");
        for (const auto& q : queries) {
            int ui = q.first;
            int vj = q.second;
            directStep(U, V, ui, vj);
        }

        // Read MPC item results and user results
        auto mpc_res_items = readMPC_result("mpc_V_results.txt", k);
        auto mpc_res_users = readMPC_result("mpc_results.txt", k);
        bool all_match = true;

        // Compare items
        cout << "Verifying MPC results against direct update (items)" << "\n";
        for (const auto& kv : mpc_res_items) {
            int idx = kv.first;
            const Share& mpc_v = kv.second;
            const Share& dir_v = V[idx];
            bool match = (mpc_v.size() == dir_v.size());
            for (int t = 0; match && t < mpc_v.size(); ++t)
                match &= (mpc_v.data[t] == dir_v.data[t]);
            cout << "Item " << idx << ": " << (match ? "Matched" : "Not matched") << "\n";
            all_match &= match;
        }

        // Compare users
        cout << "Verifying MPC results against direct update (users)" << "\n";
        for (const auto& kv : mpc_res_users) {
            int idx = kv.first;
            const Share& mpc_u = kv.second;
            const Share& dir_u = U[idx];
            bool match = (mpc_u.size() == dir_u.size());
            for (int t = 0; match && t < mpc_u.size(); ++t)
                match &= (mpc_u.data[t] == dir_u.data[t]);
            cout << "User " << idx << ": " << (match ? "Matched" : "Not matched") << "\n";
            all_match &= match;
        }

        if (all_match) cout << "Successful match between MPC and direct updates (users and items).\n";
        return all_match ? 0 : 3;
    } catch (const exception& e) {
        cerr << "verify error: " << e.what() << "\n";
        return 1;
    }
}