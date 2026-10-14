#pragma once
#include "halo2/native_runtime_address.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace halo2 {
// Xbox TU1.5 uses a 12-byte public prefix and a 76-byte extended record.
// Retain unknown extended fields without borrowing Vista's different layout.
struct NativeTu15NetworkConfiguration {
    std::array<std::uint8_t,76> bytes{};
    std::size_t private_pool_bytes()const{return std::size_t(bytes[2])*4096;}
    unsigned max_sockets()const{return bytes[6];}
    unsigned receive_buffer_bytes()const{return unsigned(bytes[7])*1024;}
    unsigned send_buffer_bytes()const{return unsigned(bytes[8])*1024;}
    unsigned key_slots()const{return bytes[9];}
    unsigned association_slots()const{return bytes[10];}
    unsigned qos_data_bytes()const{return unsigned(bytes[11])*4;}
};
NativeTu15NetworkConfiguration native_tu15_network_configuration(std::span<const std::byte> input={});
struct NativeIpv4Endpoint {
    std::array<std::uint8_t,4> address{};
    std::uint16_t port{};
    auto operator<=>(const NativeIpv4Endpoint&)const=default;
};
struct NativeTu15NetworkState;
// Actual native UDP resources. This is the PC port transport, not an Xbox
// Ethernet driver, Xbox secure packet format or Xbox Live service emulator.
class NativeTu15Datagram final {
public:
    NativeTu15Datagram(NativeTu15Datagram&&)noexcept;
    NativeTu15Datagram& operator=(NativeTu15Datagram&&)noexcept;
    ~NativeTu15Datagram();
    NativeTu15Datagram(const NativeTu15Datagram&)=delete;
    NativeTu15Datagram& operator=(const NativeTu15Datagram&)=delete;
    NativeIpv4Endpoint local_endpoint()const;
    void bind(NativeIpv4Endpoint);
    void set_nonblocking(bool);
    void set_broadcast(bool);
    void send(std::span<const std::byte>,NativeIpv4Endpoint);
    std::optional<std::size_t> receive(std::span<std::byte>,NativeIpv4Endpoint&);
    std::array<unsigned,2> observed_buffer_bytes()const;
private:
    friend class NativeTu15NetworkService;
    NativeTu15Datagram(std::shared_ptr<NativeTu15NetworkState>,std::uint64_t);
    void close()noexcept;
    std::shared_ptr<NativeTu15NetworkState> state_;
    std::uint64_t identity_{};
};
class NativeTu15NetworkService final {
public:
    explicit NativeTu15NetworkService(NativeTu15NetworkConfiguration);
    ~NativeTu15NetworkService();
    NativeTu15NetworkService(const NativeTu15NetworkService&)=delete;
    NativeTu15NetworkService& operator=(const NativeTu15NetworkService&)=delete;
    const NativeTu15NetworkConfiguration& configuration()const;
    NativeRuntimeAddress private_pool_address()const;
    NativeTu15Datagram open_datagram(NativeIpv4Endpoint local={});
    NativeTu15Datagram create_datagram();
    std::size_t live_sockets()const;
    void close()noexcept;
private:
    std::shared_ptr<NativeTu15NetworkState> state_;
};
}
