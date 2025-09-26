#include "base_ot.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <vector>

using namespace std;

// Base Oblivious Transfer Protocol Implementation
class BaseOT {
public:
    // Constructor
    BaseOT(tcp::socket& socket) : sock(socket) {}

    // Function to perform the base oblivious transfer
    void transfer(const vector<int>& choices, vector<int>& outputs) {
        // Send choices to the other party
        boost::asio::write(sock, boost::asio::buffer(choices));

        // Receive the corresponding outputs based on the choices
        for (size_t i = 0; i < choices.size(); ++i) {
            int output;
            boost::asio::read(sock, boost::asio::buffer(&output, sizeof(output)));
            outputs[i] = output; // Store the output
        }
    }

private:
    tcp::socket& sock; // Socket for communication
};