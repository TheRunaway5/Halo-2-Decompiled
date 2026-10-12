#pragma once
#include "halo2/fp_sound_request.hpp"

namespace halo2 {
// Immutable original143CE0/18A800/BB0C0 inputs, imported from the selected
// executable. No pool, BSP, listener, node or velocity value belongs here.
struct SoundObjectRequestConstants {
  std::uint32_t one_bits, minus_one_bits, z_scale_bits, xy_scale_bits;
  std::array<std::byte,12> zero_vector;
};
// The reader resolves retained source32-bit address words to owned storage.
// It must not create a body, relocate a field or manufacture a current node.
// All addresses below are actual runtime owner outputs, not fresh XBE data.
struct SoundObjectRequestMemory {
  std::uint32_t object_pool, physics_pool, sound_system, render_bsp;
  std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
};
struct SoundObjectRequestServices {
  SoundObjectRequestMemory memory;
  FirstPersonSoundServices associations;
  // Complete11D4A0/14B9D0 location query at the actual event phase. The eight
  // output bytes include the actual current BSP. Required only after the
  // original unit/vehicle animation-activity predicate admits that query.
  std::function<std::array<std::byte,8>(Vec3)> point_location;
};
struct SoundObjectRequestStore {
  std::uint32_t instruction;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const SoundObjectRequestStore &,
                         const SoundObjectRequestStore &)=default;
};
using SoundObjectRequestObserver=std::function<void(const SoundObjectRequestStore &)>;

// Exact18A800 quantization under the caller's actual x87 control word.
// The SSE operations remain separated; invalid FISTP uses original80000000.
std::uint32_t source_sound_compressed_forward(Vec3,
    const SoundObjectRequestConstants &,std::uint16_t source_fpcw);
// Complete143CE0, consuming an actual52-byte scale/basis/translation matrix.
Vec3 source_sound_marker_position(std::span<const std::byte,52>,Vec3,
                                 const SoundObjectRequestConstants &);
// CompleteBB1F0 root-location and B9AC0 node-bank owners. The latter indexes
// the supplied signed node exactly; host bounds errors remain unavailable.
std::array<std::byte,8> source_sound_object_location(std::uint32_t,
                                                  const SoundObjectRequestMemory &);
std::array<std::byte,52> source_sound_object_node(std::uint32_t,std::int16_t,
                                               const SoundObjectRequestMemory &);
// CompleteBB0C0 linear-velocity path, including1D43C0 and actual physical
// record gates. It never derives velocity from native player movement.
std::array<std::byte,12> source_sound_object_velocity(std::uint32_t,
    const SoundObjectRequestMemory &,const SoundObjectRequestConstants &);
// Complete18DDE0 direct owner, including salted BBCB0, BABD0 parent rejection,
// raw root location/node/physical velocity, unit activity query, type7 PVS
// remap and18EEE0 audibility. No storage is initialized here. The required
// associations/query providers must own their source inputs; submit21F220 is
// separate. False is an original rejection, missing backing throws.
bool source_sound_object_request(std::uint32_t owner,std::uint32_t sound,
    std::span<const std::byte,40> marker,std::span<std::byte,48> location,
    const SoundObjectRequestConstants &,std::uint16_t source_fpcw,
    const SoundObjectRequestServices &,const SoundObjectRequestObserver & = {});
} // namespace halo2
