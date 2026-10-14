#include "halo2/source_projectile_effect_observer.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_effect_world_locations.hpp"
#include <atomic>
#include <bit>
#include <map>
#include <mutex>
#include <cstdlib>
#include <iostream>
namespace halo2 {
struct SourceProjectileEffectListener {
    SourceRuntimeMemory *memory{};
    std::function<void(const SourceProjectileDetonationTu15 &)> callback;
    std::atomic_bool active{true};
};
namespace {
std::mutex registry_mutex;
std::map<const SourcePhysicsContactMemory *,std::weak_ptr<SourceProjectileEffectListener>> registry;
thread_local std::map<const SourcePhysicsContactMemory *,std::vector<SourceProjectileContactCapture>> updates;
std::uint32_t word(SourcePhysicsContactMemory &m,std::uint32_t p) {
    std::array<std::byte,4> bytes;m.read(p,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);return value;
}
std::uint32_t record(SourceRuntimeMemory &m,std::uint32_t pool,std::uint32_t datum) {
    SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return m.view(p,n);}};
    return source_raw_datum_lookup(pool,datum,storage);
}
}
SourceProjectileUpdateScope::SourceProjectileUpdateScope(SourcePhysicsContactMemory &m,std::uint32_t projectile) {
    if(std::getenv("HALO2_TRACE_SOURCE_CONTACT_EFFECTS"))std::cerr<<"Projectile update="<<std::hex<<projectile<<std::dec<<'\n';
    std::shared_ptr<SourceProjectileEffectListener> listener;
    {std::lock_guard lock(registry_mutex);const auto at=registry.find(&m);
     if(at==registry.end() || !(listener=at->second.lock()) || !listener->active)return;}
    const auto pool=word(m,0x4ecbd0u),header=record(*listener->memory,pool,projectile);
    if(!header || listener->memory->view(header+3u,1)[0]!=std::byte{5})
        throw FormatError("Original FDEB0 update lost its actual projectile");
    const auto body=word(m,header+8u);if(!body)throw FormatError("Original projectile update has no body");
    if(std::getenv("HALO2_TRACE_SOURCE_CONTACT_EFFECTS")) {
        std::cerr<<"Projectile definition="<<std::hex<<word(m,body)<<" flags="<<word(m,body+0x12cu)<<std::dec;
        for(const auto offset:{0x64u,0x68u,0x6cu,0x88u,0x8cu,0x90u,0x168u})
            std::cerr<<' '<<std::bit_cast<float>(word(m,body+offset));
        std::cerr<<'\n';
    }
    updates[&m].push_back({projectile,word(m,body),0xffffffffu,word(m,body+0xc4u),word(m,body+0xc8u),listener});
    memory_=&m;
}
SourceProjectileUpdateScope::~SourceProjectileUpdateScope() {
    if(!memory_)return;auto &stack=updates.at(memory_);stack.pop_back();if(stack.empty())updates.erase(memory_);
}
SourceProjectileEffectObserver::SourceProjectileEffectObserver(SourceRuntimeMemory &m,
    std::function<void(const SourceProjectileDetonationTu15 &)> callback) {
    if(!callback)throw FormatError("Source projectile effect observer requires a callback");
    std::lock_guard lock(registry_mutex);
    if(!registry[&m].expired())throw FormatError("Source world already has a projectile contact observer");
    listener_=std::make_shared<SourceProjectileEffectListener>();listener_->memory=&m;
    listener_->callback=std::move(callback);registry[&m]=listener_;
}
SourceProjectileEffectObserver::~SourceProjectileEffectObserver() {
    if(!listener_)return;listener_->active=false;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(listener_->memory);
    if(at!=registry.end() && at->second.lock()==listener_)registry.erase(at);
}
std::optional<SourceProjectileContactCapture> source_begin_projectile_contact_effect_tu15(
    SourcePhysicsContactMemory &m,std::uint32_t frame,std::uint32_t sp,std::uint32_t site) {
    if(std::getenv("HALO2_TRACE_SOURCE_CONTACT_EFFECTS"))std::cerr<<"Contact begin site="<<std::hex<<site<<" request="<<word(m,sp)<<std::dec<<'\n';
    std::shared_ptr<SourceProjectileEffectListener> listener;
    {std::lock_guard lock(registry_mutex);const auto at=registry.find(&m);
     if(at==registry.end() || !(listener=at->second.lock()) || !listener->active)return {};}
    if(site==0x10098fu || site==0x100a19u || site==0x100c1cu || site==0x179520u) {
        const auto at=updates.find(&m);if(at==updates.end() || at->second.empty())return {};
        auto capture=at->second.back();capture.effect=word(m,word(m,sp)+4u);return capture;
    }
    // Original101330 keeps EBP as its actual projectile body.101EA0 saves
    // that value below its own frame, with the authenticated1019C8 caller.
    if(word(m,frame+4u)!=0x1019cdu)return {};
    const auto body=word(m,frame),request=word(m,sp),pool=word(m,0x4ecbd0u);
    if(word(m,request)!=4u || word(m,request+0x20u)!=6u || word(m,pool+0x24u)!=12u)
        throw FormatError("Source projectile contact request differs from101EA0");
    auto &bank=*listener->memory;const auto headers=word(m,pool+0x44u),high=word(m,pool+0x38u);
    if(high>word(m,pool+0x20u))throw FormatError("Source projectile contact pool extent differs");
    for(unsigned i=0;i<high;++i) {
        if(!(word(m,word(m,pool+0x48u)+(i>>5)*4u)&(1u<<(i&31u))))continue;
        const auto header=headers+i*12u;
        if(word(m,header+8u)!=body || bank.view(header+3u,1)[0]!=std::byte{5})continue;
        return SourceProjectileContactCapture{((word(m,header)&65535u)<<16)|i,
            word(m,body),word(m,request+4u),word(m,body+0xc4u),word(m,body+0xc8u),listener};
    }
    throw FormatError("Source projectile contact lost its live full body owner");
}
void source_complete_projectile_contact_effect_tu15(SourcePhysicsContactMemory &m,
    const SourceProjectileContactCapture &capture,std::uint32_t instance) {
    if(std::getenv("HALO2_TRACE_SOURCE_CONTACT_EFFECTS"))std::cerr<<"Contact complete effect="<<std::hex<<capture.effect<<" instance="<<instance<<std::dec<<'\n';
    if(instance==0xffffffffu || !capture.listener->active)return;
    auto &bank=*capture.listener->memory;
    const auto effect=record(bank,word(m,0x4f720cu),instance);
    if(!effect || word(m,effect+4u)!=capture.effect)
        throw FormatError("Source contact effect lost its completed allocation");
    SourceProjectileDetonationTu15 event{capture.projectile,capture.definition,capture.effect,
        instance,capture.player,capture.unit,{},{},{}};
    const auto scalar=[&](auto p){return std::bit_cast<float>(word(m,p));};
    event.colors={word(m,effect+0x104u),word(m,effect+0x108u)};
    event.controls={scalar(effect+0x6cu),scalar(effect+0x70u)};
    event.effect_locations=source_effect_world_locations_tu15(bank,instance,capture.effect);
    if(!event.effect_locations.empty())capture.listener->callback(event);
}
}
