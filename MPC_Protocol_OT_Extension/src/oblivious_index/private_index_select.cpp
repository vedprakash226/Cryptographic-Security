#include "private_index_select.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <vector>

using namespace std;

// Function to perform oblivious transfer for private index selection
void oblivious_index_select(const vector<int>& indices, int secret_index, boost::asio::ip::tcp::socket& socket) {
    // Step 1: Send the indices to the receiver
    boost::asio::write(socket, boost::asio::buffer(indices));

    // Step 2: Send the secret index to the receiver
    boost::asio::write(socket, boost::asio::buffer(&secret_index, sizeof(secret_index)));

    // Step 3: Wait for the receiver to confirm the selection
    int confirmation;
    boost::asio::read(socket, boost::asio::buffer(&confirmation, sizeof(confirmation)));

    if (confirmation == 1) {
        cout << "Index " << secret_index << " selected successfully." << endl;
    } else {
        cout << "Failed to select index " << secret_index << "." << endl;
    }
}