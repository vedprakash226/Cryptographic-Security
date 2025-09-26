#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <memory>

using boost::asio::ip::tcp;

class Channel {
public:
    Channel(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
        : resolver_(io_context), socket_(io_context) {
        connect(host, port);
    }

    void send(const std::string& message) {
        boost::asio::write(socket_, boost::asio::buffer(message));
    }

    std::string receive() {
        char buffer[1024];
        size_t len = socket_.read_some(boost::asio::buffer(buffer));
        return std::string(buffer, len);
    }

private:
    void connect(const std::string& host, const std::string& port) {
        auto endpoints = resolver_.resolve(host, port);
        boost::asio::connect(socket_, endpoints);
    }

    tcp::resolver resolver_;
    tcp::socket socket_;
};