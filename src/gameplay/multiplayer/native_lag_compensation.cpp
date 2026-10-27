#include "halo2/native_lag_compensation.hpp"
#include "halo2/content.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <map>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
// TU1.5 globals: object data array, collidable cluster heads and entries.
constexpr std::uint32_t object_array_global=0x4ecbd0u,collidable_heads_global=0x4eabb0u,
    collidable_entries_global=0x4eabb4u;
constexpr unsigned node_stride=0x34,entry_stride=32;
std::uint32_t u32(const SourceRuntimeMemory &m,std::uint32_t at) {
    std::uint32_t value{};std::memcpy(&value,m.view(at,4).data(),4);return value;
}
std::int16_t i16(const SourceRuntimeMemory &m,std::uint32_t at) {
    std::int16_t value{};std::memcpy(&value,m.view(at,2).data(),2);return value;
}
Vec3 vec3(std::span<const std::byte> bytes,std::size_t at) {
    float v[3];std::memcpy(v,bytes.data()+at,12);return {v[0],v[1],v[2]};
}
// Before-bytes of every bank write while recording, newest last. rollback()
// rewrites them in reverse, restoring exactly the bytes seen before each write.
struct Journal {
    SourceRuntimeMemory &bank;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> undo;
    bool recording=true;
    explicit Journal(SourceRuntimeMemory &b):bank(b) {
        source_runtime_register_write_observer(bank,this,[this](std::uint32_t at,std::size_t bytes) {
            const auto before=bank.view(at,bytes);undo.emplace_back(at,std::vector<std::byte>(before.begin(),before.end()));
        });
    }
    void stop() {if(recording)source_runtime_unregister_write_observer(bank,this);recording=false;}
    void rollback() {
        stop();
        for(auto at=undo.rbegin();at!=undo.rend();++at)bank.write(at->first,at->second);
        undo.clear();
    }
    bool overlaps(const Journal &other)const {
        for(const auto &[a,x]:undo)for(const auto &[b,y]:other.undo)
            if(std::uint64_t(a)<std::uint64_t(b)+y.size() && std::uint64_t(b)<std::uint64_t(a)+x.size())return true;
        return false;
    }
    Journal(const Journal &)=delete;
    Journal &operator=(const Journal &)=delete;
    ~Journal(){stop();}
};
}

std::set<std::uint32_t> lag_compensated_projectiles(const LevelContent &content,
    std::vector<LagCompensatedWeaponRow> *rows) {
    std::set<std::uint32_t> result;
    const auto consider=[&](const WeaponDefinition &weapon,std::uint32_t datum) {
        if(!weapon.instantaneous || weapon.projectile==null_datum)return;
        LagCompensatedWeaponRow row{weapon.name,datum,weapon.projectile,
            weapon.triggers.empty()?std::int16_t(-1):weapon.triggers.front().behavior,
            weapon.zoom.levels,weapon.rounds_per_second,false};
        // Trigger behavior0 is spew (full auto). Precision weapons latch.
        row.admitted=!weapon.triggers.empty() && weapon.triggers.front().behavior!=0;
        if(row.admitted)result.insert(weapon.projectile);
        if(rows)rows->push_back(std::move(row));
    };
    for(const auto &[datum,weapon]:content.weapons) {
        consider(weapon,datum);
        for(const auto &view:weapon.barrel_views)if(view)consider(*view,datum);
    }
    return result;
}

NativeLagCompensation::NativeLagCompensation(PhysicsLiveWorldTu15 &world,NetcodeMode mode,
    std::set<std::uint32_t> projectiles,std::size_t clusters,std::optional<BspSurfaceLighting> structure,unsigned rate,bool attach_now)
    :world_(world),mode_(mode),projectiles_(std::move(projectiles)),cluster_count_(clusters),structure_(std::move(structure)) {
    if(mode_==NetcodeMode::original)throw FormatError("Original netcode installs no lag compensation");
    if(!rate || clusters>512)throw FormatError("Lag compensation requires a positive tick rate and the actual BSP clusters");
    if(mode_==NetcodeMode::balanced && !structure_)throw FormatError("Balanced cover veto requires the structure collision bank");
    cap_ticks_=lag_compensation_cap_ms(mode_)*rate/1000u;
    veto_ticks_=lag_compensation_cover_veto_ms*rate/1000u;
    if(structure_) {
        // The veto consumes collision only; drop presentation-only payloads.
        structure_->diffuse.clear();structure_->diffuse.shrink_to_fit();
        structure_->lighting={};structure_->definitions.clear();structure_->definitions.shrink_to_fit();
        for(auto &cluster:structure_->clusters)cluster={};
    }
    if(attach_now)attach();
}
NativeLagCompensation::~NativeLagCompensation() {
    if(attached_)try{world_.set_hitscan_rewind_owner(nullptr);}catch(...){}
}
void NativeLagCompensation::attach() {
    world_.set_hitscan_rewind_owner(this);attached_=true;
}
void NativeLagCompensation::detach() {
    if(!attached_)return;
    world_.set_hitscan_rewind_owner(nullptr);attached_=false;
}
NativeLagCompensationHistory NativeLagCompensation::history()const {
    return {mode_,std::vector<Frame>(history_.begin(),history_.end())};
}
NativeLagCompensation::StagedHistory NativeLagCompensation::stage_history(
    const NativeLagCompensationHistory &saved,std::uint64_t present)const {
    const auto invalid=[](const char *why){return FormatError(std::string("Lag-compensation history ")+why);};
    if(saved.mode!=mode_)throw invalid("belongs to another netcode mode");
    if(saved.frames.size()>std::size_t(std::max(cap_ticks_,veto_ticks_))+2u)throw invalid("exceeds the retained window");
    std::optional<std::uint32_t> previous;
    for(const auto &frame:saved.frames) {
        if(frame.tick>present || (previous && frame.tick<=*previous))throw invalid("ticks are unordered or newer than the cut");
        previous=frame.tick;
        if(frame.bodies.size()>lag_compensation_history_bodies)throw invalid("has too many bodies");
        std::uint32_t slots{};
        for(const auto &body:frame.bodies) {
            if(body.slot>=lag_compensation_history_bodies || (slots&(1u<<body.slot)))throw invalid("repeats or exceeds a player slot");
            slots|=1u<<body.slot;
            if(body.objects.empty() || body.objects.size()>lag_compensation_history_objects ||
                body.objects.front().object!=body.root || body.objects.front().parent!=none)
                throw invalid("has an invalid on-foot attachment tree");
            for(const auto &object:body.objects) {
                if(!object.body)throw invalid("names no object body");
                if(!object.nodes.empty() && (object.node_offset<0 || object.node_bytes<=0 ||
                    object.node_bytes%node_stride || object.nodes.size()!=std::size_t(object.node_bytes)))
                    throw invalid("has a node bank that differs from its declared extent");
            }
            if(!body.registered && (!body.clusters.empty() || std::ranges::any_of(body.primitive,[](std::byte b){return b!=std::byte{};})))
                throw invalid("has cluster state for an unregistered body");
            for(std::size_t i=0;i<body.clusters.size();++i)
                if(body.clusters[i]<0 || std::size_t(body.clusters[i])>=cluster_count_ || (i && body.clusters[i]<=body.clusters[i-1]))
                    throw invalid("has an invalid cluster index");
        }
    }
    return StagedHistory(saved.frames.begin(),saved.frames.end());
}
void NativeLagCompensation::adopt_history(StagedHistory &staged)noexcept {
    history_.swap(staged);slots_={};tick_=0;armed_=false;
}
void NativeLagCompensation::begin_tick(std::uint32_t tick,const Slots &slots) {
    slots_=slots;tick_=tick;
    armed_=!history_.empty() && std::ranges::any_of(slots_,[](const Slot &s){return s.view && s.unit!=none;});
}
void NativeLagCompensation::end_tick()noexcept{armed_=false;}
std::uint32_t NativeLagCompensation::body_of(const SourceRuntimeMemory &bank,std::uint32_t object)const {
    if(object==none)return 0;
    SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return bank.view(p,n);}};
    const auto header=source_raw_datum_lookup(u32(bank,object_array_global),object,storage);
    return header?u32(bank,header+8u):0;
}
bool NativeLagCompensation::capture(const SourceRuntimeMemory &bank,std::uint32_t object,ObjectState &out)const {
    const auto body=body_of(bank,object);if(!body)return false;
    out.object=object;out.body=body;out.parent=u32(bank,body+0x14u);
    out.node_bytes=i16(bank,body+0x114u);out.node_offset=i16(bank,body+0x116u);
    out.nodes.clear();
    if(out.node_bytes>0 && out.node_offset>=0 && !(out.node_bytes%node_stride)) {
        const auto nodes=bank.view(body+std::uint32_t(out.node_offset),std::size_t(out.node_bytes));
        out.nodes.assign(nodes.begin(),nodes.end());
    }
    std::ranges::copy(bank.view(body+0x30u,out.spheres.size()),out.spheres.begin());
    std::ranges::copy(bank.view(body+0x64u,out.position.size()),out.position.begin());
    return true;
}
void NativeLagCompensation::record(std::uint32_t tick,const Slots &slots) {
    // Couch and offline sessions have no remote view and keep no history.
    if(std::ranges::none_of(slots,[](const Slot &s){return s.view.has_value();})){history_.clear();return;}
    const auto &bank=world_.memory();
    Frame frame;frame.tick=tick;
    std::map<std::uint32_t,BodyState*> roots;
    for(unsigned slot=0;slot<slots.size();++slot) {
        const auto unit=slots[slot].unit;if(unit==none)continue;
        BodyState body;body.slot=slot;body.root=unit;
        ObjectState root;
        // Seated or otherwise attached bodies move with their parent and are
        // not rewound; only on-foot roots are retained.
        if(!capture(bank,unit,root) || root.parent!=none)continue;
        body.objects.push_back(std::move(root));
        // Attachment tree: object+10 first child, +0C next sibling.
        std::vector<std::uint32_t> pending{u32(bank,body.objects.front().body+0x10u)};
        while(!pending.empty() && body.objects.size()<64) {
            const auto child=pending.back();pending.pop_back();
            if(child==none)continue;
            ObjectState state;if(!capture(bank,child,state))continue;
            pending.push_back(u32(bank,state.body+0x0cu));pending.push_back(u32(bank,state.body+0x10u));
            body.objects.push_back(std::move(state));
        }
        frame.bodies.push_back(std::move(body));
    }
    for(auto &body:frame.bodies)roots[body.root]=&body;
    // Literal collidable cluster lists: owner +4, next +8, primitive +0C..+1F.
    const auto heads=u32(bank,collidable_heads_global),records=u32(bank,u32(bank,collidable_entries_global)+0x44u);
    for(unsigned cluster=0;cluster<cluster_count_;++cluster) {
        auto token=u32(bank,heads+cluster*4u);
        for(unsigned links=0;token!=none && links<2048;++links) {
            const auto record=records+(token&65535u)*entry_stride;
            const auto owner=u32(bank,record+4u);
            if(const auto found=roots.find(owner);found!=roots.end()) {
                auto &body=*found->second;
                if(body.clusters.empty() || body.clusters.back()!=std::int16_t(cluster))body.clusters.push_back(std::int16_t(cluster));
                std::ranges::copy(bank.view(record+16u,16),body.primitive.begin());body.registered=true;
            }
            token=u32(bank,record+8u);
        }
    }
    history_.push_back(std::move(frame));
    while(history_.size()>std::size_t(std::max(cap_ticks_,veto_ticks_))+2u)history_.pop_front();
}
const NativeLagCompensation::Frame *NativeLagCompensation::frame(std::uint32_t tick)const {
    for(const auto &f:history_)if(f.tick==tick)return &f;
    return nullptr;
}
bool NativeLagCompensation::exposed(Vec3 origin,std::span<const Vec3> present,std::uint32_t root)const {
    if(!structure_)return true;
    // Structure and instanced structure only (literal map query 04800005).
    const auto visible=[&](Vec3 point) {
        const Vec3 delta{point.x-origin.x,point.y-origin.y,point.z-origin.z};
        return !original_map_vector_contact(*structure_,origin,delta,nullptr,0x04800005u).has_value();
    };
    // A failed structure query keeps the original present-time result.
    try {
    for(const auto point:present)if(visible(point))return true;
    const auto present_tick=tick_-1u;
    for(const auto &f:history_) {
        if(f.tick>present_tick || present_tick-f.tick>veto_ticks_)continue;
        for(const auto &body:f.bodies)if(body.root==root && !body.objects.empty()) {
            const auto &nodes=body.objects.front().nodes;
            for(std::size_t at=0;at+node_stride<=nodes.size();at+=node_stride)
                if(visible(vec3(nodes,at+40)))return true;
        }
    }
    }catch(const FormatError &){}
    return false;
}
void NativeLagCompensation::query(const SourceHitscanQueryTu15 &q,SourceRuntimeMemory &bank,
    const std::function<void()> &run) {
    ++stats_.queries;
    if(!projectiles_.contains(q.definition) || q.owner_player==none || !tick_){run();return;}
    // The firing player (+C4) whose current unit is the query's exclusion (+140).
    const auto shooter=std::ranges::find_if(slots_,[&](const Slot &s){
        return s.player==q.owner_player && s.unit==q.ignore_object;});
    if(shooter==slots_.end() || !shooter->view){run();return;}
    const auto present=tick_-1u;
    const auto viewed=netcode_view_tick(*shooter->view,present);
    if(!viewed){run();return;}
    ++stats_.considered;
    stats_.last_present=present;stats_.last_target_tick=present;stats_.last_rewind_ticks=0;
    stats_.last_hit=none;stats_.last_rewound=stats_.last_capped=stats_.last_vetoed=stats_.last_rewound_hit=false;
    QueryRecord record{q.projectile,q.definition,q.ignore_object,q.site,present,*viewed,present,0};
    const auto root_of=[&](std::uint32_t object) {
        for(unsigned depth=0;depth<16 && object!=none;++depth) {
            const auto body=body_of(bank,object);if(!body)break;
            const auto parent=u32(bank,body+0x14u);if(parent==none)break;
            object=parent;
        }
        return object;
    };
    const auto final_hit=[&]{return u32(bank,q.result)==4u?root_of(u32(bank,q.result+0x40u)):none;};
    // Every exit reports the final result the original caller receives.
    struct Report {
        const std::function<void()> fn;
        ~Report(){if(std::uncaught_exceptions()==0)fn();}
    } report{[&]{
        record.hit=final_hit();stats_.last_hit=record.hit;
        record.result_type=u32(bank,q.result);record.origin=q.origin;record.delta=q.delta;
        record.point=vec3(bank.view(q.result+8u,12),0);
        if(observer_)observer_(record);
    }};
    // Displayed time in ticks; the nearest retained tick is used, so every
    // applied pose is one the host actually published.
    auto behind=double(present)-(double(*viewed)+double(shooter->view->fraction)/255.0);
    if(behind>double(cap_ticks_)){behind=double(cap_ticks_);++stats_.capped;stats_.last_capped=record.capped=true;}
    const auto rewind=behind>0?unsigned(std::lround(behind)):0u;
    if(!rewind){run();return;}
    const auto target_tick=present-rewind;
    record.target_tick=target_tick;record.rewind_ticks=rewind;
    const auto *past=frame(target_tick);
    if(!past){++stats_.no_history;run();return;}
    const auto shooter_slot=unsigned(shooter-slots_.begin());
    // Targets: other players whose same on-foot unit still exists.
    struct Patch {std::uint32_t at;std::span<const std::byte> bytes;};
    std::vector<Patch> patches;std::set<std::uint32_t> targets;
    std::map<std::uint32_t,std::vector<Vec3>> present_nodes;
    const auto heads=u32(bank,collidable_heads_global),records=u32(bank,u32(bank,collidable_entries_global)+0x44u);
    for(const auto &body:past->bodies) {
        if(body.slot==shooter_slot || slots_[body.slot].unit!=body.root || body.objects.empty())continue;
        const auto &root=body.objects.front();
        if(body_of(bank,root.object)!=root.body || u32(bank,root.body+0x14u)!=none)continue;
        targets.insert(body.root);
        auto &points=present_nodes[body.root];
        for(const auto &object:body.objects) {
            if(body_of(bank,object.object)!=object.body || u32(bank,object.body+0x14u)!=object.parent)continue;
            if(i16(bank,object.body+0x114u)!=object.node_bytes || i16(bank,object.body+0x116u)!=object.node_offset)continue;
            if(&object==&root) {
                const auto live=bank.view(object.body+std::uint32_t(object.node_offset),object.nodes.size());
                for(std::size_t at=0;at+node_stride<=live.size();at+=node_stride)points.push_back(vec3(live,at+40));
            }
            if(!object.nodes.empty())patches.push_back({object.body+std::uint32_t(object.node_offset),object.nodes});
            patches.push_back({object.body+0x30u,object.spheres});
            patches.push_back({object.body+0x64u,object.position});
        }
        if(!body.registered)continue;
        // Present collidable entries take the past sphere; membership itself
        // is not re-linked, so past-only clusters cannot admit this target.
        std::vector<std::int16_t> current;
        for(unsigned cluster=0;cluster<cluster_count_;++cluster) {
            auto token=u32(bank,heads+cluster*4u);
            for(unsigned links=0;token!=none && links<2048;++links) {
                const auto entry=records+(token&65535u)*entry_stride;
                if(u32(bank,entry+4u)==body.root) {
                    patches.push_back({entry+16u,body.primitive});
                    if(current.empty() || current.back()!=std::int16_t(cluster))current.push_back(std::int16_t(cluster));
                }
                token=u32(bank,entry+8u);
            }
        }
        if(std::ranges::any_of(body.clusters,[&](auto c){return std::ranges::find(current,c)==current.end();}))
            ++stats_.membership_mismatches;
    }
    record.targets=unsigned(targets.size());
    if(targets.empty()){run();return;}
    std::vector<std::vector<std::byte>> before;before.reserve(patches.size());
    for(const auto &patch:patches){const auto v=bank.view(patch.at,patch.bytes.size());before.emplace_back(v.begin(),v.end());}
    const auto verify=[&] {
        for(std::size_t i=0;i<patches.size();++i)
            if(!std::ranges::equal(bank.view(patches[i].at,before[i].size()),before[i]))++stats_.restore_mismatches;
    };
    // Part of this step's replay transcript: a replay world holds the same
    // history (authority checkpoint) and receives the same displayed views
    // (MatchStepViews), so it applies these same patches and runs this query.
    Journal applied(bank);
    try{for(const auto &patch:patches)bank.write(patch.at,patch.bytes);}
    catch(...){applied.rollback();throw;}
    applied.stop();
    Journal queried(bank);
    try{run();}
    catch(...){queried.rollback();applied.rollback();throw;}
    queried.stop();
    if(applied.overlaps(queried))++stats_.restore_conflicts;
    stats_.last_rewound=record.rewound=true;stats_.last_target_tick=target_tick;stats_.last_rewind_ticks=rewind;
    const auto hit=final_hit();
    stats_.last_rewound_hit=record.rewound_hit=targets.contains(hit);
    if(mode_==NetcodeMode::balanced && record.rewound_hit &&
        !exposed(q.origin,present_nodes[hit],hit)) {
        // Cover veto: discard the rewound query entirely and test the present.
        queried.rollback();applied.rollback();verify();
        ++stats_.vetoed;stats_.last_vetoed=record.vetoed=true;
        run();
        return;
    }
    applied.rollback();verify();
    ++stats_.rewound;
}
} // namespace halo2
