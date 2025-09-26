#pragma once

#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <iostream>
#include <random>

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;
namespace this_coro = boost::asio::this_coro;

inline uint32_t random_uint32() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}

// Blind by XOR mask
inline uint32_t blind_value(uint32_t v) {
    return v ^ 0xDEADBEEF;
}

// Function to perform oblivious transfer for the query index
awaitable<void> oblivious_transfer(tcp::socket& sock, int query_index) {
    // Implementation of the oblivious transfer protocol to keep query_index confidential
    // This function will handle the sending and receiving of the index securely
    // Placeholder for actual oblivious transfer logic
    co_return;
}