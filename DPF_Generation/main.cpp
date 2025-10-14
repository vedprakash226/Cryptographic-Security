#include<bits/stdc++.h>
using namespace std;

// typedefs for convinence
typedef uint64_t u64;
typedef uint8_t u8;
typedef long long int ll;

static random_device rd;  // obtain a random number from hardware
static mt19937 gen(rd()); // seed the generator
static uniform_int_distribution<u64> dist_u64; // define the range

// generate a random u64
u64 random_u64() {
    return dist_u64(gen);
}

struct ExpandOut{
    u64 sL, sR;  //child seeds
    u64 oL, oR;  //output masks
    u8 tL, tR;
};

ExpandOut Expand(u64 seed, u64 ctr_base = 0) {
    (void)seed;      // unused — randomness independent each call
    (void)ctr_base;  // unused — kept for interface consistency
    ExpandOut out;
    out.sL = random_u64();
    out.sR = random_u64();
    out.oL = random_u64();
    out.oR = random_u64();
    out.tL = (u8)(random_u64() & 1ULL);
    out.tR = (u8)(random_u64() & 1ULL);
    return out;
}

struct DPFKey{
    u64 seed;
    u8 t0;
    vector<u64> cw_s;
    vector<u8> cw_t;
    u64 final_cw;
};

pair<DPFKey, DPFKey> generateDPF(u64 location, u64 value, ll N){
    //checking if the location is within the bounds
    if(location>=N) throw runtime_error("location must in [0,N)");

    //finding the depth of the DPF tree
    ll depth = N<=1?0:(int)ceil(log2(N));

    //initializing the two DPF keys
    DPFKey k0, k1;
    k0.seed = random_u64();
    k1.seed = random_u64();
    k0.t0 = 0;
    k1.t0 = 1;

    k0.cw_s.resize(depth, 0);
    k1.cw_s.resize(depth, 0);
    k0.cw_t.resize(depth, 0);
    k1.cw_t.resize(depth, 0);
    k0.final_cw = 0;
    k1.final_cw = 0;

    u64 s0 = k0.seed, s1 = k1.seed;
    u8 t0 = k0.t0, t1 = k1.t0;

    // level order traversal of the DPF tree
    for(ll i=0;i<depth;i++){
         
    }

}

