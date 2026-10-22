#include "halo2/multiplayer_hud_continuity.hpp"
#include "halo2/hud.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include <exception>
#include <limits>
#include <set>
#include <vector>

namespace halo2 {
namespace {
enum class Registry:unsigned {Primary,Offhand,Vehicle,Unit,Parent};
struct Key {
    Registry registry{};unsigned representation{};std::uint32_t definition{};
    bool operator==(const Key &)const=default;
};
template<class F>bool visit(const MultiplayerHudContinuity::Targets &t,F &&f){
    const auto definitions=[&](const auto &map,Registry kind){
        for(const auto &[key,owner]:map)if(!f(Key{kind,key.first,key.second},owner.get()))return false;return true;
    };
    if(!definitions(t.primary,Registry::Primary)||!definitions(t.offhand,Registry::Offhand)||
        !definitions(t.vehicle,Registry::Vehicle))return false;
    for(const auto &[representation,owner]:t.unit)if(!f(Key{Registry::Unit,representation,0},owner.get()))return false;
    return definitions(t.parent,Registry::Parent);
}
bool same(const ValidatedAuthorityCut &a,const ValidatedAuthorityCut &b)noexcept{
    return a.peer==b.peer&&a.context==b.context&&a.seal==b.seal;
}
struct SavedHud {Key key;MultiplayerHud *owner{};MultiplayerHud::Checkpoint values;};
struct Payload {std::vector<SavedHud> huds;std::optional<MultiplayerScoreboard::Checkpoint> scoreboard;};
}
struct MultiplayerHudContinuity::Impl {
    struct Entry {Key key;MultiplayerHud *owner{};MultiplayerHud::Checkpoint identity;};
    Targets targets;
    const std::thread::id executor=std::this_thread::get_id();
    const std::shared_ptr<const void> owner=std::make_shared<const unsigned char>(0);
    std::uint64_t revision{};mutable bool busy{};
    std::vector<Entry> entries;
    MultiplayerScoreboard *scoreboard{};
    std::optional<MultiplayerScoreboard::Checkpoint> scoreboard_identity;
    Statistics statistics;
    explicit Impl(Targets t):targets(t),scoreboard(t.scoreboard.get()){
        std::set<const MultiplayerHud*> unique;
        visit(targets,[&](Key key,MultiplayerHud *hud){
            if(!hud||!unique.insert(hud).second)throw FormatError("HUD group has a null or aliased resident owner");
            // The frozen child API exposes its weak lifetime witness through
            // Checkpoint/owns_checkpoint. This one initial CPU copy is retained
            // with the census; it never pins a HUD/font/Mesh object.
            entries.push_back({key,hud,hud->checkpoint()});++statistics.huds[unsigned(key.registry)];return true;
        });
        if(scoreboard)scoreboard_identity=scoreboard->checkpoint();
        statistics.scoreboard=bool(scoreboard);
    }
    bool census()const noexcept{
        if(targets.primary.size()!=statistics.huds[0]||targets.offhand.size()!=statistics.huds[1]||
            targets.vehicle.size()!=statistics.huds[2]||targets.unit.size()!=statistics.huds[3]||
            targets.parent.size()!=statistics.huds[4]||targets.scoreboard.get()!=scoreboard||
            bool(scoreboard_identity)!=bool(scoreboard))return false;
        if(scoreboard&&!scoreboard->owns_checkpoint(*scoreboard_identity))return false;
        std::size_t n{};
        return visit(targets,[&](Key key,MultiplayerHud *hud){
            if(n>=entries.size())return false;const auto &saved=entries[n++];
            return hud&&saved.key==key&&saved.owner==hud&&hud->owns_checkpoint(saved.identity);
        })&&n==entries.size();
    }
    void own()const{
        if(std::this_thread::get_id()!=executor||busy||!census())
            throw FormatError("HUD group lost its completed executor or exact resident census");
    }
    struct Call {
        const Impl &owner;
        explicit Call(const Impl &p):owner(p){owner.own();owner.busy=true;}
        ~Call(){owner.busy=false;}
        Call(const Call &)=delete;Call &operator=(const Call &)=delete;
    };
};
struct MultiplayerHudContinuity::PreparedData {
    struct Child {Key key;MultiplayerHud *owner{};MultiplayerHud::PreparedRestore values;};
    std::weak_ptr<const void> owner;
    std::thread::id executor;
    std::uint64_t revision{};
    MultiplayerSnapshotOrigin origin;
    ValidatedAuthorityCut cut;
    std::vector<Child> huds;
    MultiplayerScoreboard *scoreboard_owner{};
    std::optional<MultiplayerScoreboard::PreparedRestore> scoreboard;
    bool consumed{};
};
MultiplayerHudContinuity::MultiplayerHudContinuity(Targets targets):impl_(std::make_unique<Impl>(targets)){}
MultiplayerHudContinuity::~MultiplayerHudContinuity()=default;
void MultiplayerHudContinuity::mutating(){
    Impl::Call call(*impl_);if(impl_->revision==std::numeric_limits<std::uint64_t>::max())
        throw FormatError("HUD group revision exhausted");++impl_->revision;
}
bool MultiplayerHudContinuity::owns_snapshot(const Snapshot &snapshot)const noexcept{
    const auto &p=*impl_;return std::this_thread::get_id()==p.executor&&!p.busy&&p.census()&&snapshot.data_&&
        snapshot.data_->owner.lock()==p.owner&&snapshot.data_->executor==p.executor;
}
MultiplayerHudContinuity::Snapshot MultiplayerHudContinuity::capture(const MultiplayerSnapshotOrigin &origin)const{
    const auto &p=*impl_;Impl::Call call(p);
    if(!origin.context()||origin.local_slots().empty()||origin.local_slots().size()>4)
        throw FormatError("HUD group requires an actual admitted receipt");
    (void)origin.accepted_state();
    auto payload=std::make_shared<Payload>();payload->huds.reserve(p.entries.size());
    for(const auto &entry:p.entries)payload->huds.push_back({entry.key,entry.owner,entry.owner->checkpoint()});
    if(p.scoreboard)payload->scoreboard=p.scoreboard->checkpoint();
    if(!p.census())throw FormatError("HUD group census changed during capture");
    Snapshot out;out.data_=std::make_shared<const SnapshotData>(SnapshotData{p.owner,p.executor,origin,std::move(payload),p.statistics});return out;
}
MultiplayerHudContinuity::PreparedRestore::PreparedRestore()=default;
MultiplayerHudContinuity::PreparedRestore::~PreparedRestore()=default;
MultiplayerHudContinuity::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
MultiplayerHudContinuity::PreparedRestore &MultiplayerHudContinuity::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
MultiplayerHudContinuity::PreparedRestore MultiplayerHudContinuity::prepare_restore(
    const Snapshot &snapshot,const MultiplayerSnapshotProof &proof)const{
    const auto &p=*impl_;Impl::Call call(p);
    if(p.revision==std::numeric_limits<std::uint64_t>::max()||!snapshot.data_||snapshot.data_->owner.lock()!=p.owner||
        snapshot.data_->executor!=p.executor||!proof.matches(snapshot.data_->origin))
        throw FormatError("HUD group has a different actual owner or native receipt proof");
    const auto &cut=proof.cut();const auto &origin=snapshot.data_->origin;
    if(origin.context()!=cut.context||origin.tick()!=cut.seal.tick)
        throw FormatError("HUD group native cut differs from the exact accepted receipt");
    const auto saved=std::static_pointer_cast<const Payload>(snapshot.data_->payload);
    if(!saved||saved->huds.size()!=p.entries.size()||saved->scoreboard.has_value()!=bool(p.scoreboard))
        throw FormatError("HUD group snapshot has a different complete resident census");
    std::vector<PreparedData::Child> children;children.reserve(p.entries.size());
    for(std::size_t n=0;n<p.entries.size();++n){const auto &entry=p.entries[n];const auto &child=saved->huds[n];
        if(entry.key!=child.key||entry.owner!=child.owner||!entry.owner->owns_checkpoint(child.values))
            throw FormatError("HUD group snapshot has a replaced resident child lifetime");
        children.push_back({entry.key,entry.owner,entry.owner->prepare_restore(child.values)});
    }
    std::optional<MultiplayerScoreboard::PreparedRestore> scoreboard;
    if(p.scoreboard)scoreboard=p.scoreboard->prepare_restore(*saved->scoreboard);
    if(!p.census())throw FormatError("HUD group census changed during preparation");
    PreparedRestore out;out.data_=std::make_unique<PreparedData>(PreparedData{p.owner,p.executor,p.revision,origin,cut,
        std::move(children),p.scoreboard,std::move(scoreboard),false});return out;
}
bool MultiplayerHudContinuity::ready(const PreparedRestore &prepared)const noexcept{
    const auto &p=*impl_;const auto *saved=prepared.data_.get();
    if(std::this_thread::get_id()!=p.executor||p.busy||!saved||saved->consumed||saved->owner.lock()!=p.owner||
        saved->executor!=p.executor||saved->revision!=p.revision||p.revision==std::numeric_limits<std::uint64_t>::max()||
        !p.census()||saved->huds.size()!=p.entries.size()||saved->scoreboard_owner!=p.scoreboard||
        saved->scoreboard.has_value()!=bool(p.scoreboard))return false;
    for(std::size_t n=0;n<p.entries.size();++n){const auto &entry=p.entries[n];const auto &child=saved->huds[n];
        if(entry.key!=child.key||entry.owner!=child.owner||!entry.owner->restore_ready(child.values))return false;
    }
    return !p.scoreboard||p.scoreboard->restore_ready(*saved->scoreboard);
}
bool MultiplayerHudContinuity::ready(const PreparedRestore &prepared,const MultiplayerSnapshotProof &proof)const noexcept{
    return ready(prepared)&&proof.matches(prepared.data_->origin)&&same(proof.cut(),prepared.data_->cut);
}
bool MultiplayerHudContinuity::commit(PreparedRestore &prepared)noexcept{
    if(!ready(prepared))return false;auto &p=*impl_;auto &saved=*prepared.data_;
    // Census forbids aliased children; every independent CPU owner was checked
    // before the first swap. No callback/allocation/retirement intervenes.
    p.busy=true;
    for(auto &child:saved.huds)if(!child.owner->commit_restore(child.values))std::terminate();
    if(p.scoreboard&&!p.scoreboard->commit_restore(*saved.scoreboard))std::terminate();
    saved.consumed=true;++p.revision;p.busy=false;return true;
}
}
