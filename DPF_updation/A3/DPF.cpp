#include <bits/stdc++.h>
#include "DPF.hpp"
#include "utility.hpp" // for mod/norm if needed
using namespace std;

// typedefs for convinence
// typedef uint64_t u64;   // moved to header
static random_device rd;  // obtain a random number from hardware

// structure to hold the left and right child seeds and flags
struct child{
    u64 leftSeed, rightSeed;
    bool leftFlag, rightFlag;
};

// child PRG
static child Expand(u64 seed, u64 ctr_base = 0){
    mt19937_64 prng(seed);
    child next;
    next.leftSeed = prng();
    next.rightSeed = prng();
    next.leftFlag = (prng() & 1ULL);
    next.rightFlag = (prng() & 1ULL);
    return next;
}

DPFKey::DPFKey(int n){
    seed = rd();
    cw_s.resize(n);
    t0 = 0;
    final_cw = 0;
}

pair<DPFKey, DPFKey> generateDPF(u64 location, u64 value, u64 N){
    if(location>=N) throw runtime_error("location must in [0,N)");
    u64 depth = N<=1?0:(int)ceil(log2(N));

    DPFKey k0(depth), k1(depth);
    k0.t0 = 1;
    k1.t0 = 0;

    u64 lSeed = k0.seed, rSeed = k1.seed;
    bool lFlag = k0.t0,   rFlag = k1.t0;

    for(u64 level = 0; level < depth; level++){
        bool pathBit = (location&(1ULL<<(depth-1-level)));
        child ns0 = Expand(lSeed, level);
        child ns1 = Expand(rSeed, level);

        u64 correction_word;
        bool leftAdvice  = (ns0.leftFlag  ^ ns1.leftFlag  ^ (pathBit == 0));
        bool rightAdvice = (ns0.rightFlag ^ ns1.rightFlag ^ (pathBit == 1));

        if(pathBit){
            correction_word = ns0.leftSeed ^ ns1.leftSeed;
        }else{
            correction_word = ns0.rightSeed ^ ns1.rightSeed;
        }

        if(lFlag){
            ns0.leftSeed  ^= correction_word;
            ns0.leftFlag  ^= leftAdvice;
            ns0.rightSeed ^= correction_word;
            ns0.rightFlag ^= rightAdvice;
        }else{
            ns1.leftSeed  ^= correction_word;
            ns1.leftFlag  ^= leftAdvice;
            ns1.rightSeed ^= correction_word;
            ns1.rightFlag ^= rightAdvice;
        }

        if(pathBit){
            lSeed = ns0.rightSeed; lFlag = ns0.rightFlag;
            rSeed = ns1.rightSeed; rFlag = ns1.rightFlag;
        }else{
            lSeed = ns0.leftSeed;  lFlag = ns0.leftFlag;
            rSeed = ns1.leftSeed;  rFlag = ns1.leftFlag;
        }

        correctionWord cw = {correction_word, leftAdvice, rightAdvice};
        k0.cw_s[level] = cw;
        k1.cw_s[level] = cw;
    }

    // Final CW for zero payload
    u64 final_correctionWord = (lSeed ^ rSeed) ^ value;

    // Split additively mod p so FCW0 + FCW1 = 0 (so Step 3 yields FCWm = M)
    ll r = (ll)(rd() % mod);
    k0.final_cw =  r;
    k1.final_cw = norm(-r);

    return {k0, k1};
}

// Return the leaf flag at a location
bool evalFlagAt(const DPFKey& key, u64 location, u64 N) {
    u64 depth = N<=1?0:(int)ceil(log2(N));
    u64 currSeed = key.seed;
    bool currentFlag = key.t0;
    for (u64 level = 0; level < depth; ++level) {
        bool pathBit = ((location >> (depth - 1 - level)) & 1ULL);
        child ns0 = Expand(currSeed, level);
        if (currentFlag) {
            ns0.leftSeed  ^= key.cw_s[level].cw;
            ns0.rightSeed ^= key.cw_s[level].cw;
            ns0.leftFlag  ^= key.cw_s[level].leftAdviceBit;
            ns0.rightFlag ^= key.cw_s[level].rightAdviceBit;
        }
        if (pathBit){
            currSeed = ns0.rightSeed;
            currentFlag = ns0.rightFlag;
        }else{
            currSeed = ns0.leftSeed;
            currentFlag = ns0.leftFlag;
        }
    }
    return currentFlag;
}

// Signs in {+1,-1}; optional global negation
vector<int8_t> evalSigns(const DPFKey& key, u64 N, bool negateThisParty) {
    vector<int8_t> s(N, 0);
    for (u64 j = 0; j < N; ++j) {
        int v = evalFlagAt(key, j, N) ? -1 : 1;
        if (negateThisParty) v = -v;
        s[j] = (int8_t)v;
    }
    return s;
}

// Serialization (one line per key)
void writeKey(ostream& out, const DPFKey& k) {
    int depth = (int)k.cw_s.size();
    out << depth << " " << k.seed << " " << (int)k.t0 << " " << norm(k.final_cw);
    for (int i = 0; i < depth; ++i) {
        out << " " << k.cw_s[i].cw << " " << (int)k.cw_s[i].leftAdviceBit
            << " " << (int)k.cw_s[i].rightAdviceBit;
    }
    out << "\n";
}

DPFKey readKey(istream& in) {
    int depth; u64 seed; int t0i; long long fcw;
    if (!(in >> depth >> seed >> t0i >> fcw)) throw runtime_error("Malformed DPF key");
    DPFKey k(depth);
    k.seed = seed;
    k.t0 = (t0i != 0);
    k.final_cw = norm(fcw);
    for (int i = 0; i < depth; ++i) {
        unsigned long long cw; int l, r;
        if (!(in >> cw >> l >> r)) throw runtime_error("Malformed DPF CW entries");
        k.cw_s[i] = correctionWord{(u64)cw, (bool)l, (bool)r};
    }
    return k;
}

ll DPF_getFinalCW(const DPFKey& k) { return norm(k.final_cw); }