#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2/source_player_admission.hpp"
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/peer_snapshot_candidate.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/multiplayer_snapshot_origin.hpp"
#include <memory>
#include <chrono>
namespace halo2 {
struct LanSessionDescription {
  std::uint64_t fingerprint{};
  UiVariant variant;
};
// Read the host's complete configuration before cold world/model preparation.
// This query does not admit a player, allocate a body or start a peer timeout.
LanSessionDescription query_lan_session(const std::string &address,
    std::chrono::milliseconds timeout = std::chrono::seconds(5));
LanSessionDescription query_lan_session(std::shared_ptr<MultiplayerDatagrams>,unsigned host,
    std::chrono::milliseconds timeout = std::chrono::seconds(5));
// Native port protocol. It does not speak the retail Xbox System Link protocol.
// Paths themselves are excluded: identical imported data may live in different
// directories on each computer. Every supported shared cache and font is part
// of the identity, together with the BSP and starting-weapon identity. Prepared
// MCC worlds supply their validated configured selector through
// network_starting_weapon_identity(), rather than a birth-dependent live tag.
// MCC v13 additionally identifies native TU1.5 gameplay and its actual resource
// banks/h2_fonts; it never reads or accepts an original executable as input.
std::uint64_t multiplayer_fingerprint(const std::filesystem::path &map,
                                     const std::filesystem::path &shared_maps,
                                     std::uint32_t bsp,
                                     std::uint32_t starting_weapon,
                                     const std::filesystem::path &gameplay_xbe = {});
class LanClient;
class LanHost {
public:
  // TU1.5 peer-table operations run on their native world's executor. The
  // configuration I/O worker only exchanges immutable configuration/packets.
  LanHost(MultiplayerMatch &, unsigned port, std::uint64_t fingerprint);
  LanHost(MultiplayerMatch &, unsigned port, std::uint64_t fingerprint,const UiVariant &);
  LanHost(MultiplayerMatch &, std::shared_ptr<MultiplayerDatagrams>, std::uint64_t,
          std::vector<OnlineAdmission>);
  LanHost(MultiplayerMatch &, std::shared_ptr<MultiplayerDatagrams>, std::uint64_t,
          std::vector<OnlineAdmission>,const UiVariant &);
  ~LanHost();
  unsigned port() const;
  void receive(std::array<Controls, multiplayer_capacity> &);
  void broadcast();
  // Negotiated preparation chooses an actual future native broadcast. The
  // opaque receipt is valid only while those exact per-peer stamps remain
  // current; it does not certify a native world or permit migration.
  class CandidateEmission {
  public:
    const PeerSnapshotToken &token()const noexcept;
    std::uint32_t tick()const noexcept;
  private:
    friend class LanHost;
    struct Data;std::shared_ptr<const Data> data_;
  };
  struct CandidateBroadcast {
    bool emitted{};
    std::optional<CandidateEmission> candidate;
  };
  CandidateBroadcast broadcast_candidate(std::shared_ptr<PeerSnapshotCandidate>,
      const PeerSnapshotToken &);
  bool candidate_current(const CandidateEmission &,const std::shared_ptr<PeerSnapshotCandidate> &)const noexcept;
  unsigned peers() const;
  enum class PeerRemovalReason { authenticated_leave, idle_timeout };
  // Passive current-host diagnostics only. No address, cookie, nonce or player
  // identity is retained. Times use steady_clock nanoseconds in this process;
  // input timestamps remain unknown after resume until a fresh input passes
  // the normal decoder/serial checks. This row is not a continuation or proof.
  struct PeerRemoval {
    PeerRemovalReason reason{};
    std::uint64_t ordinal{},input_acceptances{};
    std::uint32_t source_tick{},input_sequence{};
    std::uint16_t slot_mask{};
    unsigned local_players{};
    bool input_seen{};
    std::int64_t observed_at_ns{},removed_at_ns{},idle_age_ns{};
    std::optional<std::int64_t> last_accepted_input_at_ns,last_accepted_input_age_ns;
  };
  // Copy only on the host's existing native executor. At most one completed
  // removal is retained; reading it performs no native world operation.
  std::optional<PeerRemoval> last_peer_removal()const;

  class Continuation {
  public:
    Continuation()=default;
  private:
    friend class LanHost;
    friend class LanClient;
    struct Data;std::shared_ptr<const Data> data_;
  };
  // Owning input/connection/camera values at one completed TU1.5 world cut.
  // Only the authenticated logical-endpoint transport is supported. No socket,
  // pending transport packet, device, executor or borrowed world is retained.
  Continuation checkpoint(MultiplayerMatch::AuthorityDigest)const;
  static std::uint64_t continuation_allocation(const Continuation &);
  // Restore the complete world first, then pair it with this exact sidecar and
  // independently accepted full native roster. Recreates the same remote view
  // mapping without joining/respawning anyone. The caller must quiesce/revoke
  // the previous host and isolate the new transport generation before use.
  // This does not elect/promote a host, change membership/local ownership,
  // authenticate the cut, or import hardware/voice.
  static std::unique_ptr<LanHost> resume(MultiplayerMatch &,
      std::shared_ptr<MultiplayerDatagrams>,std::uint64_t,std::vector<OnlineAdmission>,
      const Continuation &,MultiplayerMatch::AuthorityDigest,
      std::span<const SourcePlayerAdmission> accepted_roster);
  struct ContinuationBinding {
    AuthorityTransferContext context;
    AuthorityCutSeal world;
    bool operator==(const ContinuationBinding &)const=default;
  };
  // Codec functions live in halo2_authority_codec. Authenticate the outer peer
  // and supply its independently agreed context, exact content seal and
  // rosters before decoding. The typed full-world/replay envelopes define
  // what that seal proves. These bytes are not an election certificate and
  // do not include sockets, pending transport packets, hardware or voice.
  static std::vector<std::byte> encode_continuation(const Continuation &,const ContinuationBinding &);
  static Continuation decode_continuation(std::span<const std::byte>,const ContinuationBinding &,
      std::uint64_t fingerprint,std::span<const OnlineAdmission>,std::span<const SourcePlayerAdmission>);

  // Owning former-remote view/input state for the newly local couch players.
  // The controller owner must adopt this bank before accepting fresh hardware
  // input. Capture does not include a physical controller or microphone.
  struct LocalContinuation {
    std::vector<unsigned> slots;
    std::array<Controls,4> controls{};
    std::array<std::array<std::uint32_t,10>,4> received_presses{},delivered_presses{};
    std::array<std::array<std::uint16_t,2>,4> completed{};
    std::array<std::optional<SourcePlayerActionTarget>,4> completed_vehicle{};
    std::optional<NativeVehicleCamera::Checkpoint> camera;
    std::optional<std::uint32_t> camera_tick;
    std::array<std::optional<NativeDeathCameraView>,4> vehicle_views,death_views;
    std::array<std::uint32_t,4> camera_jump_serial{};
  };
  struct HostLossResume {std::unique_ptr<LanHost> host;LocalContinuation local;};
  // Drain saved held controls and each pending pulse once on the native game
  // executor, before fresh hardware takes over the promoted local seats.
  static void receive_local_continuation(LocalContinuation &,
      std::array<Controls,multiplayer_capacity> &);
  // Invoke only after the session has independently certified this exact cut
  // and next authority context, and the complete world has been rebound to
  // next.host's views. The next context removes the failed old host. Preserves
  // surviving remote cookies, input serials, snapshot sequences and cameras;
  // returns the promoted peer's whole local input/camera bank. Does not depart
  // players, elect a host, activate transport or bind hardware/voice.
  static HostLossResume resume_after_host_loss(MultiplayerMatch &,
      std::shared_ptr<MultiplayerDatagrams>,std::uint64_t,std::vector<OnlineAdmission>,
      const Continuation &,const AuthorityTransferContext &previous,
      const AuthorityTransferContext &next,std::span<const SourcePlayerAdmission> accepted_roster);

private:
  friend struct SourceHostContinuationAccess;
  MultiplayerMatch &continuation_match()const;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
class LanClient {
public:
  LanClient(const std::string &address, std::uint64_t fingerprint,
            unsigned local_players, std::span<const std::int8_t> selected_teams = {},
            std::span<const SourcePlayerConfiguration> selected_profiles = {});
  LanClient(std::shared_ptr<MultiplayerDatagrams>, unsigned host, std::uint64_t fingerprint,
            std::span<const std::int8_t> selected_teams = {},
            std::span<const SourcePlayerConfiguration> selected_profiles = {});
  ~LanClient();
  void send(std::span<const Controls>);
  void receive();
  bool ready() const;
  const MultiplayerState &state() const;
  // Receipt of the most recently adopted state, retained across empty polls.
  // A real continuity bind/rebind clears it until another state is accepted;
  // pre-admission receipts do not own decoded values or prove an ONLINE cut.
  MultiplayerSnapshotOrigin state_origin() const;
  // Complete accepted snapshots from the most recent receive(), in delivery
  // order. Presentation owners must consume each real tick before rendering
  // the latest state. Empty receives clear this bank; no missing ticks are
  // synthesized and stale/duplicate packets never enter it.
  std::span<const MultiplayerState> received_states() const;
  // One opaque receipt per accepted state, in the same order. Pre-admission
  // receipts have no authority context and cannot prove an ONLINE cut.
  std::span<const MultiplayerSnapshotOrigin> received_origins() const;
  // The caller supplies the host sidecar from this exact independently
  // authenticated/natively validated paired cut. Validate the saved local
  // receipt against its connection, epoch and last sent body/sequence.
  // Works after host loss and later receives; never changes input or state.
  MultiplayerSnapshotProof prove_snapshot(const LanHost::Continuation &,
      const ValidatedAuthorityCut &,const MultiplayerSnapshotOrigin &)const;
  std::span<const unsigned> slots() const;
  std::string error() const;
  // Read-only diagnostics for ignored malformed/rejected datagrams. They do
  // not turn an unaccepted snapshot into state or a connection-level failure.
  std::uint64_t rejected_packet_count()const;
  std::string last_packet_rejection()const;
  std::size_t snapshot_buffered_bytes()const;
  // Bind once from independent session admission after native hello has
  // completed. A cut or an unauthenticated datagram cannot change this context.
  void bind_continuity(const AuthorityTransferContext &,unsigned physical_self,
      std::span<const OnlineAdmission>);
  // Before promotion retires this client, merge its latest owning input into
  // the independently validated predecessor bank. Validates context, endpoint,
  // fingerprint, cookies, couch slots/profiles and monotonic delivery serials.
  // Uses the ordinary wire decoder; never consumes a delivered press, changes
  // native world/camera state or accepts fresh hardware. Repeated calls are
  // idempotent. Any rejection leaves the supplied bank unchanged.
  void merge_local_input(const LanHost::Continuation &,const AuthorityTransferContext &,
      LanHost::LocalContinuation &)const;
  class PreparedRebind {
  public:
    ~PreparedRebind();
    PreparedRebind(const PreparedRebind &)=delete;
    PreparedRebind &operator=(const PreparedRebind &)=delete;
  private:
    friend class LanClient;
    struct Impl;std::unique_ptr<Impl> impl_;
    PreparedRebind();
  };
  // Preparation does not change the endpoint or receive baseline. This client
  // must outlive the prepared value. Input sends may continue without losing
  // held controls or pulse counters. Commit rejects another/stale native role
  // without consuming the caller's prepared value; success is allocation-free.
  std::unique_ptr<PreparedRebind> prepare_rebind_host(const AuthorityTransferContext &previous,
      const AuthorityTransferContext &next,const AuthorityCutSeal &)const;
  // Complete decoded-state counterpart: restore the exact historical receipt
  // independently proven against this connection's paired native cut. Merely
  // matching its tick cannot authorize rollback. Input/couch ownership stays
  // live, while the enclosing adopter restores the matching scene/effect bank.
  std::unique_ptr<PreparedRebind> prepare_rebind_host(const AuthorityTransferContext &previous,
      const AuthorityTransferContext &next,const AuthorityCutSeal &,const MultiplayerSnapshotProof &)const;
  bool can_commit_rebind_host(const PreparedRebind &)const noexcept;
  // Successful commit invalidates eligibility but leaves the caller's token
  // owning retired receive/state storage. Destroy it after locked publication.
  bool commit_rebind_host(std::unique_ptr<PreparedRebind> &&)noexcept;
  // Caller first certifies next/cut and isolates the old transport generation.
  // Keeps all outgoing input/held/pulse counters and accepted couch IDs. The
  // first complete new-host snapshot resets only the old receive baseline.
  void rebind_host(const AuthorityTransferContext &previous,const AuthorityTransferContext &next,
      const AuthorityCutSeal &accepted_cut);

private:
  friend class EffectPresentationHistory;
  struct SnapshotCandidate;
  std::shared_ptr<SnapshotCandidate> arm_snapshot_candidate(std::shared_ptr<PeerSnapshotCandidate>,const PeerSnapshotToken &);
  static bool snapshot_candidate_live(const std::shared_ptr<SnapshotCandidate> &)noexcept;
  static bool snapshot_candidate_matches(const std::shared_ptr<SnapshotCandidate> &,const MultiplayerSnapshotOrigin &)noexcept;
  static bool capture_snapshot_candidate(const std::shared_ptr<SnapshotCandidate> &)noexcept;
  static bool can_retire_snapshot_candidate(const std::shared_ptr<SnapshotCandidate> &)noexcept;
  static bool retire_snapshot_candidate(const std::shared_ptr<SnapshotCandidate> &)noexcept;
  static void reject_snapshot_candidate(const std::shared_ptr<SnapshotCandidate> &)noexcept;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
