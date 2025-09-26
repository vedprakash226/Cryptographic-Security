#include "kkrt.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <vector>

using namespace std;

// Function to perform the KKRT oblivious transfer protocol
awaitable<void> kkrt_ot(tcp::socket& sender_socket, tcp::socket& receiver_socket, const vector<int>& choices) {
    try {
        // Step 1: Sender sends their values
        vector<string> values; // Values to be sent
        // Populate values with the sender's data (e.g., secret shares)
        // ...

        // Send the values to the receiver
        co_await boost::asio::async_write(sender_socket, boost::asio::buffer(values), use_awaitable);

        // Step 2: Receiver sends their choices
        co_await boost::asio::async_write(receiver_socket, boost::asio::buffer(choices), use_awaitable);

        // Step 3: Receiver receives the selected values based on their choices
        vector<string> selected_values(choices.size());
        for (size_t i = 0; i < choices.size(); ++i) {
            // Logic to select the appropriate value based on the choice
            selected_values[i] = values[choices[i]];
        }

        // Send the selected values back to the receiver
        co_await boost::asio::async_write(receiver_socket, boost::asio::buffer(selected_values), use_awaitable);
    } catch (const std::exception& e) {
        cerr << "KKRT OT error: " << e.what() << endl;
    }
}