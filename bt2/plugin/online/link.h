#pragma once
#include <cstdint>
#include <string>
#include <vector>

// The direct line between the two games of an online match (UDP). The matchmaking service (the app talks to it) only tells each
// side the other's name and addresses; this is what makes sure the other game is really there and how long a packet takes.
//
// Both sides open a socket before they ask the service for a room, and give it their addresses ("ip:port"). Once each has the
// other's, both send a HELLO to every address at the same time, over and over: a router that only lets in what the home sent
// out lets the other side's HELLO in after its own was sent (the "hole punch"). The first HELLO or HELLO_ACK that comes back
// from the other game, with the room's session number, makes the line UP; after that a PING goes every half second and its PONG
// gives the round trip. Without an answer in 15 seconds the line has FAILED (no relay yet: it works on one network and through
// routers that keep a port; the relay is the next piece).
//
// A packet is 20 bytes, little-endian: magic "BT2N", version 1, type, 2 reserved, the session (a hash of the room's code), a
// number, and the sender's clock in milliseconds (echoed back by a PONG).
class Link {
public:
    enum State {Closed, Open, Linking, Up, Failed};

    ~Link();
    bool open();                                         // the socket, any port; false: none could be made
    void close();
    const std::vector<std::string>& localAddresses() const {return local_;}
    // Starts the exchange with the other game: `code` names the session, `addresses` are the other side's "ip:port" strings.
    void start(const std::string& code, const std::vector<std::string>& addresses);
    void poll();                                         // once a frame: sends what is due, reads what came
    State state() const {return state_;}
    int roundTripMs() const {return rtt_;}               // smoothed, milliseconds; -1 before the first PONG
    bool peerAddress(std::string& out) const;            // where the other game answered from, once UP

private:
    struct Candidate {std::uint32_t ip = 0;std::uint16_t port = 0;};
    void send(std::uint8_t type, std::uint32_t number, std::uint32_t echo, const Candidate& to);
    static std::uint32_t now();

    int socket_ = -1;
    State state_ = Closed;
    std::uint16_t port_ = 0;
    std::uint32_t session_ = 0, number_ = 0, started_ = 0, lastHello_ = 0, lastPing_ = 0;
    std::vector<std::string> local_;
    std::vector<Candidate> candidates_;
    Candidate remote_;
    int rtt_ = -1;
};
