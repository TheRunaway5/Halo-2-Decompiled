#pragma once
#include "halo2/multiplayer_authority.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_player_admission.hpp"

namespace halo2 {
// One native session executor's coupled TU1.5 world/Match transaction. All raw
// memory, scene, view and callback borrowers must be quiescent throughout.
// This is owning in-process state, not a peer wire format or authentication.
class SourceMatchAuthority {
public:
  using AuthorityDigest=MultiplayerMatch::AuthorityDigest;
  struct Checkpoint {
    std::uint32_t version=1;
    MultiplayerMatch::AuthorityCheckpoint match;
    MultiplayerObjectRuntime::AuthorityCheckpoint world;
  };
  SourceMatchAuthority(MultiplayerMatch &,MultiplayerObjectRuntime &);
  SourceMatchAuthority(const SourceMatchAuthority &)=delete;
  SourceMatchAuthority &operator=(const SourceMatchAuthority &)=delete;
  Checkpoint checkpoint(AuthorityDigest)const;
  // Export actual allocated source identities at a completed native boundary,
  // independently of any received cut. Slots come from the accepted roster.
  std::vector<SourcePlayerAdmission> player_admissions(std::span<const unsigned>)const;
  class Prepared {
  public:
    ~Prepared();
    Prepared(Prepared &&) noexcept;
    Prepared &operator=(Prepared &&) noexcept;
    bool for_owner(const SourceMatchAuthority &)const noexcept;
  private:
    friend class SourceMatchAuthority;
    Prepared();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  Prepared prepare(const Checkpoint &,AuthorityDigest);
  // Rebind an incoming cut to this PC's already configured local views.
  // workspace must be a separate, quiescent, disposable executor configured
  // for the incoming cut, with private files and local-only observers. It is
  // consumed/reconfigured even if the returned preparation is cancelled.
  // Original TU1.5 local-view/input/first-person calls run there, never on this
  // live target before publication. No hardware/profile/voice or peer wire
  // ownership is inferred by this operation.
  using LocalPlayerAdmission=SourcePlayerAdmission;
  // Admissions must come from the already accepted session roster, bound by
  // its executor to this PC. Do not derive them from the untrusted incoming
  // cut, a screen name or a newly constructed offline prototype on this PC.
  Prepared prepare_with_local_views(const Checkpoint &,AuthorityDigest,
      std::span<const LocalPlayerAdmission>,SourceMatchAuthority &workspace);
  // World backing publishes before both native pool views. This owner and
  // its Match/world must outlive preparation; references expire on success.
  bool commit(Prepared &&)noexcept;
private:
  MultiplayerMatch &match_;
  MultiplayerObjectRuntime &world_;
  mutable NativeAuthorityBoundary calls_;
};
}
