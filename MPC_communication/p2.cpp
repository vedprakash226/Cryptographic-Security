#include "common.hpp"
#include "shares.hpp"
#include <boost/asio.hpp>
#include <iostream>

// Handles a pair of connected clients (P0 and P1) to provide them with Beaver triples.
awaitable<void> handle_clients(tcp::socket p0_socket, tcp::socket p1_socket) {
    try {
        // P0 will send the number of triples needed
        uint32_t k;
        co_await boost::asio::async_read(p0_socket, boost::asio::buffer(&k, sizeof(k)), use_awaitable);
        std::cout << "P2: Received request for " << k << " triples." << std::endl;

        // Generate and send triples for each request until one party disconnects
        while(k > 0) {
            std::vector<BeaverTriple> p0_triples(k);
            std::vector<BeaverTriple> p1_triples(k);

            for(uint32_t i=0; i<k; ++i) {
                uint32_t a = random_uint32();
                uint32_t b = random_uint32();
                uint32_t c = a * b;

                uint32_t a0 = random_uint32();
                uint32_t b0 = random_uint32();
                uint32_t c0 = random_uint32();

                uint32_t a1 = a - a0;
                uint32_t b1 = b - b0;
                uint32_t c1 = c - c0;

                p0_triples[i] = {a0, b0, c0};
                p1_triples[i] = {a1, b1, c1};
            }

            co_await boost::asio::async_write(p0_socket, boost::asio::buffer(p0_triples), use_awaitable);
            co_await boost::asio::async_write(p1_socket, boost::asio::buffer(p1_triples), use_awaitable);
            
            // Wait for next request
            co_await boost::asio::async_read(p0_socket, boost::asio::buffer(&k, sizeof(k)), use_awaitable);
             std::cout << "P2: Received request for " << k << " triples." << std::endl;
        }

    } catch (std::exception& e) {
        std::cout << "P2 closing connection: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9002));

        std::cout << "P2 listening on port 9002..." << std::endl;

        // Accept connections from P0 and P1
        tcp::socket socket_p0 = acceptor.accept();
        std::cout << "P2 accepted connection 1." << std::endl;
        tcp::socket socket_p1 = acceptor.accept();
        std::cout << "P2 accepted connection 2." << std::endl;

        // Spawn a coroutine to handle this pair of clients
        co_spawn(io_context, handle_clients(std::move(socket_p0), std::move(socket_p1)), detached);

        io_context.run();

    } catch (std::exception& e) {
        std::cerr << "Exception in P2: " << e.what() << "\n";
    }
    return 0;
}
