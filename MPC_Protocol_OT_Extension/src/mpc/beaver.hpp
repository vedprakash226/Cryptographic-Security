#pragma once

#include "shares.hpp"
#include <vector>
#include <cstdint>

// Structure to hold Beaver triples
struct BeaverTriple {
    ll a; // First share
    ll b; // Second share
    ll c; // Third share (a * b)
};

// Function to generate Beaver triples
std::vector<BeaverTriple> generateBeaverTriples(int count) {
    std::vector<BeaverTriple> triples(count);
    for (int i = 0; i < count; ++i) {
        ll a = random_uint32() % mod;
        ll b = random_uint32() % mod;
        triples[i] = {a, b, (a * b) % mod};
    }
    return triples;
}

// Function to securely send Beaver triples to a socket
awaitable<void> sendBeaverTriples(tcp::socket& sock, const std::vector<BeaverTriple>& triples) {
    co_await boost::asio::async_write(sock, boost::asio::buffer(triples), use_awaitable);
}

// Function to securely receive Beaver triples from a socket
awaitable<std::vector<BeaverTriple>> recvBeaverTriples(tcp::socket& sock, int count) {
    std::vector<BeaverTriple> triples(count);
    co_await boost::asio::async_read(sock, boost::asio::buffer(triples), use_awaitable);
    co_return triples;
}