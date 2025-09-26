#include "common.hpp"
#include "shares.hpp"
#include <boost/asio.hpp>
#include <iostream>
using namespace std;
typedef long long int ll;
static inline ll norm(ll x){ x%=mod; if(x<0) x+=mod; return x; }
static inline ll addm(ll a,ll b){ return norm(a+b); }
static inline ll subm(ll a,ll b){ return norm(a-b); }
static inline ll mulm(ll a,ll b){ return norm(a*b); }

// provides connected clients (P0 and P1) with Beaver triples.
awaitable<void> handle_clients(tcp::socket p0_socket, tcp::socket p1_socket) {
    try {
        ll k;
        co_await boost::asio::async_read(p0_socket, boost::asio::buffer(&k, sizeof(k)), use_awaitable);
        cout << "P2: Received request for " << k << " triples." << endl;

        while(k > 0) {
            vector<BeaverTriple> p0_triples(k), p1_triples(k);

            // Single 'a' across k (matches scalarVecProd)
            ll a  = norm(random_uint32()%mod);
            ll a0 = norm(random_uint32()%mod);
            ll a1 = subm(a, a0);

            for(ll i=0; i<k; ++i) {
                ll b  = norm(random_uint32()%mod);
                ll c  = mulm(a, b);

                ll b0 = norm(random_uint32()%mod);
                ll c0 = norm(random_uint32()%mod);

                ll b1 = subm(b, b0);
                ll c1 = subm(c, c0);

                p0_triples[i] = {a0, b0, c0};
                p1_triples[i] = {a1, b1, c1};
            }

            co_await boost::asio::async_write(p0_socket, boost::asio::buffer(p0_triples), use_awaitable);
            co_await boost::asio::async_write(p1_socket, boost::asio::buffer(p1_triples), use_awaitable);

            co_await boost::asio::async_read(p0_socket, boost::asio::buffer(&k, sizeof(k)), use_awaitable);
            cout << "P2: Received request for " << k << " triples." << endl;
        }
    } catch (exception& e) { cout << "P2 closing connection: " << e.what() << "\n"; }
}

int main(int argc, char* argv[]) {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9002));

        cout << "P2 listening on port 9002..." << endl;

        // Accept connections from P0 and P1
        tcp::socket socket_p0 = acceptor.accept();
        cout << "P2 accepted connection 1." << endl;
        tcp::socket socket_p1 = acceptor.accept();
        cout << "P2 accepted connection 2." << endl;

        // Spawn a coroutine to handle this pair of clients
        co_spawn(io_context, handle_clients(move(socket_p0), move(socket_p1)), detached);
        io_context.run();
    } catch (exception& e) {
        cerr << "Exception in P2: " << e.what() << "\n";
    }
    return 0;
}
