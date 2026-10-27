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
// Predicted motion of the non-host's own biped after this input (protocol
// 113, NETCODE_ENHANCED_GOAL item 1): absolute position in 1/1024 world units
// (signed 24-bit), velocity in 1/256 world units per second, and the input
// distance to the acknowledged baseline the prediction restarted from (255:
// unknown). Wire: flags (bit0 present, bit1 grounded, bit2 crouching), then
// age, 3 x i24 and 3 x i16 when present; absent is exactly one zero byte.
struct NativeMotionReport {
    bool grounded{},crouching{};
    std::uint8_t baseline_age{};
    std::array<std::int32_t,3> position{};
    std::array<std::int16_t,3> velocity{};
    bool operator==(const NativeMotionReport &)const=default;
};
inline constexpr std::int32_t native_motion_position_limit=(1<<23)-1;
struct NativeInputControl {
    float forward{},right{},yaw{},pitch{};
    std::uint16_t flags{};
    std::int16_t zoom{};
    std::uint32_t zoom_reset_serial{};
    std::uint16_t pickup_milliseconds{},dual_wield_milliseconds{};
    std::array<std::uint16_t,2> completed{};
    std::array<std::uint32_t,10> presses{};
    std::optional<NativeVehicleIntent> vehicle,completed_vehicle;
    std::optional<NativeMotionReport> motion;
};
// Held fields of an earlier input repeated for loss tolerance (item 5). Wire:
// one mask byte per view (forward, right, yaw, pitch, flags, zoom; bits 6-7
// reserved), then each masked value, which must differ from the next newer
// input's. Serials, completions, intents and the motion report travel only
// with the newest input (the host adopts only the newest report).
struct NativeRedundantControl {
    float forward{},right{},yaw{},pitch{};
    std::uint16_t flags{};
    std::int16_t zoom{};
};
struct NativeRedundantInput {
    std::uint32_t sequence{}; // newest sequence - 1, - 2
    std::vector<NativeRedundantControl> controls;
};
inline constexpr std::size_t native_max_redundant_inputs=2;
struct NativeHelloRequest {
    std::uint64_t nonce{},fingerprint{};
    std::vector<NativeLocalPlayerRequest> players;
};
// Snapshot stream acknowledgement (newest decoded sequence, the 32 before it
// as a mask, bit 0 of flags requests a self-contained reset).
struct NativeSnapshotAck {std::uint32_t latest{},mask{};std::uint8_t flags{};};
// Displayed snapshot time (protocol 113): low 16 bits of the older
// interpolation endpoint's host tick, blend toward the next tick in 1/255
// steps (0..254). The wire's fraction 255 with tick 0 means absent.
struct NativeSnapshotView {
    std::uint16_t tick{};std::uint8_t fraction{};
    bool operator==(const NativeSnapshotView &)const=default;
};
struct NativeInputRequest {
    std::uint64_t nonce{},cookie{};
    std::uint32_t sequence{};
    std::vector<NativeInputControl> controls;
    NativeSnapshotAck ack;
    std::optional<NativeSnapshotView> view;
    // Newest first; at most native_max_redundant_inputs, sequences >= 1.
    std::vector<NativeRedundantInput> redundant;
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
