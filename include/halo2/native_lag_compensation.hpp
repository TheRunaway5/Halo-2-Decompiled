#pragma once
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/native_lag_compensation_history.hpp"
#include "halo2/netcode_mode.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <array>
#include <deque>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <vector>

namespace halo2 {
struct LevelContent;
// Precision hitscan classification from authored tag data, not names: the
// weapon's projectile travels instantaneously (proj+BC bit5) and its primary
// trigger is not the spew (fully automatic) behavior. Returns projectile
// definitions; rows describe every instantaneous weapon for diagnostics.
struct LagCompensatedWeaponRow {
    std::string name;
    std::uint32_t weapon{},projectile{};
    std::int16_t trigger_behavior=-1,zoom_levels{};
    float rounds_per_second{};
    bool admitted{};
};
std::set<std::uint32_t> lag_compensated_projectiles(const LevelContent &,
    std::vector<LagCompensatedWeaponRow> *rows=nullptr);

// Host-side lag-compensated hitscan (docs/NETCODE_ENHANCED_GOAL.md item 2).
// After each completed host tick it retains every on-foot player body's
// collision state: the WORLD node bank, the object+30..+4F spheres and the
// root position of the biped and its attachment tree, and the biped's
// collidable cluster-entry sphere. When a remote shooter's instantaneous
// precision projectile tests its path, the targets' bytes are temporarily
// replaced by those of the tick the shooter displayed (capped), the original
// 16C620 query runs unchanged, and every replaced byte is restored from a
// write journal. Cluster membership is never re-linked: a target is found
// through the clusters it occupies at present (mismatches are counted).
// Balanced vetoes a rewound hit on a target that was not exposed to the shot
// origin, by native structure collision, at present or within the veto window;
// the query writes are then rolled back and it runs at present instead.
// The rewind is ordinary work of the step that fires: the step records the
// shooters' displayed views (MatchStepViews) and the authority checkpoint
// carries the retained history, so a replay world holding the same history
// runs this identical rewind and reaches the same bank bytes.
class NativeLagCompensation final : public SourceHitscanRewindOwnerTu15 {
public:
    struct Slot {
        std::uint32_t player=0xffffffffu,unit=0xffffffffu;
        std::optional<NetcodeViewTime> view; // Remote displayed time; local slots have none.
    };
    using Slots=std::array<Slot,16>;
    struct Stats {
        std::uint64_t queries{},considered{},rewound{},vetoed{},capped{},
            no_history{},membership_mismatches{},restore_conflicts{},restore_mismatches{};
        // Most recent considered query (a precision projectile of a remote shooter).
        std::uint32_t last_present{},last_target_tick{},last_rewind_ticks{};
        std::uint32_t last_hit=0xffffffffu; // Root object hit by the final query result.
        bool last_rewound{},last_capped{},last_vetoed{},last_rewound_hit{};
    };
    // One considered query: a precision projectile path test of a remote shooter.
    struct QueryRecord {
        std::uint32_t projectile{},definition{},shooter{},site{};
        std::uint32_t present{},viewed{},target_tick{},rewind_ticks{};
        std::uint32_t hit=0xffffffffu; // Root object of the final result, NONE if none.
        std::uint32_t result_type{};   // Final 16C620 packet type (0 none, 1/3 structure, 4 object).
        Vec3 origin,delta,point;       // Query segment and final packet point.
        unsigned targets{};
        bool capped{},rewound{},rewound_hit{},vetoed{};
    };
    using QueryObserver=std::function<void(const QueryRecord &)>;
    // attach=false builds a detached owner (continuation staging); attach()
    // later makes it the world's hitscan owner.
    NativeLagCompensation(PhysicsLiveWorldTu15 &,NetcodeMode,std::set<std::uint32_t> projectiles,
        std::size_t cluster_count,std::optional<BspSurfaceLighting> structure,unsigned tick_rate=30,bool attach=true);
    ~NativeLagCompensation() override;
    NativeLagCompensation(const NativeLagCompensation &)=delete;
    NativeLagCompensation &operator=(const NativeLagCompensation &)=delete;
    // Before the source step that produces `tick` (its snapshot tick).
    void begin_tick(std::uint32_t tick,const Slots &);
    void end_tick()noexcept;
    // After the completed step `tick`. Records only while a remote view exists.
    void record(std::uint32_t tick,const Slots &);
    bool armed()const noexcept override{return armed_;}
    void query(const SourceHitscanQueryTu15 &,SourceRuntimeMemory &,const std::function<void()> &run) override;
    NetcodeMode mode()const noexcept{return mode_;}
    unsigned cap_ticks()const noexcept{return cap_ticks_;}
    unsigned veto_ticks()const noexcept{return veto_ticks_;}
    const Stats &stats()const noexcept{return stats_;}
    const std::set<std::uint32_t> &projectiles()const noexcept{return projectiles_;}
    // Read-only diagnostics after each considered query's final result.
    void observe(QueryObserver observer){observer_=std::move(observer);}
    // Continuation (Match authority checkpoint): exactly the retained frames.
    NativeLagCompensationHistory history()const;
    using StagedHistory=std::deque<NativeLagCompensationHistory::Frame>;
    // Validate a received history for this owner (same mode, the retained
    // window, ordered ticks no newer than `present`, bounded well-formed
    // banks and cluster indices) and stage it without changing this owner.
    StagedHistory stage_history(const NativeLagCompensationHistory &,std::uint64_t present)const;
    // Publication of staged frames. Allocates nothing.
    void adopt_history(StagedHistory &)noexcept;
    // Make this the world's hitscan owner, replacing any other owner.
    void attach();
    // Clear the world's owner if this is it.
    void detach();
    // Another owner's attach() replaced this one in the world.
    void displaced()noexcept{attached_=false;}
    bool attached()const noexcept{return attached_;}
private:
    using ObjectState=NativeLagCompensationHistory::Object;
    using BodyState=NativeLagCompensationHistory::Body;
    using Frame=NativeLagCompensationHistory::Frame;
    PhysicsLiveWorldTu15 &world_;
    NetcodeMode mode_;
    std::set<std::uint32_t> projectiles_;
    std::size_t cluster_count_{};
    std::optional<BspSurfaceLighting> structure_;
    unsigned cap_ticks_{},veto_ticks_{};
    std::deque<Frame> history_;
    Slots slots_{};
    std::uint32_t tick_{};
    bool armed_{},attached_{};
    Stats stats_;
    QueryObserver observer_;
    std::uint32_t body_of(const SourceRuntimeMemory &,std::uint32_t object)const;
    bool capture(const SourceRuntimeMemory &,std::uint32_t object,ObjectState &)const;
    const Frame *frame(std::uint32_t tick)const;
    bool exposed(Vec3 origin,std::span<const Vec3> present,std::uint32_t root)const;
};
} // namespace halo2
