#pragma once
#include "halo2/multiplayer_locomotion_continuity.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_locomotion.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
#include <vector>

// Enable deliberately in the single native receipt-test translation unit,
// before including this header. Other consumers do not replace allocation.
// This is test instrumentation, never a production allocator or runtime gate.
#ifdef HALO2_LOCOMOTION_GROUP_STORAGE_AUDIT
#ifdef _MSC_VER
#include <malloc.h>
#endif
namespace locomotion_group_storage_witness {
inline thread_local bool watching{};
inline thread_local std::size_t allocations{},retirements{};
inline void *allocate(std::size_t count){
    if(watching)++allocations;if(auto *p=std::malloc(count?count:1))return p;throw std::bad_alloc();
}
inline void release(void *p)noexcept{if(p&&watching)++retirements;std::free(p);}
inline void *aligned(std::size_t count,std::size_t alignment){
    if(watching)++allocations;count=count?count:1;
#ifdef _MSC_VER
    if(auto *p=_aligned_malloc(count,alignment))return p;
#else
    if(count>std::numeric_limits<std::size_t>::max()-(alignment-1))throw std::bad_alloc();
    if(auto *p=std::aligned_alloc(alignment,(count+alignment-1)/alignment*alignment))return p;
#endif
    throw std::bad_alloc();
}
inline void aligned_release(void *p)noexcept{
    if(p&&watching)++retirements;
#ifdef _MSC_VER
    _aligned_free(p);
#else
    std::free(p);
#endif
}
}
void *operator new(std::size_t n){return locomotion_group_storage_witness::allocate(n);}
void *operator new[](std::size_t n){return locomotion_group_storage_witness::allocate(n);}
void operator delete(void *p)noexcept{locomotion_group_storage_witness::release(p);}
void operator delete[](void *p)noexcept{locomotion_group_storage_witness::release(p);}
void operator delete(void *p,std::size_t)noexcept{locomotion_group_storage_witness::release(p);}
void operator delete[](void *p,std::size_t)noexcept{locomotion_group_storage_witness::release(p);}
void *operator new(std::size_t n,std::align_val_t a){return locomotion_group_storage_witness::aligned(n,std::size_t(a));}
void *operator new[](std::size_t n,std::align_val_t a){return locomotion_group_storage_witness::aligned(n,std::size_t(a));}
void operator delete(void *p,std::align_val_t)noexcept{locomotion_group_storage_witness::aligned_release(p);}
void operator delete[](void *p,std::align_val_t)noexcept{locomotion_group_storage_witness::aligned_release(p);}
void operator delete(void *p,std::size_t,std::align_val_t)noexcept{locomotion_group_storage_witness::aligned_release(p);}
void operator delete[](void *p,std::size_t,std::align_val_t)noexcept{locomotion_group_storage_witness::aligned_release(p);}
#endif

// The existing native snapshot fixture supplies real accepted origins and
// independently proven early/later native cuts. These authored CPU adapters
// are deliberately driven component controls, not invented match ticks or
// claims that a dormant slot is a connected authority player.
struct NativeLocomotionGroupFixture {
    using Group=halo2::MultiplayerLocomotionContinuity;
    using Gait=halo2::NativeLocomotion;
    halo2::MultiplayerPreparedWorld &world;
    Group::Slots gait,witness;
    Group::Flags dual{},unavailable{};
    Group::Handles units{},weapons{};
    std::unique_ptr<Group> continuity;
    Group::Snapshot early;
    Group::Flags early_dual{},early_unavailable{};
    Group::Handles early_units{},early_weapons{};
    struct Spec {unsigned representation{};std::uint32_t graph{},biped{},weapon{};bool dual{};halo2::SourceLandingDefinition landing;};
    std::array<Spec,halo2::multiplayer_capacity> specs;
    struct Frame {
        std::uint32_t graph{},index{},activity{},clock{};
        bool operator==(const Frame &)const=default;
    };
    struct Observation {
        std::array<std::byte,0x90> manager{};
        std::array<Frame,5> samples{};
        std::array<std::uint32_t,5> facing{};
        bool operator==(const Observation &)const=default;
    };
    struct Foot {unsigned timing{},kind{},marker{};std::uint32_t clock{};bool operator==(const Foot &)const=default;};
    struct Live {
        std::array<const Gait*,halo2::multiplayer_capacity> pointers{};
        std::array<std::optional<Observation>,halo2::multiplayer_capacity> values;
        Group::Flags dual{},unavailable{};
        Group::Handles units{},weapons{};
        bool operator==(const Live &)const=default;
    };
    std::array<std::optional<Observation>,halo2::multiplayer_capacity> captured;
    std::array<const halo2::AnimationClip*,halo2::multiplayer_capacity> captured_clips{};
    static constexpr unsigned cut=47,suffix=420;
    static constexpr std::array<unsigned,3> historical_slots{1,2,15};
    static void require(bool yes,const char *why){if(!yes)throw halo2::FormatError(why);}
    template<class F>static void rejects(F &&f,const char *why){bool rejected=false;try{f();}catch(const halo2::FormatError&){rejected=true;}require(rejected,why);}
    static std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
    static Frame frame(const halo2::NativeLocomotionFrame &f){
        require(f.clip,"Grouped gait lost its actual authored clip");return {f.clip->source_graph,f.clip->source_index,f.activity,bits(f.frame)};
    }
    static Observation observe(const Gait &g){
        constexpr std::array<float,5> fractions{0,.25f,.5f,.75f,1};Observation result;
        const auto bytes=g.manager();require(bytes.size()==result.manager.size(),"Grouped gait manager extent changed");
        std::copy(bytes.begin(),bytes.end(),result.manager.begin());
        for(unsigned n=0;n<fractions.size();++n){result.samples[n]=frame(g.sample(fractions[n]));result.facing[n]=bits(g.facing(fractions[n]));}
        require(std::equal(g.manager().begin(),g.manager().end(),result.manager.begin()),"Grouped gait sampling changed discrete manager");
        require(frame(g.frame())==result.samples.back(),"Grouped gait interpolation lost completed endpoint");return result;
    }
    Live live()const{
        Live out;out.dual=dual;out.unavailable=unavailable;out.units=units;out.weapons=weapons;
        for(unsigned n=0;n<halo2::multiplayer_capacity;++n)if(gait[n]){out.pointers[n]=gait[n].get();out.values[n]=observe(*gait[n]);}
        return out;
    }
    Spec definition(unsigned representation,bool is_dual)const{
        const auto &biped=world.content.player_representations.at(representation).biped;
        const auto b=world.cache->load_source_tag_metadata(biped.datum);const halo2::Bytes br(b.bytes_at(b.source_address(),0x304));
        const auto m=world.cache->load_source_tag_metadata(br.u32(0x38));const halo2::Bytes mr(m.bytes_at(m.source_address(),0x94));
        const auto weapon=world.cache->slayer_starting_weapon(5);
        require(world.content.weapons.at(weapon).dual_wieldable,"Grouped gait lacks actual dual-wieldable authored weapon");
        return {representation,mr.u32(0x14),biped.datum,weapon,is_dual,biped.landing};
    }
    std::unique_ptr<Gait> make(const Spec &s)const{
        auto result=std::make_unique<Gait>(world.cache,s.graph,s.weapon,std::array<std::uint32_t,2>{0x12345678,0x23456789},s.dual);
        result->bind_facing(s.biped,0);return result;
    }
    static std::vector<Foot> advance(Gait &g,unsigned tick,const Spec &s,const std::function<void()> &hook={}){
        const auto phase=tick%420;float forward{},right{},yaw{};bool walking=true,crouch=false;halo2::Vec3 velocity{};
        halo2::SourceLandingState landing;bool landed=false;
        if(phase<90){forward=1;velocity={2,0,0};yaw=.02f*float(phase);}
        else if(phase<150)yaw=-1.8f;
        else if(phase<210){right=1;crouch=true;velocity={0,2,0};yaw=1.4f;}
        else if(phase<226){forward=1;walking=false;velocity={0,0,3};yaw=1.4f;}
        else if(phase<232){bool forced=false;halo2::begin_source_landing(landing,s.landing,(s.landing.soft_speed+s.landing.hard_speed)*.5f,30,forced);
            for(unsigned n=226;n<phase;++n)halo2::advance_source_landing(landing,false);landed=true;yaw=1.4f;}
        else if(phase<330){forward=-1;crouch=true;velocity={-2,0,0};yaw=-.7f;}
        else yaw=1.8f;
        std::vector<Foot> events;
        g.tick(forward,right,walking,crouch,velocity,[&](const halo2::NativeFootstepEvent &event){
            require(g.frame().clip&&std::isfinite(g.facing()),"Grouped active footstep lost permitted observation");
            events.push_back({event.timing,event.kind,event.marker,bits(event.frame)});if(hook)hook();
        },landed?&landing:nullptr,&yaw);return events;
    }
    explicit NativeLocomotionGroupFixture(halo2::MultiplayerPreparedWorld &w):world(w){
        specs[1]=definition(2,false);specs[2]=definition(3,false);specs[15]=definition(2,true);
        for(auto n:historical_slots){gait[n]=make(specs[n]);
            // This independent normally advanced witness cannot checkpoint.
            witness[n]=std::make_unique<Gait>(*world.cache,specs[n].graph,specs[n].weapon,
                std::array<std::uint32_t,2>{0x12345678,0x23456789},specs[n].dual);
            witness[n]->bind_facing(specs[n].biped,0);}
        continuity=std::make_unique<Group>(targets());
    }
    Group::Targets targets(){return {gait,dual,units,weapons,unavailable};}
    void capture(const halo2::MultiplayerSnapshotOrigin &origin){
        const auto &state=origin.accepted_state();require(origin.local_slots().size()==4,"Grouped gait fixture lacks genuine four-seat receipt");
        continuity->mutating();
        for(unsigned n=0;n<halo2::multiplayer_capacity;++n){units[n]=state.players[n].source_unit;weapons[n]=state.players[n].source_weapon;
            dual[n]=state.players[n].secondary_weapon.datum!=halo2::null_datum;}
        // A dormant slot may retain its last salted ownership/availability
        // independently of current accepted connected-player membership.
        require(!state.players[15].connected,"Grouped dormant-slot control is actually an active authority player");
        units[15]=state.players[origin.local_slots()[0]].source_unit;weapons[15]=state.players[origin.local_slots()[0]].source_weapon;
        dual[15]=true;unavailable[13]=true;
        for(unsigned tick=0;tick<=cut;++tick)for(auto n:historical_slots){
            require(advance(*gait[n],tick,specs[n])==advance(*witness[n],tick,specs[n]),"Grouped initial authored footsteps differ");
            require(observe(*gait[n])==observe(*witness[n]),"Grouped owning and normally advanced authored gaits differ");
        }
        for(auto n:historical_slots){captured[n]=observe(*gait[n]);captured_clips[n]=gait[n]->frame().clip;
            require(captured[n]->samples.front()!=captured[n]->samples.back()&&captured[n]->facing.front()!=captured[n]->facing.back(),
                "Grouped capture did not exercise real interpolation/facing history");}
        early_dual=dual;early_unavailable=unavailable;early_units=units;early_weapons=weapons;early=continuity->capture(origin);
        require(early.matches(origin)&&continuity->owns_snapshot(early)&&early.statistics()->occupied==historical_slots.size(),
            "Grouped gait snapshot lost its actual receipt or complete slot census");
        rejects([&]{(void)continuity->capture(halo2::MultiplayerSnapshotOrigin{});},"Grouped gait fabricated accepted receipt");
    }
    template<class F>static bool audited_commit(F &&operation){
#ifdef HALO2_LOCOMOTION_GROUP_STORAGE_AUDIT
        using namespace locomotion_group_storage_witness;
        allocations=retirements=0;watching=true;const bool result=operation();watching=false;
        require(result&&allocations==0&&retirements==0,"Grouped gait readiness/commit allocated or retired storage");return result;
#else
        return operation();
#endif
    }
    void restore(const halo2::MultiplayerSnapshotOrigin &origin,const halo2::MultiplayerSnapshotProof &proof,
        const halo2::MultiplayerSnapshotProof &alternate,const halo2::MultiplayerSnapshotOrigin &later,const halo2::MultiplayerSnapshotProof &later_proof){
        require(later.tick()>origin.tick()&&origin.same_connection(later),"Grouped gait lacks genuinely later accepted receipt");
        const auto native_tick=world.match.simulation().ticks();const auto native_mutations=world.source_objects->execution_mutations();
        const auto source_unchanged=[&]{return world.match.simulation().ticks()==native_tick&&world.source_objects->execution_mutations()==native_mutations;};
        continuity->mutating();for(unsigned tick=cut+1;tick<200;++tick)for(auto n:historical_slots)(void)advance(*gait[n],tick,specs[n]);
        gait[2]=make(specs[1]);gait[15].reset();specs[3]=specs[1];specs[14]=specs[2];gait[3]=make(specs[3]);gait[14]=make(specs[14]);
        for(unsigned n=0;n<halo2::multiplayer_capacity;++n){dual[n]=!dual[n];unavailable[n]=!unavailable[n];
            units[n]^=0x01010000u+n;weapons[n]^=0x02020000u+n;}
        const auto later_snapshot=continuity->capture(later);require(later_snapshot.matches(later),"Grouped later receipt association missing");
        const auto changed=live();
        {auto canceled=continuity->prepare_restore(early,proof);require(continuity->ready(canceled,proof),"Grouped historical slots failed fresh preparation");}
        require(live()==changed&&source_unchanged(),"Grouped cancellation changed actual slots/metadata/source state");
        rejects([&]{(void)continuity->prepare_restore(early,halo2::MultiplayerSnapshotProof{});},"Grouped restore admitted absent proof");
        rejects([&]{(void)continuity->prepare_restore(early,later_proof);},"Grouped restore relabeled later native receipt");
        rejects([&]{(void)continuity->prepare_restore(later_snapshot,proof);},"Grouped restore relabeled earlier native receipt");
        Group foreign(targets());require(!foreign.owns_snapshot(early),"Grouped gait crossed owner identity");
        rejects([&]{(void)foreign.prepare_restore(early,proof);},"Foreign group prepared another owner's history");
        {auto stale=continuity->prepare_restore(early,proof);continuity->mutating();
            require(!continuity->ready(stale,proof)&&!continuity->commit(stale)&&live()==changed,"Grouped revision admitted stale publication");}
        // Metadata aliases can change without an aggregate writer mark. Full
        // expected-current comparisons still reject every actual array/slot.
        for(unsigned n=0;n<halo2::multiplayer_capacity;++n){auto p=continuity->prepare_restore(early,proof);
            dual[n]=!dual[n];require(!continuity->ready(p,proof)&&!continuity->commit(p),"Grouped ready ignored dual metadata");dual[n]=!dual[n];
            unavailable[n]=!unavailable[n];require(!continuity->ready(p,proof)&&!continuity->commit(p),"Grouped ready ignored unavailable metadata");unavailable[n]=!unavailable[n];
            ++units[n];require(!continuity->ready(p,proof)&&!continuity->commit(p),"Grouped ready ignored salted unit metadata");--units[n];
            ++weapons[n];require(!continuity->ready(p,proof)&&!continuity->commit(p),"Grouped ready ignored salted weapon metadata");--weapons[n];
            require(continuity->ready(p,proof),"Restored current metadata lost exact preparation eligibility");}
        for(unsigned n:{1u,3u,14u}){auto p=continuity->prepare_restore(early,proof);const auto before=live();
            (void)advance(*gait[n],200,specs[n]);const auto after=live();
            require(!continuity->ready(p,proof)&&!continuity->commit(p)&&live()==after&&before!=after,
                "Grouped all-child preflight partially published before a later changed child");}
        {auto p=continuity->prepare_restore(early,proof);auto old=std::move(gait[2]);gait[2]=make(specs[2]);
            require(!continuity->ready(p,proof)&&!continuity->commit(p),"Grouped replacement admitted a changed live destination");
            gait[2]=std::move(old);require(continuity->ready(p,proof),"Unchanged destination lifetime did not recover eligibility");}
        {auto p=continuity->prepare_restore(early,proof);gait[4]=make(specs[1]);
            require(!continuity->ready(p,proof)&&!continuity->commit(p),"Grouped absent/absent preflight ignored new live child");gait[4].reset();}
        // Same address is not the same current lifetime for historical clear.
        {auto p=continuity->prepare_restore(early,proof);auto *address=gait[3].release();address->~NativeLocomotion();
            try{gait[3].reset(::new(address) Gait(world.cache,specs[3].graph,specs[3].weapon,
                std::array<std::uint32_t,2>{0x12345678,0x23456789},specs[3].dual));}
            catch(...){::operator delete(address);throw;}
            gait[3]->bind_facing(specs[3].biped,0);
            require(gait[3].get()==address&&!continuity->ready(p,proof)&&!continuity->commit(p),
                "Grouped clear admitted an unrelated adapter at the same address");}
        // Explicit borrowed-resource admission failure, without weakening the
        // NativeLocomotion API or pretending a borrowed bank is owning.
        {Group::Slots slots;Group::Flags d{},u{};Group::Handles h{},w{};
            slots[0]=std::make_unique<Gait>(*world.cache,specs[1].graph,specs[1].weapon,std::array<std::uint32_t,2>{1,2});
            Group borrowed({slots,d,h,w,u});rejects([&]{(void)borrowed.capture(origin);},"Grouped capture certified borrowed Cache resources");}
        rejects([&]{Group alias({gait,dual,units,weapons,dual});},"Grouped constructor admitted aliased flag arrays");
        rejects([&]{Group alias({gait,dual,units,units,unavailable});},"Grouped constructor admitted aliased handle arrays");
        {auto p=continuity->prepare_restore(early,proof);auto *borrowed=gait[1].get();gait[4].reset(borrowed);
            bool constructor=false,capture=false;const bool rejected=!continuity->ready(p,proof)&&!continuity->commit(p);
            try{try{Group alias(targets());}catch(const halo2::FormatError&){constructor=true;}
                try{(void)continuity->capture(origin);}catch(const halo2::FormatError&){capture=true;}}
            catch(...){gait[4].release();throw;}
            gait[4].release();require(rejected&&constructor&&capture&&continuity->ready(p,proof),"Grouped constructor/preflight accepted aliased unique slots");}
        auto prepared=continuity->prepare_restore(early,proof),competing=continuity->prepare_restore(early,proof);
        require(!continuity->ready(prepared,alternate)&&!continuity->ready(prepared,later_proof),"Grouped readiness ignored complete native cut seal");
        bool executor_ok=false;std::thread wrong([&]{bool capture=false,prepare=false;
            try{(void)continuity->capture(origin);}catch(const halo2::FormatError&){capture=true;}
            try{(void)continuity->prepare_restore(early,proof);}catch(const halo2::FormatError&){prepare=true;}
            executor_ok=capture&&prepare&&!continuity->owns_snapshot(early)&&
                !continuity->ready(prepared,proof)&&!continuity->commit(prepared);});wrong.join();
        require(executor_ok&&continuity->ready(prepared,proof),"Grouped executor rejection changed original eligibility");
        const auto outgoing=live();std::array<std::span<const std::byte>,halo2::multiplayer_capacity> borrowed_managers;
        std::array<const halo2::AnimationClip*,halo2::multiplayer_capacity> outgoing_clips{};
        for(unsigned n=0;n<halo2::multiplayer_capacity;++n)if(gait[n]){borrowed_managers[n]=gait[n]->manager();outgoing_clips[n]=gait[n]->frame().clip;}
        require(audited_commit([&]{return continuity->ready(prepared,proof)&&continuity->commit(prepared);}),"Grouped exact publication failed");
        require(!continuity->commit(prepared)&&!continuity->commit(competing),"Grouped publication reused consumed or competing token");
        require(dual==early_dual&&unavailable==early_unavailable&&units==early_units&&weapons==early_weapons,
            "Grouped publication lost exact full16 dormant/salted metadata arrays");
        for(unsigned n=0;n<halo2::multiplayer_capacity;++n){require(bool(gait[n])==captured[n].has_value(),"Grouped restore changed exact occupied/empty census");
            if(gait[n])require(observe(*gait[n])==*captured[n]&&gait[n]->frame().clip==captured_clips[n],"Grouped restore reconstructed different values or clip bank");
            if(outgoing.values[n])require(std::equal(borrowed_managers[n].begin(),borrowed_managers[n].end(),outgoing.values[n]->manager.begin())&&
                outgoing_clips[n]->source_graph==outgoing.values[n]->samples.back().graph,
                "Grouped consumed token destroyed outgoing manager/clip borrowers");}
        unsigned foot_count=0;bool reentry=false;
        for(unsigned offset=0;offset<suffix;++offset)for(auto n:historical_slots){
            auto callback=[&]{if(reentry)return;reentry=true;
                rejects([&]{(void)continuity->capture(origin);},"Grouped reentrant capture admitted incomplete active gait");
                rejects([&]{(void)continuity->prepare_restore(early,proof);},"Grouped reentrant preparation admitted active bank");};
            const auto actual=advance(*gait[n],cut+1+offset,specs[n],callback),expected=advance(*witness[n],cut+1+offset,specs[n]);
            require(actual==expected&&observe(*gait[n])==observe(*witness[n]),"Grouped continuation changed exact subsequent sample/facing/footsteps");foot_count+=unsigned(expected.size());
        }
        require(reentry&&foot_count>4&&source_unchanged(),"Grouped authored continuation missed real events or mutated native source world");
        // A destroyed group cannot be kept eligible by retained snapshots or
        // prepared tokens. The resource payload still retires safely later.
        Group::Snapshot expired;std::optional<Group::PreparedRestore> expired_token;
        {Group local(targets());expired=local.capture(later);expired_token.emplace(local.prepare_restore(expired,later_proof));}
        require(!expired.valid()&&!continuity->ready(*expired_token,later_proof)&&!continuity->commit(*expired_token),
            "Grouped snapshot/token admitted a destroyed owner lifetime");
        std::cout<<"PASS proof-bound full16 locomotion slots: real four-seat/native receipt proof, exact dormant metadata, occupied/empty/replaced/destroyed slots, all-child preflight, cancellation/stale/foreign/full-seal/once controls\n";
        std::cout<<"PASS grouped authored locomotion suffix: ticks="<<suffix<<" adapters="<<historical_slots.size()<<" footstep_events="<<foot_count
            <<" exact manager/sample/facing/clip ownership and retained outgoing borrowers\n";
#ifdef HALO2_LOCOMOTION_GROUP_STORAGE_AUDIT
        std::cout<<"PASS grouped locomotion publication storage: full ready+commit zero allocations and zero retirements\n";
#else
        std::cout<<"LIMIT grouped commit allocation qualification omitted: enable HALO2_LOCOMOTION_GROUP_STORAGE_AUDIT in the single driver before including this fixture\n";
#endif
        std::cout<<"LIMIT authored CPU adapter/component control with genuine receipts; no source physical locomotion, runtime integration, GPU/audio/device activation or full Ready qualification\n";
    }
};
