#pragma once

#include <boost/asio.hpp>
#include <vector>
#include "shares.hpp"

class PrivateIndexSelector {
public:
    PrivateIndexSelector(boost::asio::ip::tcp::socket& socket);

    // Function to perform oblivious transfer for index selection
    void selectIndex(const std::vector<Share>& shares, int index);

private:
    boost::asio::ip::tcp::socket& socket;

    // Helper function to send a share
    void sendShare(const Share& share);

    // Helper function to receive a share
    Share receiveShare();
};