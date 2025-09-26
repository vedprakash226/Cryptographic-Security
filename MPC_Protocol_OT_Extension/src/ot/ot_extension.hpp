#pragma once

#include <boost/asio.hpp>
#include <vector>
#include "shares.hpp"

class OTExtension {
public:
    OTExtension(boost::asio::ip::tcp::socket& sender, boost::asio::ip::tcp::socket& receiver);

    // Function to perform oblivious transfer for private index selection
    awaitable<void> performOT(const std::vector<Share>& sender_shares, const std::vector<Share>& receiver_choices, int index);

private:
    boost::asio::ip::tcp::socket& sender_socket;
    boost::asio::ip::tcp::socket& receiver_socket;

    // Helper function to send shares
    awaitable<void> sendShares(const std::vector<Share>& shares);

    // Helper function to receive shares
    awaitable<std::vector<Share>> receiveShares(size_t count);
};