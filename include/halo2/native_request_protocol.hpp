#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace halo2 {
inline constexpr std::size_t native_request_profile_bytes=0x90;
struct NativeLocalPlayerRequest {
    std::int8_t team{};
    std::optional<std::array<std::uint8_t,native_request_profile_bytes>> profile;
};
struct NativeVehicleIntent {
    std::int16_t action{},index{};
    std::uint32_t object{};
    bool operator==(const NativeVehicleIntent &)const=default;
};
struct NativeInputControl {
    float forward{},right{},yaw{},pitch{};
    std::uint16_t flags{};
    std::int16_t zoom{};
    std::uint32_t zoom_reset_serial{};
    std::uint16_t pickup_milliseconds{},dual_wield_milliseconds{};
    std::array<std::uint16_t,2> completed{};
    std::array<std::uint32_t,10> presses{};
    std::optional<NativeVehicleIntent> vehicle,completed_vehicle;
};
struct NativeHelloRequest {
    std::uint64_t nonce{},fingerprint{};
    std::vector<NativeLocalPlayerRequest> players;
};
struct NativeInputRequest {
    std::uint64_t nonce{},cookie{};
    std::uint32_t sequence{};
    std::vector<NativeInputControl> controls;
};
struct NativeLeaveRequest {std::uint64_t nonce{},cookie{};};
struct NativeSessionQuery {std::uint64_t nonce{};};
using NativeRequest=std::variant<NativeHelloRequest,NativeInputRequest,NativeLeaveRequest,NativeSessionQuery>;

// Complete decoding of native host-bound requests only. This namespace's kind
// 6 is a session query, not the separate ONLINE voice format. Owned results
// grant no endpoint, cookie, account, seat, team or gameplay-object authority.
// No legacy decoder fallback. Native policy and lifecycle checks remain with
// the caller; malformed frames fail before it receives any command.
std::optional<NativeRequest> decode_native_request(std::span<const std::byte>,std::uint16_t protocol);
}
