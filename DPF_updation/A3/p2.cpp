#include "common.hpp"
#include "shares.hpp"
#include <boost/asio.hpp>
#include <boost/asio/read.hpp>
#include <iostream>
#include "utility.hpp"
using namespace std;
typedef long long int ll;

// provides connected clients (P0 and P1) with Beaver triples.
awaitable<void> handle_clients(tcp::socket p0_socket, tcp::socket p1_socket) {
    try {
        ll k;
        // read requests from P0 only
        co_await boost::asio::async_read(p0_socket, boost::asio::buffer(&k, sizeof(k)), use_awaitable);
        cout << "P2: Received request for " << k << " triples." << endl;

        while(k>0){
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

            // Next request (still from P0 only)
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

        // Accept two connections (order is non-deterministic)
        tcp::socket sA = acceptor.accept();
        cout << "P2 accepted connection A." << endl;
        tcp::socket sB = acceptor.accept();
        cout << "P2 accepted connection B." << endl;

        // Read 1-byte role from each: 0 => P0, 1 => P1
        uint8_t rA = 255, rB = 255;
        boost::asio::read(sA, boost::asio::buffer(&rA, 1));
        boost::asio::read(sB, boost::asio::buffer(&rB, 1));
        if (!((rA == 0 || rA == 1) && (rB == 0 || rB == 1) && (rA != rB))) {
            cerr << "P2: invalid role handshake: rA=" << (int)rA << " rB=" << (int)rB <<endl;
            return 1;
        }

        tcp::socket p0_socket = (rA == 0) ? std::move(sA) : std::move(sB);
        tcp::socket p1_socket = (rA == 1) ? std::move(sA) : std::move(sB);

        // Spawn coroutine to serve triples (reads k only from P0)
        co_spawn(io_context, handle_clients(std::move(p0_socket), std::move(p1_socket)), detached);
        io_context.run();
    } catch (exception& e) {
        cerr << "Exception in P2: " << e.what() << "\n";
    }
    return 0;
}
