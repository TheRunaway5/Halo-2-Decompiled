#include "halo2/source_presented_model_pose_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_presentation_resource_regions_tu15.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <iostream>
#include <optional>
#include <set>
#include <thread>

namespace halo2 {
namespace {
using Poses=std::map<std::uint32_t,SourcePresentedModelPoseTu15>;
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p,unsigned n=4) {
    const auto b=m.view(p,n);std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return v;
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
    std::vector<Store> stores;std::size_t bytes{};std::vector<std::byte> &arena=journal_arena();
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
            for(const auto &st:stores) {
                const auto a=st.address;bool classified=false;
                for(const auto &[owner,r]:bodies)if(a>=r.first && a<r.end){auto &f=field[a-r.first];++f.first;f.second+=st.size;++body_stores;body_bytes+=st.size;classified=true;break;}
                if(classified)continue;
                for(const auto &[owner,r]:banks)if(a>=r.first && a<r.end){++bank_stores;bank_bytes+=st.size;classified=true;break;}
                if(classified)continue;
                if(a>=0x80000000u){++physical;physical_bytes+=st.size;continue;}
                auto &g=region[a&~0xffffu];++g.first;g.second+=st.size;
            }
            std::cerr<<"Presented scope census: objects="<<poses.size()<<" stores="<<stores.size()<<" bytes="<<bytes
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
    void restore()noexcept {
        census();
        try{for(auto it=stores.rbegin();it!=stores.rend();++it)
                memory.write(it->address,std::span<const std::byte>(arena).subspan(it->offset,it->size));
            stores.clear();arena.resize(arena_base);}
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
            if(resources&&resources->persistent(at,n))return;
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
            for(const auto &[owner,pose]:p.poses) {
                const auto body=world.source_object_body(owner);
                if(word(p.memory,body+0x14u)==~0u) {
                    if(present_without_relocation)
                        p.memory.write(body+0x64u,std::as_bytes(std::span(&pose.root.position,1)));
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
