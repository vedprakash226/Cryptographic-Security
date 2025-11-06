#pragma once  //this is to ensure the file is called once (for security)
#include<vector>
#include<cstdint>
#include<numeric>
#include<stdexcept>
#include "common.hpp"

const int MOD = 1000000007;

typedef long long int ll;
using namespace std;

struct Share{
     vector<ll> data;
    Share()=default;

    explicit Share(size_t size): data(size,0) {}
    Share(vector<ll> d): data(move(d)) {}

    void randomizer(){
        for(auto &val: data) val = random_uint32()%MOD; //random value generation
    }

    int size() const{
        int size = data.size();
        return size;
    }
};

//vector addition, subtraction, multiplication
inline Share operator+(const Share &a, const Share &b){
    if(a.size() != b.size()) throw invalid_argument("Vectors must be of same size");
    Share result(a.size());
    for(size_t i=0; i<a.size(); i++){
        result.data[i] = (a.data[i] + b.data[i]) % MOD;
    }
    return result;
}

inline Share operator-(const Share &a, const Share &b){
    if(a.size() != b.size()) throw invalid_argument("Vectors must be of same size");
    Share result(a.size());
    for(size_t i=0; i<a.size(); i++){
        result.data[i] = (a.data[i] - b.data[i] + MOD) % MOD;
    }
    return result;
}

inline Share operator*(const Share &a, const Share &b){
    if(a.size() != b.size()) throw invalid_argument("Vectors must be of same size");
    Share result(a.size());
    for(size_t i=0; i<a.size(); i++){
        result.data[i] = (a.data[i] *b.data[i])%MOD;
    }
    return result;
}

struct BeaverTriple{
    ll a, b, c;
};