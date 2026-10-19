#pragma once
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2/game_voice.hpp"
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/peer_source_admissions.hpp"
#include "halo2/peer_mesh_quality.hpp"
#include "halo2/peer_host_agreement.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace halo2 {
struct UiVariant;
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
};
class OnlineMatchSession final : public MultiplayerDatagrams {
public:
  enum class Phase { Connecting, Loading, Running, Reviewing, Postgame, Failed,
                     AwaitingWorld, Migrating, Rebinding };
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
  bool prepare_peer_migration(const ValidatedAuthorityCut &);
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
  void pump_postgame_voice();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
