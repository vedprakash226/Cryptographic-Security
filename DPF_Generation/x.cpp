#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <stdexcept>
#include <cstdint>
#include <string>

// Typedef for convenience
typedef uint64_t u64;

// --- PRNG and Seed Expansion ---
static std::random_device rd;

struct child {
    u64 leftSeed, rightSeed;
    bool leftFlag, rightFlag;
};

child Expand(u64 seed, u64 ctr_base = 0) {
    u64 init = seed ^ 0x243f6a8885a308d3ULL ^ (0x9e3779b97f4a7c15ULL * (ctr_base + 1));
    std::mt19937_64 prng(init);
    child out;
    out.leftSeed = prng();
    out.rightSeed = prng();
    out.leftFlag = (prng() & 1ULL);
    out.rightFlag = (prng() & 1ULL);
    return out;
}

// --- DPF Key Structure (Full Domain Correction) ---
class DPFKey {
public:
    u64 seed;
    bool t0;
    std::vector<u64> cw_sL, cw_sR;
    std::vector<bool> cw_tL, cw_tR;
    u64 final_cw;
};

// MSB-first bit helper
static inline bool bit_msb(u64 x, u64 level, u64 depth) {
    if (depth == 0) return 0;
    return static_cast<bool>((x >> (depth - 1 - level)) & 1ULL);
}

// --- Correct DPF Generation ---
std::pair<DPFKey, DPFKey> generateDPF(u64 location, u64 value, u64 N) {
    if (location >= N && N > 0) throw std::runtime_error("location must be in [0, N)");
    u64 depth = (N <= 1) ? 0 : static_cast<u64>(ceil(log2((long double)N)));

    DPFKey k0, k1;
    std::mt19937_64 prng(rd());
    k0.seed = prng();
    k1.seed = prng();

    k0.t0 = 1;
    k1.t0 = 0;

    k0.cw_sL.resize(depth); k0.cw_sR.resize(depth);
    k1.cw_sL.resize(depth); k1.cw_sR.resize(depth);
    k0.cw_tL.resize(depth); k0.cw_tR.resize(depth);
    k1.cw_tL.resize(depth); k1.cw_tR.resize(depth);

    u64 s0 = k0.seed, s1 = k1.seed;
    bool t0 = k0.t0, t1 = k1.t0;

    for (u64 level = 0; level < depth; ++level) {
        bool b = bit_msb(location, level, depth);

        child ns0 = Expand(s0, level);
        child ns1 = Expand(s1, level);

        u64 cw_sL = ns0.leftSeed ^ ns1.leftSeed;
        u64 cw_sR = ns0.rightSeed ^ ns1.rightSeed;
        bool cw_tL = (ns0.leftFlag ^ ns1.leftFlag) ^ b;
        bool cw_tR = (ns0.rightFlag ^ ns1.rightFlag) ^ !b;

        k0.cw_sL[level] = cw_sL; k0.cw_sR[level] = cw_sR;
        k1.cw_sL[level] = cw_sL; k1.cw_sR[level] = cw_sR;
        k0.cw_tL[level] = cw_tL; k0.cw_tR[level] = cw_tR;
        k1.cw_tL[level] = cw_tL; k1.cw_tR[level] = cw_tR;

        if (t0) {
            ns0.leftSeed ^= cw_sL; ns0.rightSeed ^= cw_sR;
            ns0.leftFlag ^= cw_tL; ns0.rightFlag ^= cw_tR;
        }
        if (t1) {
            ns1.leftSeed ^= cw_sL; ns1.rightSeed ^= cw_sR;
            ns1.leftFlag ^= cw_tL; ns1.rightFlag ^= cw_tR;
        }

        if (!b) {
            s0 = ns0.leftSeed; t0 = ns0.leftFlag;
            s1 = ns1.leftSeed; t1 = ns1.leftFlag;
        } else {
            s0 = ns0.rightSeed; t0 = ns0.rightFlag;
            s1 = ns1.rightSeed; t1 = ns1.rightFlag;
        }
    }

    u64 final_cw = (s0 ^ s1) ^ value;
    k0.final_cw = final_cw;
    k1.final_cw = final_cw;

    return {k0, k1};
}

// --- Correct DPF Evaluation ---
static u64 evalOne(const DPFKey& key, u64 x, u64 N) {
    u64 depth = (N <= 1) ? 0 : static_cast<u64>(ceil(log2((long double)N)));
    u64 s = key.seed;
    bool t = key.t0;

    for (u64 level = 0; level < depth; ++level) {
        child ns = Expand(s, level);
        if (t) {
            ns.leftSeed ^= key.cw_sL[level];
            ns.rightSeed ^= key.cw_sR[level];
            ns.leftFlag ^= key.cw_tL[level];
            ns.rightFlag ^= key.cw_tR[level];
        }
        bool b = bit_msb(x, level, depth);
        if (!b) { s = ns.leftSeed; t = ns.leftFlag; }
        else { s = ns.rightSeed; t = ns.rightFlag; }
    }
    return s ^ (t ? key.final_cw : 0ULL);
}

// --- Helper Functions to Test Correctness ---
u64 evalDPF(DPFKey& key1, DPFKey& key2, u64 location, u64 N) {
    return evalOne(key1, location, N) ^ evalOne(key2, location, N);
}

bool EvalFull(DPFKey &k0, DPFKey &k1, u64 N, u64 index, u64 value) {
    for (u64 x = 0; x < N; ++x) {
        u64 y = evalDPF(k0, k1, x, N);
        u64 want = (x == index) ? value : 0ULL;
        if (y != want) {
             std::cerr << "Mismatch at x=" << x << ": got " << y << ", want " << want << std::endl;
            return false;
        }
    }
    return true;
}

// --- Main Test Driver ---
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./main <DPF_size> <num_tests>" << std::endl;
        return 1;
    }

    u64 N = std::stoull(argv[1]);
    u64 count = std::stoull(argv[2]);

    for (int i = 0; i < count; i++) {
        u64 location = rd() % N;
        u64 value = (static_cast<u64>(rd()) << 32) ^ rd();

        auto [d0, d1] = generateDPF(location, value, N);
        std::cout << "Case " << i + 1 << " (N=" << N << ", alpha=" << location << "): "
                  << (EvalFull(d0, d1, N, location, value) ? "TEST Passed" : "TEST Failed") << std::endl;
    }
    return 0;
}