#include "stun.h"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <netdb.h>
#include <random>
#include <sys/socket.h>

namespace {
constexpr std::uint8_t cookie[4] = {0x21, 0x12, 0xA4, 0x42};
constexpr std::uint32_t ask_every = 400;
constexpr int tries = 8;

std::uint32_t nowMs() {
    return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}
}

Stun::~Stun() {if (resolver_.joinable()) resolver_.join();}

void Stun::reset() {
    if (resolver_.joinable()) resolver_.join();
    servers_.clear();
    resolved_ = 0;
    began_ = false;
    rounds_ = 0;
}

void Stun::begin() {
    reset();
    began_ = true;
    servers_.resize(2);
    servers_[0].host = "stun.l.google.com";     servers_[0].port = "19302";
    servers_[1].host = "stun.cloudflare.com";   servers_[1].port = "3478";
    static std::mt19937 random{std::random_device{}() ^ nowMs()};
    for (auto& s : servers_) for (auto& b : s.transaction) b = std::uint8_t(random());
    resolver_ = std::thread([this] {
        for (auto& s : servers_) {
            addrinfo hints{}, *res = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            if (::getaddrinfo(s.host.c_str(), s.port.c_str(), &hints, &res) == 0 && res) {
                const auto* a = reinterpret_cast<sockaddr_in*>(res->ai_addr);
                s.ip = a->sin_addr.s_addr;
                s.portNet = a->sin_port;
            }
            if (res) ::freeaddrinfo(res);
        }
        resolved_ = 1;
    });
}

void Stun::poll(int socket) {
    if (!began_ || resolved_ != 1 || socket < 0 || finished()) return;
    const std::uint32_t t = nowMs();
    if (rounds_ > 0 && t - lastAsk_ < ask_every) return;
    lastAsk_ = t;
    ++rounds_;
    for (auto& s : servers_) {
        if (s.answered || s.ip == 0) continue;
        std::uint8_t m[20] = {0x00, 0x01, 0x00, 0x00};   // a Binding request without attributes
        std::memcpy(m + 4, cookie, 4);
        std::memcpy(m + 8, s.transaction, 12);
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = s.ip;
        a.sin_port = s.portNet;
        ::sendto(socket, m, sizeof m, 0, reinterpret_cast<sockaddr*>(&a), sizeof a);
    }
}

bool Stun::handle(const std::uint8_t* p, int size, std::uint32_t fromIp, std::uint16_t fromPort) {
    if (!began_ || size < 20 || std::memcmp(p + 4, cookie, 4) != 0) return false;
    for (auto& s : servers_) {
        if (s.ip != fromIp || s.portNet != fromPort || std::memcmp(p + 8, s.transaction, 12) != 0) continue;
        if ((unsigned(p[0]) << 8 | p[1]) == 0x0101) {   // Binding success: XOR-MAPPED-ADDRESS (0x0020), or the older MAPPED-ADDRESS (0x0001)
            for (int at = 20; at + 4 <= size;) {
                const unsigned type = unsigned(p[at]) << 8 | p[at + 1];
                const int length = int(p[at + 2]) << 8 | p[at + 3];
                if (at + 4 + length > size) break;
                if ((type == 0x0020 || type == 0x0001) && length >= 8 && p[at + 5] == 1) {
                    const std::uint8_t* v = p + at + 4;
                    const bool x = type == 0x0020;
                    const std::uint8_t portBytes[2] = {std::uint8_t(v[2] ^ (x ? cookie[0] : 0)), std::uint8_t(v[3] ^ (x ? cookie[1] : 0))};
                    const std::uint8_t ipBytes[4] = {std::uint8_t(v[4] ^ (x ? cookie[0] : 0)), std::uint8_t(v[5] ^ (x ? cookie[1] : 0)),
                                                     std::uint8_t(v[6] ^ (x ? cookie[2] : 0)), std::uint8_t(v[7] ^ (x ? cookie[3] : 0))};
                    std::memcpy(&s.mappedPort, portBytes, 2);
                    std::memcpy(&s.mappedIp, ipBytes, 4);
                    s.answered = true;
                    if (x) break;
                }
                at += 4 + ((length + 3) & ~3);
            }
        }
        return true;
    }
    return false;
}

bool Stun::finished() const {
    if (!began_) return true;
    if (resolved_ == 1) {
        bool all = true;
        for (const auto& s : servers_) all = all && (s.answered || s.ip == 0);
        if (all) return true;
    }
    return rounds_ >= tries;
}

std::vector<std::string> Stun::mapped() const {
    std::vector<std::string> out;
    for (const auto& s : servers_) {
        if (!s.answered) continue;
        char text[INET_ADDRSTRLEN];
        in_addr ip{};
        ip.s_addr = s.mappedIp;
        if (!::inet_ntop(AF_INET, &ip, text, sizeof text)) continue;
        const std::string one = std::string(text) + ":" + std::to_string(ntohs(s.mappedPort));
        bool have = false;
        for (const auto& o : out) have = have || o == one;
        if (!have) out.push_back(one);
    }
    return out;
}

bool Stun::symmetric() const {
    std::uint16_t port = 0;
    bool first = true;
    for (const auto& s : servers_) {
        if (!s.answered) continue;
        if (first) {port = s.mappedPort; first = false;}
        else if (s.mappedPort != port) return true;
    }
    return false;
}
