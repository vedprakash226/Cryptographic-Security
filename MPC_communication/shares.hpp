#pragma once  //this is to ensure the file is called once (for security)
#include<vector>
#include<cstdint>
#include<numeric>
#include<stdexcept>
#include "common.hpp"

struct Share{
    std:: vector<uint32_t> data;
    Share()=default;
    
    explicit Share(size_t size): data(size,0) {}    
    Share(std::vector<uint32_t> d): data(std::move(d)) {}

    void randomizer(){
        for(auto &val: data) val = random_uint32();
    }

    size_t size() const{
        return data.size();
    }
};

//vector addition, subtraction, multiplication
inline Share operator+(const Share &a, const Share &b){
    if(a.size() != b.size()) throw std::invalid_argument("Shares must be of the same size for addition.");
    Share result(a.size());
    for(size_t i=0; i<a.size(); i++){
        result.data[i] = a.data[i] + b.data[i];
    }
    return result;
}

inline Share operator-(const Share &a, const Share &b){
    if(a.size() != b.size()) throw std::invalid_argument("Shares must be of the same size for subtraction.");
    Share result(a.size());
    for(size_t i=0; i<a.size(); i++){
        result.data[i] = a.data[i] - b.data[i];
    }
    return result;
}

inline Share operator*(const Share &a, const Share &b){
    if(a.size() != b.size()) throw std::invalid_argument("Shares must be of the same size for multiplication.");
    Share result(a.size());
    for(size_t i=0; i<a.size(); i++){
        result.data[i] = a.data[i] * b.data[i];
    }
    return result;
}

struct BeaverTriple{
    uint32_t a, b, c;
};