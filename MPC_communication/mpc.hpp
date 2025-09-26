#pragma once

#include "common.hpp"
#include "shares.hpp"
#include <vector>
typedef long long int ll;

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
awaitable<void> send_val(tcp::socket& sock, ll val) {
    co_await boost::asio::async_write(sock, boost::asio::buffer(&val, sizeof(val)), use_awaitable);
}

// coroutine to receive a single value
awaitable<ll> recv_val(tcp::socket& sock) {
    ll val;
    co_await boost::asio::async_read(sock, boost::asio::buffer(&val, sizeof(val)), use_awaitable);
    co_return val;
}

class MPCProtocol {
private:
    tcp::socket& peer_sock;
    tcp::socket& p2_sock;
    // Securely computes the dot product of two secret-shared vectors based on the image provided.
    awaitable<ll> MPC_DOTPRODUCT(const Share& x_b, const Share& y_b, int k) {
        // Get Beaver triples from P2 for vector multiplication
        std::vector<BeaverTriple> triples = co_await getBeaverTriple(k);
        Share a_b(k), b_b(k);
        std::vector<ll> c_b(k);
        for(int i=0; i<k; i++) {
            a_b.data[i] = triples[i].a;
            b_b.data[i] = triples[i].b;
            c_b[i] = triples[i].c;
        }
        
        // Compute masked values alpha and beta using addition
        Share alpha_b = x_b + a_b;
        Share beta_b = y_b + b_b;
        
        // Exchange masked values to reconstruct them publicly
        co_await send_vec(peer_sock, alpha_b);
        Share alpha_peer = co_await recv_vec(peer_sock, k);
        Share alpha = alpha_b + alpha_peer;
        
        co_await send_vec(peer_sock, beta_b);
        Share beta_peer = co_await recv_vec(peer_sock, k);
        Share beta = beta_b + beta_peer;
        
        // Compute share of the dot product using the reconstruction formula from the image
        ll prodShare = 0;
        for (int i = 0; i < k; i++) {
            // z0 <- share of the dot product x.y
            prodShare += alpha.data[i] * y_b.data[i] - beta.data[i] * a_b.data[i] + c_b[i];
        }
        co_return prodShare;
    }
    
    // Securely computes the product of a secret-shared scalar and a secret-shared vector
    awaitable<Share> scalarVecProd(ll scalar_share, const Share& vec_share, int k) {
        // Get Beaver triples from P2
        std::vector<BeaverTriple> triples = co_await getBeaverTriple(k);
        Share a_b(k), b_b(k); // a is scalar, b is vector
        std::vector<ll> c_b(k);
        for(int i=0; i<k; ++i) {
            a_b.data[i] = triples[i].a; // Re-using vector share for scalar shares
            b_b.data[i] = triples[i].b;
            c_b[i] = triples[i].c;
        }
        
        // Mask scalar and vector using addition
        ll alpha_b = scalar_share + a_b.data[0]; // Mask scalar
        Share beta_b = vec_share + b_b; // Mask vector
        
        // Exchange masked values to reconstruct them publicly
        co_await send_val(peer_sock, alpha_b);
        ll alpha_peer = co_await recv_val(peer_sock);
        ll alpha = alpha_b + alpha_peer;
        
        co_await send_vec(peer_sock, beta_b);
        Share beta_peer = co_await recv_vec(peer_sock, k);
        Share beta = beta_b + beta_peer;
        
        // Compute share of the product vector
        Share result(k);
        for (int i = 0; i < k;i++) {
            result.data[i] = alpha * vec_share.data[i] - beta.data[i] * a_b.data[0] + c_b[i];
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
    awaitable<Share> updateProtocol(const Share& ui, const Share& vj, int k) {
        // dot product <u_i, v_j>
        ll prodShare = co_await MPC_DOTPRODUCT(ui, vj, k);
        
        // delta share
        ll delta_share;
        #ifdef ROLE_p0
            delta_share = 1 - prodShare;
        #else // ROLE_p1
            delta_share = -prodShare;
        #endif

        // compute scalar-vector product securely ((v_j0 + v_j1) * ((1-prodshare0) + (0-prodshare1)))
        Share prod_vec_share = co_await scalarVecProd(delta_share, vj, k);

        // Add shares locally to get the final updated user profile share
        Share u_prime_b = ui + prod_vec_share;

        co_return u_prime_b;
    }
};

