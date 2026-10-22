#include "halo2/multiplayer_receipt_continuity.hpp"
#include <exception>
#include <type_traits>
#include <utility>

namespace halo2 {
namespace {
bool same(const ValidatedAuthorityCut &a,const ValidatedAuthorityCut &b)noexcept {
    return a.peer==b.peer&&a.context==b.context&&a.seal==b.seal;
}
}
struct MultiplayerReceiptContinuity::PreparedData {
    std::shared_ptr<const int> owner;
    ValidatedAuthorityCut cut;
    MultiplayerRuntimeReceiptValues values;
    MultiplayerState current,previous;
    MultiplayerSnapshotOrigin current_origin;
    MultiplayerSnapshotProof proof;
    EffectPresentationContinuity::PreparedRestore effects;
    NativeEffectSounds::PreparedRestore sounds;
    MultiplayerPoseContinuity::PreparedRestore poses;
};
MultiplayerReceiptContinuity::MultiplayerReceiptContinuity(Targets targets,
    EffectPresentationContinuity &effects,NativeEffectSounds &sounds,MultiplayerPoseContinuity &poses)
    :targets_(targets),effects_(effects),sounds_(sounds),poses_(poses){}
MultiplayerReceiptContinuity::Prepared::Prepared()=default;
MultiplayerReceiptContinuity::Prepared::~Prepared()=default;
MultiplayerReceiptContinuity::Prepared::Prepared(Prepared &&)noexcept=default;
MultiplayerReceiptContinuity::Prepared &MultiplayerReceiptContinuity::Prepared::operator=(Prepared &&)noexcept=default;
MultiplayerReceiptContinuity::Prepared MultiplayerReceiptContinuity::prepare(
    const EffectPresentationHistory::RuntimeCheckpoint &checkpoint,
    ParticleCollisionQuery muzzle_collision,ParticleCollisionQuery world_collision)const {
    if(std::this_thread::get_id()!=executor_)
        throw FormatError("Receipt presentation preparation crossed its executor");
    const auto &cut=checkpoint.cut();const auto &frontier=checkpoint.frontier();
    if(!checkpoint.proof().matches(checkpoint.origin())||
        !frontier.replica.matches(checkpoint.origin()))
        throw FormatError("Receipt presentation preparation has a different authentic frontier");
    auto effects=effects_.prepare_restore(checkpoint.effects(),cut,
        std::move(muzzle_collision),std::move(world_collision));
    auto sounds=sounds_.prepare_restore(frontier.pending_effect_sounds);
    auto poses=poses_.prepare_restore(frontier.pose,checkpoint.proof());
    Prepared result;result.data_=std::make_unique<PreparedData>(PreparedData{
        owner_,cut,static_cast<const MultiplayerRuntimeReceiptValues &>(frontier),
        checkpoint.current(),checkpoint.previous(),checkpoint.origin(),checkpoint.proof(),
        std::move(effects),std::move(sounds),std::move(poses)});
    return result;
}
bool MultiplayerReceiptContinuity::ready(const Prepared &prepared,const ValidatedAuthorityCut &cut)const noexcept {
    const auto *p=prepared.data_.get();return std::this_thread::get_id()==executor_&&
        p&&p->owner==owner_&&same(p->cut,cut)&&effects_.restore_ready(p->effects,cut)&&
        sounds_.restore_ready(p->sounds)&&poses_.ready(p->poses,p->proof);
}
bool MultiplayerReceiptContinuity::commit(Prepared &&prepared,const ValidatedAuthorityCut &cut)noexcept {
    if(!ready(prepared,cut))return false;
    static_assert(std::is_nothrow_swappable_v<MultiplayerRuntimeReceiptValues>);
    static_assert(std::is_nothrow_swappable_v<MultiplayerState>);
    static_assert(std::is_nothrow_swappable_v<MultiplayerSnapshotOrigin>);
    auto &p=*prepared.data_;
    if(!effects_.commit_restore(std::move(p.effects),cut)||!sounds_.commit_restore(std::move(p.sounds))||
        !poses_.commit(p.poses))std::terminate();
    using std::swap;
    swap(targets_.values,p.values);swap(targets_.current,p.current);
    swap(targets_.previous,p.previous);swap(targets_.current_origin,p.current_origin);
    p.owner.reset();return true;
}
}
