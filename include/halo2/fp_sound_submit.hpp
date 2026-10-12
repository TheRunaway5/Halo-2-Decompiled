#pragma once
#include "halo2/fp_sound_request.hpp"

namespace halo2 {
// The FP caller owns all of this storage. In particular, location+30..43 and
// every unwritten raw request byte retain their caller values. They are not
// source constructor defaults and must not be synthesized from the semantic
// request. A retained host private frame can supply this backing explicitly.
struct FirstPersonRawSoundBacking {
  FirstPersonSoundBacking event;
  std::array<std::byte,20> location_tail;
  std::array<std::byte,160> request;
};
struct FirstPersonRawSoundStore {
  std::uint32_t instruction;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const FirstPersonRawSoundStore &,
                         const FirstPersonRawSoundStore &)=default;
};
using FirstPersonRawSoundObserver=std::function<void(const FirstPersonRawSoundStore &)>;
struct FirstPersonRawSoundServices {
  // The same original event/marker/world owners as the semantic producer.
  // Its semantic submit/attention callbacks are deliberately not used here.
  FirstPersonSoundServices owners;
  // Complete21F220 must receive these actual160 bytes and own any mutations.
  // Missing availability/admission/cache/voice services have no default reply.
  std::function<std::uint32_t(std::uint32_t sound,
                              std::span<std::byte,160> request)> submit;
  // Required only after genuine success and the actual reread class flags.
  // It receives the post-submit raw request, including possible effect-owner
  // mutations. The earlier semantic packet cannot supply these current bits.
  std::function<void(std::uint32_t sound,std::span<const std::byte,160> request,
                      std::span<const std::byte,20> definition)> successful_attention;
};
// Existing complete represented FP event/18AD70 plus the actual FP18BA10 raw
// construction path. This caller always supplies marker40, interfaceNONE and
// no selected range/permutation pointer; other18BA10 callers are not covered.
// It preserves the original raw-construction write order before whole submit.
bool source_first_person_sound_raw_event(FirstPersonSoundState &,
    bool source_view_storage_present,unsigned view,const FirstPersonHandBinding &,
    const FirstPersonAnimationEvent &,std::uint16_t current_source_bsp,
    const FirstPersonSoundConstants &,FirstPersonRawSoundBacking &,
    const FirstPersonRawSoundServices &,const FirstPersonRawSoundObserver & = {});
} // namespace halo2
