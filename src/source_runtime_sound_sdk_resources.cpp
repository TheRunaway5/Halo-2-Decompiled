#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_sound_sdk_pool.hpp"
#include <array>
#include <exception>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
namespace halo2 {namespace {
constexpr std::uint32_t sound_pool_first=0x18000000u;
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b;m.read(p,b);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
}
struct SourceRuntimeSoundSdkResources::Impl {
    SourceRuntimeMemory &bank;
    std::uint32_t next=sound_pool_first;
    struct Allocation {std::uint32_t region,bytes,capacity;};
    const std::uint32_t fs_base;
    const std::thread::id thread=std::this_thread::get_id();
    std::recursive_mutex critical;
    unsigned locks{};
    std::map<std::uint32_t,Allocation> allocations;
    bool fault{};
    std::shared_ptr<const SourceTu15UserSettings> settings;
    Impl(SourceRuntimeMemory&m,std::uint32_t fs):bank(m),fs_base(fs){
        if(!fs||fs>0xffffffdbu)throw FormatError("Native sound passive-thread context is unavailable");
        // This executor is a native user thread, with no interrupt/raised-IRQL
        // domain. Bind its passive level explicitly; elevated domains stop.
        const std::array passive{std::byte{0}};bank.write(fs+0x24u,passive);
    }
    void require_thread(const SourcePhysicsWorldRuntimeAbi&a)const{
        if(fault||std::this_thread::get_id()!=thread||a.fs_base!=fs_base||bank.view(fs_base+0x24u,1)[0]!=std::byte{0})
            throw FormatError("Native SDK resource requires its owned passive execution context");
    }
};
SourceRuntimeSoundSdkResources::SourceRuntimeSoundSdkResources(SourceRuntimeMemory&m,std::uint32_t fs):impl_(std::make_unique<Impl>(m,fs)){}
SourceRuntimeSoundSdkResources::~SourceRuntimeSoundSdkResources(){abort_new_resources(0,0);}
std::size_t SourceRuntimeSoundSdkResources::live_allocations()const noexcept{return impl_->allocations.size();}
unsigned SourceRuntimeSoundSdkResources::lock_depth()const noexcept{return impl_->locks;}
bool SourceRuntimeSoundSdkResources::quiescent()const noexcept{
    if(impl_->fault||!impl_->allocations.empty()||impl_->locks||impl_->next!=sound_pool_first||std::this_thread::get_id()!=impl_->thread)return false;
    try{return impl_->bank.view(impl_->fs_base+0x24u,1)[0]==std::byte{0};}catch(...){return false;}
}
void SourceRuntimeSoundSdkResources::require_quiescent()const{if(!quiescent())throw FormatError("Live/faulted SDK factory resources cannot cross a world checkpoint");}
void SourceRuntimeSoundSdkResources::abort_new_resources(std::uint32_t cursor,unsigned locks)noexcept{
    bool changed=false;
    for(auto it=impl_->allocations.lower_bound(cursor);it!=impl_->allocations.end();){
        changed=true;try{impl_->bank.release(it->second.region);}catch(...){impl_->fault=true;}it=impl_->allocations.erase(it);
    }
    while(impl_->locks>locks){changed=true;impl_->critical.unlock();--impl_->locks;}
    // Original CPU partial object/global/accounting writes remain diagnostic.
    // Their source rollback is not invented; reject reuse/export after abort.
    if(changed)impl_->fault=true;
}
SourceRuntimeSoundSdkResources::CallScope::CallScope(SourceRuntimeSoundSdkResources*p):owner_(p),cursor_(p?p->impl_->next:0),locks_(p?p->lock_depth():0),exceptions_(std::uncaught_exceptions()){}
SourceRuntimeSoundSdkResources::CallScope::~CallScope(){if(owner_&&std::uncaught_exceptions()>exceptions_)owner_->abort_new_resources(cursor_,locks_);}
void SourceRuntimeSoundSdkResources::bind_user_settings(SourceTu15UserSettings settings){
    require_quiescent();impl_->settings=std::make_shared<const SourceTu15UserSettings>(std::move(settings));
}
bool SourceRuntimeSoundSdkResources::dispatch(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&observe){
    if(c.instruction==0x2e101cu && word(m,a.gp[4]+4u)==9u) {
        if(&m!=&impl_->bank)throw FormatError("Native audio settings query bank ownership differs");
        if(!impl_->settings)return false;
        impl_->require_thread(a);
        return source_try_tu15_audio_settings_query(impl_->bank,c,a,*impl_->settings,observe);
    }
    struct Request {std::uint32_t instruction,slot;unsigned words;};
    constexpr std::array requests{Request{0x383f92u,0x41b680u,1},Request{0x3878b0u,0x41b67cu,1},Request{0x3842cau,0x41b67cu,1},Request{0x384311u,0x41b67cu,1},Request{0x387975u,0x41b79cu,2},Request{0x387982u,0x41b7ecu,1}};
    const Request*r=nullptr;for(const auto&candidate:requests)if(c.instruction==candidate.instruction){r=&candidate;break;}if(!r)return false;
    if(&m!=&impl_->bank||c.tail||c.target!=word(m,r->slot)||word(m,a.gp[4])!=c.instruction+6u)throw FormatError("Native SDK platform call differs from its actual instruction/import/frame");
    impl_->require_thread(a);const auto sp=a.gp[4],arg=word(m,sp+4u);
    if(r->slot==0x41b680u){
        if(arg!=0x390bf8u)throw FormatError("Native SDK lock identity differs");impl_->critical.lock();++impl_->locks;
    }else if(r->slot==0x41b67cu){
        if(arg!=0x390bf8u||!impl_->locks)throw FormatError("Native SDK lock release has no owned acquisition");--impl_->locks;impl_->critical.unlock();
    }else if(r->slot==0x41b79cu){
        const auto tag=word(m,sp+8u);if(tag!=0x626f5344u&&tag!=0x61645344u)throw FormatError("Unsupported native SDK raw-pool tag");
        const auto layout=source_sound_sdk_pool_layout(arg);
        const auto base=(std::uint64_t(impl_->next)+layout.alignment-1u)&~std::uint64_t(layout.alignment-1u);
        const auto end=base+layout.backing_bytes;if(end>0xffffffffu)throw FormatError("Native SDK source-address storage exhausted");
        std::vector<std::byte> backing(layout.backing_bytes);
        // Private native allocation metadata describes the real owned extent.
        // Only the size byte consumed by the original small-block getter is
        // represented; Xbox previous-size/type/tag/free-list records are not
        // manufactured. Large allocations own an actual page-sized extent.
        if(layout.client_offset)backing[3]=std::byte(layout.backing_bytes>>5u);
        impl_->bank.mount(std::uint32_t(base),backing,SourceRuntimeMemory::Access::ReadWrite,"actual native sound SDK pool backing with recovered capacity");
        const auto pointer=std::uint32_t(base)+layout.client_offset;
        try{if(!impl_->allocations.emplace(pointer,Impl::Allocation{std::uint32_t(base),layout.backing_bytes,layout.client_capacity}).second)throw FormatError("Native SDK allocation identity collision");}catch(...){impl_->bank.release(std::uint32_t(base));throw;}
        impl_->next=std::uint32_t(end);a.gp[0]=pointer;
    }else{
        const auto found=impl_->allocations.find(arg);if(found==impl_->allocations.end())throw FormatError("Native SDK capacity query lacks actual owned allocation");
        const auto&block=found->second;(void)impl_->bank.view(block.region,block.bytes);
        auto capacity=block.capacity;
        if(arg&0xfffu){capacity=std::to_integer<std::uint32_t>(impl_->bank.view(arg-5u,1)[0])*32u-8u;}
        if(capacity!=block.capacity)throw FormatError("Native SDK pool size metadata lost its actual backing binding");a.gp[0]=capacity;
    }
    a.gp[4]+=4u*(r->words+1u);return true;
}
}
