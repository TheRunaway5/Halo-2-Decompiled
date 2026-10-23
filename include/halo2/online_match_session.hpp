#pragma once
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2/game_voice.hpp"
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/peer_source_admissions.hpp"
#include "halo2/peer_mesh_quality.hpp"
#include "halo2/peer_host_agreement.hpp"
#include "halo2/peer_common_cut.hpp"
#include "halo2/peer_survivor_selection.hpp"
#include "halo2/peer_snapshot_candidate.hpp"
#include "halo2/online_peer_transport.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
namespace halo2 {
struct UiVariant;
class MultiplayerRuntimeAuthority;
struct MatchVoicePacket { unsigned peer{};bool preferences{},enabled{};std::uint16_t muted{};GameVoiceFrame frame; };
struct OnlineWorldSeal {
  std::uint32_t epoch{};
  std::uint64_t tick{};
  std::string digest;
};
struct OnlineContinuityState {
  std::uint32_t epoch{};
  unsigned host=16;
  std::uint16_t members{};
  std::optional<OnlineWorldSeal> selected;
};
struct OnlineMatchTransportOptions {
  bool direct_udp{};
  std::vector<std::string> stun_servers;
  // Internal native-handoff qualification gate. All admitted endpoints must
  // use the epoch-prefixed direct protocol. Ordinary matchmaking must leave
  // this gate disabled until native promotion and compatibility negotiate it.
  bool peer_host_migration{};
  // Separate opt-in native qualification. Default delivery cannot claim the
  // reserved snapshot contract until every physical peer negotiates it.
  bool snapshot_candidates{};
  // Separate native qualification gate. Every predecessor physical peer must
  // negotiate it. The default diagnostic migration fixtures omit this feature;
  // an enabled mixed-capability group cannot fall back to their Ready path.
  bool survivor_cut_selection{};
  std::optional<OnlineEosOptions> eos;
};
class OnlineMatchSession final : public MultiplayerDatagrams {
public:
  enum class Phase { Connecting, Loading, Running, Reviewing, Postgame, Failed,
                     AwaitingWorld, Migrating, Rebinding };
  enum class MigrationPreparationKind { Prepare,Retire };
  enum class MigrationPreparationOutcome { Inactive,Preparing,Retiring,Published,Retryable,Terminal,Refused };
  // A request is created only by this Session's actual Rust driver. A Retire
  // request reserves local publication while the executor retires genuine
  // owners outside the Session lock. It certifies no native resource or Ready.
  class MigrationPreparationRequest {
  public:
    MigrationPreparationKind kind()const noexcept{return kind_;}
    const AuthorityTransferContext &context()const noexcept{return context_;}
    unsigned self()const noexcept{return self_;}
    std::uint64_t id()const noexcept{return id_;}
  private:
    friend class OnlineMatchSession;
    MigrationPreparationRequest(MigrationPreparationKind kind,AuthorityTransferContext context,
        unsigned self,std::uint64_t id,std::weak_ptr<const unsigned char> owner)
        :kind_(kind),context_(std::move(context)),self_(self),id_(id),owner_(std::move(owner)){}
    MigrationPreparationKind kind_;
    AuthorityTransferContext context_;
    unsigned self_;
    std::uint64_t id_;
    std::weak_ptr<const unsigned char> owner_;
  };
  struct MigrationPreparationStep {
    MigrationPreparationOutcome outcome;
    std::optional<MigrationPreparationRequest> request;
  };
  OnlineMatchSession(std::string origin, std::string room, std::string bearer,
                     unsigned self, std::vector<OnlineAdmission>, bool loopback = false,std::optional<bool> team_mode = std::nullopt);
  OnlineMatchSession(std::string origin, std::string room, std::string bearer,
                     unsigned self, std::vector<OnlineAdmission>, bool loopback,
                     std::optional<bool> team_mode,bool complete_world_continuity);
  OnlineMatchSession(std::string origin, std::string room, std::string bearer,
                     unsigned self, std::vector<OnlineAdmission>, bool loopback,
                     std::optional<bool> team_mode,bool complete_world_continuity,
                     OnlineMatchTransportOptions);
  ~OnlineMatchSession();
  Phase phase() const;
  unsigned self() const;
  unsigned host() const;
  std::optional<bool> team_mode() const;
  std::vector<unsigned> local_slots() const;
  const std::vector<OnlineAdmission> &roster() const;
  std::string error() const;
  bool direct_udp() const;
  // Saturating diagnostic for exceptions rejected by the direct dispatch
  // loop. Protocol owners which consume/reject internally keep their own
  // counters. This number grants no admission, readiness or peer authority.
  std::uint64_t rejected_direct_dispatches() const noexcept;
  bool snapshot_candidates_enabled()const;
  std::shared_ptr<PeerSnapshotCandidate> snapshot_candidates()const;
  bool master_connected() const;
  std::optional<OnlineContinuityState> continuity() const;
  std::vector<OnlineWorldSeal> world_seals();
  std::optional<AuthorityTransferContext> authority_context() const;
  bool authority_supported(unsigned physical_peer) const;
  bool authority_replay_supported(unsigned physical_peer) const;
  bool authority_pipeline_supported(unsigned physical_peer) const;
  bool authority_host_continuation_supported(unsigned physical_peer) const;
  std::optional<std::uint64_t> loaded_fingerprint()const;
  bool authority_delivered(unsigned physical_peer,const AuthorityTransferContext &,const AuthorityCutSeal &)const;
  // Fresh inter-player measurements, never a promotion certificate. The
  // caller must separately retain a validated native continuation and agree
  // the authority generation before using a replacement host.
  std::vector<PeerHostQuality> host_candidates()const;
  // The native owner freezes/retains the exact paired world/host before ready.
  // Agreement does not validate native state or finish a role change itself.
  std::optional<PeerHostAgreementState> peer_migration()const;
  // One lock and one actual HostAgreement time observation drive Rust policy.
  // Prepare completion binds the genuine retained inventory and rechecks
  // suspicion/provenance under the publication lock. Retire completion is
  // permitted only after actual outer borrowers and native owners are gone;
  // fresh fencing can make it terminal rather than authorizing a retry.
  MigrationPreparationStep migration_preparation_request();
  MigrationPreparationOutcome complete_migration_preparation(const MigrationPreparationRequest &,
      const std::optional<PeerSurvivorInventory> & = {});
  bool prepare_peer_migration(const ValidatedAuthorityCut &);
  // The executor must ALREADY hold real frozen native/foreground leases for
  // every reported cut, and genuine irreversible Common phase witnesses.
  // Session fencing does not freeze those owners or prepare renderer, camera,
  // input or audio. Retain leases until the generation ends, including failure.
  // Incoming controls cannot publish this endpoint's inventory or native Ready.
  bool freeze_survivor_selection(const AuthorityTransferContext &,const PeerSurvivorInventory &);
  std::optional<PeerSurvivorSelectionState> survivor_selection()const;
  bool invalidate_survivor_selection(const AuthorityTransferContext &,std::optional<AuthorityCutSeal> = {});
  bool propose_peer_migration(unsigned physical_peer);
  // Choose a natively ready PC from fresh reciprocal survivor measurements.
  // Only the agreed predecessor host is excluded. A sole remaining PC needs
  // its own retained cut; no fabricated bandwidth/NAT value is substituted.
  std::optional<unsigned> migration_candidate()const;
  // The direct-session worker drives measured proposals and paced retries
  // automatically once native owners publish retained readiness. Explicit
  // proposals remain available for diagnostics; they are not required by the
  // ordinary gated path and must not be called once per render frame.
  // Same measured choice and proposal under one session-generation lock.
  // Preference cannot override an earlier accepted Paxos host/cut value.
  bool propose_peer_migration();
  // Call after applying this exact decision to the native world/peer role.
  // Gameplay and voice stay fenced until then. Certificates supplied without
  // this endpoint's independently learned decision cannot unlock the channel.
  bool finish_peer_migration(const PeerMigrationDecision &);
  // Runs on this caller's executor after decision/admission/audio validation
  // and all successor allocations, before publishing the new transport epoch.
  // The callback must not call this session. False must leave the native owner
  // unchanged; a successful native commit must not allocate or throw.
  using NativeCommit=bool(*)(void *)noexcept;
  bool finish_peer_migration(const PeerMigrationDecision &,NativeCommit,void *);
  // Current host's native allocations, exchanged independently of world cuts
  // over direct encrypted peers and bound to the accepted couch roster.
  bool publish_source_players(const AuthorityTransferContext &,std::span<const SourcePlayerAdmission>);
  std::optional<AcceptedSourcePlayers> source_players()const;
  bool source_players_confirmed(unsigned physical_peer)const;
  AuthorityCutSeal publish_authority(const AuthorityTransferContext &,std::uint64_t tick,
      std::vector<std::byte>,std::span<const unsigned> physical_targets);
  AuthorityCutSeal publish_authority_ahead(const AuthorityTransferContext &,const AuthorityCutSeal &previous,
      std::uint64_t tick,std::vector<std::byte>,unsigned physical_target);
  std::optional<ReceivedAuthorityCut> received_authority();
  bool acknowledge_authority(const AuthorityTransferContext &,const AuthorityCutSeal &);
  bool reject_authority(const AuthorityTransferContext &,const AuthorityCutSeal &);
  std::vector<ValidatedAuthorityCut> validated_authority();
  // Negotiated metadata-only association barrier for the full physical group.
  // Absent until every physical participant advertises common-cut support.
  // Pins, local recovery-pin publication/retirement and full owner readiness
  // remain the caller's responsibility; no operation enables migration.
  std::optional<PeerCommonCutState> common_cut()const;
  bool offer_common_cut(const PeerCommonCut &);
  bool confirm_common_cut_native(const PeerCommonCut &);
  bool associate_common_cut(const PeerCommonCut &);
  bool apply_common_cut(const PeerCommonCut &);
  bool release_common_cut(const PeerCommonCut &);
  bool reject_common_cut(const PeerCommonCut &);
  bool abort_common_cut(const PeerCommonCut &);
  bool acknowledge_common_cut_abort(const PeerCommonCut &);
  // These controls certify the complete authoritative continuation owner,
  // never MultiplayerState. The caller must capture/validate that owner before
  // sealing, acknowledging, accepting promotion or rebinding a peer.
  void seal_world(std::uint64_t completed_tick,const std::string &digest);
  void acknowledge_world(const OnlineWorldSeal &);
  void accept_migration(std::uint64_t completed_tick,const std::string &digest);
  void rebind_migration(std::uint64_t completed_tick,const std::string &digest);
  void loaded(std::uint64_t fingerprint,const UiVariant&);
  void send(std::span<const std::byte>, unsigned peer) override;
  bool receive(std::vector<std::byte> &, unsigned &peer) override;
  void voice(unsigned target, const GameVoiceFrame &);
  void voice_preferences(unsigned target,bool enabled,std::uint16_t muted);
  std::vector<MatchVoicePacket> voice_packets();
  std::uint16_t voice_blocks() const;
  // One microphone/codec owner survives world teardown through PCR Done.
  void retain_voice(std::shared_ptr<GameVoice>);
  std::shared_ptr<GameVoice> retained_voice() const;
  void update_postgame_voice(bool focused);
  bool postgame_voice_enabled(unsigned) const;
  void close_postgame();
  void complete(std::span<const std::byte> consumed_report);
  // Call only after validating the report against the final native snapshot.
  void confirm_result();
  std::vector<std::byte> postgame() const;
private:
  friend class MultiplayerRuntimeAuthority;
  bool migration_preparation_retiring()const;
  void pump_postgame_voice();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
