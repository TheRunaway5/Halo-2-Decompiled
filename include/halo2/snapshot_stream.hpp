#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>
namespace halo2 {
// Socket adapter for the bandwidth-bounded snapshot stream. Delta coding,
// acknowledgements, the per-client budget and pose replication are owned by
// Rust (rust/online_protocol/src/netcode). The native encoder keeps the
// snapshot schema; this stream only sees bytes, float offsets and the pose
// bank range. H2_NET_PROFILE selects the budget (default: retail Halo 2).
using SnapshotStreamDigest=std::array<std::byte,32>;
struct SnapshotStreamAck {std::uint32_t latest{},mask{};std::uint8_t flags{};};
class SnapshotStreamSender {
public:
  struct Emission {
    bool emitted{},keyframe{};
    SnapshotStreamDigest digest{};
    std::uint32_t view_bytes{};
    std::vector<std::vector<std::byte>> packets;
  };
  struct Frame {
    std::span<const std::byte> body;
    std::span<const std::uint32_t> floats;
    // Keyed list/entry starts, increasing; equal keys re-align the baseline.
    std::span<const std::uint32_t> anchor_offsets;
    std::span<const std::uint64_t> anchor_keys;
    std::optional<std::pair<std::size_t,std::size_t>> poses;
    std::uint32_t tick{},sequence{};
    std::uint64_t now_ms{};
    std::span<const float> viewers;
    std::span<const std::uint32_t> own;
    // Bodies playing a short action and their held weapons: never held.
    std::span<const std::uint32_t> hot;
  };
  struct Stats {
    std::uint64_t messages{},wire_bytes{},packets{},keyframes{},resets{},over_budget{};
    std::uint64_t full_refreshes{},root_updates{},forced_refreshes{},held_refreshes{},lost_messages{};
    std::uint32_t budget_bps{};std::uint64_t rtt_ms{};float loss_percent{};bool retired{};
  };
  // chunk: fragment payload per datagram; overhead: wire bytes outside it.
  SnapshotStreamSender(std::size_t chunk,std::size_t overhead);
  ~SnapshotStreamSender();
  // nullopt means the stream cannot carry this snapshot; send it complete.
  // An emission with emitted=false is a budget-skipped tick.
  std::optional<Emission> encode(const Frame &);
  void ack(const SnapshotStreamAck &,std::uint64_t now_ms);
  Stats stats()const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
class SnapshotStreamReceiver {
public:
  struct Delivery {
    std::uint32_t sequence{},tick{},view_bytes{};
    SnapshotStreamDigest digest{};
    std::vector<std::byte> body;
  };
  struct Stats {std::uint64_t messages{},failures{},held_poses{},missing_poses{};};
  explicit SnapshotStreamReceiver(std::size_t chunk);
  ~SnapshotStreamReceiver();
  // A completed message stays uncommitted until the native decoder accepts
  // (commit) or rejects it. `failed` reports a malformed/undecodable message.
  std::optional<Delivery> push(std::span<const std::byte>,std::uint64_t now_ms,bool &failed);
  bool commit(std::uint32_t sequence);
  void reject(std::uint32_t sequence);
  SnapshotStreamAck ack()const;
  void reset();
  Stats stats()const;
  std::size_t buffered_bytes()const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
