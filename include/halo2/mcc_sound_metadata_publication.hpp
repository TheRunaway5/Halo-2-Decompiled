#pragma once
#include "halo2/sound_variation.hpp"
#include <memory>

namespace halo2 {
// Immutable converted metadata for native PCM consumers with no local World.
// Production runs the same complete Cache delivery used by a native World.
// A provenance-admitted original catalog joins metadata and encoded payloads
// in this same immutable publication. It does not consume a World's prepared
// resource registry or grant a native epoch.
// This owner provides neither a timing lease nor script/voice readiness.
class MccSoundMetadataPublication final {
  struct Impl;
  std::shared_ptr<const Impl> impl_;
  explicit MccSoundMetadataPublication(std::shared_ptr<const Impl>);
public:
  static MccSoundMetadataPublication create(std::shared_ptr<const Cache>);
  void require_owner(const Cache &)const;
  void require_owner(const std::shared_ptr<const Cache> &)const;
  bool same_owner(const MccSoundMetadataPublication &)const noexcept;
  SoundVariationDefinition load(const Cache &,TagHandle,unsigned pitch=0)const;
  EncodedSound encoded(const Cache &,TagHandle,unsigned pitch,unsigned permutation)const;
};
}
