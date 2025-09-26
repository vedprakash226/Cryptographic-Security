#include "shares.hpp"
#include "utility.hpp"
#include "common.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdlib>
using namespace std;

void generator(const string& prefix, int row, int k) {
    // naming of the files
    ofstream(prefix + "0.txt").close(); 
    ofstream(prefix + "1.txt").close();

    for (int i=0;i<row;i++){
        Share v(k);
        v.randomizer();

        // creating share for P0
        Share v0(k);
        v0.randomizer();

        // creating share for P1
        Share v1 = v - v0;

        // for(auto &i: v0.data) cout<<i<<" "; cout<<endl; 
        // for(auto &i: v1.data) cout<<i<<" "; cout<<endl;

        write(prefix + "0.txt", v0);
        write(prefix + "1.txt", v1);
    }
    cout << "Generated shares for " << prefix << " matrix." << endl;
}

int main(int argc, char* argv[]) {
    // checks whether the number of arguments is correct
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <num_users> <num_items> <num_features> <queries>\n";
        return 1;
    }

    try {
        int m = stoi(argv[1]);
        int n = stoi(argv[2]);
        int k = stoi(argv[3]);
        int queries = stoi(argv[4]);

        // User & Item matrix generation
        generator("U", m, k);
        generator("V", n, k);

        // Generate random queries
        ofstream queries_file("queries.txt");
        if (!queries_file.is_open()) {
            throw runtime_error("Could not open queries.txt for writing.");
        }

        // New: user-only queries and selector shares
        ofstream queries_users("queries_users.txt");
        if (!queries_users.is_open()) {
            throw runtime_error("Could not open queries_users.txt for writing.");
        }
        ofstream s0f("S0.txt");
        ofstream s1f("S1.txt");
        if (!s0f.is_open() || !s1f.is_open()) {
            throw runtime_error("Could not open S0.txt/S1.txt for writing.");
        }

        for (int i=0;i<queries;i++) {
            int ui = random_uint32() % m;
            int vj = random_uint32() % n;

            // Full query only for the direct updation (just to verify the protocol) This is not used in the MPC protocol it remains secure 
            queries_file << ui << " " << vj << endl;

            // User-only line (for MPC parties)
            queries_users << ui << "\n";

            // Build one-hot e_j and additive shares s0, s1 mod mod
            vector<ll> s0(n), s1(n);
            for (int idx = 0; idx < n; ++idx) {
                ll r = random_uint32() % mod;
                s0[idx] = r;
                ll e = (idx == vj) ? 1 : 0;
                s1[idx] = subm(e,r);
            }
            for (int idx = 0; idx < n; ++idx) {
                s0f << s0[idx] << (idx+1== n?"":" ");
            }
            s0f << "\n";
            for (int idx = 0; idx < n; ++idx) {
                s1f << s1[idx] << (idx + 1 == n ? "" : " ");
            }
            s1f << "\n";
        }
        queries_file.close();
        queries_users.close();
        s0f.close();
        s1f.close();
        cout << "Generated " << queries << " queries (public for verify), users-only, and secret selectors S0/S1." << endl;

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
