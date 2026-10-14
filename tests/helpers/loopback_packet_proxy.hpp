#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <vector>
#if defined(__linux__)
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace halo2::test {
struct PacketFaultStatistics {
    std::uint64_t dropped{}, delayed{}, duplicated{}, delivered{}, peak_queued_bytes{};
};

// Test-only, thread-free forwarding between real loopback UDP sockets.
// Startup and client controls pass unchanged. Once enabled, downstream
// datagrams are dropped, delayed or duplicated by a deterministic schedule.
// Delay uses the simulation scheduler's ticks; no gameplay clock is changed.
#if defined(__linux__)
class LoopbackPacketProxy {
    struct Socket {
        int fd = -1;
        unsigned port{};
        Socket() {
            fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
            if (fd < 0) throw FormatError("Cannot create loopback test socket");
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            const int capacity = 32768;
            socklen_t size = sizeof(address);
            if (::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &capacity, sizeof(capacity)) ||
                ::bind(fd, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) ||
                ::getsockname(fd, reinterpret_cast<sockaddr *>(&address), &size)) {
                ::close(fd); fd = -1;
                throw FormatError("Cannot bind bounded loopback test socket");
            }
            port = ntohs(address.sin_port);
        }
        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;
        ~Socket() { if (fd >= 0) ::close(fd); }
    };
    struct Pending { std::uint32_t due{}; std::vector<std::byte> bytes; };
public:
    explicit LoopbackPacketProxy(unsigned host_port) {
        host_.sin_family = AF_INET;
        host_.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        host_.sin_port = htons(host_port);
    }
    unsigned port() const { return front_.port; }
    void enable() { enabled_ = true; }
    const PacketFaultStatistics &statistics() const { return statistics_; }

    void pump(std::uint32_t tick) {
        receive(front_, [&](std::span<const std::byte> bytes, const sockaddr_in &from) {
            if (client_ && !same_endpoint(*client_, from))
                throw FormatError("Loopback proxy received an unexpected client");
            client_ = from;
            send(back_, host_, bytes);
        });
        receive(back_, [&](std::span<const std::byte> bytes, const sockaddr_in &from) {
            if (!same_endpoint(from, host_) || !client_)
                throw FormatError("Loopback proxy received an unexpected host");
            if (!enabled_) { send(front_, *client_, bytes); return; }
            const auto sequence = ++sequence_;
            if (sequence % 53 == 0) { ++statistics_.dropped; return; }
            const bool delayed = sequence % 11 == 0;
            if (delayed) ++statistics_.delayed;
            enqueue(bytes, tick + (delayed ? 2 : 0));
            if (sequence % 41 == 0) {
                ++statistics_.duplicated;
                enqueue(bytes, tick + 1);
            }
        });
        for (auto item = pending_.begin(); item != pending_.end();) {
            if (item->due > tick) { ++item; continue; }
            send(front_, *client_, item->bytes);
            queued_bytes_ -= item->bytes.size();
            ++statistics_.delivered;
            item = pending_.erase(item);
        }
    }
private:
    static bool same_endpoint(const sockaddr_in &a, const sockaddr_in &b) {
        return a.sin_family == b.sin_family && a.sin_port == b.sin_port &&
               a.sin_addr.s_addr == b.sin_addr.s_addr;
    }
    static void send(const Socket &socket, const sockaddr_in &to, std::span<const std::byte> bytes) {
        const auto count = ::sendto(socket.fd, bytes.data(), bytes.size(), 0,
                                   reinterpret_cast<const sockaddr *>(&to), sizeof(to));
        if (count < 0 || std::size_t(count) != bytes.size())
            throw FormatError("Loopback test forwarding failed");
    }
    template<class Consumer> void receive(const Socket &socket, Consumer consume) {
        for (unsigned packet = 0; packet < 512; ++packet) {
            sockaddr_in from{};
            socklen_t size = sizeof(from);
            const auto count = ::recvfrom(socket.fd, scratch_.data(), scratch_.size(), MSG_TRUNC,
                                         reinterpret_cast<sockaddr *>(&from), &size);
            if (count < 0) {
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                throw FormatError("Loopback test receive failed");
            }
            if (std::size_t(count) > scratch_.size() || size != sizeof(from))
                throw FormatError("Loopback test datagram was truncated");
            consume(std::span<const std::byte>(scratch_.data(), std::size_t(count)), from);
        }
        throw FormatError("Loopback test receive exceeded its bounded packet pump");
    }
    void enqueue(std::span<const std::byte> bytes, std::uint32_t due) {
        if (pending_.size() >= 96 || bytes.size() > 262144 - queued_bytes_)
            throw FormatError("Loopback fault queue exceeded its test budget");
        pending_.push_back({due, {bytes.begin(), bytes.end()}});
        queued_bytes_ += bytes.size();
        statistics_.peak_queued_bytes = std::max(statistics_.peak_queued_bytes, std::uint64_t(queued_bytes_));
    }
    Socket front_, back_;
    sockaddr_in host_{};
    std::optional<sockaddr_in> client_;
    bool enabled_{};
    std::uint64_t sequence_{};
    std::size_t queued_bytes_{};
    std::array<std::byte, 65536> scratch_;
    std::vector<Pending> pending_;
    PacketFaultStatistics statistics_;
};
#else
class LoopbackPacketProxy {
public:
    explicit LoopbackPacketProxy(unsigned) { throw FormatError("Packet-fault simulations currently require Linux"); }
    unsigned port() const { return 0; }
    void enable() {}
    void pump(std::uint32_t) {}
    PacketFaultStatistics statistics() const { return {}; }
};
#endif
} // namespace halo2::test
