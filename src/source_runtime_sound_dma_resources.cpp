#include "halo2/source_runtime_sound_dma_resources.hpp"
#include <algorithm>
#include <array>
#include <exception>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p) {
    const auto raw=memory.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
    return value;
}
void constant(SourceRuntimeMemory& memory,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));
    memory.mount(p,raw,SourceRuntimeMemory::Access::ReadOnly,"source sound SDK physical resource import identity");
}
}
SourceRuntimeSoundDmaResources::SourceRuntimeSoundDmaResources(SourceRuntimeMemory& memory,std::shared_ptr<SdlSoundPlatform> output)
    :memory_(memory),output_(std::move(output)),physical_(memory) {
    if(!output_||!output_->device_id()||output_->output_channels()!=2)
        throw std::invalid_argument("Sound DMA resources require an actual retained supported stereo output");
    device_id_=output_->device_id();
}
SourceRuntimeSoundDmaResources::~SourceRuntimeSoundDmaResources()=default;
void SourceRuntimeSoundDmaResources::require_owner()const {
    if(std::this_thread::get_id()!=thread_)
        throw std::runtime_error("Sound DMA resource requires its creating owner thread");
    if(fault_||!output_||output_->device_id()!=device_id_||output_->output_channels()!=2)
        throw std::runtime_error("Sound DMA resource owner is unavailable, faulted or on another thread");
}
const SourceRuntimeSoundDmaResources::Allocation& SourceRuntimeSoundDmaResources::resolve(std::uint32_t source)const {
    require_owner();const auto found=allocations_.find(source);
    if(found==allocations_.end())throw std::invalid_argument("Sound DMA allocation is not owned by this device");
    const auto& row=found->second;
    if(memory_.native_address(source,row.capacity)!=row.native)
        throw std::runtime_error("Sound DMA resource lost its actual native backing");
    memory_.validate_mutable_view(source,row.capacity);return row;
}
bool SourceRuntimeSoundDmaResources::dispatch(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,
    SourcePhysicsWorldRuntimeAbi& abi) {
    if(dispatch_gp_scratch_translation(bank,call,abi)||dispatch_gp_pool(bank,call,abi))return true;
    const bool allocate=call.instruction==0x3879dc,capacity=call.instruction==0x3879e9,address=call.instruction==0x38e74b;
    if(!allocate&&!capacity&&!address)return false;
    const auto import=allocate?0x41b640u:capacity?0x41b644u:0x41b7bcu;
    const auto expected=allocate?0x800000a6u:capacity?0x800000b4u:0x800000adu;
    if(&bank!=&memory_)throw std::invalid_argument("Sound DMA request bank ownership differs");
    require_owner();
    if(call.tail||call.target!=expected||word(memory_,import)!=expected)
        throw std::invalid_argument("Sound DMA request bank, import or call identity differs");
    const auto sp=abi.gp[4];
    if(word(memory_,sp)!=call.instruction+6)
        throw std::invalid_argument("Sound DMA kernel return frame differs");
    if(allocate) {
        if(word(memory_,sp+28)!=0x38e734)
            throw std::invalid_argument("Sound DMA allocation lacks its original38E71C parent");
        const SourcePhysicalAllocationRequest request{word(memory_,sp+4),word(memory_,sp+8),word(memory_,sp+12),word(memory_,sp+16),word(memory_,sp+20)};
        const auto descriptor=abi.gp[7];memory_.validate_mutable_view(descriptor,12);
        if(request.lowest||request.highest!=0xffffffffu||request.protection!=4||word(memory_,descriptor)||
            word(memory_,descriptor+8)!=request.bytes)
            throw std::invalid_argument("Sound DMA allocation differs from the original descriptor request");
        // Native namespace policy: the retained game/renderer arena occupies
        // 81000000..85000000. Choose sound pages above that actual owner,
        // within the original unrestricted request. This is an allocation
        // choice, never a recovered Xbox address or modified source frame.
        auto native_request=request;native_request.lowest=0x05000000;
        const auto source=physical_.allocate(native_request);
        if(!source)throw std::runtime_error("Native sound DMA resource namespace is exhausted");
        try {
            const auto ownership=physical_.ownership_checkpoint();
            const auto found=std::ranges::find_if(ownership.allocations,[&](const auto& row){return row.base==source;});
            if(found==ownership.allocations.end())throw std::runtime_error("Native sound DMA allocation has no physical owner");
            const auto native=memory_.native_address(source,found->bytes);
            if(native.value<=0xffffffffu)throw std::runtime_error("Native sound DMA backing is not native64");
            if(!allocations_.emplace(source,Allocation{source,source&0x7fffffffu,request.bytes,found->bytes,
                request.alignment,request.protection,descriptor,native}).second)
                throw std::runtime_error("Native sound DMA resource identity collision");
        }catch(...){fault_=true;throw;}
        abi.gp[0]=source;abi.gp[4]+=24;return true;
    }
    const auto source=word(memory_,sp+4);const auto& row=resolve(source);
    if(capacity) {
        if(word(memory_,sp+12)!=0x38e734||source!=abi.gp[6])
            throw std::invalid_argument("Sound DMA capacity request lacks its original allocation caller");
        abi.gp[0]=row.capacity;
    }else {
        if(abi.gp[7]!=row.source_descriptor||word(memory_,row.source_descriptor)!=source)
            throw std::invalid_argument("Sound DMA physical-address request lost its source descriptor");
        abi.gp[0]=row.physical;
    }
    abi.gp[4]+=8;return true;
}
std::vector<SourceRuntimeSoundDmaResources::Allocation> SourceRuntimeSoundDmaResources::allocations()const {
    require_owner();std::vector<Allocation> rows;rows.reserve(allocations_.size());
    for(const auto& [source,row]:allocations_){(void)resolve(source);rows.push_back(row);}return rows;
}
NativeRuntimeAddress SourceRuntimeSoundDmaResources::native_address(std::uint32_t source)const{return resolve(source).native;}
std::uint32_t SourceRuntimeSoundDmaResources::capacity(std::uint32_t source)const{return resolve(source).capacity;}
std::uint32_t SourceRuntimeSoundDmaResources::device_id()const{require_owner();return output_->device_id();}
bool SourceRuntimeSoundDmaResources::quiescent()const noexcept{return std::this_thread::get_id()==thread_&&!fault_&&allocations_.empty()&&gp_pools_.empty();}
SourceRuntimeSoundDmaResources::CallScope::CallScope(SourceRuntimeSoundDmaResources* owner)
    :owner_(owner),count_(0),exceptions_(std::uncaught_exceptions()) {
    if(owner_){owner_->require_owner();count_=owner_->allocations_.size()+owner_->gp_pools_.size();gp_epoch_=owner_->gp_pool_epoch_;}
}
SourceRuntimeSoundDmaResources::CallScope::~CallScope() {
    // Partial original descriptor/statistics writes are diagnostic. Retain
    // their actual pages until owner teardown and reject reuse; no fabricated
    // SDK rollback, release result or hardware readiness is supplied.
    if(owner_&&std::uncaught_exceptions()>exceptions_&&
       (owner_->allocations_.size()+owner_->gp_pools_.size()!=count_||owner_->gp_pool_epoch_!=gp_epoch_))owner_->fault_=true;
}
void mount_native_tu15_sound_dma_imports(SourceRuntimeMemory& memory) {
    constant(memory,0x41b644,0x800000b4);constant(memory,0x41b7bc,0x800000ad);
}
#include "source_runtime_sound_gp_pool.inc"
#include "source_runtime_sound_gp_scratch.inc"
#include "source_runtime_sound_ep_scratch.inc"
}
