#pragma once
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_delta.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/online_match_session.hpp"
#include <chrono>
#include <optional>
#include <string>

namespace halo2 {
struct SourceAuthorityPublisherState {
    bool encoding{},awaiting_validation{};
    std::uint64_t captures{},publications{},validations{};
    std::uint64_t capture_microseconds{},encoding_microseconds{},payload_bytes{},full_bytes{};
    bool delta{};
    std::optional<ValidatedAuthorityCut> ready;
    std::string error;
};
// One selected physical shadow recipient. The agreed QoS/shadow policy supplies
// that endpoint; this owner does not elect a host or change session membership.
// Sole consumer of this session's validated_authority() receipt queue.
// All public calls belong to the constructing source executor thread. Only an
// owning captured Checkpoint and immutable canonical base enter the worker.
class SourceAuthorityPublisher {
public:
    SourceAuthorityPublisher(SourceMatchAuthority &,std::shared_ptr<OnlineMatchSession>,unsigned physical_target,
        std::chrono::milliseconds validation_timeout=std::chrono::minutes(5));
    ~SourceAuthorityPublisher();
    SourceAuthorityPublisher(const SourceAuthorityPublisher &)=delete;
    SourceAuthorityPublisher &operator=(const SourceAuthorityPublisher &)=delete;
    // Nonblocking except for source capture itself, which needs a quiescent
    // native boundary. Refuses overlap; unsupported capture is a diagnostic,
    // not a prepared shadow or permission to advance its delta base.
    bool capture();
    // Publish completed worker output and consume matching executor receipts.
    void update();
    // Discard queued/stale encoding, pending validation tracking and ready/base
    // state. Already queued peer datagrams retain their transfer lifetime.
    // Does not stop a source world or revoke its authority generation.
    void cancel();
    SourceAuthorityPublisherState state()const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
