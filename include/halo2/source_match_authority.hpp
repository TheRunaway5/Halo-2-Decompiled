#pragma once
#include "halo2/multiplayer_authority.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_player_admission.hpp"
#include "halo2/source_execution_inputs.hpp"

namespace halo2 {
// One native session executor's coupled TU1.5 world/Match transaction. All raw
// memory, scene, view and callback borrowers must be quiescent throughout.
// This is owning in-process state, not a peer wire format or authentication.
class SourceMatchAuthority {
public:
  using AuthorityDigest=MultiplayerMatch::AuthorityDigest;
  struct Checkpoint {
    std::uint32_t version=2;
    MultiplayerMatch::AuthorityCheckpoint match;
    MultiplayerObjectRuntime::AuthorityCheckpoint world;
  };
  SourceMatchAuthority(MultiplayerMatch &,MultiplayerObjectRuntime &);
  SourceMatchAuthority(const SourceMatchAuthority &)=delete;
  SourceMatchAuthority &operator=(const SourceMatchAuthority &)=delete;
  bool for_match(const MultiplayerMatch &match)const noexcept{return &match_==&match;}
  Checkpoint checkpoint(AuthorityDigest)const;
  std::uint64_t execution_tick()const;
  struct ExecutionBoundary {
    std::uint64_t tick{};
    std::array<std::uint64_t,4> mutations{};
    bool operator==(const ExecutionBoundary &)const=default;
  };
  ExecutionBoundary execution_boundary()const;
  // Executor-local scope for an entire real frame, including early loop exits.
  // The authority and its native world must outlive this scope. Destruction
  // cancels the transcript and releases its guards; it never reexecutes work.
  class ExecutionRecording {
  public:
    ~ExecutionRecording();
    ExecutionRecording(ExecutionRecording &&)noexcept;
    ExecutionRecording &operator=(ExecutionRecording &&)noexcept;
    SourceExecutionPhase finish();
  private:
    friend class SourceMatchAuthority;
    struct Impl;std::unique_ptr<Impl> impl_;
    ExecutionRecording(SourceMatchAuthority &,std::size_t);
  };
  ExecutionRecording start_execution_phase(std::size_t limit=SourceExecutionInputs::maximum_entries);
  // Caller supplies the complete admitted source call order and operands.
  // This is an owning in-process phase, not a peer codec or promotion proof.
  // Capture failure rejects publication without invalidating the real host.
  SourceExecutionPhase capture_execution_phase(const std::function<void()> &,
      std::size_t limit=SourceExecutionInputs::maximum_entries);
  // Replay failure leaves this candidate non-exportable until complete repair.
  // Successful replay anchors subsequent untaped clocks, not view/role/media.
  void replay_execution_phase(const SourceExecutionPhase &,const std::function<void()> &);
  // Execute the validated owning call journal through actual native APIs.
  // No caller closure, return override, hardware or host election is imported.
  void replay_execution_phase(const SourceExecutionPhase &);
  // Export actual allocated source identities at a completed native boundary,
  // independently of any received cut. Slots come from the accepted roster.
  std::vector<SourcePlayerAdmission> player_admissions(std::span<const unsigned>)const;
  // Complete live native allocation census, independent of any incoming cut.
  std::vector<SourcePlayerAdmission> allocated_player_admissions()const;
  class Prepared {
  public:
    ~Prepared();
    Prepared(Prepared &&) noexcept;
    Prepared &operator=(Prepared &&) noexcept;
    bool for_owner(const SourceMatchAuthority &)const noexcept;
    void retain_live_multiplayer_render_history();
    void retain_live_multiplayer_render_history_from(const SourceMatchAuthority &);
  private:
    friend class SourceMatchAuthority;
    Prepared();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  Prepared prepare(const Checkpoint &,AuthorityDigest);
  // Same producer view mapping, with every player allocation/identity checked
  // against independently accepted source admissions before publication.
  Prepared prepare(const Checkpoint &,AuthorityDigest,std::span<const SourcePlayerAdmission> full_roster);
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
  // Production receiver also validates every allocated player against the
  // independently accepted full roster before changing this live target.
  Prepared prepare_with_local_views(const Checkpoint &,AuthorityDigest,
      std::span<const LocalPlayerAdmission>,SourceMatchAuthority &workspace,
      std::span<const SourcePlayerAdmission> full_roster);
  // Reuse a private workspace previously rebound to the recipient. Producer
  // views must come from the independently accepted peer roster, never the
  // cut. Its view metadata becomes non-exportable until the next full world
  // commits there. Do not step or borrow the workspace between attempts.
  Prepared prepare_with_local_views(const Checkpoint &,AuthorityDigest,
      std::span<const LocalPlayerAdmission>,SourceMatchAuthority &workspace,
      std::span<const SourcePlayerAdmission> full_roster,std::span<const unsigned> producer_views);
  // World backing publishes before both native pool views. This owner and
  // its Match/world must outlive preparation; references expire on success.
  bool commit(Prepared &&)noexcept;
private:
  friend class SourceNativePublication;
  friend class SourceShadowPresentationOwner;
  // Only the double-buffered presentation owner may consume this private
  // staging world. A failure can alter staging, never the displayed owner.
  void import_presentation(const Checkpoint &,AuthorityDigest,
      std::span<const LocalPlayerAdmission>,std::span<const SourcePlayerAdmission>,
      std::span<const unsigned> producer_views,SourceMatchAuthority &displayed);
  // Only the coupled publication owner borrows these genuine counters, after
  // validating the complete native boundary and before taking exclusion.
  std::array<const std::uint64_t *,2> publication_call_counters()const {
    return match_.execution_call_counters();
  }
  MultiplayerMatch &match_;
  MultiplayerObjectRuntime &world_;
  mutable NativeAuthorityBoundary calls_;
};
}
