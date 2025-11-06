#pragma once
#include <cstdint>
#include <vector>
#include <iosfwd>

using u64 = uint64_t;
using ll  = long long;

struct correctionWord {
    u64 cw;
    bool leftAdviceBit, rightAdviceBit;
};

class DPFKey {
public:
    u64 seed{};
    bool t0{};
    std::vector<correctionWord> cw_s;
    ll final_cw{}; // additive share mod p

    DPFKey() = default;
    explicit DPFKey(int depth);
};

// Generation with zero payload at target (user side)
std::pair<DPFKey, DPFKey> generateDPF(u64 location, u64 value, u64 N);

// Flag/sign evaluation helpers
bool evalFlagAt(const DPFKey& key, u64 location, u64 N);
std::vector<int8_t> evalSigns(const DPFKey& key, u64 N, bool negateThisParty);

// Serialization (one key per line)
void writeKey(std::ostream& out, const DPFKey& k);
DPFKey readKey(std::istream& in);

// Accessor for final_cw
ll DPF_getFinalCW(const DPFKey& k);