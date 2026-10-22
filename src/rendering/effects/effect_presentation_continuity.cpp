#include "halo2/effect_presentation_continuity.hpp"
#include <exception>
#include <limits>

namespace halo2 {
namespace {
bool same(const ValidatedAuthorityCut &a,const ValidatedAuthorityCut &b)noexcept {
    return a.peer==b.peer && a.context==b.context && a.seal==b.seal;
}
}
struct EffectPresentationContinuity::SnapshotData {
    std::shared_ptr<const int> owner;
    AuthorityTransferContext context;
    std::uint64_t tick{};
    NativeMuzzleEffects::Checkpoint muzzle;
    NativeWorldEffects::Checkpoint world;
    SourceMapEffects::Checkpoint map;
    SourceCombatAttachments::Checkpoint combat;
    std::array<std::uint32_t,2> random;
    std::optional<MultiplayerSnapshotOrigin> origin;
};
struct EffectPresentationContinuity::Checkpoint::Data {
    ValidatedAuthorityCut cut;
    std::shared_ptr<const SnapshotData> snapshot;
};
struct EffectPresentationContinuity::PreparedData {
    std::shared_ptr<const int> owner;
    ValidatedAuthorityCut cut;
    NativeMuzzleEffects::PreparedRestore muzzle;
    NativeWorldEffects::PreparedRestore world;
    SourceMapEffects::PreparedRestore map;
    SourceCombatAttachments::PreparedRestore combat;
    std::array<std::uint32_t,2> random;
    std::shared_ptr<EffectAudioAuxiliary::Target> audio_target;
    // Keep the displaced publication record beyond every aggregate hold.
    std::shared_ptr<EffectAudioAuxiliary::Target> retired_audio_target;
    PreparedData(std::shared_ptr<const int> o,const ValidatedAuthorityCut&c,
        NativeMuzzleEffects::PreparedRestore m,NativeWorldEffects::PreparedRestore w,
        SourceMapEffects::PreparedRestore a,SourceCombatAttachments::PreparedRestore b,
        std::array<std::uint32_t,2> r,std::shared_ptr<EffectAudioAuxiliary::Target> target)
        :owner(std::move(o)),cut(c),muzzle(std::move(m)),world(std::move(w)),
         map(std::move(a)),combat(std::move(b)),random(r),audio_target(std::move(target)){}
    ~PreparedData(){
        if(audio_target){
            auto expected=EffectAudioAuxiliary::Phase::Prepared;
            audio_target->phase.compare_exchange_strong(expected,
                EffectAudioAuxiliary::Phase::Cancelled,std::memory_order_acq_rel);
        }
    }
};
EffectPresentationContinuity::EffectPresentationContinuity(NativeMuzzleEffects &m,NativeWorldEffects &w,
    SourceMapEffects &map,SourceCombatAttachments &combat,SourceEffectRandom random)
    :muzzle_(m),world_(w),map_(map),combat_(combat),random_(random),
     audio_auxiliary_owner_(std::make_shared<EffectAudioAuxiliary::Owner>(executor_)){}
EffectPresentationContinuity::~EffectPresentationContinuity(){
    if(std::this_thread::get_id()!=executor_)std::terminate();
    audio_auxiliary_owner_->alive.store(false,std::memory_order_release);
}
void EffectPresentationContinuity::own()const {
    if(std::this_thread::get_id()!=executor_)throw FormatError("Effect presentation cut crossed its executor");
}
const ValidatedAuthorityCut &EffectPresentationContinuity::Checkpoint::cut()const {
    if(!data_)throw FormatError("Missing grouped effect presentation checkpoint");return data_->cut;
}
std::optional<std::uint64_t> EffectPresentationContinuity::Snapshot::tick()const noexcept {
    return data_?std::optional<std::uint64_t>(data_->tick):std::nullopt;
}
EffectPresentationContinuity::Snapshot EffectPresentationContinuity::capture(const AuthorityTransferContext &context)const {
    return capture_context(context,{});
}
EffectPresentationContinuity::Snapshot EffectPresentationContinuity::capture(const MultiplayerSnapshotOrigin &origin)const {
    const auto context=origin.context();if(!context)throw FormatError("Presentation snapshot has no admitted authority receipt");
    return capture_context(*context,origin);
}
EffectPresentationContinuity::Snapshot EffectPresentationContinuity::capture_context(const AuthorityTransferContext &context,
    std::optional<MultiplayerSnapshotOrigin> origin)const {
    own();auto muzzle=muzzle_.checkpoint();auto world=world_.checkpoint();
    auto map=map_.checkpoint();auto combat=combat_.checkpoint();
    const auto tick=world.tick();const std::array random{random_.primary,random_.auxiliary};
    if(!tick || muzzle.tick()!=tick || (map.tick() && map.tick()!=tick) ||
        (combat.tick() && combat.tick()!=tick) || (origin && origin->tick()!=tick) ||
        muzzle.random()!=random || world.random()!=random)
        throw FormatError("Effect presentation owners do not share the accepted cut");
    Snapshot result;result.data_=std::make_shared<SnapshotData>(SnapshotData{
        owner_,context,*tick,std::move(muzzle),std::move(world),std::move(map),std::move(combat),random,std::move(origin)});return result;
}
EffectPresentationContinuity::Checkpoint EffectPresentationContinuity::bind(const Snapshot &snapshot,const ValidatedAuthorityCut &cut)const {
    own();if(snapshot.data_ && snapshot.data_->origin)throw FormatError("Received presentation snapshot requires its native body/sequence proof");
    return bind_cut(snapshot,cut);
}
EffectPresentationContinuity::Checkpoint EffectPresentationContinuity::bind(const Snapshot &snapshot,const MultiplayerSnapshotProof &proof)const {
    own();if(!snapshot.data_ || !snapshot.data_->origin || !proof.matches(*snapshot.data_->origin))
        throw FormatError("Presentation snapshot has a different accepted body receipt");
    return bind_cut(snapshot,proof.cut());
}
EffectPresentationContinuity::Checkpoint EffectPresentationContinuity::bind_cut(const Snapshot &snapshot,const ValidatedAuthorityCut &cut)const {
    own();if(!snapshot.data_ || snapshot.data_->owner!=owner_ || snapshot.data_->context!=cut.context || snapshot.data_->tick!=cut.seal.tick)
        throw FormatError("Presentation snapshot does not match the validated room/generation/tick");
    Checkpoint result;result.data_=std::make_shared<Checkpoint::Data>(Checkpoint::Data{cut,snapshot.data_});return result;
}
EffectPresentationContinuity::Checkpoint EffectPresentationContinuity::checkpoint(const ValidatedAuthorityCut &cut)const {
    return bind(capture(cut.context),cut);
}
EffectPresentationContinuity::PreparedRestore::PreparedRestore()=default;
EffectPresentationContinuity::PreparedRestore::~PreparedRestore()=default;
EffectPresentationContinuity::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
EffectPresentationContinuity::PreparedRestore &EffectPresentationContinuity::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
EffectPresentationContinuity::PreparedRestore EffectPresentationContinuity::prepare_restore(const Checkpoint &cut,
    const ValidatedAuthorityCut &expected,ParticleCollisionQuery muzzle_collision,ParticleCollisionQuery world_collision)const {
    own();if(!cut.data_ || cut.data_->snapshot->owner!=owner_ || !same(cut.data_->cut,expected))
        throw FormatError("Grouped effect presentation checkpoint has the wrong owner or native cut");
    if(audio_auxiliary_owner_->generation==std::numeric_limits<std::uint64_t>::max())
        throw FormatError("Effects auxiliary publication generation exhausted");
    const auto &s=*cut.data_->snapshot;
    auto muzzle=muzzle_.prepare_restore(s.muzzle,random_,std::move(muzzle_collision));
    auto world=world_.prepare_restore(s.world,random_,std::move(world_collision));
    auto map=map_.prepare_restore(s.map);auto combat=combat_.prepare_restore(s.combat);
    PreparedRestore result;result.data_=std::make_unique<PreparedData>(owner_,expected,
        std::move(muzzle),std::move(world),std::move(map),std::move(combat),s.random,
        std::make_shared<EffectAudioAuxiliary::Target>(audio_auxiliary_owner_,
            &random_.auxiliary,s.random[1],expected));return result;
}
bool EffectPresentationContinuity::restore_ready(const PreparedRestore &prepared,const ValidatedAuthorityCut &expected)const noexcept {
    const auto *p=prepared.data_.get();
    return std::this_thread::get_id()==executor_ && p && p->owner==owner_ && same(p->cut,expected) &&
        p->audio_target && p->audio_target->owner.lock()==audio_auxiliary_owner_ &&
        p->audio_target->phase.load(std::memory_order_acquire)==EffectAudioAuxiliary::Phase::Prepared &&
        p->audio_target->generation==audio_auxiliary_owner_->generation &&
        audio_auxiliary_owner_->generation!=std::numeric_limits<std::uint64_t>::max() &&
        muzzle_.restore_ready(p->muzzle) && world_.restore_ready(p->world) &&
        map_.restore_ready(p->map) && combat_.restore_ready(p->combat);
}
EffectAudioAuxiliary EffectPresentationContinuity::audio_auxiliary(
    const PreparedRestore &prepared,const ValidatedAuthorityCut &expected)const {
    own();if(!restore_ready(prepared,expected))
        throw FormatError("Effects auxiliary agreement requires the exact ready four-owner target");
    return EffectAudioAuxiliary(prepared.data_->audio_target);
}
bool EffectPresentationContinuity::commit_restore(PreparedRestore &&prepared,const ValidatedAuthorityCut &expected)noexcept {
    if(!restore_ready(prepared,expected))return false;
    auto &p=*prepared.data_;
    auto phase=EffectAudioAuxiliary::Phase::Prepared;
    if(!p.audio_target->phase.compare_exchange_strong(phase,
        EffectAudioAuxiliary::Phase::Committing,std::memory_order_acq_rel))return false;
    // Closed, unexposed tokens and executor quiescence make this preflight
    // stable: no allocation, callback or other owner operation can intervene.
    const bool world=world_.commit_restore(std::move(p.world));
    const bool muzzle=muzzle_.commit_restore(std::move(p.muzzle));
    const bool map=map_.commit_restore(std::move(p.map));
    const bool combat=combat_.commit_restore(std::move(p.combat));
    if(!(world && muzzle && map && combat))std::terminate();
    random_.primary=p.random[0];random_.auxiliary=p.random[1];
    p.retired_audio_target.swap(audio_auxiliary_owner_->current);
    audio_auxiliary_owner_->current=p.audio_target;
    ++audio_auxiliary_owner_->generation;
    p.audio_target->phase.store(EffectAudioAuxiliary::Phase::Committed,std::memory_order_release);
    p.owner.reset();return true;
}
}
