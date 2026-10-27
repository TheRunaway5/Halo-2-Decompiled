#pragma once
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <bit>
#include <functional>
#include <utility>

namespace halo2 {
// The actual CB7BD / TU1.5 CC93D request, after the original suppression and
// normalized-damage gates. Sound selection and playback remain separate owners.
struct SourceDamageVoiceRequestTu15 {
  std::uint32_t caller{},target{},definition{},report_flags{};
  float recent_body{},recent_shield{},visible_body{};
};
namespace source_damage_voice_request_detail {
struct Binding {
  const SourcePhysicsContactMemory *memory{};
  std::function<void(const SourceDamageVoiceRequestTu15 &)> observe;
};
inline thread_local Binding *active{};
inline std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
  std::array<std::byte,4> bytes;memory.read(address,bytes);
  std::uint32_t value{};
  for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(n*8);
  return value;
}
}
// A native authority step owns this synchronous observation. Binding the bank
// prevents another native world on the same thread from publishing its request
// into this journal. Nested scopes restore the preceding owner exactly.
class SourceDamageVoiceRequestScopeTu15 {
  source_damage_voice_request_detail::Binding binding_;
  source_damage_voice_request_detail::Binding *previous_;
public:
  SourceDamageVoiceRequestScopeTu15(const SourcePhysicsContactMemory *memory,
      std::function<void(const SourceDamageVoiceRequestTu15 &)> observe)
      :binding_{memory,std::move(observe)},previous_(source_damage_voice_request_detail::active) {
    source_damage_voice_request_detail::active=memory?&binding_:nullptr;
  }
  ~SourceDamageVoiceRequestScopeTu15(){source_damage_voice_request_detail::active=previous_;}
  SourceDamageVoiceRequestScopeTu15(const SourceDamageVoiceRequestScopeTu15 &)=delete;
  SourceDamageVoiceRequestScopeTu15 &operator=(const SourceDamageVoiceRequestScopeTu15 &)=delete;
};
inline void source_damage_voice_request_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t caller,std::uint32_t target,std::uint32_t report,std::uint32_t body) {
  using namespace source_damage_voice_request_detail;
  const auto *owner=active;
  if(!owner || owner->memory!=&memory || !owner->observe)return;
  // Copy the actual request while its source frame is live. Bit casts preserve
  // the source scalar words without running another damage or voice selector.
  owner->observe({caller,target,word(memory,report+8),word(memory,report+4),
      std::bit_cast<float>(word(memory,body+0x100)),
      std::bit_cast<float>(word(memory,body+0xf4)),
      std::bit_cast<float>(word(memory,body+0xf8))});
}
}
