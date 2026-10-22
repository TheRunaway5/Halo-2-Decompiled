#include "halo2/multiplayer_locomotion_continuity.hpp"
#include "halo2/native_locomotion.hpp"
#include <exception>
#include <limits>

namespace halo2 {
namespace {
using Group=MultiplayerLocomotionContinuity;
struct Metadata {
    Group::Flags dual;
    Group::Handles units,weapons;
    Group::Flags unavailable;
    explicit Metadata(const Group::Targets &t):dual(t.dual),units(t.units),weapons(t.weapons),unavailable(t.unavailable){}
    bool same(const Group::Targets &t)const noexcept{
        return dual==t.dual&&units==t.units&&weapons==t.weapons&&unavailable==t.unavailable;
    }
    void exchange(Group::Targets &t)noexcept{
        dual.swap(t.dual);units.swap(t.units);weapons.swap(t.weapons);unavailable.swap(t.unavailable);
    }
};
struct Payload {
    std::array<std::optional<NativeLocomotion::Checkpoint>,multiplayer_capacity> slots;
    Metadata metadata;
    explicit Payload(const Group::Targets &t):metadata(t){}
};
bool same_cut(const ValidatedAuthorityCut &a,const ValidatedAuthorityCut &b)noexcept{
    return a.peer==b.peer&&a.context==b.context&&a.seal==b.seal;
}
bool distinct(const Group::Targets &t)noexcept{
    if(&t.dual==&t.unavailable||&t.units==&t.weapons)return false;
    for(unsigned i=0;i<multiplayer_capacity;++i)if(t.gait[i])
        for(unsigned j=0;j<i;++j)if(t.gait[i].get()==t.gait[j].get())return false;
    return true;
}
}
struct MultiplayerLocomotionContinuity::Impl {
    Targets targets;
    const std::thread::id executor=std::this_thread::get_id();
    const std::shared_ptr<const void> owner=std::make_shared<const unsigned char>(0);
    std::uint64_t revision{};mutable bool busy{};
    explicit Impl(Targets t):targets(t){
        if(!distinct(t))throw FormatError("Locomotion group has aliased fixed slots or metadata arrays");
    }
    void own()const{
        if(std::this_thread::get_id()!=executor||busy||!distinct(targets))
            throw FormatError("Locomotion group lost its completed executor or independent fixed slots");
    }
    struct Call {
        const Impl &owner;
        explicit Call(const Impl &p):owner(p){owner.own();owner.busy=true;}
        ~Call(){owner.busy=false;}
        Call(const Call &)=delete;Call &operator=(const Call &)=delete;
    };
};
struct MultiplayerLocomotionContinuity::PreparedData {
    enum class Action {Absent,Replace,Clear};
    struct Slot {
        Action action=Action::Absent;
        const NativeLocomotion *expected{};
        std::optional<NativeLocomotion::PreparedReplacement> replacement;
        std::optional<NativeLocomotion::PreparedRestore> clear_guard;
        // A successful historical clear retires the actual old adapter here.
        // Its manager/clip/cache bank survives until the enclosing token dies.
        std::unique_ptr<NativeLocomotion> retired;
    };
    std::weak_ptr<const void> owner;
    std::thread::id executor;
    std::uint64_t revision{};
    const Slots *slots{};
    const Flags *dual{},*unavailable{};
    const Handles *units{},*weapons{};
    MultiplayerSnapshotOrigin origin;
    ValidatedAuthorityCut cut;
    Metadata expected,replacement;
    std::array<Slot,multiplayer_capacity> children;
    bool consumed{};
    PreparedData(const Impl &p,const MultiplayerSnapshotOrigin &o,const ValidatedAuthorityCut &c,const Metadata &target)
        :owner(p.owner),executor(p.executor),revision(p.revision),slots(&p.targets.gait),dual(&p.targets.dual),
        unavailable(&p.targets.unavailable),units(&p.targets.units),weapons(&p.targets.weapons),origin(o),cut(c),
        expected(p.targets),replacement(target){}
};
MultiplayerLocomotionContinuity::MultiplayerLocomotionContinuity(Targets t):impl_(std::make_unique<Impl>(t)){}
MultiplayerLocomotionContinuity::~MultiplayerLocomotionContinuity()=default;
void MultiplayerLocomotionContinuity::mutating(){
    Impl::Call call(*impl_);
    if(impl_->revision==std::numeric_limits<std::uint64_t>::max())throw FormatError("Locomotion group revision exhausted");
    ++impl_->revision;
}
bool MultiplayerLocomotionContinuity::owns_snapshot(const Snapshot &snapshot)const noexcept{
    const auto &p=*impl_;
    return std::this_thread::get_id()==p.executor&&!p.busy&&snapshot.data_&&
        snapshot.data_->owner.lock()==p.owner&&snapshot.data_->executor==p.executor;
}
MultiplayerLocomotionContinuity::Snapshot MultiplayerLocomotionContinuity::capture(const MultiplayerSnapshotOrigin &origin)const{
    const auto &p=*impl_;Impl::Call call(p);
    if(!origin.context()||origin.local_slots().empty()||origin.local_slots().size()>4)
        throw FormatError("Locomotion group requires an actual admitted receipt");
    (void)origin.accepted_state();
    auto payload=std::make_shared<Payload>(p.targets);
    std::array<const NativeLocomotion*,multiplayer_capacity> observed{};
    Statistics statistics;
    for(unsigned n=0;n<multiplayer_capacity;++n)if(const auto &slot=p.targets.gait[n]){
        observed[n]=slot.get();payload->slots[n]=slot->checkpoint();++statistics.occupied;
    }
    if(!payload->metadata.same(p.targets)||!distinct(p.targets))
        throw FormatError("Locomotion group metadata changed during capture");
    for(unsigned n=0;n<multiplayer_capacity;++n)
        if(p.targets.gait[n].get()!=observed[n]||
            (observed[n]&&!p.targets.gait[n]->owns_checkpoint(*payload->slots[n])))
            throw FormatError("Locomotion group slot lifetime changed during capture");
    Snapshot out;out.data_=std::make_shared<const SnapshotData>(SnapshotData{
        p.owner,p.executor,origin,std::move(payload),statistics});return out;
}
MultiplayerLocomotionContinuity::PreparedRestore::PreparedRestore()=default;
MultiplayerLocomotionContinuity::PreparedRestore::~PreparedRestore()=default;
MultiplayerLocomotionContinuity::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
MultiplayerLocomotionContinuity::PreparedRestore &MultiplayerLocomotionContinuity::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
MultiplayerLocomotionContinuity::PreparedRestore MultiplayerLocomotionContinuity::prepare_restore(
    const Snapshot &snapshot,const MultiplayerSnapshotProof &proof)const{
    PreparedRestore out;
    {
    const auto &p=*impl_;Impl::Call call(p);
    if(p.revision==std::numeric_limits<std::uint64_t>::max()||!snapshot.data_||snapshot.data_->owner.lock()!=p.owner||
        snapshot.data_->executor!=p.executor||!proof.matches(snapshot.data_->origin))
        throw FormatError("Locomotion group has a different actual owner or native receipt proof");
    const auto &origin=snapshot.data_->origin;const auto &cut=proof.cut();
    if(origin.context()!=cut.context||origin.tick()!=cut.seal.tick)
        throw FormatError("Locomotion group native cut differs from its exact accepted receipt");
    const auto saved=std::static_pointer_cast<const Payload>(snapshot.data_->payload);
    if(!saved)throw FormatError("Locomotion group has no exact owning slot history");
    auto data=std::make_unique<PreparedData>(p,origin,cut,saved->metadata);
    for(unsigned n=0;n<multiplayer_capacity;++n){
        auto &child=data->children[n];const auto &slot=p.targets.gait[n];child.expected=slot.get();
        if(saved->slots[n]){
            child.action=PreparedData::Action::Replace;
            child.replacement.emplace(NativeLocomotion::prepare_replacement(*saved->slots[n],slot));
        }else if(slot){
            child.action=PreparedData::Action::Clear;
            // The existing same-owner restore is only a completed current
            // lifetime/revision guard. Do not publish these copied values:
            // successful clear swaps the actual adapter into the empty token.
            const auto current=slot->checkpoint();
            child.clear_guard.emplace(slot->prepare_restore(current));
        }
    }
    if(!data->expected.same(p.targets)||!distinct(p.targets))
        throw FormatError("Locomotion group metadata changed during preparation");
    out.data_=std::move(data);
    }
    if(!ready(out))throw FormatError("Locomotion group child changed during preparation");
    return out;
}
bool MultiplayerLocomotionContinuity::ready(const PreparedRestore &prepared)const noexcept{
    const auto &p=*impl_;const auto *saved=prepared.data_.get();
    if(std::this_thread::get_id()!=p.executor||p.busy||!saved||saved->consumed||saved->owner.lock()!=p.owner||
        saved->executor!=p.executor||saved->revision!=p.revision||p.revision==std::numeric_limits<std::uint64_t>::max()||
        saved->slots!=&p.targets.gait||saved->dual!=&p.targets.dual||saved->unavailable!=&p.targets.unavailable||
        saved->units!=&p.targets.units||saved->weapons!=&p.targets.weapons||!saved->expected.same(p.targets)||
        !distinct(p.targets))return false;
    for(unsigned n=0;n<multiplayer_capacity;++n){
        const auto &slot=p.targets.gait[n];const auto &child=saved->children[n];
        if(slot.get()!=child.expected||child.retired)return false;
        switch(child.action){
        case PreparedData::Action::Absent:
            if(slot||child.replacement||child.clear_guard)return false;break;
        case PreparedData::Action::Replace:
            if(!child.replacement||child.clear_guard||!NativeLocomotion::replacement_ready(*child.replacement,slot))return false;break;
        case PreparedData::Action::Clear:
            if(!slot||child.replacement||!child.clear_guard||!slot->restore_ready(*child.clear_guard))return false;break;
        }
    }
    return true;
}
bool MultiplayerLocomotionContinuity::ready(const PreparedRestore &prepared,const MultiplayerSnapshotProof &proof)const noexcept{
    return ready(prepared)&&proof.matches(prepared.data_->origin)&&same_cut(proof.cut(),prepared.data_->cut);
}
bool MultiplayerLocomotionContinuity::commit(PreparedRestore &prepared)noexcept{
    if(!ready(prepared))return false;auto &p=*impl_;auto &saved=*prepared.data_;
    p.busy=true;
    for(unsigned n=0;n<multiplayer_capacity;++n){
        auto &child=saved.children[n];auto &slot=p.targets.gait[n];
        if(child.action==PreparedData::Action::Replace){
            if(!NativeLocomotion::commit_replacement(*child.replacement,slot))std::terminate();
        }else if(child.action==PreparedData::Action::Clear){
            static_assert(noexcept(slot.swap(child.retired)));slot.swap(child.retired);
        }
    }
    saved.replacement.exchange(p.targets);saved.consumed=true;++p.revision;p.busy=false;return true;
}
}
