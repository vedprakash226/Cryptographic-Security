#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <cstdint>

using boost::asio::awaitable;
using boost::asio::ip::tcp;

class KKRT {
public:
    KKRT(tcp::socket& peer_socket);

    awaitable<void> send_index(const std::vector<uint8_t>& index);
    awaitable<std::vector<uint8_t>> receive_index();

private:
    tcp::socket& peer_socket;
    void oblivious_transfer(const std::vector<uint8_t>& index, std::vector<uint8_t>& received);
};