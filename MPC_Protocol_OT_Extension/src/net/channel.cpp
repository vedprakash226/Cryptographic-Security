#include "channel.hpp"
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

void establish_connection(tcp::socket& socket, const std::string& host, const std::string& port) {
    try {
        tcp::resolver resolver(socket.get_executor().context());
        auto endpoints = resolver.resolve(host, port);
        boost::asio::connect(socket, endpoints);
        std::cout << "Connected to " << host << " on port " << port << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error establishing connection: " << e.what() << std::endl;
        throw;
    }
}

void send_data(tcp::socket& socket, const std::vector<uint8_t>& data) {
    boost::asio::write(socket, boost::asio::buffer(data));
}

std::vector<uint8_t> receive_data(tcp::socket& socket, std::size_t length) {
    std::vector<uint8_t> data(length);
    boost::asio::read(socket, boost::asio::buffer(data));
    return data;
}