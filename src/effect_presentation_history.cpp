#include "halo2/effect_presentation_history.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/multiplayer_network.hpp"
#include <algorithm>
#include <bit>

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
    own();const auto admitted=origin.context();
    if(!admitted || admitted!=state_.context)throw FormatError("Effect presentation history requires its independently admitted context");
    if(frontier && (!replica_ || !frontier->replica.valid() ||
        !replica_->owns_replica_checkpoint(frontier->replica) ||
        !frontier->replica.matches(origin) || frontier->replica.tick()!=origin.tick()))
        throw FormatError("Runtime frontier lacks its exact completed replica");
    if(frontier && frontier->pose.valid() && !frontier->pose.matches(origin))
        throw FormatError("Runtime pose history has a different authentic receipt");
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
        if(!runtime_previous_ || !frontier->previous.same(*runtime_previous_))
            throw FormatError("Runtime frontier differs from its tracked completed-tick predecessor");
        auto owned=std::make_shared<MultiplayerRuntimeReceiptFrontier>(*frontier);
        owned->pending_effect_sounds=sounds_->checkpoint();runtime=std::move(owned);
    }
    Entry entry{origin,effects_.capture(origin),std::move(runtime)};
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
    ++state_.captured;
    while(entries_.size()>state_.capacity){entries_.pop_front();++state_.evicted;}
    state_.retained=entries_.size();state_.newest_tick=entries_.back().snapshot.tick();state_.newest_sequence=sequence;return true;
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
}
