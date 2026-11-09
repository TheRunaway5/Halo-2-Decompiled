#include "halo2/source_presented_model_pose_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_presentation_resource_regions_tu15.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <iomanip>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <thread>

namespace halo2 {
namespace {
using Poses=std::map<std::uint32_t,SourcePresentedModelPoseTu15>;
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p,unsigned n=4) {
    const auto b=m.view(p,n);std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return v;
}
// HALO2_RELOCATION_PATH_CENSUS=1 (diagnostic, off by default): classify each
// relocated root by the 1CD620/1DC270 Havok path and BF0C0 branch it takes
// (docs/native_replacement/RELOCATION_NATIVE_PLAN.md Phase 0) and accumulate
// the host time of its relocation per class; printed every 120 frames.
std::string relocation_path_class(const SourceRuntimeMemory &m,std::uint32_t body) {
    try {
        std::string c="t"+std::to_string(word(m,body+0xaau,1));
        if(word(m,body+0x10u)!=~0u)c+="+child";
        if(word(m,body+0x11cu,2))c+="+attach";
        const auto flags=word(m,body+4u);
        if(flags&(1u<<29))c+="+b29";
        if(word(m,body+0x12au,2)!=0xffffu && !word(m,body+0xb3u,1))c+="+anim";
        if(word(m,body+0xc0u,1)&0x40u)c+="+c0b6";
        const auto component=word(m,body+0xb4u);
        if(component==~0u)return c+" nocomp";
        const auto row=word(m,word(m,0x52be5cu)+0x44u)+(component&0xffffu)*0xa0u;
        const auto cf=word(m,row+4u);
        if(cf&0x100u)return c+" anchored";
        if(cf&0x80u)return c+" machine";
        c+=(cf&0x400u)?" rebuild":(cf&0x200u)?" single":" multi";
        const auto bodies=word(m,row+0x74u);bool fixed=false,asleep=false;
        for(std::uint32_t i=0;i<bodies && i<64;++i) {
            const auto rb=word(m,word(m,row+0x70u)+i*0x60u+0x40u);if(!rb)continue;
            if(word(m,rb+0x40u,1) && (cf&0x20u))fixed=true;
            const auto island=word(m,rb+0x44u);if(island && !word(m,island+0x29u,1))asleep=true;
        }
        if(fixed)c+="+fixed";
        if(asleep)c+="+asleep";
        return c+" n"+std::to_string(bodies);
    }catch(...){return "unreadable";}
}
void relocation_path_record(const std::string &kind,double microseconds,bool frame_end) {
    static std::map<std::string,std::pair<std::size_t,double>> stats;static unsigned frames=0;
    if(!frame_end){auto &e=stats[kind];++e.first;e.second+=microseconds;return;}
    if(++frames%120u)return;
    double total=0;for(const auto &[k,v]:stats)total+=v.second;
    std::cerr<<"Relocation path census: frames="<<frames<<" total_us="<<std::fixed<<std::setprecision(0)<<total;
    std::vector<std::pair<std::string,std::pair<std::size_t,double>>> rows(stats.begin(),stats.end());
    std::sort(rows.begin(),rows.end(),[](const auto &a,const auto &b){return a.second.second>b.second.second;});
    for(const auto &[k,v]:rows)std::cerr<<" | "<<k<<" calls="<<v.first<<" us="<<v.second<<" ("<<std::setprecision(1)<<100.0*v.second/total<<"%)"<<std::setprecision(0);
    std::cerr<<std::defaultfloat<<'\n';
}
bool finite(const Transform &m) {
    const auto values=std::bit_cast<std::array<float,13>>(m);
    return std::ranges::all_of(values,[](float v){return std::isfinite(v);});
}
// One journal entry: the original bytes live in a contiguous arena that the
// executor thread keeps across frames, so a frame's thousands of observed
// stores neither allocate nor return heap pages. Restore order and bytes are
// those of the per-store copies this replaces.
struct Store {std::uint32_t address;std::size_t offset,size;};
std::vector<std::byte> &journal_arena() {
    static thread_local std::vector<std::byte> arena;
    return arena;
}
// The entry index follows the same LIFO base rule as the arena, so its
// capacity is kept across frames too. HALO2_REFERENCE_PRESENTED_STORE_INDEX=1
// gives each scope its own vector, as before.
std::vector<Store> &journal_stores() {
    static thread_local std::vector<Store> stores;
    static const bool reference=[]{const auto *p=std::getenv("HALO2_REFERENCE_PRESENTED_STORE_INDEX");return p && *p;}();
    if(reference){static thread_local std::vector<Store> fresh;return fresh;}
    return stores;
}
}
void validate_source_presented_model_poses_tu15(std::span<const SourcePresentedModelPoseTu15> poses) {
    if(poses.size()>source_presented_model_owner_capacity_tu15)
        throw FormatError("Presented WORLD input exceeds the native2048-object lifetime pool");
    // Exact named-field wire size: sequence length, three datum words,
    //13-float root, four-float sphere and node-sequence length per owner.
    // Keep aggregate input allocation within the existing typed-call budget.
    std::size_t wire_bytes=4+poses.size()*84;
    std::map<std::uint32_t,bool> owners;
    for(const auto &pose:poses) {
        if(pose.object==~0u || pose.definition==~0u || pose.render_model==~0u ||
           !owners.emplace(pose.object,true).second || !finite(pose.root) ||
           !std::isfinite(pose.sphere.center.x) || !std::isfinite(pose.sphere.center.y) ||
           !std::isfinite(pose.sphere.center.z) || !std::isfinite(pose.sphere.radius) || pose.sphere.radius<0 ||
           pose.nodes.empty() || pose.nodes.size()>source_presented_model_node_capacity_tu15 ||
           !std::ranges::all_of(pose.nodes,[](const auto &node){return finite(node);}))
            throw FormatError("Presented WORLD input lost its full-salt model/root/pose owner");
        if(pose.nodes.size()>(SourceExecutionInputs::maximum_call_bytes-wire_bytes)/sizeof(Transform))
            throw FormatError("Presented WORLD aggregate node bank exceeds the native typed-call byte budget");
        wire_bytes+=pose.nodes.size()*sizeof(Transform);
    }
}
struct SourcePresentedModelPoseScopeTu15::Impl {
    PhysicsLiveWorldTu15 &world;SourceRuntimeMemory &memory;
    const std::thread::id executor=std::this_thread::get_id();Poses poses;
    std::uint32_t frame{};std::set<std::uint32_t> already_built;
    std::unique_ptr<SourcePresentedModelInputBindingTu15> binding;
    const int exceptions=std::uncaught_exceptions();
    // Entries index the executor's shared arena above this scope's base, so a
    // scope that fails while another is live never touches the other's bytes.
    std::vector<Store> &stores=journal_stores();const std::size_t stores_base=stores.size();
    std::size_t bytes{};std::vector<std::byte> &arena=journal_arena();
    std::size_t persistent_writes{},persistent_bytes{}; // relocation stores into resource owners (census)
    std::size_t moved_roots{},unmoved_roots{}; // roots relocated / presented in place (census)
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> persistent_before; // census only
    const std::size_t arena_base=arena.size();
    explicit Impl(PhysicsLiveWorldTu15 &w):world(w),memory(w.memory()){}
    void record(std::uint32_t address,std::span<const std::byte> before) {
        stores.push_back({address,arena.size(),before.size()});
        arena.insert(arena.end(),before.begin(),before.end());bytes+=before.size();
    }
    // HALO2_PRESENTED_SCOPE_CENSUS=1: every 120 frames, what the relocation
    // journal touched — presented bodies by field offset, presented node
    // banks, physical pages, and everything else by 64 KB region. Diagnostic
    // only; it reads the journal and prints.
    void census()const noexcept {
        static const bool enabled=[]{const auto *e=std::getenv("HALO2_PRESENTED_SCOPE_CENSUS");return e && *e;}();
        if(!enabled)return;
        static unsigned frames=0;if(frames++%120u)return;
        try {
            struct Range{std::uint32_t first,end;};
            std::vector<std::pair<std::uint32_t,Range>> bodies,banks;
            for(const auto &[owner,pose]:poses) {
                const auto body=world.source_object_body(owner);
                bodies.push_back({owner,{body,body+0x200u}});
                const auto at=body+word(memory,body+0x116u,2);
                banks.push_back({owner,{at,at+std::uint32_t(pose.nodes.size()*sizeof(Transform))}});
            }
            std::map<std::uint32_t,std::pair<unsigned,std::size_t>> field,region;
            unsigned physical=0,bank_stores=0,body_stores=0;std::size_t physical_bytes=0,bank_bytes=0,body_bytes=0;
            for(const auto &st:std::span<const Store>(stores).subspan(stores_base)) {
                const auto a=st.address;bool classified=false;
                for(const auto &[owner,r]:bodies)if(a>=r.first && a<r.end){auto &f=field[a-r.first];++f.first;f.second+=st.size;++body_stores;body_bytes+=st.size;classified=true;break;}
                if(classified)continue;
                for(const auto &[owner,r]:banks)if(a>=r.first && a<r.end){++bank_stores;bank_bytes+=st.size;classified=true;break;}
                if(classified)continue;
                if(a>=0x80000000u){++physical;physical_bytes+=st.size;continue;}
                auto &g=region[a&~0xffffu];++g.first;g.second+=st.size;
            }
            std::cerr<<"Presented scope census: objects="<<poses.size()<<" stores="<<stores.size()-stores_base<<" bytes="<<bytes
                <<" | body stores="<<body_stores<<" ("<<body_bytes<<" B) node-bank stores="<<bank_stores<<" ("<<bank_bytes
                <<" B) physical="<<physical<<" ("<<physical_bytes<<" B)\n  body fields:";
            std::vector<std::pair<std::uint32_t,std::pair<unsigned,std::size_t>>> fields(field.begin(),field.end());
            std::sort(fields.begin(),fields.end(),[](const auto &x,const auto &y){return x.second.first>y.second.first;});
            for(std::size_t n=0;n<fields.size() && n<16;++n)std::cerr<<" +0x"<<std::hex<<fields[n].first<<std::dec<<"x"<<fields[n].second.first;
            std::vector<std::pair<std::uint32_t,std::pair<unsigned,std::size_t>>> regions(region.begin(),region.end());
            std::sort(regions.begin(),regions.end(),[](const auto &x,const auto &y){return x.second.second>y.second.second;});
            std::cerr<<"\n  other regions:";
            for(std::size_t n=0;n<regions.size() && n<10;++n)std::cerr<<" 0x"<<std::hex<<regions[n].first<<std::dec<<":"<<regions[n].second.first<<"/"<<regions[n].second.second<<"B";
            std::cerr<<'\n';
        }catch(...){}
    }
    // Relocation consumption diagnostics (off by default).
    // HALO2_RELOCATION_CHANGE_CENSUS=1 prints, every 120 frames, the bytes the
    // relocation changed (first journaled original vs current) per memory
    // region owner. HALO2_EXPERIMENT_RELOCATION_RESTORE=a,b,... puts the
    // original bytes back in every region whose owner contains one of the
    // substrings ("*" = all), keeping the scope's own root/sphere words, so a
    // frame comparison shows whether rendering consumes those changes.
    void relocation_diagnostics() {
        static const char *census_env=std::getenv("HALO2_RELOCATION_CHANGE_CENSUS");
        static const char *restore_env=std::getenv("HALO2_EXPERIMENT_RELOCATION_RESTORE");
        const bool census_on=census_env && *census_env,restore_on=restore_env && *restore_env;
        if(!census_on && !restore_on)return;
        static unsigned frames=0;const bool print=census_on && frames++%120u==0;
        std::size_t persistent_changed=0;std::map<std::uint32_t,unsigned> persistent_sites;
        {std::set<std::uint32_t> first;
         for(const auto &[at,before]:persistent_before) {
            const auto now=memory.view(at,before.size());
            for(std::size_t i=0;i<before.size();++i)if(first.insert(at+std::uint32_t(i)).second && now[i]!=before[i]){++persistent_changed;++persistent_sites[(at+std::uint32_t(i))&~0xfffu];}
         }}
        struct Owner{std::uint32_t first,end;std::string name;};
        std::vector<Owner> owners;
        memory.visit_region_metadata([&](const auto &r){owners.push_back({r.address,std::uint32_t(r.address+r.bytes),std::string(r.owner)});});
        std::sort(owners.begin(),owners.end(),[](const auto &a,const auto &b){return a.first<b.first;});
        const auto owner_of=[&](std::uint32_t a)->const Owner*{
            auto it=std::upper_bound(owners.begin(),owners.end(),a,[](auto v,const auto &o){return v<o.first;});
            if(it==owners.begin())return nullptr;--it;return a<it->end?&*it:nullptr;};
        std::vector<std::string> wanted;
        if(restore_on){std::string all(restore_env);std::size_t at=0;
            while(at<=all.size()){auto comma=all.find(',',at);if(comma==std::string::npos)comma=all.size();
                if(comma>at)wanted.push_back(all.substr(at,comma-at));at=comma+1;}}
        const auto selected=[&](const Owner *o){
            if(!o)return false;for(const auto &w:wanted)if(w=="*"||o->name.find(w)!=std::string::npos)return true;return false;};
        std::vector<std::pair<std::uint32_t,std::uint32_t>> keep;
        for(const auto &[owner,pose]:poses) {
            const auto body=world.source_object_body(owner);
            keep.push_back({body+0x30u,body+0x40u});
            if(word(memory,body+0x14u)==~0u){keep.push_back({body+0x64u,body+0x88u});keep.push_back({body+0xa0u,body+0xa4u});}
        }
        const auto kept=[&](std::uint32_t a){for(const auto &[f,e]:keep)if(a>=f && a<e)return true;return false;};
        std::set<std::uint32_t> seen;std::map<std::string,std::pair<std::size_t,std::size_t>> changed;
        std::vector<std::pair<std::uint32_t,std::byte>> put_back;
        for(const auto &st:std::span<const Store>(stores).subspan(stores_base)) {
            const auto now=memory.view(st.address,st.size);
            for(std::size_t i=0;i<st.size;++i) {
                const auto a=st.address+std::uint32_t(i);
                if(!seen.insert(a).second)continue;
                const auto before=arena[st.offset+i];
                const auto *o=owner_of(a);
                auto &c=changed[o?o->name:std::string("<none>")];++c.second;
                if(now[i]==before)continue;
                ++c.first;
                if(restore_on && !kept(a) && selected(o))put_back.push_back({a,before});
            }
        }
        // HALO2_EXPERIMENT_RELOCATION_POISON=<hex byte> writes that byte
        // instead of the original: any read of a selected change then shows.
        static const int poison=[]{const auto *e=std::getenv("HALO2_EXPERIMENT_RELOCATION_POISON");
            return e && *e?int(std::strtoul(e,nullptr,16)&0xffu):-1;}();
        for(const auto &[a,b]:put_back) {
            const auto value=poison<0?b:std::byte(poison);
            memory.write(a,std::span<const std::byte>(&value,1));
        }
        if(print) {
            std::vector<std::pair<std::string,std::pair<std::size_t,std::size_t>>> rows(changed.begin(),changed.end());
            std::sort(rows.begin(),rows.end(),[](const auto &x,const auto &y){return x.second.first>y.second.first;});
            std::cerr<<"Relocation change census: objects="<<poses.size()<<" restored="<<put_back.size()
                <<" persistent_writes="<<persistent_writes<<" ("<<persistent_bytes<<" B)"
                <<" persistent_changed_bytes="<<persistent_changed
                <<" moved_roots="<<moved_roots<<" unmoved_roots="<<unmoved_roots;
            for(const auto &[page,n]:persistent_sites)std::cerr<<" pchg@0x"<<std::hex<<page<<std::dec<<"="<<n;
            for(const auto &[name,c]:rows)std::cerr<<" | "<<name<<" changed="<<c.first<<"/"<<c.second;
            std::cerr<<'\n';
        }
    }
    void restore()noexcept {
        census();
        try{for(auto it=stores.rbegin();it!=stores.rend()-std::ptrdiff_t(stores_base);++it)
                memory.write(it->address,std::span<const std::byte>(arena).subspan(it->offset,it->size));
            stores.resize(stores_base);arena.resize(arena_base);}
        catch(...){std::terminate();}
    }
    ~Impl(){if(executor!=std::this_thread::get_id())std::terminate();binding.reset();restore();}
};
SourcePresentedModelPoseScopeTu15::SourcePresentedModelPoseScopeTu15(PhysicsLiveWorldTu15 &world,
    std::span<const SourcePresentedModelPoseTu15> poses):impl_(std::make_unique<Impl>(world)) {
    auto &p=*impl_;
    SourceExecutionCallScope journal(&p.memory,SourceExecutionCallKind::BeginPresentedModelPoses,
        [&]{return source_execution_arguments(poses);});
    try {
    world.require_presentation_boundary();validate_source_presented_model_poses_tu15(poses);
    if(source_has_presented_model_pose_tu15(p.memory))throw FormatError("Native presented WORLD inputs already have a frame owner");
    p.frame=word(p.memory,0x4c6854u)&0x3fffffffu;
    const auto metadata=[&](auto tag) {
        if(tag==~0u)throw FormatError("Presented WORLD input lacks an authored model owner");
        return word(p.memory,word(p.memory,0x4f0414u)+(tag&65535u)*16u+8u);
    };
    for(const auto &pose:poses) {
        const auto body=world.source_object_body(pose.object);
        if(!p.poses.emplace(pose.object,pose).second || word(p.memory,body)!=pose.definition ||
           !finite(pose.root) || !std::isfinite(pose.sphere.center.x) || !std::isfinite(pose.sphere.center.y) ||
           !std::isfinite(pose.sphere.center.z) || !std::isfinite(pose.sphere.radius) || pose.sphere.radius<0 ||
           pose.nodes.empty() || pose.nodes.size()>source_presented_model_node_capacity_tu15 ||
           !std::ranges::all_of(pose.nodes,[](const auto &node){return finite(node);}))
            throw FormatError("Presented WORLD input lost its full-salt model/root/pose owner");
        const auto model=word(p.memory,metadata(word(p.memory,metadata(pose.definition)+0x38u))+4u);
        const auto bytes=std::bit_cast<std::int16_t>(std::uint16_t(word(p.memory,body+0x114u,2)));
        const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(p.memory,body+0x116u,2)));
        const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return p.memory.view(at,n);}};
        const auto header=source_raw_datum_lookup(word(p.memory,0x4ecbd0u),pose.object,storage);
        if(model!=pose.render_model || word(p.memory,metadata(model)+0x48u)!=pose.nodes.size() ||
           bytes!=pose.nodes.size()*sizeof(Transform) || offset<0 ||
           std::uint32_t(offset)+std::uint32_t(bytes)>word(p.memory,header+6u,2))
            throw FormatError("Presented WORLD bank differs from the actual native model hierarchy");
        p.memory.validate_mutable_view(body+std::uint32_t(offset),std::size_t(bytes));
        p.memory.validate_mutable_view(body+0x30u,16);
        const auto token=word(p.memory,body+0xccu);
        if(token!=~0u && (token&65535u)<256) {
            const auto pool=word(p.memory,0x515f14u);
            const auto record=word(p.memory,pool+0x44u)+(token&65535u)*256u;
            if(word(p.memory,record+4u)==pose.object && word(p.memory,record+0x4cu)!=~0u &&
               (word(p.memory,record+0x50u)&0x3fffffffu)==p.frame)
                p.already_built.insert(pose.object);
        }
    }
    // BF640 may publish through the same WORLD array while the temporary root
    // is installed. Restore that complete simulation input at scope end too.
    for(const auto &[owner,pose]:p.poses) {
        const auto body=world.source_object_body(owner);
        const auto at=body+word(p.memory,body+0x116u,2);
        const auto original=p.memory.view(at,pose.nodes.size()*sizeof(Transform));
        if(original.size()>16u*1024u*1024u-p.bytes)
            throw FormatError("Presented WORLD input exceeds its bounded original-node journal");
        p.record(at,original);
    }
    // Observe only the adoption calls, not the later native render work. Thus
    // source cache/selection updates persist while temporary position/spatial
    // state returns exactly to the simulation endpoint after rendering.
    std::optional<SourcePresentationResourceRegionsTu15> resources;
    if(!poses.empty())resources.emplace(p.memory);
    source_runtime_register_topology_guard(p.memory,&p,[](auto,auto){
        throw FormatError("Presented WORLD adoption cannot change source memory topology");});
    try {
        source_runtime_register_write_observer(p.memory,&p,[&p,&resources](auto at,auto n) {
            // The world's real source caller stack/argument arena is reused
            // by subsequent render kernels. It is not simulation input and
            // must retain their later uncleared caller residue at frame end.
            if(at>=0x60000000u && std::uint64_t(at)+n<=0x60100000ull)return;
            // B95D0 can admit/evict animation resources. Later rendering uses
            // the same owners, so an older partial cache rollback is invalid.
            if(resources&&resources->persistent(at,n)){
                ++p.persistent_writes;p.persistent_bytes+=n;
                static const bool census=[]{const auto *e=std::getenv("HALO2_RELOCATION_CHANGE_CENSUS");return e && *e;}();
                if(census){const auto b=p.memory.view(at,n);p.persistent_before.push_back({at,std::vector<std::byte>(b.begin(),b.end())});}
                return;
            }
            if(n>16u*1024u*1024u-p.bytes)throw FormatError("Presented WORLD adoption exceeds its bounded store journal");
            p.record(at,p.memory.view(at,n));
        });
        try {
            // Diagnostic experiment (off by default): install the presented
            // root position words directly instead of running the original
            // relocation B95D0/B9460. Frame parity against the normal route
            // shows whether the render kernels consume anything else from the
            // relocation (cluster membership, attachment moves, node rebuild).
            static const bool present_without_relocation=[]{
                const auto *e=std::getenv("HALO2_EXPERIMENT_PRESENT_WITHOUT_RELOCATION");return e && *e;}();
            // Presented roots normally differ from their simulation placement
            // only by presentation rounding (static crates, scenery, items:
            // below 1e-4 in every position/forward/up/scale component). For
            // such a root without children or attachments, B95D0 would only
            // disconnect/reconnect the same cluster set, remove and re-add its
            // Havok bodies and rebuild nodes/spheres that the presented
            // binding and sphere publication overwrite, and the journal rolls
            // everything back at scope end (its resource-owner stores rewrite
            // identical bytes). Install the shown root position directly for
            // those roots. Moving roots, roots with children (riders, held
            // weapons) and roots with attachments keep the original relocation.
            // Evidence (2026-11-07, Zanzibar): restoring every relocation store
            // except root/sphere words, and skipping the relocation for every
            // root, were frame-identical on moving, mid-tick and endpoint
            // captures. HALO2_REFERENCE_PRESENTED_RELOCATION=1 relocates every
            // root as before.
            static const bool reference_relocation=[]{
                const auto *e=std::getenv("HALO2_REFERENCE_PRESENTED_RELOCATION");return e && *e;}();
            const auto near=[&](std::uint32_t at,const auto &value) {
                constexpr unsigned count=sizeof(value)/sizeof(float);
                std::array<float,count> shown,now;
                std::memcpy(shown.data(),&value,sizeof(value));
                std::memcpy(now.data(),p.memory.view(at,sizeof(value)).data(),sizeof(value));
                for(unsigned n=0;n<count;++n)if(!(std::fabs(now[n]-shown[n])<=1e-4f))return false;
                return true;
            };
            for(const auto &[owner,pose]:p.poses) {
                const auto body=world.source_object_body(owner);
                if(word(p.memory,body+0x14u)==~0u) {
                    // A root whose shown placement is bit-identical to its
                    // simulation placement is exactly where TU1.5 renders it
                    // (the original never relocates before rendering); B95D0
                    // to the same placement would only rebuild its Havok
                    // component and re-prepend its cluster references. This
                    // holds with children and attachments too (e.g. the
                    // welded, attachment-carrying Zanzibar crates whose 0x400
                    // full Havok rebuild was 43% of all relocation time).
                    const auto same=[&](std::uint32_t at,const auto &value) {
                        return std::memcmp(p.memory.view(at,sizeof(value)).data(),&value,sizeof(value))==0;
                    };
                    const bool exact=same(body+0x64u,pose.root.position) && same(body+0x70u,pose.root.basis[0]) &&
                        same(body+0x7cu,pose.root.basis[2]) && same(body+0xa0u,pose.root.scale);
                    const bool unmoved=!reference_relocation && (exact || (word(p.memory,body+0x10u)==~0u &&
                        word(p.memory,body+0x11cu,2)==0u && near(body+0x64u,pose.root.position) &&
                        near(body+0x70u,pose.root.basis[0]) && near(body+0x7cu,pose.root.basis[2]) &&
                        near(body+0xa0u,pose.root.scale)));
                    if(unmoved)++p.unmoved_roots;else ++p.moved_roots;
                    static const bool explain=[]{const auto *e=std::getenv("HALO2_RELOCATION_MOVED_DIAGNOSTICS");return e && *e;}();
                    if(explain && !unmoved) {
                        static unsigned printed=0;
                        if(printed<400) {
                            ++printed;
                            const auto f=[&](std::uint32_t at){float v;std::memcpy(&v,p.memory.view(at,4).data(),4);return v;};
                            std::cerr<<"Relocation moved root: object="<<std::hex<<owner<<std::dec<<" type="<<word(p.memory,body+0xaau,1)
                                <<" frame="<<p.frame<<std::setprecision(9)
                                <<" pos="<<f(body+0x64)<<","<<f(body+0x68)<<","<<f(body+0x6c)
                                <<" shown="<<pose.root.position.x<<","<<pose.root.position.y<<","<<pose.root.position.z
                                <<" fwd="<<f(body+0x70)<<","<<f(body+0x74)<<","<<f(body+0x78)
                                <<" shown="<<pose.root.basis[0].x<<","<<pose.root.basis[0].y<<","<<pose.root.basis[0].z
                                <<" up="<<f(body+0x7c)<<","<<f(body+0x80)<<","<<f(body+0x84)
                                <<" shown="<<pose.root.basis[2].x<<","<<pose.root.basis[2].y<<","<<pose.root.basis[2].z
                                <<" scale="<<f(body+0xa0)<<" shown="<<pose.root.scale
                                <<" child="<<std::hex<<word(p.memory,body+0x10u)<<std::dec<<" attach="<<word(p.memory,body+0x11cu,2)
                                <<" sphere="<<f(body+0x30)<<","<<f(body+0x34)<<","<<f(body+0x38)<<"r"<<f(body+0x3c)
                                <<" shown="<<pose.sphere.center.x<<","<<pose.sphere.center.y<<","<<pose.sphere.center.z<<"r"<<pose.sphere.radius
                                <<" cluster="<<f(body+0x40)<<","<<f(body+0x44)<<","<<f(body+0x48)<<"r"<<f(body+0x4c)<<'\n';
                        }
                    }
                    static const bool path_census=[]{const auto *e=std::getenv("HALO2_RELOCATION_PATH_CENSUS");return e && *e;}();
                    if(present_without_relocation || unmoved)
                        p.memory.write(body+0x64u,std::as_bytes(std::span(&pose.root.position,1)));
                    else if(path_census) {
                        const auto kind=relocation_path_class(p.memory,body);
                        const auto started=std::chrono::steady_clock::now();
                        world.set_object_position(owner,pose.root.position);
                        relocation_path_record(kind,std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-started).count(),false);
                    }
                    else world.set_object_position(owner,pose.root.position);
                    p.memory.write(body+0x70u,std::as_bytes(std::span(&pose.root.basis[0],1)));
                    p.memory.write(body+0x7cu,std::as_bytes(std::span(&pose.root.basis[2],1)));
                    p.memory.write(body+0xa0u,std::as_bytes(std::span(&pose.root.scale,1)));
                }
            }
            // Parent movement may refresh descendant spheres. Publish every
            // exact shown sphere afterward, independently of input/salt order.
            for(const auto &[owner,pose]:p.poses) {
                const auto body=world.source_object_body(owner);
                p.memory.write(body+0x30u,std::as_bytes(std::span(&pose.sphere.center,1)));
                p.memory.write(body+0x3cu,std::as_bytes(std::span(&pose.sphere.radius,1)));
            }
        }catch(...){source_runtime_unregister_write_observer(p.memory,&p);throw;}
        source_runtime_unregister_write_observer(p.memory,&p);
        p.relocation_diagnostics();
        {static const bool path_census=[]{const auto *e=std::getenv("HALO2_RELOCATION_PATH_CENSUS");return e && *e;}();
         if(path_census)relocation_path_record({},0,true);}
    }catch(...){source_runtime_unregister_topology_guard(p.memory,&p);throw;}
    source_runtime_unregister_topology_guard(p.memory,&p);
    p.binding.reset(new SourcePresentedModelInputBindingTu15(p.memory,p.poses,p.frame,std::move(p.already_built)));
    }catch(...){p.restore();throw;}
}
SourcePresentedModelPoseScopeTu15::~SourcePresentedModelPoseScopeTu15()noexcept(false) {
    if(!impl_)return;
    if(std::uncaught_exceptions()>impl_->exceptions){
        SourceExecutionFailedCleanupScope cleanup(impl_->memory);impl_.reset();return;
    }
    try {
        SourceExecutionCallScope journal(&impl_->memory,SourceExecutionCallKind::EndPresentedModelPoses,
            []{return std::vector<std::byte>{};});
        impl_.reset();
    }catch(...){SourceExecutionFailedCleanupScope cleanup(impl_->memory);impl_.reset();throw;}
}
}
