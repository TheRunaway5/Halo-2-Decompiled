#include "halo2/multiplayer_pose_continuity.hpp"
#include <limits>
#include <type_traits>
#include <utility>

namespace halo2 {
struct MultiplayerPoseContinuity::Snapshot::Data {
    std::shared_ptr<const void> owner;
    std::thread::id executor;
    MultiplayerSnapshotOrigin origin;
    Values values;
};
struct MultiplayerPoseContinuity::PreparedRestore::Data {
    std::shared_ptr<const void> owner;
    std::thread::id executor;
    std::uint64_t revision{};
    MultiplayerSnapshotOrigin origin;
    ValidatedAuthorityCut cut;
    Values replacement;
    bool consumed{};
};
void MultiplayerPoseContinuity::own()const {
    if(std::this_thread::get_id()!=executor_)throw FormatError("Pose history crossed its executor");
}
void MultiplayerPoseContinuity::mutating(){
    own();if(revision_==std::numeric_limits<std::uint64_t>::max())throw FormatError("Pose history revision exhausted");
    ++revision_;
}
MultiplayerPoseContinuity::Values &MultiplayerPoseContinuity::state(){mutating();return values_;}
const MultiplayerPoseContinuity::Values &MultiplayerPoseContinuity::state()const {own();return values_;}
bool MultiplayerPoseContinuity::Snapshot::valid()const noexcept{return bool(data_);}
bool MultiplayerPoseContinuity::Snapshot::matches(const MultiplayerSnapshotOrigin &origin)const noexcept {
    return data_&&data_->origin.same(origin);
}
bool MultiplayerPoseContinuity::owns_snapshot(const Snapshot &snapshot)const noexcept {
    return std::this_thread::get_id()==executor_&&snapshot.data_&&snapshot.data_->owner==owner_&&snapshot.data_->executor==executor_;
}
MultiplayerPoseContinuity::Snapshot MultiplayerPoseContinuity::capture(const MultiplayerSnapshotOrigin &origin)const {
    own();if(!origin.context()||origin.local_slots().empty()||origin.local_slots().size()>4)
        throw FormatError("Pose history requires an actual admitted receipt");
    (void)origin.accepted_state();
    Snapshot out;out.data_=std::make_shared<const Snapshot::Data>(Snapshot::Data{owner_,executor_,origin,values_});return out;
}
MultiplayerPoseContinuity::PreparedRestore::PreparedRestore()=default;
MultiplayerPoseContinuity::PreparedRestore::~PreparedRestore()=default;
MultiplayerPoseContinuity::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
MultiplayerPoseContinuity::PreparedRestore &MultiplayerPoseContinuity::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
MultiplayerPoseContinuity::PreparedRestore MultiplayerPoseContinuity::prepare_restore(
    const Snapshot &snapshot,const MultiplayerSnapshotProof &proof)const {
    own();if(revision_==std::numeric_limits<std::uint64_t>::max()||!owns_snapshot(snapshot)||!proof.matches(snapshot.data_->origin))
        throw FormatError("Pose history does not match its actual owner/native receipt proof");
    const auto &cut=proof.cut();
    if(snapshot.data_->origin.context()!=cut.context||snapshot.data_->origin.tick()!=cut.seal.tick)
        throw FormatError("Pose history native cut differs from its exact receipt");
    PreparedRestore out;out.data_=std::make_unique<PreparedRestore::Data>(PreparedRestore::Data{
        owner_,executor_,revision_,snapshot.data_->origin,cut,snapshot.data_->values,false});return out;
}
bool MultiplayerPoseContinuity::ready(const PreparedRestore &prepared)const noexcept {
    const auto *p=prepared.data_.get();return std::this_thread::get_id()==executor_&&p&&!p->consumed&&p->owner==owner_&&
        p->executor==executor_&&p->revision==revision_&&revision_!=std::numeric_limits<std::uint64_t>::max();
}
bool MultiplayerPoseContinuity::ready(const PreparedRestore &prepared,const MultiplayerSnapshotProof &proof)const noexcept {
    if(!ready(prepared)||!proof.matches(prepared.data_->origin))return false;
    const auto &expected=prepared.data_->cut,&actual=proof.cut();
    return expected.peer==actual.peer&&expected.context==actual.context&&expected.seal==actual.seal;
}
bool MultiplayerPoseContinuity::commit(PreparedRestore &prepared)noexcept {
    if(!ready(prepared))return false;
    static_assert(std::is_nothrow_swappable_v<Values>);
    auto &p=*prepared.data_;using std::swap;swap(values_,p.replacement);p.consumed=true;++revision_;return true;
}
}
