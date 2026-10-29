#include "halo2/source_presented_model_pose_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_presentation_resource_regions_tu15.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <map>
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
struct Store {std::uint32_t address;std::vector<std::byte> before;};
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
    std::vector<Store> stores;std::size_t bytes{};
    explicit Impl(PhysicsLiveWorldTu15 &w):world(w),memory(w.memory()){}
    void restore()noexcept {
        try{for(auto it=stores.rbegin();it!=stores.rend();++it)memory.write(it->address,it->before);stores.clear();}
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
        p.stores.push_back({at,{original.begin(),original.end()}});p.bytes+=original.size();
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
            const auto b=p.memory.view(at,n);p.stores.push_back({at,{b.begin(),b.end()}});p.bytes+=n;
        });
        try {
            for(const auto &[owner,pose]:p.poses) {
                const auto body=world.source_object_body(owner);
                if(word(p.memory,body+0x14u)==~0u) {
                    world.set_object_position(owner,pose.root.position);
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
