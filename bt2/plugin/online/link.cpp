#include "link.h"
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr std::uint32_t magic = 0x4E325442; // "BT2N"
constexpr std::uint8_t version = 1;
enum Type : std::uint8_t {hello = 1, hello_ack = 2, ping = 3, pong = 4};
constexpr std::size_t packet_size = 20;
constexpr std::uint32_t hello_every = 100, ping_every = 500, give_up_after = 15000;

void put32(std::uint8_t* p, std::uint32_t v) {for (int i = 0; i < 4; ++i) p[i] = std::uint8_t(v >> (8 * i));}
std::uint32_t get32(const std::uint8_t* p) {return std::uint32_t(p[0]) | std::uint32_t(p[1]) << 8 | std::uint32_t(p[2]) << 16 | std::uint32_t(p[3]) << 24;}
std::uint32_t fnv(const std::string& text) {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : text) h = (h ^ c) * 16777619u;
    return h;
}
}

std::uint32_t Link::now() {
    return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

Link::~Link() {close();}

bool Link::open() {
    close();
    socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0) return false;
    ::fcntl(socket_, F_SETFL, ::fcntl(socket_, F_GETFL, 0) | O_NONBLOCK);
    sockaddr_in any{};
    any.sin_family = AF_INET;
    any.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(socket_, reinterpret_cast<sockaddr*>(&any), sizeof any) != 0) {close(); return false;}
    socklen_t length = sizeof any;
    ::getsockname(socket_, reinterpret_cast<sockaddr*>(&any), &length);
    port_ = ntohs(any.sin_port);
    // this phone's addresses on its networks (not the loopback), each with the socket's port
    local_.clear();
    ifaddrs* list = nullptr;
    if (::getifaddrs(&list) == 0) {
        for (ifaddrs* a = list; a; a = a->ifa_next) {
            if (!a->ifa_addr || a->ifa_addr->sa_family != AF_INET || !(a->ifa_flags & IFF_UP) || (a->ifa_flags & IFF_LOOPBACK)) continue;
            char text[INET_ADDRSTRLEN];
            const auto* in = reinterpret_cast<sockaddr_in*>(a->ifa_addr);
            if (::inet_ntop(AF_INET, &in->sin_addr, text, sizeof text) && local_.size() < 4) local_.push_back(std::string(text) + ":" + std::to_string(port_));
        }
        ::freeifaddrs(list);
    }
    state_ = Open;
    rtt_ = -1;
    return true;
}

void Link::close() {
    if (socket_ >= 0) ::close(socket_);
    socket_ = -1;
    state_ = Closed;
    candidates_.clear();
    local_.clear();
    rtt_ = -1;
}

void Link::start(const std::string& code, const std::vector<std::string>& addresses) {
    if (state_ != Open) return;
    session_ = fnv(code);
    candidates_.clear();
    for (const auto& a : addresses) {
        const auto colon = a.rfind(':');
        if (colon == std::string::npos) continue;
        in_addr ip{};
        const int port = std::atoi(a.c_str() + colon + 1);
        if (a.compare(0, 6, "relay=") == 0 || a.compare(0, 5, "seen=") == 0 || port <= 0 || port > 65535) continue;
        if (::inet_pton(AF_INET, a.substr(0, colon).c_str(), &ip) != 1) continue;
        candidates_.push_back({ip.s_addr, std::uint16_t(port)});
    }
    started_ = now();
    lastHello_ = started_ - hello_every;
    lastPing_ = 0;
    number_ = 0;
    state_ = candidates_.empty() ? Failed : Linking;
}

void Link::send(std::uint8_t type, std::uint32_t number, std::uint32_t echo, const Candidate& to) {
    if (socket_ < 0) return;
    std::uint8_t p[packet_size] = {};
    put32(p, magic);
    p[4] = version;
    p[5] = type;
    put32(p + 8, session_);
    put32(p + 12, number);
    put32(p + 16, echo);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = to.ip;
    a.sin_port = htons(to.port);
    ::sendto(socket_, p, sizeof p, 0, reinterpret_cast<sockaddr*>(&a), sizeof a);
}

bool Link::peerAddress(std::string& out) const {
    if (state_ != Up) return false;
    char text[INET_ADDRSTRLEN];
    in_addr ip{};
    ip.s_addr = remote_.ip;
    if (!::inet_ntop(AF_INET, &ip, text, sizeof text)) return false;
    out = std::string(text) + ":" + std::to_string(remote_.port);
    return true;
}

void Link::poll() {
    if (socket_ < 0 || state_ == Open || state_ == Closed || state_ == Failed) return;
    const std::uint32_t t = now();
    if (state_ == Linking) {
        if (t - started_ > give_up_after) {state_ = Failed; return;}
        if (t - lastHello_ >= hello_every) {
            lastHello_ = t;
            for (const auto& c : candidates_) send(hello, ++number_, t, c); // (to every address: the one that works answers)
        }
    } else if (t - lastPing_ >= ping_every) {
        lastPing_ = t;
        send(ping, ++number_, t, remote_);
    }
    // what came: at most a few dozen packets a frame
    for (int n = 0; n < 64; ++n) {
        std::uint8_t p[64];
        sockaddr_in from{};
        socklen_t length = sizeof from;
        const ssize_t got = ::recvfrom(socket_, p, sizeof p, 0, reinterpret_cast<sockaddr*>(&from), &length);
        if (got < 0) break;
        if (got < static_cast<ssize_t>(packet_size) || get32(p) != magic || p[4] != version || get32(p + 8) != session_) continue;
        const Candidate source{from.sin_addr.s_addr, ntohs(from.sin_port)};
        const std::uint32_t echo = get32(p + 16);
        switch (p[5]) {
        case hello:
            send(hello_ack, get32(p + 12), echo, source);
            if (state_ == Linking) {state_ = Up; remote_ = source;}
            break;
        case hello_ack:
            if (state_ == Linking) {state_ = Up; remote_ = source;}
            break;
        case ping:
            send(pong, get32(p + 12), echo, source);
            if (state_ == Linking) {state_ = Up; remote_ = source;} // (the other side is already up and has heard us)
            break;
        case pong: {
            if (state_ == Linking) {state_ = Up; remote_ = source;}
            const int sample = static_cast<int>(t - echo);
            if (sample >= 0 && sample < 10000) rtt_ = rtt_ < 0 ? sample : (rtt_ * 3 + sample) / 4;
            break;
        }
        default:
            break;
        }
    }
}
