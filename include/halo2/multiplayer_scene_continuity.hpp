#pragma once
#include "halo2/multiplayer_snapshot_origin.hpp"
#include <memory>
#include <thread>

namespace halo2 {
class SourceSceneryScene;
class SourceMachineScene;
class SourcePropScene;
class SourceDynamicObjectScene;
class NativeWeaponProjectileScene;
class SourceGrenadeScene;
class MaterialPresentationContinuity;
struct MultiplayerSceneRuntimeValues;
namespace pstencil { class PointLights; }

// Exact completed local scene owners at one authentic accepted receipt. This
// subtransaction is independent of native-world activation, HUD/camera/input
// and playing audio. Those owners must join the complete enclosing adopter
// before any caller advertises readiness or enables windowed migration.
class MultiplayerSceneContinuity {
public:
    // Optional caller-owned diagnostic storage. All counters are fixed POD;
    // none authorizes an owner, receipt, checkpoint or prepared publication.
    struct CaptureCosts {
        struct Work {double wall_ms,thread_cpu_ms;std::uint64_t samples,thread_cpu_samples;};
        std::uint64_t attempts,completed;
        Work grenade,values,scenery,machine,prop,dynamic,projectile,point_light,material,total;
    };
    // Exactly one scope per thread; the collector must outlive it. Scope and
    // captures stay on that executor. Other threads and ordinary unscoped
    // captures do not read clocks or write this collector. Initialize with{}.
    class CaptureCostScope {
    public:
        explicit CaptureCostScope(CaptureCosts &);
        ~CaptureCostScope()noexcept;
        CaptureCostScope(const CaptureCostScope &)=delete;
        CaptureCostScope &operator=(const CaptureCostScope &)=delete;
        CaptureCostScope(CaptureCostScope &&)=delete;
        CaptureCostScope &operator=(CaptureCostScope &&)=delete;
    private:
        CaptureCosts *costs_;
        std::thread::id executor_;
    };
    struct Statistics {
        // Material-owned checkpoint payload only, as scoped by the material
        // owner; external PRT banks and complete scene/history costs differ.
        std::size_t material_roots{},material_nodes{},material_links{},material_logical_bytes{},material_unique_bytes{},
            new_material_value_bytes{},shared_material_value_bytes{},resident_material_schema_bytes{},
            generated_material_image_bytes{};
    };
private:
    struct SnapshotData {
        std::weak_ptr<const void> owner;
        std::thread::id executor;
        MultiplayerSnapshotOrigin origin;
        std::shared_ptr<const void> payload;
        Statistics statistics;
    };
    struct PreparedData;
public:
    struct Targets {
        MultiplayerSceneRuntimeValues &values;
        SourceSceneryScene &scenery;
        SourceMachineScene &machines;
        SourcePropScene &props;
        SourceDynamicObjectScene &dynamic;
        NativeWeaponProjectileScene &projectiles;
        SourceGrenadeScene *grenades;
        pstencil::PointLights &point_lights;
        MaterialPresentationContinuity &materials;
    };
    explicit MultiplayerSceneContinuity(Targets);
    MultiplayerSceneContinuity(const MultiplayerSceneContinuity &)=delete;
    MultiplayerSceneContinuity &operator=(const MultiplayerSceneContinuity &)=delete;
    // Actual field aliases stay live in the runtime. Admit each mutation batch
    // before it writes them and keep every borrowed scene/material/callback
    // owner quiescent through prepare/preflight/publication. Child owners also
    // independently reject incomplete, reentrant or stale operations.
    void mutating();
    class Snapshot {
    public:
        bool valid()const noexcept { return data_&&!data_->owner.expired(); }
        bool matches(const MultiplayerSnapshotOrigin &origin)const noexcept {
            return valid()&&data_->origin.same(origin);
        }
        std::optional<Statistics> statistics()const noexcept {
            return valid()?std::optional{data_->statistics}:std::nullopt;
        }
    private:
        friend class MultiplayerSceneContinuity;
        std::shared_ptr<const SnapshotData> data_;
    };
    Snapshot capture(const MultiplayerSnapshotOrigin &)const;
    bool owns_snapshot(const Snapshot &)const noexcept;
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class MultiplayerSceneContinuity;
        PreparedRestore();
        std::unique_ptr<PreparedData> data_;
    };
    PreparedRestore prepare_restore(const Snapshot &,const MultiplayerSnapshotProof &)const;
    bool ready(const PreparedRestore &,const MultiplayerSnapshotProof &)const noexcept;
    // All swaps follow one complete preflight. The caller retains the consumed
    // token, including displaced graph/container owners, until after native /
    // renderer publication and after old borrowed views have retired.
    bool commit(PreparedRestore &)noexcept;
private:
    void own()const;
    bool ready(const PreparedRestore &)const noexcept;
    Targets targets_;
    const std::thread::id executor_=std::this_thread::get_id();
    const std::shared_ptr<const void> owner_=std::make_shared<const unsigned char>(0);
    std::uint64_t revision_{};
};
}
