#include "halo2/multiplayer_scene_continuity.hpp"
#include "halo2/multiplayer_scene_runtime_values.hpp"
#include "halo2/material_presentation_continuity.hpp"
#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_dynamic_object_scene.hpp"
#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/source_grenade_scene.hpp"
#include "halo2/pstencil/point_lights.hpp"
#include <exception>
#include <limits>
#include <chrono>
#include <type_traits>
#if defined(__linux__)
#include <time.h>
#endif

namespace halo2 {
namespace {
using CaptureCosts=MultiplayerSceneContinuity::CaptureCosts;
static_assert(std::is_trivial_v<CaptureCosts> && std::is_standard_layout_v<CaptureCosts>);
thread_local CaptureCosts *capture_costs{};
thread_local unsigned captures_in_flight{};
bool capture_thread_cpu(double &out)noexcept {
#if defined(__linux__)
    timespec at{};
    if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&at)==0) {
        out=double(at.tv_sec)*1000.+double(at.tv_nsec)/1e6;return true;
    }
#endif
    return false;
}
class CaptureWork {
public:
    explicit CaptureWork(CaptureCosts::Work *work):work_(work) {
        if(work_){begin_=std::chrono::steady_clock::now();cpu_=capture_thread_cpu(cpu_begin_);}
    }
    ~CaptureWork()noexcept {
        if(!work_)return;
        const auto end=std::chrono::steady_clock::now();double cpu_end{};
        const bool valid=capture_thread_cpu(cpu_end);
        work_->wall_ms+=std::chrono::duration<double,std::milli>(end-begin_).count();++work_->samples;
        if(cpu_&&valid&&cpu_end>=cpu_begin_){work_->thread_cpu_ms+=cpu_end-cpu_begin_;++work_->thread_cpu_samples;}
    }
private:
    CaptureCosts::Work *work_;
    std::chrono::steady_clock::time_point begin_;
    double cpu_begin_{};bool cpu_{};
};
template<class Call>decltype(auto) capture_leaf(CaptureCosts::Work *work,Call &&call) {
    CaptureWork measured(work);
    // Return the original expression directly: prvalue checkpoint results
    // initialize their final aggregate fields without new intermediate copies.
    return call();
}
class CaptureAttempt {
public:
    explicit CaptureAttempt(CaptureCosts *costs):costs_(costs) {
        if(costs_){++captures_in_flight;++costs_->attempts;}
    }
    ~CaptureAttempt()noexcept {if(costs_)--captures_in_flight;}
private:
    CaptureCosts *costs_;
};
struct ScenePayload {
    MultiplayerSceneRuntimeValues values;
    SourceSceneryScene::Checkpoint scenery;
    SourceMachineScene::Checkpoint machines;
    SourcePropScene::Checkpoint props;
    SourceDynamicObjectScene::Checkpoint dynamic;
    NativeWeaponProjectileScene::Checkpoint projectiles;
    std::optional<SourceGrenadeScene::Checkpoint> grenades;
    pstencil::PointLights::Checkpoint point_lights;
    MaterialPresentationContinuity::Checkpoint materials;
};
bool same(const ValidatedAuthorityCut &a,const ValidatedAuthorityCut &b)noexcept {
    return a.peer==b.peer&&a.context==b.context&&a.seal==b.seal;
}
}
MultiplayerSceneContinuity::CaptureCostScope::CaptureCostScope(CaptureCosts &costs)
    :costs_(&costs),executor_(std::this_thread::get_id()) {
    if(capture_costs)throw FormatError("Scene capture cost scopes cannot nest on one executor");
    capture_costs=costs_;
}
MultiplayerSceneContinuity::CaptureCostScope::~CaptureCostScope()noexcept {
    if(executor_!=std::this_thread::get_id()||capture_costs!=costs_||captures_in_flight)std::terminate();
    capture_costs=nullptr;
}
struct MultiplayerSceneContinuity::PreparedData {
    std::shared_ptr<const void> owner;
    std::thread::id executor;
    std::uint64_t revision{};
    MultiplayerSnapshotOrigin origin;
    ValidatedAuthorityCut cut;
    MultiplayerSceneRuntimeValues values;
    SourceSceneryScene::PreparedRestore scenery;
    SourceMachineScene::PreparedRestore machines;
    SourcePropScene::PreparedRestore props;
    SourceDynamicObjectScene::PreparedRestore dynamic;
    NativeWeaponProjectileScene::PreparedRestore projectiles;
    std::optional<SourceGrenadeScene::PreparedRestore> grenades;
    pstencil::PointLights::PreparedRestore point_lights;
    MaterialPresentationContinuity::PreparedRestore materials;
    bool consumed{};
};
MultiplayerSceneContinuity::MultiplayerSceneContinuity(Targets targets):targets_(targets){}
void MultiplayerSceneContinuity::own()const {
    if(std::this_thread::get_id()!=executor_)throw FormatError("Scene receipt history crossed its executor");
}
void MultiplayerSceneContinuity::mutating() {
    own();if(revision_==std::numeric_limits<std::uint64_t>::max())throw FormatError("Scene receipt history revision exhausted");
    ++revision_;
}
bool MultiplayerSceneContinuity::owns_snapshot(const Snapshot &snapshot)const noexcept {
    return std::this_thread::get_id()==executor_&&snapshot.data_&&
        snapshot.data_->owner.lock()==owner_&&snapshot.data_->executor==executor_;
}
MultiplayerSceneContinuity::Snapshot MultiplayerSceneContinuity::capture(const MultiplayerSnapshotOrigin &origin)const {
    auto *costs=capture_costs;
    CaptureWork total(costs?&costs->total:nullptr);
    CaptureAttempt attempt(costs);
    own();if(!origin.context()||origin.local_slots().empty()||origin.local_slots().size()>4)
        throw FormatError("Scene receipt history requires an actual admitted receipt");
    (void)origin.accepted_state();
    auto &t=targets_;
    std::optional<SourceGrenadeScene::Checkpoint> grenades;
    if(t.grenades)capture_leaf(costs?&costs->grenade:nullptr,[&]{grenades=t.grenades->checkpoint();});
    auto payload=std::make_shared<const ScenePayload>(ScenePayload{
        capture_leaf(costs?&costs->values:nullptr,[&]{return t.values;}),
        capture_leaf(costs?&costs->scenery:nullptr,[&]{return t.scenery.checkpoint();}),
        capture_leaf(costs?&costs->machine:nullptr,[&]{return t.machines.checkpoint();}),
        capture_leaf(costs?&costs->prop:nullptr,[&]{return t.props.checkpoint();}),
        capture_leaf(costs?&costs->dynamic:nullptr,[&]{return t.dynamic.checkpoint();}),
        capture_leaf(costs?&costs->projectile:nullptr,[&]{return t.projectiles.checkpoint();}),std::move(grenades),
        capture_leaf(costs?&costs->point_light:nullptr,[&]{return t.point_lights.checkpoint();}),
        capture_leaf(costs?&costs->material:nullptr,[&]{return t.materials.checkpoint();})});
    if(payload->point_lights.tick()!=origin.tick())
        throw FormatError("Scene point-light state does not share the accepted receipt tick");
    const auto material_stats=payload->materials.stats();
    Statistics statistics{material_stats.roots,material_stats.nodes,material_stats.links,material_stats.logical_bytes,material_stats.unique_bytes,
        material_stats.new_value_bytes,material_stats.shared_value_bytes,material_stats.resident_schema_bytes,
        material_stats.generated_image_bytes};
    Snapshot out;out.data_=std::make_shared<const SnapshotData>(SnapshotData{
        owner_,executor_,origin,std::move(payload),statistics});
    if(costs)++costs->completed;
    return out;
}
MultiplayerSceneContinuity::PreparedRestore::PreparedRestore()=default;
MultiplayerSceneContinuity::PreparedRestore::~PreparedRestore()=default;
MultiplayerSceneContinuity::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
MultiplayerSceneContinuity::PreparedRestore &MultiplayerSceneContinuity::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
MultiplayerSceneContinuity::PreparedRestore MultiplayerSceneContinuity::prepare_restore(
    const Snapshot &snapshot,const MultiplayerSnapshotProof &proof)const {
    own();if(revision_==std::numeric_limits<std::uint64_t>::max()||!owns_snapshot(snapshot)||
        !proof.matches(snapshot.data_->origin))
        throw FormatError("Scene history has a different actual owner or native receipt proof");
    const auto &cut=proof.cut();
    if(snapshot.data_->origin.context()!=cut.context||snapshot.data_->origin.tick()!=cut.seal.tick)
        throw FormatError("Scene history native cut differs from its exact accepted receipt");
    const auto saved=std::static_pointer_cast<const ScenePayload>(snapshot.data_->payload);
    auto &t=targets_;
    if(!saved||bool(t.grenades)!=saved->grenades.has_value())
        throw FormatError("Scene history has a different resident grenade owner");
    auto materials=t.materials.prepare_restore(saved->materials);
    const auto graph=materials.target_graph();
    auto scenery=t.scenery.prepare_restore(saved->scenery);
    auto machines=t.machines.prepare_restore(saved->machines,graph);
    auto props=t.props.prepare_restore(saved->props,graph);
    auto dynamic=t.dynamic.prepare_restore(saved->dynamic);
    auto projectiles=t.projectiles.prepare_restore(saved->projectiles);
    std::optional<SourceGrenadeScene::PreparedRestore> grenades;
    if(t.grenades)grenades=t.grenades->prepare_restore(*saved->grenades);
    auto points=t.point_lights.prepare_restore(saved->point_lights);
    PreparedRestore out;out.data_=std::make_unique<PreparedData>(PreparedData{owner_,executor_,revision_,
        snapshot.data_->origin,cut,saved->values,std::move(scenery),std::move(machines),std::move(props),
        std::move(dynamic),std::move(projectiles),std::move(grenades),std::move(points),std::move(materials),false});
    return out;
}
bool MultiplayerSceneContinuity::ready(const PreparedRestore &prepared)const noexcept {
    const auto *p=prepared.data_.get();const auto &t=targets_;
    return std::this_thread::get_id()==executor_&&p&&!p->consumed&&p->owner==owner_&&p->executor==executor_&&
        p->revision==revision_&&revision_!=std::numeric_limits<std::uint64_t>::max()&&
        bool(t.grenades)==p->grenades.has_value()&&t.materials.restore_ready(p->materials)&&
        t.scenery.restore_ready(p->scenery)&&t.machines.restore_ready(p->machines)&&t.props.restore_ready(p->props)&&
        t.dynamic.restore_ready(p->dynamic)&&t.projectiles.restore_ready(p->projectiles)&&
        (!t.grenades||t.grenades->restore_ready(*p->grenades))&&t.point_lights.restore_ready(p->point_lights);
}
bool MultiplayerSceneContinuity::ready(const PreparedRestore &prepared,const MultiplayerSnapshotProof &proof)const noexcept {
    return ready(prepared)&&proof.matches(prepared.data_->origin)&&same(proof.cut(),prepared.data_->cut);
}
bool MultiplayerSceneContinuity::commit(PreparedRestore &prepared)noexcept {
    if(!ready(prepared))return false;
    auto &p=*prepared.data_;auto &t=targets_;
    // Each child was preflighted against this same prepared graph before any
    // mutation. TargetGraph accepts the now-applied exact graph, never an
    // arbitrary caller flag. No callback/allocation/retirement can intervene.
    if(!t.materials.commit_restore(p.materials)||!t.scenery.commit_restore(p.scenery)||
        !t.machines.commit_restore(p.machines)||!t.props.commit_restore(p.props)||
        !t.dynamic.commit_restore(p.dynamic)||!t.projectiles.commit_restore(p.projectiles)||
        (t.grenades&&!t.grenades->commit_restore(*p.grenades))||!t.point_lights.commit_restore(p.point_lights))
        std::terminate();
    t.values.swap(p.values);p.consumed=true;++revision_;return true;
}
}
