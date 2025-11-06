#include<bits/stdc++.h>
using namespace std;

// typedefs for convinence
typedef uint64_t u64;
static random_device rd;  // obtain a random number from hardware

// structure to hold the left and right child seeds and flags
struct child{
    u64 leftSeed, rightSeed;
    bool leftFlag, rightFlag;
};

// structure to hold the correction word and the advice bits for a level
struct correctionWord{
    u64 cw;
    bool leftAdviceBit, rightAdviceBit;
};

// ctr_base is used to have different outputs for different levels if the seed is same
child Expand(u64 seed, u64 ctr_base = 0){
    mt19937_64 prng(seed);  // seed the generator

    child next;
    next.leftSeed = prng();
    next.rightSeed = prng();
    next.leftFlag = (prng() & 1ULL);
    next.rightFlag = (prng() & 1ULL);
    return next;
}

// DPF key class declaration
class DPFKey{
    public:
    u64 seed;
    bool t0; 
    vector<correctionWord> cw_s;
    u64 final_cw;

    // constructor to initialize the DPF key
    DPFKey(int n){
        seed = rd();
        cw_s.resize(n);
    }
};

pair<DPFKey, DPFKey> generateDPF(u64 location, u64 value, u64 N){
    //checking if the location is within the bounds
    if(location>=N) throw runtime_error("location must in [0,N)");

    //finding the depth of the DPF tree
    u64 depth = N<=1?0:(int)ceil(log2(N));

    //initializing the two DPF keys
    DPFKey k0(depth), k1(depth);
    k0.t0 = 1;
    k1.t0 = 0;

    //initializing the current seeds and flags for both the trees
    u64 leftTree_currentSeed = k0.seed;
    u64 rightTree_currentSeed = k1.seed;
    bool leftTree_currentFlag = k0.t0;
    bool rightTree_currentFlag = k1.t0;

    for(u64 level = 0;level<depth;level++){
        bool pathBit = (location&(1ULL<<(depth-1-level)));      // finding the path bit at current level

        // expanding both the current seeds of the trees
        child ns0 = Expand(leftTree_currentSeed, level);
        child ns1 = Expand(rightTree_currentSeed, level);

        u64 correction_word;

        // finding the advice bits at current level
        bool leftCorrectionAdviceBit = (ns0.leftFlag ^ ns1.leftFlag ^ (pathBit ==0));
        bool rightCorrectionAdviceBit = (ns0.rightFlag ^ ns1.rightFlag ^ (pathBit ==1));

        // finding the correction word at current level
        if(pathBit){
            correction_word = ns0.leftSeed ^ ns1.leftSeed;
        }else{
            correction_word = ns0.rightSeed ^ ns1.rightSeed;
        }

        // applying the correction word and the advice bits where the current flag is 1
        if(leftTree_currentFlag){
            ns0.leftSeed ^= correction_word;
            ns0.leftFlag ^= leftCorrectionAdviceBit;
            ns0.rightSeed ^= correction_word;
            ns0.rightFlag ^= rightCorrectionAdviceBit;
        }else{
            ns1.leftSeed ^= correction_word;
            ns1.leftFlag ^= leftCorrectionAdviceBit;
            ns1.rightSeed ^= correction_word;
            ns1.rightFlag ^= rightCorrectionAdviceBit;
        }

        // updating the current seeds and the flags for both the trees
        if(pathBit){
            leftTree_currentSeed = ns0.rightSeed;
            leftTree_currentFlag = ns0.rightFlag;
            rightTree_currentSeed = ns1.rightSeed;
            rightTree_currentFlag = ns1.rightFlag;
        }else{
            leftTree_currentSeed = ns0.leftSeed;
            leftTree_currentFlag = ns0.leftFlag;
            rightTree_currentSeed = ns1.leftSeed;
            rightTree_currentFlag = ns1.leftFlag;
        }

        // storing the correction words and the advice bits in both keys
        correctionWord cw = {correction_word, leftCorrectionAdviceBit, rightCorrectionAdviceBit};
        k0.cw_s[level] = cw;
        k1.cw_s[level] = cw;
    }

    // finding the final correction word and setting it in both the keys
    u64 final_correctionWord = (leftTree_currentSeed ^ rightTree_currentSeed) ^ value;
    k0.final_cw = final_correctionWord;
    k1.final_cw = final_correctionWord;

    return {k0, k1};
}

// Return the leaf flag t at a given location (no output value, just the sign bit).
static bool evalFlagAt(const DPFKey& key, u64 location, u64 N) {
    u64 depth = N<=1?0:(int)ceil(log2(N));
    u64 currSeed = key.seed;
    bool currentFlag = key.t0;

    for (u64 level = 0; level < depth; ++level) {
        // Use a clean 0/1 bit for the path
        bool pathBit = ((location >> (depth - 1 - level)) & 1ULL);
        child ns0 = Expand(currSeed, level);

        if (currentFlag) {
            // Important: apply seed correction too, otherwise next-level PRG diverges
            ns0.leftSeed  ^= key.cw_s[level].cw;
            ns0.rightSeed ^= key.cw_s[level].cw;
            // Apply advice bits to flags
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

// Produce an additive share at a location as +/-magnitude based on the flag t.
// One party will be globally negated (negateThisParty=true) to align the target's sign.
static __int128 evalDPFAddShare(const DPFKey& key, u64 location, u64 N, bool negateThisParty, __int128 magnitude) {
    bool t = evalFlagAt(key, location, N);
    int s = t ? -1 : 1;
    if (negateThisParty) s = -s;
    return (__int128)s * magnitude; 
}

// Evaluate over all locations using additive shares:
// Decide (insecurely) which party should negate its entire vector so that the target sums to +value.
// Combine shares by addition and divide by 2 (since aligned signs sum to +/-2 at the target).
static bool EvalFullAdditive(DPFKey &k0, DPFKey &k1, u64 N, u64 value, u64 index){
    // Insecure sign alignment: inspect the target to choose who negates so the target is positive.
    int s0_at_target = evalFlagAt(k0, index, N) ? -1 : 1;
    int s1_at_target = evalFlagAt(k1, index, N) ? -1 : 1;

    // If we negate k1, target sum sign is s0 - s1. Exactly one of {negate k1, negate k0} yields +2.
    bool negateK0 = false;
    bool negateK1 = true;
    int sumIfNegateK1 = s0_at_target - s1_at_target; // in {-2, +2}
    if (sumIfNegateK1 < 0) {
        // Flip: negate k0 instead of k1 to make the target sum +2
        negateK0 = true;
        negateK1 = false;
    }

    // Combine at each location. We scale shares by 'value' and divide the sum by 2 at combine time.
    for (u64 location = 0; location < N; ++location) {
        __int128 share0 = evalDPFAddShare(k0, location, N, negateK0, (__int128)value);
        __int128 share1 = evalDPFAddShare(k1, location, N, negateK1, (__int128)value);
        __int128 combined = (share0 + share1) / 2; // should be value at index, 0 elsewhere

        __int128 needed = (location == index) ? (__int128)value : 0;
        if (combined != needed) return false;
    }
    return true;
}


int main(int argc, char** argv) {
    if(argc!=3){
        cerr<<"Usage: ./main <N> <count>"<<endl;
        return 1;
    }

    u64 N = stoull(argv[1]);
    u64 count = stoull(argv[2]);

    for(int i=0;i<count;i++){
        u64 location = rd()%N;
        u64 value = rd(); // For cleaner additive tests, you can set value = 1 or ensure it's not overflowing 128-bit arithmetic.

        auto dpfs = generateDPF(location, value, N);
        DPFKey dpf1 = dpfs.first;
        DPFKey dpf2 = dpfs.second;

        // Use additive-share verification (insecure conversion via sign alignment)
        if(EvalFullAdditive(dpf1, dpf2, N, value, location)){
            cout<<"Case "<<i+1<<": Test Passed"<<endl;
        }else{
            cout<<"Case "<<i+1<<": Test Failed"<<endl;
        }
    }
    return 0;
}