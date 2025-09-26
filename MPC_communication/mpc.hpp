#pragma once

#include "common.hpp"
#include "shares.hpp"
#include <vector>
#include "utility.hpp"
using namespace std;
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
        // beaver triplit
        vector<BeaverTriple> triples = co_await getBeaverTriple(k);
        Share a_b(k), b_b(k);
        vector<ll> c_b(k);
        for(int i=0; i<k; i++) {
            a_b.data[i] = triples[i].a;
            b_b.data[i] = triples[i].b;
            c_b[i] = triples[i].c;
        }
        
        // blinding the values 
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
            // (x+a)*y_b - (y+b)*a_b + c_b <- beaver method to get mulmiplication share
            ll term = addm(subm(mulm(alpha.data[i], y_b.data[i]),mulm(beta.data[i],  a_b.data[i])),c_b[i]);
            prodShare = addm(prodShare, term);
        }
        co_return prodShare;
    }
    
    // Securely computes the product of a secret-shared scalar and a secret-shared vector
    awaitable<Share> scalarVecProd(ll scalar_share, const Share& vec_share, int k) {
        // Get Beaver triples from P2
        vector<BeaverTriple> triples = co_await getBeaverTriple(k);
        Share a_b(k), b_b(k); // a is scalar, b is vector
        vector<ll> c_b(k);
        for(int i=0; i<k; ++i) {
            a_b.data[i] = triples[i].a; // Re-using vector share for scalar shares
            b_b.data[i] = triples[i].b;
            c_b[i] = triples[i].c;
        }
        
        // Mask scalar and vector (mod)
        ll alpha_b = addm(scalar_share, a_b.data[0]);
        Share beta_b = vec_share + b_b;

        // Exchange and reconstruct (mod)
        co_await send_val(peer_sock, alpha_b);
        ll alpha_peer = co_await recv_val(peer_sock);
        ll alpha = addm(alpha_b, alpha_peer);

        co_await send_vec(peer_sock, beta_b);
        Share beta_peer = co_await recv_vec(peer_sock, k);
        Share beta = beta_b + beta_peer;

        Share result(k);
        for (int i = 0; i < k; i++) {
            // (s+a)*v_b[i] - (v[i]+b[i])*a + c[i]
            result.data[i] = addm(subm(mulm(alpha,vec_share.data[i]), mulm(beta.data[i],a_b.data[0])), c_b[i]);
        }
        co_return result;
    }
    
    // request for k Beaver mulmiplication triples from P2
    awaitable<vector<BeaverTriple>> getBeaverTriple(int k) {
        // Only P0 sends the request to P2; P1 passively receives triples.
        #ifdef ROLE_p0
        co_await send_val(p2_sock, k);
        #endif

        vector<BeaverTriple> triples(k);
        co_await boost::asio::async_read(p2_sock, boost::asio::buffer(triples), use_awaitable);
        co_return triples;
    }

    // Select item v_j obliviously using secret-shared one-hot s (length n):
    // v_sel[d] = <s, V_col[d]> for d=0..k-1
    awaitable<Share> select_item_oblivious(const Share& s_b,const vector<Share>& V_rows_b, int n, int k) {
        Share select_b(k);
        for (int d = 0; d < k; d++) {
            Share col_d(n);
            for (int t = 0; t < n; ++t) col_d.data[t] = V_rows_b[t].data[d];
            ll coord_share = co_await MPC_DOTPRODUCT(s_b, col_d, n);
            select_b.data[d] = coord_share;
        }
        co_return select_b;
    }

public:

    MPCProtocol(tcp::socket& peer, tcp::socket& p2) : peer_sock(peer), p2_sock(p2) {}

    // function for full secure update protocol for u_i <- u_i + v_j * (1 - <u_i, v_j>)
    awaitable<Share> updateProtocol(const Share& ui, const Share& vj, int k) {
        ll prodShare = co_await MPC_DOTPRODUCT(ui, vj, k);

        ll delta_share;
        #ifdef ROLE_p0
            delta_share = subm(1, prodShare);
        #else
            delta_share = subm(0, prodShare);
        #endif

        Share prod_vec_share = co_await scalarVecProd(delta_share, vj, k);
        Share u_prime_b = ui + prod_vec_share;
        co_return u_prime_b;
    }

    // Expose oblivious selection for caller
    awaitable<Share> OT_select(const Share& s_b, const vector<Share>& V_rows_b, int n, int k) {
        co_return co_await select_item_oblivious(s_b, V_rows_b, n, k);
    }
};

