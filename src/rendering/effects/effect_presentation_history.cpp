#include "halo2/effect_presentation_history.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/multiplayer_network.hpp"
#include <algorithm>
#include <bit>
#include <string>
#include <type_traits>

namespace halo2 {
struct EffectPresentationHistory::RuntimeReservation::Data {
    std::weak_ptr<const void> owner;
    std::thread::id executor;
    AuthorityTransferContext context;
    AuthorityCutSeal seal;
    std::vector<Entry> entries;
    bool active{true},overflow{};
    void own()const {
        if(executor!=std::this_thread::get_id())throw FormatError("Runtime reservation crossed its history executor");
    }
};
struct EffectPresentationHistory::RuntimeCandidate::Data {
    std::weak_ptr<const void> owner;
    std::thread::id executor;
    PeerSnapshotToken token;
    std::weak_ptr<PeerSnapshotCandidate> source;
    std::shared_ptr<LanClient::SnapshotCandidate> client;
    std::optional<Entry> entry;
    bool active{true},captured{},retired{},rejected{};
    void own()const {
        if(executor!=std::this_thread::get_id())throw FormatError("Runtime candidate crossed its history executor");
    }
};
bool EffectPresentationHistory::RuntimeCandidate::valid()const {
    if(!data_)return false;data_->own();
    return data_->active&&!data_->retired&&!data_->rejected&&!data_->owner.expired()&&
        LanClient::snapshot_candidate_live(data_->client);
}
bool EffectPresentationHistory::RuntimeCandidate::captured()const {
    return valid()&&data_->captured&&data_->entry.has_value();
}
std::size_t EffectPresentationHistory::RuntimeCandidate::retained()const {
    if(!data_)return 0;data_->own();return data_->entry.has_value()?1:0;
}
const PeerSnapshotToken &EffectPresentationHistory::RuntimeCandidate::token()const {
    if(!data_)throw FormatError("Missing provisional runtime candidate");data_->own();return data_->token;
}
bool EffectPresentationHistory::RuntimeReservation::valid()const {
    if(!data_)return false;data_->own();return data_->active&&!data_->owner.expired();
}
bool EffectPresentationHistory::RuntimeReservation::overflowed()const {
    if(!data_)return false;data_->own();return data_->overflow;
}
std::size_t EffectPresentationHistory::RuntimeReservation::retained()const {
    if(!data_)return 0;data_->own();return data_->entries.size();
}
EffectPresentationHistory::EffectPresentationHistory(EffectPresentationContinuity &effects,std::size_t capacity)
    :effects_(effects) {
    if(!capacity)throw FormatError("Effect presentation history requires a positive capacity");
    state_.capacity=capacity;
}
EffectPresentationHistory::EffectPresentationHistory(EffectPresentationContinuity &effects,NativeEffectSounds &sounds,std::size_t capacity)
    :EffectPresentationHistory(effects,capacity){sounds_=&sounds;}
EffectPresentationHistory::EffectPresentationHistory(EffectPresentationContinuity &effects,NativeEffectSounds &sounds,
    const Simulation &replica,std::size_t capacity)
    :EffectPresentationHistory(effects,sounds,capacity){replica_=&replica;}
void EffectPresentationHistory::own()const {
    if(std::this_thread::get_id()!=executor_)throw FormatError("Effect presentation history crossed its executor");
}
void EffectPresentationHistory::context(std::optional<AuthorityTransferContext> context) {
    own();if(context==state_.context)return;
    if(candidate_&&!candidate_->retired){
        // Abort may not have reached its closing state yet. Preserve the
        // checked-out delivery and full entry until explicit retirement wins.
        candidate_->active=false;candidate_->rejected=true;
        LanClient::reject_snapshot_candidate(candidate_->client);
    }
    for(const auto &weak:reservations_)if(const auto reservation=weak.lock()){
        reservation->active=false;reservation->entries.clear();
    }
    reservations_.clear();
    entries_.clear();state_.context=std::move(context);state_.retained=0;state_.newest_tick.reset();state_.newest_sequence.reset();
    runtime_current_.reset();runtime_previous_.reset();
}
void EffectPresentationHistory::begin_runtime(const LanClient &client) {
    own();if(runtime_current_)return;
    const auto origin=client.state_origin();
    if(!sounds_ || !state_.context || origin.context()!=state_.context ||
        origin.local_slots().empty() || state_.newest_sequence)
        throw FormatError("Runtime history lacks an initial adopted client receipt");
    (void)origin.accepted_state();runtime_current_=origin;runtime_previous_=origin;
}
bool EffectPresentationHistory::remember(const MultiplayerSnapshotOrigin &origin) {
    return remember_entry(origin,nullptr);
}
bool EffectPresentationHistory::remember(const MultiplayerSnapshotOrigin &origin,const MultiplayerRuntimeReceiptFrontier &frontier) {
    return remember_entry(origin,&frontier);
}
bool EffectPresentationHistory::remember_entry(const MultiplayerSnapshotOrigin &origin,const MultiplayerRuntimeReceiptFrontier *frontier) {
    own();const auto candidate=candidate_;
    const bool marked=candidate&&candidate->active&&!candidate->retired&&!candidate->rejected&&
        LanClient::snapshot_candidate_matches(candidate->client,origin);
    try {
    const auto admitted=origin.context();
    if(!admitted || admitted!=state_.context)throw FormatError("Effect presentation history requires its independently admitted context");
    if(marked&&!frontier)throw FormatError("Provisional runtime candidate requires its full completed frontier");
    if(frontier && (!replica_ || !frontier->replica.valid() ||
        !replica_->owns_replica_checkpoint(frontier->replica) ||
        !frontier->replica.matches(origin) || frontier->replica.tick()!=origin.tick()))
        throw FormatError("Runtime frontier lacks its exact completed replica");
    if(frontier && frontier->pose.valid() && !frontier->pose.matches(origin))
        throw FormatError("Runtime pose history has a different authentic receipt");
    if(frontier && frontier->scene.valid() && !frontier->scene.matches(origin))
        throw FormatError("Runtime scene history has a different authentic receipt");
    if(frontier && frontier->hud.valid() && !frontier->hud.matches(origin))
        throw FormatError("Runtime HUD history has a different authentic receipt");
    if(frontier && frontier->locomotion.valid() && !frontier->locomotion.matches(origin))
        throw FormatError("Runtime locomotion history has a different authentic receipt");
    if(std::ranges::any_of(entries_,[&](const auto &entry){return entry.origin.same(origin);}))return false;
    const auto sequence=origin.sequence();
    // Same modulo32 ordering as the native accepted-snapshot transport.
    if(!sequence || (state_.newest_sequence && std::bit_cast<std::int32_t>(*sequence-*state_.newest_sequence)<=0))
        throw FormatError("Effect presentation history cannot recapture an old snapshot receipt");
    std::shared_ptr<const MultiplayerRuntimeReceiptFrontier> runtime;
    if(frontier){
        if(!sounds_ || frontier->pending_effect_sounds.valid() ||
            !frontier->client_state_bound || !frontier->joined ||
            !std::ranges::equal(frontier->local_slots,origin.local_slots()) ||
            frontier->local_slots.empty() || frontier->local_slots.size()>4 ||
            frontier->world_effect_tick!=origin.tick() || frontier->muzzle_pose_tick!=origin.tick())
            throw FormatError("Runtime frontier does not share its admitted receipt/owners");
        const auto &current=origin.accepted_state();
        // Valid receipt observation progresses even when a later capture fails;
        // retained entries/counters change only after all copies succeed.
        if(!runtime_current_){runtime_current_=origin;runtime_previous_.reset();}
        else if(!runtime_current_->same(origin)){
            if(!runtime_current_->same_connection(origin) ||
                std::bit_cast<std::int32_t>(*origin.sequence()-*runtime_current_->sequence())<=0 ||
                std::bit_cast<std::int32_t>(current.tick-runtime_current_->accepted_state().tick)<0)
                throw FormatError("Runtime history observed a retired or reordered receipt");
            if(current.tick!=runtime_current_->accepted_state().tick)runtime_previous_=runtime_current_;
            runtime_current_=origin;
        }
        if(!runtime_previous_ || !frontier->previous.same(*runtime_previous_)){
            const auto number=[](const auto value){return value?std::to_string(*value):std::string("none");};
            const auto receipt=[&](const MultiplayerSnapshotOrigin &value){
                return std::string("{tick=")+number(value.tick())+",seq="+number(value.sequence())+"}";
            };
            const auto tracked=[&](const std::optional<MultiplayerSnapshotOrigin> &value){
                return value?receipt(*value):std::string("none");
            };
            throw FormatError(std::string("Runtime frontier differs from its tracked completed-tick predecessor")+
                "; origin="+receipt(origin)+" frontier.previous="+receipt(frontier->previous)+
                " tracked.previous="+tracked(runtime_previous_)+" tracked.current="+tracked(runtime_current_)+
                " same_connection(previous,origin)="+std::to_string(frontier->previous.same_connection(origin))+
                " same_connection(tracked_previous,origin)="+
                    std::to_string(runtime_previous_&&runtime_previous_->same_connection(origin))+
                " same_connection(previous,tracked_previous)="+
                    std::to_string(runtime_previous_&&frontier->previous.same_connection(*runtime_previous_)));
        }
        auto owned=std::make_shared<MultiplayerRuntimeReceiptFrontier>(*frontier);
        owned->pending_effect_sounds=sounds_->checkpoint();runtime=std::move(owned);
    }
    Entry entry{origin,effects_.capture(origin),std::move(runtime)};
    std::optional<Entry> provisional;
    if(marked){
        if(candidate->entry)throw FormatError("Provisional runtime candidate already owns a different completed receipt");
        static_assert(std::is_nothrow_copy_constructible_v<Entry>);
        provisional.emplace(entry);
    }
    // Allocate all reservation copies before publishing any insertion. A
    // failed copy leaves the retained entries/counters and every pin intact.
    struct Pending {std::shared_ptr<RuntimeReservation::Data> data;std::vector<Entry> entries;bool overflow;};
    std::vector<Pending> pending;
    for(const auto &weak:reservations_)if(const auto data=weak.lock();data&&data->active&&
        data->context==*admitted&&data->seal.tick==origin.tick()&&entry.runtime){
        auto copies=data->entries;const bool overflow=copies.size()>=state_.capacity;
        if(!overflow)copies.push_back(entry);
        pending.push_back({data,std::move(copies),data->overflow||overflow});
    }
    entries_.push_back(std::move(entry));
    for(auto &change:pending){change.data->entries.swap(change.entries);change.data->overflow=change.overflow;}
    if(marked){
        static_assert(std::is_nothrow_swappable_v<std::optional<Entry>>);
        candidate->entry.swap(provisional);
    }
    ++state_.captured;
    while(entries_.size()>state_.capacity){entries_.pop_front();++state_.evicted;}
    state_.retained=entries_.size();state_.newest_tick=entries_.back().snapshot.tick();state_.newest_sequence=sequence;
    if(marked){
        // The exact full Entry is published before delivery capture can lift
        // the ordinary-adoption fence. A concurrent close may refuse capture;
        // preserve both owners for Abort retirement without faking its ACK.
        candidate->captured=LanClient::capture_snapshot_candidate(candidate->client);
        if(!candidate->captured){candidate->rejected=true;LanClient::reject_snapshot_candidate(candidate->client);}
    }
    return true;
    }catch(...){
        if(marked){candidate->rejected=true;LanClient::reject_snapshot_candidate(candidate->client);}
        throw;
    }
}
std::optional<EffectPresentationContinuity::Checkpoint> EffectPresentationHistory::bind(
    const SourceAuthorityHostProof &proof,const LanClient &client)const {
    own();const auto &cut=proof.cut();if(state_.context!=cut.context)return {};
    for(auto at=entries_.rbegin();at!=entries_.rend();++at) {
        if(at->origin.tick()!=cut.seal.tick)continue;
        std::optional<MultiplayerSnapshotProof> exact;
        try{exact=proof.prove_snapshot(client,at->origin);}catch(const FormatError &){continue;}
        return effects_.bind(at->snapshot,*exact);
    }
    return {};
}
EffectPresentationHistoryState EffectPresentationHistory::state()const {own();return state_;}
struct EffectPresentationHistory::RuntimeCheckpoint::Data {
    EffectPresentationContinuity::Checkpoint effects;
    MultiplayerSnapshotOrigin origin;
    MultiplayerSnapshotProof proof;
    std::shared_ptr<const MultiplayerRuntimeReceiptFrontier> runtime;
};
const EffectPresentationContinuity::Checkpoint &EffectPresentationHistory::RuntimeCheckpoint::effects()const {
    if(!data_)throw FormatError("Missing receipt-bound runtime checkpoint");return data_->effects;
}
const ValidatedAuthorityCut &EffectPresentationHistory::RuntimeCheckpoint::cut()const {return effects().cut();}
const MultiplayerSnapshotOrigin &EffectPresentationHistory::RuntimeCheckpoint::origin()const {
    if(!data_)throw FormatError("Missing receipt-bound runtime checkpoint");return data_->origin;
}
const MultiplayerSnapshotProof &EffectPresentationHistory::RuntimeCheckpoint::proof()const {
    if(!data_)throw FormatError("Missing receipt-bound runtime checkpoint");return data_->proof;
}
const MultiplayerRuntimeReceiptFrontier &EffectPresentationHistory::RuntimeCheckpoint::frontier()const {
    if(!data_)throw FormatError("Missing receipt-bound runtime checkpoint");return *data_->runtime;
}
const MultiplayerState &EffectPresentationHistory::RuntimeCheckpoint::current()const {return origin().accepted_state();}
const MultiplayerState &EffectPresentationHistory::RuntimeCheckpoint::previous()const {return frontier().previous.accepted_state();}
std::optional<EffectPresentationHistory::RuntimeCheckpoint> EffectPresentationHistory::bind_runtime(
    const SourceAuthorityHostProof &proof,const LanClient &client)const {
    own();const auto &cut=proof.cut();if(state_.context!=cut.context)return {};
    for(auto at=entries_.rbegin();at!=entries_.rend();++at){
        if(at->origin.tick()!=cut.seal.tick || !at->runtime)continue;
        std::optional<MultiplayerSnapshotProof> exact;
        try{exact=proof.prove_snapshot(client,at->origin);}catch(const FormatError &){continue;}
        RuntimeCheckpoint result;
        result.data_=std::make_shared<RuntimeCheckpoint::Data>(RuntimeCheckpoint::Data{
            effects_.bind(at->snapshot,*exact),at->origin,*exact,at->runtime});
        return result;
    }
    return {};
}
EffectPresentationHistory::RuntimeReservation EffectPresentationHistory::reserve_runtime(
    const AuthorityTransferContext &context,const AuthorityCutSeal &seal) {
    own();
    if(!replica_||!sounds_||state_.context!=context)
        throw FormatError("Runtime reservation requires its independently admitted typed history");
    std::erase_if(reservations_,[](const auto &weak){const auto data=weak.lock();return !data||!data->active;});
    for(const auto &weak:reservations_)if(const auto data=weak.lock();data&&data->context==context&&data->seal==seal){
        RuntimeReservation result;result.data_=data;return result;
    }
    if(reservations_.size()>=2)throw FormatError("Runtime history already owns two cut reservations");
    auto data=std::make_shared<RuntimeReservation::Data>();
    data->owner=reservation_owner_;data->executor=executor_;data->context=context;data->seal=seal;
    for(const auto &entry:entries_)if(entry.runtime&&entry.origin.tick()==seal.tick)data->entries.push_back(entry);
    reservations_.push_back(data);RuntimeReservation result;result.data_=std::move(data);return result;
}
std::optional<EffectPresentationHistory::RuntimeCheckpoint> EffectPresentationHistory::bind_runtime(
    const SourceAuthorityHostProof &proof,const LanClient &client,const RuntimeReservation &reservation)const {
    own();const auto &cut=proof.cut();const auto &data=reservation.data_;
    if(!data||!data->active||data->owner.lock()!=reservation_owner_||state_.context!=cut.context||
        data->context!=cut.context||data->seal!=cut.seal)return {};
    for(auto at=data->entries.rbegin();at!=data->entries.rend();++at){
        std::optional<MultiplayerSnapshotProof> exact;
        try{exact=proof.prove_snapshot(client,at->origin);}catch(const FormatError &){continue;}
        RuntimeCheckpoint result;
        result.data_=std::make_shared<RuntimeCheckpoint::Data>(RuntimeCheckpoint::Data{
            effects_.bind(at->snapshot,*exact),at->origin,*exact,at->runtime});
        return result;
    }
    return {};
}
EffectPresentationHistory::RuntimeCandidate EffectPresentationHistory::arm_runtime(LanClient &client,
    std::shared_ptr<PeerSnapshotCandidate> source,const PeerSnapshotToken &token) {
    own();
    if(!replica_||!sounds_||!state_.context||state_.context!=token.context||!source)
        throw FormatError("Provisional runtime candidate requires its independently admitted typed history");
    if(candidate_){
        if(candidate_->token==token&&candidate_->source.lock()==source&&candidate_->active&&
            !candidate_->retired&&!candidate_->rejected&&LanClient::snapshot_candidate_live(candidate_->client)){
            RuntimeCandidate result;result.data_=candidate_;return result;
        }
        throw FormatError("Runtime history still owns a provisional candidate awaiting retirement");
    }
    const auto states=client.received_states();const auto origins=client.received_origins();
    if(states.size()!=origins.size())throw FormatError("Provisional runtime candidate has an inconsistent accepted batch");
    const auto tail=origins.empty()?client.state_origin():origins.back();
    // Preserve the original short-circuit order and reject exactly the same
    // state. Name the first failed predicate without exposing peer identities
    // or rereading the connection/history after the failed preflight.
    const char *incomplete=!client.ready()?"client-not-ready":
        tail.context()!=state_.context?"tail-context":
        tail.local_slots().empty()?"tail-local-slots":
        entries_.empty()?"history-empty":
        !entries_.back().runtime?"history-runtime-missing":
        !entries_.back().origin.same(tail)?"history-origin-not-tail":
        !runtime_current_?"runtime-current-missing":
        !runtime_current_->same(tail)?"runtime-current-not-tail":
        state_.newest_sequence!=tail.sequence()?"history-sequence-not-tail":nullptr;
    if(incomplete)
        throw FormatError(std::string("Provisional runtime candidate requires the full completed accepted batch tail; first_failed=")+incomplete);
    // Every history allocation precedes the client's fence/Armed transaction.
    // The private client adapter owns its live witness and delivery lease; no
    // raw client address or post-destruction callback enters this owner.
    auto data=std::make_shared<RuntimeCandidate::Data>();
    data->owner=reservation_owner_;data->executor=executor_;data->token=token;data->source=source;
    data->client=client.arm_snapshot_candidate(std::move(source),token);
    if(!data->client)throw FormatError("Client refused the provisional snapshot adoption fence");
    candidate_=data;RuntimeCandidate result;result.data_=std::move(data);return result;
}
bool EffectPresentationHistory::retire_runtime_candidate(const RuntimeCandidate &candidate) {
    own();const auto &data=candidate.data_;
    if(!data||data->owner.lock()!=reservation_owner_)return false;
    data->own();if(data->retired)return true;
    if(!LanClient::can_retire_snapshot_candidate(data->client))return false;
    // The immutable closing round admits exact retirement. Remove eligibility
    // and release actual provisional ownership before Retired can become
    // visible to the transport/host and admit a successor Arm. No session lock
    // is held while the historical foreground owners are destroyed.
    data->active=false;data->rejected=true;data->entry.reset();
    if(!LanClient::retire_snapshot_candidate(data->client))return false;
    data->retired=true;
    if(candidate_==data)candidate_.reset();return true;
}
std::optional<EffectPresentationHistory::RuntimeCheckpoint> EffectPresentationHistory::bind_runtime(
    const SourceAuthorityHostProof &proof,const LanClient &client,const RuntimeCandidate &candidate)const {
    own();const auto &data=candidate.data_;const auto &cut=proof.cut();
    if(!data||data->owner.lock()!=reservation_owner_||!data->active||data->retired||data->rejected||
        !data->captured||!data->entry||!data->entry->runtime||state_.context!=cut.context||
        data->token.context!=cut.context||data->entry->origin.tick()!=cut.seal.tick||
        !LanClient::snapshot_candidate_live(data->client)||
        !LanClient::snapshot_candidate_matches(data->client,data->entry->origin))return {};
    data->own();const auto &entry=*data->entry;
    std::optional<MultiplayerSnapshotProof> exact;
    try{exact=proof.prove_snapshot(client,entry.origin);}catch(const FormatError &){return {};}
    RuntimeCheckpoint result;
    result.data_=std::make_shared<RuntimeCheckpoint::Data>(RuntimeCheckpoint::Data{
        effects_.bind(entry.snapshot,*exact),entry.origin,*exact,entry.runtime});
    return result;
}
}
