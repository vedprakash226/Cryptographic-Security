#pragma once

#include "common.hpp"
#include "shares.hpp"
#include <vector>

// coroutine to send a vector share
awaitable<void> send_vec(tcp::socket& sock, const Share& vec) {
    co_await boost::asio::async_write(sock, boost::asio::buffer(vec.data), use_awaitable);
}

// coroutine to receive a vector share
awaitable<Share> recv_vec(tcp::socket& sock, size_t k) {
    Share vec(k);
    co_await boost::asio::async_read(sock, boost::asio::buffer(vec.data), use_awaitable);
    co_return vec;
}

// coroutine to send a single value
awaitable<void> send_val(tcp::socket& sock, uint32_t val) {
    co_await boost::asio::async_write(sock, boost::asio::buffer(&val, sizeof(val)), use_awaitable);
}

// coroutine to receive a single value
awaitable<uint32_t> recv_val(tcp::socket& sock) {
    uint32_t val;
    co_await boost::asio::async_read(sock, boost::asio::buffer(&val, sizeof(val)), use_awaitable);
    co_return val;
}

class MPCProtocol {
    private:
    tcp::socket& peer_sock;
    tcp::socket& p2_sock;
    // Securely computes the dot product of two secret-shared vectors
    awaitable<uint32_t> MPC_DOTPRODUCT(const Share& x_b, const Share& y_b, int k) {
        // Get Beaver triples from P2 for vector multiplication
        std::vector<BeaverTriple> triples = co_await getBeaverTriple(k);
        Share a_b(k), b_b(k);
        std::vector<uint32_t> c_b(k);
        for(int i=0; i<k; i++) {
            a_b.data[i] = triples[i].a;
            b_b.data[i] = triples[i].b;
            c_b[i] = triples[i].c;
        }
        
        // Compute masked values epsilon and delta
        Share epsilon_b = x_b - a_b;
        Share delta_b = y_b - b_b;
        
        // Exchange masked values to reconstruct them
        co_await send_vec(peer_sock, epsilon_b);
        Share epsilon_peer = co_await recv_vec(peer_sock, k);
        Share epsilon = epsilon_b + epsilon_peer;
        
        co_await send_vec(peer_sock, delta_b);
        Share delta_peer = co_await recv_vec(peer_sock, k);
        Share delta = delta_b + delta_peer;
        
        // Compute share of the dot product
        uint32_t prodShare = 0;
        for (int i = 0; i < k; ++i) {
            #ifdef ROLE_p0
            prodShare += epsilon.data[i] * delta.data[i] + epsilon.data[i] * b_b.data[i] + delta.data[i] * a_b.data[i] + c_b[i];
            #else // ROLE_p1
            prodShare += epsilon.data[i] * b_b.data[i] + delta.data[i] * a_b.data[i] + c_b[i];
            #endif
        }
        co_return prodShare;
    }
    
    // Securely computes the product of a secret-shared scalar and a secret-shared vector
    awaitable<Share> scalarVecProd(uint32_t scalar_share, const Share& vec_share, int k) {
        // Get Beaver triples from P2
        std::vector<BeaverTriple> triples = co_await getBeaverTriple(k);
        Share a_b(k), b_b(k); // a is scalar, b is vector
        std::vector<uint32_t> c_b(k);
        for(int i=0; i<k; ++i) {
            a_b.data[i] = triples[i].a; // Re-using vector share for scalar shares
            b_b.data[i] = triples[i].b;
            c_b[i] = triples[i].c;
        }
        
        // Mask scalar and vector
        uint32_t epsilon_b = scalar_share - a_b.data[0]; // Mask scalar
        Share delta_b = vec_share - b_b; // Mask vector
        
        // Exchange masked values
        co_await send_val(peer_sock, epsilon_b);
        uint32_t epsilon_peer = co_await recv_val(peer_sock);
        uint32_t epsilon = epsilon_b + epsilon_peer;
        
        co_await send_vec(peer_sock, delta_b);
        Share delta_peer = co_await recv_vec(peer_sock, k);
        Share delta = delta_b + delta_peer;
        
        // Compute share of the product vector
        Share result(k);
        for (int i = 0; i < k; ++i) {
            #ifdef ROLE_p0
            result.data[i] = epsilon * delta.data[i] + epsilon * b_b.data[i] + delta.data[i] * a_b.data[0] + c_b[i];
            #else // ROLE_p1
            result.data[i] = epsilon * b_b.data[i] + delta.data[i] * a_b.data[0] + c_b[i];
            #endif
        }
        co_return result;
    }
    
    // request for k Beaver multiplication triples from P2
    awaitable<std::vector<BeaverTriple>> getBeaverTriple(int k) {
        // reqesting k triples
        co_await send_val(p2_sock, k);
        
        // receiving k triples
        std::vector<BeaverTriple> triples(k);
        co_await boost::asio::async_read(p2_sock, boost::asio::buffer(triples), use_awaitable);
        
        co_return triples;
    }

    public:
        MPCProtocol(tcp::socket& peer, tcp::socket& p2) : peer_sock(peer), p2_sock(p2) {}
    
        // function for full secure update protocol for u_i <- u_i + v_j * (1 - <u_i, v_j>)
        awaitable<Share> secure_update(const Share& ui, const Share& vj, int k) {
            // dot product <u_i, v_j>
            uint32_t prodShare = co_await MPC_DOTPRODUCT(ui, vj, k);
            
            // delta share
            uint32_t delta_share;
            #ifdef ROLE_p0
                delta_share = 1 - prodShare;
            #else // ROLE_p1
                delta_share = -prodShare;
            #endif
    
            // compute scalar-vector product securely
            Share prod_vec_share = co_await scalarVecProd(delta_share, vj, k);
    
            // Add shares locally to get the final updated user profile share
            Share u_prime_b = ui + prod_vec_share;
    
            co_return u_prime_b;
        }
};
