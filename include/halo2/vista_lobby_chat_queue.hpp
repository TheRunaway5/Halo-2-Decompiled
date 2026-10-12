#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
inline constexpr std::size_t vista_lobby_chat_queue_source_size=0x2128;
inline constexpr std::size_t vista_lobby_chat_record_size=0x108;
using VistaLobbyChatRecord=std::array<std::byte,vista_lobby_chat_record_size>;
// Fresh source9035F8 storage is zero before5EC66F enables byte0. Enabling
// retained storage does not clear either ring, indices, messages or padding.
void vista_lobby_chat_queue_enable(std::span<std::byte> source);
// Complete5EC80B and602D10/602DBE: a full16-entry ring drops its oldest
// message before inserting. No local identity or message field is invented.
void vista_lobby_chat_queue_enqueue(std::span<std::byte> source,
    const VistaLobbyChatRecord&);
// Complete602D67: false leaves the output untouched; only the exact108h
// record is copied. The outer source reader below clears rejected output.
bool vista_lobby_chat_queue_pop(std::span<std::byte> source,
    VistaLobbyChatRecord& output);

struct VistaLobbyChatQueueSession {
  std::span<const std::byte> bank;
  // Actual owned object bound by this bank's source session+10. The binding
  // is explicit because retained source IDs are not native pointer values.
  std::span<std::byte> queue;
};
struct VistaLobbyChatQueueServices {
  // Actual5AD736 queries, separately sampled at the wrapper and every
  // consumed queued record. A failed outer query leaves output untouched.
  std::function<std::optional<VistaLobbyChatQueueSession>()> current_session;
  // Complete5C2F5C's consumed6A8685/XUserMuteListQuery. Source initializes
  // muted0 and passes controller0 plus the occupied player's actual XUID.
  // Any HRESULT!=0 or muted!=0 rejects the message. Class!=2 skips it.
  std::function<std::uint32_t(std::uint32_t controller,std::uint64_t xuid,
      std::uint32_t& muted)> mute_list_query;
};
// Complete5B18B3 state gate and5EC831 consumer, including identity, occupied
// player, mute, invalid-message draining and empty-output clearing. Reaching
// a lost required inner session or null identity is an explicit unowned
// source boundary: the original would spin or dereference null, respectively.
bool vista_lobby_chat_receive(VistaLobbyChatRecord& output,
    const VistaLobbyChatQueueServices&);
} //namespace halo2
