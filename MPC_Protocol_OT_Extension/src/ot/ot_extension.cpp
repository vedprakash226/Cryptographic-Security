#include "ot_extension.hpp"
#include "base_ot.hpp"
#include <boost/asio.hpp>
#include <vector>
#include <iostream>

using namespace std;

class OTExtension {
public:
    OTExtension(tcp::socket& socket, int num_requests) 
        : socket(socket), num_requests(num_requests) {}

    void run() {
        // Step 1: Send the number of requests to the receiver
        send_num_requests();

        // Step 2: Receive the base OTs
        vector<Share> base_ot_shares = receive_base_ot_shares();

        // Step 3: Execute the oblivious transfer for the private index
        execute_ot(base_ot_shares);
    }

private:
    tcp::socket& socket;
    int num_requests;

    void send_num_requests() {
        co_await boost::asio::async_write(socket, boost::asio::buffer(&num_requests, sizeof(num_requests)), use_awaitable);
    }

    vector<Share> receive_base_ot_shares() {
        vector<Share> base_ot_shares(num_requests);
        co_await boost::asio::async_read(socket, boost::asio::buffer(base_ot_shares), use_awaitable);
        return base_ot_shares;
    }

    void execute_ot(const vector<Share>& base_ot_shares) {
        // Implement the logic for executing the oblivious transfer using the base OTs
        for (int i = 0; i < num_requests; ++i) {
            // Logic for oblivious transfer for each request
            // This could involve sending and receiving shares based on the index
        }
    }
};