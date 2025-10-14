// gen_queries.cpp
// Simple (naive) DPF generator and tester
// Usage: ./gen_queries <DPF_size> <num_DPFs>
//
// This naive implementation stores, for each key, a full-length array of
// shares such that share0[i] XOR share1[i] = f(i), where f is the point function:
// f(i) = value if i == location, else 0.
// This meets the assignment interface and EvalFull correctness checks.
// To implement the compact, tree-based DPF (PRG + correction words) see notes below.

#include <bits/stdc++.h>
using namespace std;

using u64 = unsigned long long;
using i64 = long long;

// A DPF key (naive representation): full vector of 64-bit shares
struct DPFKey {
    vector<u64> shares;  // length = domain size
};

// Crypto-style RNG per assignment suggestion
static std::mt19937_64 make_rng() {
    std::random_device rd;
    // random_device may be non-deterministic; seed mt19937_64 with it
    std::seed_seq seq{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    return std::mt19937_64(seq);
}

// generateDPF: naive full-array construction
// location: target index in [0, size)
// value: target 64-bit value at location
// returns pair of DPFKey (k0, k1)
pair<DPFKey, DPFKey> generateDPF(size_t size, size_t location, u64 value, std::mt19937_64 &rng) {
    DPFKey k0, k1;
    k0.shares.assign(size, 0ULL);
    k1.shares.assign(size, 0ULL);

    // choose random shares for key0 (full array)
    for (size_t i = 0; i < size; ++i) {
        // uniform 64-bit random
        u64 r = rng();
        k0.shares[i] = r;
    }

    // key1 shares = key0 shares XOR f
    // f(i) = value if i == location else 0
    for (size_t i = 0; i < size; ++i) {
        if (i == location) k1.shares[i] = k0.shares[i] ^ value;
        else k1.shares[i] = k0.shares[i] ^ 0ULL;
    }

    return {k0, k1};
}

// evalDPF: returns the share for a key at an index
u64 evalDPF(const DPFKey &key, size_t index) {
    if (index >= key.shares.size()) {
        throw out_of_range("evalDPF: index out of range");
    }
    return key.shares[index];
}

// EvalFull: evaluate both keys across all domain points, combine by XOR, and check correctness.
// Returns true if test passes, false otherwise.
bool EvalFull(const DPFKey &k0, const DPFKey &k1, size_t size, size_t location, u64 value, bool verbose=false) {
    if (k0.shares.size() != size || k1.shares.size() != size) {
        if (verbose) cerr << "EvalFull: key sizes do not match given size\n";
        return false;
    }

    bool ok = true;
    for (size_t i = 0; i < size; ++i) {
        u64 a = evalDPF(k0, i);
        u64 b = evalDPF(k1, i);
        u64 comb = a ^ b;
        if (i == location) {
            if (comb != value) {
                if (verbose) cerr << "Mismatch at location " << i << ": expected " << value << " got " << comb << "\n";
                ok = false;
                // do not break; check entire domain for full diagnostics
            }
        } else {
            if (comb != 0ULL) {
                if (verbose) cerr << "Non-zero at index " << i << ": got " << comb << "\n";
                ok = false;
            }
        }
    }
    return ok;
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <DPF_size> <num_DPFs>\n";
        return 1;
    }

    size_t DPF_size = stoull(argv[1]);
    size_t num_DPFs = stoull(argv[2]);

    if (DPF_size == 0) {
        cerr << "DPF_size must be > 0\n";
        return 1;
    }

    auto rng = make_rng();
    std::uniform_int_distribution<size_t> loc_dist(0, DPF_size - 1);
    std::uniform_int_distribution<u64> val_dist(0, std::numeric_limits<u64>::max());

    for (size_t t = 0; t < num_DPFs; ++t) {
        size_t loc = loc_dist(rng);
        u64 val = val_dist(rng);

        auto keys = generateDPF(DPF_size, loc, val, rng);
        bool passed = EvalFull(keys.first, keys.second, DPF_size, loc, val);

        cout << "DPF " << t+1 << " (size=" << DPF_size << ", location=" << loc << ", value=" << val << "): ";
        cout << (passed ? "Test Passed" : "Test Failed") << "\n";
    }

    return 0;
}