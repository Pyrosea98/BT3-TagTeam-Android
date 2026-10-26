#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

// What the other player must be told to reach this game from another network: the address the home's router shows to the internet.
// A STUN server (public and free: no account) answers a request with "you appear to me as ip:port". The request goes out of the
// line's own socket, so the answer is the address and port the router gave THAT socket, the one the other game will send to.
//
// Two servers are asked. The same port from both means the router keeps one port for the socket whoever is on the other side
// (a "cone" router: the hole punch works). Different ports mean one port per destination ("symmetric": the other game cannot
// know the port in advance, and only a relay will do).
class Stun {
public:
    ~Stun();
    void begin();                                        // the names are resolved on a thread of their own
    void reset();
    void poll(int socket);                               // sends the requests that are due
    // A packet that came in: true when it was a STUN answer (consumed).
    bool handle(const std::uint8_t* packet, int size, std::uint32_t fromIp, std::uint16_t fromPort);
    bool finished() const;                               // every server answered, or the tries are used up
    std::vector<std::string> mapped() const;             // the distinct "ip:port" seen, in the order they came
    bool symmetric() const;                              // two answers with different ports

private:
    struct Server {
        std::string host, port;
        std::uint32_t ip = 0;
        std::uint16_t portNet = 0;
        std::uint8_t transaction[12] = {};
        bool answered = false;
        std::uint32_t mappedIp = 0;
        std::uint16_t mappedPort = 0;
    };
    std::vector<Server> servers_;
    std::thread resolver_;
    std::atomic<int> resolved_{0};
    bool began_ = false;
    int rounds_ = 0;
    std::uint32_t lastAsk_ = 0;
};
