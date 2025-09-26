#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <cstdint>

using boost::asio::ip::tcp;

class BaseOT {
public:
    virtual ~BaseOT() = default;

    // Sends the sender's messages to the receiver
    virtual void send(const std::vector<std::vector<uint8_t>>& messages, tcp::socket& socket) = 0;

    // Receives the receiver's choice and returns the corresponding message
    virtual std::vector<uint8_t> receive(tcp::socket& socket, size_t choice) = 0;
};