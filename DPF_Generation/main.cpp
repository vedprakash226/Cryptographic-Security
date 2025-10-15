#include<bits/stdc++.h>
using namespace std;

// typedefs for convinence
typedef uint64_t u64;

static random_device rd;  // obtain a random number from hardware

struct child{
    u64 leftSeed, rightSeed;
    bool leftFlag, rightFlag;
};

// ctr_base is used to have different outputs for different levels if the seed is same
child Expand(u64 seed, u64 ctr_base = 0){
    u64 init = seed ^ 0x243f6a8885a308d3ULL^(0x9e3779b97f4a7c15ULL * (ctr_base + 1));
    mt19937_64 prng(init);

    child out;
    out.leftSeed = prng();
    out.rightSeed = prng();
    out.leftFlag = (prng() & 1ULL);
    out.rightFlag = (prng() & 1ULL);
    return out;
}

// DPF key class declaration
class DPFKey{
    public:
    u64 seed;
    bool t0;
    vector<u64> cw_s;
    vector<bool> leftAdviceBits, rightAdviceBits;
    u64 final_cw;
};

pair<DPFKey, DPFKey> generateDPF(u64 location, u64 value, u64 N){
    //checking if the location is within the bounds
    if(location>=N) throw runtime_error("location must in [0,N)");

    //finding the depth of the DPF tree
    u64 depth = N<=1?0:(int)ceil(log2(N));

    //initializing the two DPF keys
    DPFKey k0, k1;
    k0.seed = rd();
    k1.seed = rd();
    k0.t0 = 1;
    k1.t0 = 0;

    k0.cw_s.resize(depth);
    k1.cw_s.resize(depth);
    k0.leftAdviceBits.resize(depth);
    k1.leftAdviceBits.resize(depth);
    k0.rightAdviceBits.resize(depth);
    k1.rightAdviceBits.resize(depth);

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
        k0.cw_s[level] = correction_word;
        k1.cw_s[level] = correction_word;
        k0.leftAdviceBits[level] = leftCorrectionAdviceBit;
        k1.leftAdviceBits[level] = leftCorrectionAdviceBit;
        k0.rightAdviceBits[level] = rightCorrectionAdviceBit;
        k1.rightAdviceBits[level] = rightCorrectionAdviceBit;
    }

    // finding the final correction word and setting it in both the keys
    u64 final_correctionWord = (leftTree_currentSeed ^ rightTree_currentSeed) ^ value;
    k0.final_cw = final_correctionWord;
    k1.final_cw = final_correctionWord;

    return {k0, k1};

}

// function to evaluate the DPF at a particular location
u64 evalDPF(DPFKey& key, u64 location, u64 N){
    u64 depth = N<=1?0:(int)ceil(log2(N));
    u64 currSeed = key.seed;
    bool currentFlag = key.t0;

    // traversing the tree according to the location bits
    for(u64 level = 0;level<depth;level++){
        bool pathBit = (location&(1ULL<<(depth-1-level)));
        child ns0 = Expand(currSeed, level);

        // applying the correction word and the advice bits where the current flag is 1
        if(currentFlag){
            ns0.leftSeed ^= key.cw_s[level];
            ns0.rightSeed ^= key.cw_s[level];
            ns0.leftFlag ^= key.leftAdviceBits[level];
            ns0.rightFlag ^= key.rightAdviceBits[level];
        }

        // updating the current seed and the flag according to the path bit
        if(pathBit){
            currSeed = ns0.rightSeed;
            currentFlag = ns0.rightFlag;
        }else{
            currSeed = ns0.leftSeed;
            currentFlag = ns0.leftFlag;
        }
    }

    // applying the final correction word if the current flag is 1
    u64 finalCorrectionWord = (currentFlag ? key.final_cw : 0ULL);
    return (currSeed ^ finalCorrectionWord);
}

// evaluating the DPFs at all locations and checking if they match the expected values
bool EvalFull(DPFKey &k0, DPFKey &k1, u64 N, u64 value, u64 index){
    for(int location = 0;location<N;location++){
        u64 valueNeeded = (location==index)?value:0ULL;
        u64 valueGot = evalDPF(k0, location, N)^evalDPF(k1, location, N);
        if(valueNeeded!=valueGot) return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if(argc!=3){
        cerr<<"Usage: ./main <location> <value>"<<endl;
        return 1;
    }

    u64 N = stoull(argv[1]);
    u64 count = stoull(argv[2]);

    for(int i=0;i<count;i++){
        u64 location = rd()%N;
        u64 value = rd();
        auto dpfs = generateDPF(location, value, N);
        DPFKey dpf1 = dpfs.first;
        DPFKey dpf2 = dpfs.second;

        if(EvalFull(dpf1, dpf2, N, value, location)){
            cout<<"Case "<<i+1<<": Test Passed"<<endl;
        }else{
            cout<<"Case "<<i+1<<": Test Failed"<<endl;
        }
    }
}