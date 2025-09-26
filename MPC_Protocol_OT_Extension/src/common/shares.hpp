#pragma once
#include <vector>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include "common.hpp"

typedef long long int ll;
const int mod = 1000007;

struct Share {
    std::vector<ll> data;
    Share() = default;

    explicit Share(size_t size) : data(size, 0) {}
    Share(std::vector<ll> d) : data(std::move(d)) {}

    void randomizer() {
        for (auto &val : data) val = random_uint32() % mod; // random value generation
    }

    size_t size() const {
        return data.size();
    }
};

// Vector addition, subtraction, multiplication
inline Share operator+(const Share &a, const Share &b) {
    if (a.size() != b.size()) throw std::invalid_argument("Shares must be of the same size for addition.");
    Share result(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        result.data[i] = (a.data[i] + b.data[i]) % mod;
    }
    return result;
}

inline Share operator-(const Share &a, const Share &b) {
    if (a.size() != b.size()) throw std::invalid_argument("Shares must be of the same size for subtraction.");
    Share result(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        result.data[i] = (a.data[i] - b.data[i] + mod) % mod;
    }
    return result;
}

inline Share operator*(const Share &a, const Share &b) {
    if (a.size() != b.size()) throw std::invalid_argument("Shares must be of the same size for multiplication.");
    Share result(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        result.data[i] = (a.data[i] * b.data[i]) % mod;
    }
    return result;
}

struct BeaverTriple {
    ll a, b, c;
};