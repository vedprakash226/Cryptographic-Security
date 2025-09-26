#pragma once

#include "common.hpp"
#include "shares.hpp"
#include <vector>
#include <boost/asio.hpp>
#include <unordered_map>
#include "oblivious_index/private_index_select.hpp"

typedef long long int ll;

class MPCProtocol {
private:
    tcp::socket& peer_sock;
    tcp::socket& p2_sock;

    // Securely computes the dot product of two secret-shared vectors
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
        Share beta_b  = y_b + b_b;

        // Exchange masked values to reconstruct them publicly
        co_await send_vec(peer_sock, alpha_b);
        Share alpha_peer = co_await recv_vec(peer_sock, k);
        Share alpha = alpha_b + alpha_peer;

        co_await send_vec(peer_sock, beta_b);
        Share beta_peer = co_await recv_vec(peer_sock, k);
        Share beta = beta_b + beta_peer;

        ll prodShare = 0;
        for (int i = 0; i < k; i++) {
            ll term = add(sub(mult(alpha.data[i], y_b.data[i]),
                              mult(beta.data[i],  a_b.data[i])),
                          c_b[i]);
            prodShare = add(prodShare, term);
        }
        co_return prodShare;
    }

    // Securely computes the product of a secret-shared scalar and a secret-shared vector
    awaitable<Share> scalarVecProd(ll scalar_share, const Share& vec_share, int k) {
        // Get Beaver triples from P2
        std::vector<BeaverTriple> triples = co_await getBeaverTriple(k);
        Share a_b(k), b_b(k);
        std::vector<ll> c_b(k);
        for(int i=0; i<k; ++i) {
            a_b.data[i] = triples[i].a;
            b_b.data[i] = triples[i].b;
            c_b[i] = triples[i].c;
        }

        // Mask scalar and vector (mod)
        ll alpha_b = add(scalar_share, a_b.data[0]);
        Share beta_b = vec_share + b_b;

        // Exchange and reconstruct (mod)
        co_await send_val(peer_sock, alpha_b);
        ll alpha_peer = co_await recv_val(peer_sock);
        ll alpha = add(alpha_b, alpha_peer);

        co_await send_vec(peer_sock, beta_b);
        Share beta_peer = co_await recv_vec(peer_sock, k);
        Share beta = beta_b + beta_peer;

        Share result(k);
        for (int i = 0; i < k; i++) {
            result.data[i] = add(sub(mult(alpha, vec_share.data[i]),
                                     mult(beta.data[i], a_b.data[0])),
                                 c_b[i]);
        }
        co_return result;
    }

    // Request for k Beaver multiplication triples from P2
    awaitable<std::vector<BeaverTriple>> getBeaverTriple(int k) {
        co_await send_val(p2_sock, k);
        std::vector<BeaverTriple> triples(k);
        co_await boost::asio::async_read(p2_sock, boost::asio::buffer(triples), use_awaitable);
        co_return triples;
    }

public:
    MPCProtocol(tcp::socket& peer, tcp::socket& p2) : peer_sock(peer), p2_sock(p2) {}

    // Function for full secure update protocol for u_i <- u_i + v_j * (1 - <u_i, v_j>)
    awaitable<Share> updateProtocol(const Share& ui, const Share& vj, int k) {
        ll prodShare = co_await MPC_DOTPRODUCT(ui, vj, k);

        ll delta_share;
        #ifdef ROLE_p0
            delta_share = sub(1, prodShare);
        #else
            delta_share = sub(0, prodShare);
        #endif

        Share prod_vec_share = co_await scalarVecProd(delta_share, vj, k);
        Share u_prime_b = ui + prod_vec_share;
        co_return u_prime_b;
    }

    // Function to securely select the query index j using oblivious transfer
    awaitable<int> selectQueryIndex(int num_queries) {
        // Implement oblivious transfer protocol to securely select the query index
        int selected_index = co_await obliviousTransfer(num_queries);
        co_return selected_index;
    }
};