#pragma once
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_delta.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/source_replay_batch.hpp"
#include "halo2/source_host_continuation_codec.hpp"
#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace halo2 {
// Evidence that every named physical recipient independently validated these
// same bytes. This is historical native acknowledgement, not a promise that
// every receiver still owns this cut or has prepared foreground/audio owners.
struct SourceAuthorityPublisherGroupCut {
    AuthorityTransferContext context;
    AuthorityCutSeal seal;
    std::uint16_t recipients{};
    bool operator==(const SourceAuthorityPublisherGroupCut &)const=default;
};
struct SourceAuthorityPublisherState {
    bool encoding{},awaiting_validation{};
    std::uint64_t captures{},publications{},validations{};
    std::uint64_t capture_microseconds{},encoding_microseconds{},payload_bytes{},full_bytes{};
    bool delta{};
    std::optional<ValidatedAuthorityCut> ready;
    std::string error;
    std::uint64_t recorded_phases{},replay_batches{},acknowledged_phases{},queued_phases{},queued_bytes{};
    bool tracking_replay{},host_ready{};
    // Legacy ready is populated only for a singleton recipient. A collective
    // acknowledgement must never invent a peer ID for ValidatedAuthorityCut.
    std::optional<SourceAuthorityPublisherGroupCut> group_ready,pending_group;
    std::uint16_t pending_native_validated_peers{};
    std::uint64_t native_receipts{};
    // Opt-in recovery-pin barrier. Issuance holds this exact cut through
    // native validation and foreground pin agreement; journal loss cannot
    // silently replace it. Default publication behavior remains unchanged.
    bool common_cut_barrier{};
    std::optional<SourceAuthorityPublisherGroupCut> held_group;
    // Only a pending native acknowledgement deadline sets this flag. Losing
    // successor recording/journal/encoding never rejects the immutable held
    // cut or its still-pending native acknowledgements.
    bool validation_timed_out{};
};
// One recorder/encoder and authority chain for the supplied physical recipients.
// The caller supplies that fixed set; this owner neither elects a host nor drops
// slow recipients from membership. Multi-recipient publication waits for all
// native acknowledgements and never uses the single-peer ahead pipeline.
// Sole consumer of this session's validated_authority() receipt queue.
// All public calls belong to the constructing source executor thread. Only
// owning world/replay/host values and immutable bases enter the worker.
class SourceAuthorityPublisher {
public:
    SourceAuthorityPublisher(SourceMatchAuthority &,std::shared_ptr<OnlineMatchSession>,unsigned physical_target,
        std::chrono::milliseconds validation_timeout=std::chrono::minutes(5));
    SourceAuthorityPublisher(SourceMatchAuthority &,std::shared_ptr<OnlineMatchSession>,unsigned physical_target,
        SourceAuthorityStreamMode,std::chrono::milliseconds validation_timeout=std::chrono::minutes(5));
    // The exact same Match must own both arguments and outlive the publisher.
    // Negotiates paired full/replay cuts; host state is captured per frame.
    SourceAuthorityPublisher(SourceMatchAuthority &,LanHost &,std::shared_ptr<OnlineMatchSession>,unsigned physical_target,
        std::chrono::milliseconds validation_timeout=std::chrono::minutes(5));
    SourceAuthorityPublisher(SourceMatchAuthority &,LanHost &,std::shared_ptr<OnlineMatchSession>,
        std::span<const unsigned> physical_targets,
        std::chrono::milliseconds validation_timeout=std::chrono::minutes(5));
    ~SourceAuthorityPublisher();
    SourceAuthorityPublisher(const SourceAuthorityPublisher &)=delete;
    SourceAuthorityPublisher &operator=(const SourceAuthorityPublisher &)=delete;
    // Nonblocking except for source capture itself, which needs a quiescent
    // native boundary. Refuses overlap; unsupported capture is a diagnostic,
    // not a prepared shadow or permission to advance its delta base.
    bool capture();
    // Execute real host work once. Replay mode records every complete phase
    // after capture, including while the initial cut is encoding/in flight.
    // Recording failure drops this stream, never real host work. Exceptions
    // from the supplied native work itself propagate to its original caller.
    bool execute(const std::function<void()> &);
    class Frame {
    public:
        ~Frame();
        // Idempotent explicit end; destructor also ends a normal frame.
        // Exceptions from real host work still propagate to their caller.
        bool finish()noexcept;
        Frame(const Frame &)=delete;
        Frame &operator=(const Frame &)=delete;
    private:
        friend class SourceAuthorityPublisher;
        struct Impl;std::unique_ptr<Impl> impl_;
        explicit Frame(SourceAuthorityPublisher &);
    };
    // Optional executor-local recording. Failure rejects this stream and
    // returns null without preventing the actual frame from running once.
    // This publisher must outlive the frame; do not call its other methods
    // until finish/destruction. Early continue/break closes a normal frame.
    std::unique_ptr<Frame> record_frame();
    // Publish completed worker output and consume matching executor receipts.
    void update();
    // Discard queued/stale encoding, pending validation tracking and ready/base
    // state. Already queued peer datagrams retain their transfer lifetime.
    // Does not stop a source world or revoke its authority generation.
    void cancel();
    // Configure only while no stream/job/frame is active. The production
    // common-cut owner releases after all recipients retire the predecessor,
    // or aborts after all recipients retire the rejected pending pin.
    void set_common_cut_barrier(bool);
    bool release_common_cut(const SourceAuthorityPublisherGroupCut &);
    bool abort_common_cut(const SourceAuthorityPublisherGroupCut &);
    SourceAuthorityPublisherState state()const;
    // Bounded delivery/validation slots, not a count of promotable worlds.
    unsigned in_flight_cuts()const;
    // Only the exact last natively acknowledged cut, never the current live
    // host table. This is recovery data, not authority to promote that peer.
    LanHost::Continuation host_checkpoint(const ValidatedAuthorityCut &)const;
    // Exact paired host value for the last fully natively acknowledged group.
    // A failed successor retains this historical value but discards its stream
    // base. Receiver-owned worlds and foreground associations need their own
    // pinned common-cut barrier before this can authorize migration.
    LanHost::Continuation host_checkpoint(const SourceAuthorityPublisherGroupCut &)const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
