#include "halo2/source_runtime_sound_ac97_output_pages.hpp"
#include <SDL.h>
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){
    const auto bytes=memory.view(address,4);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);return value;
}
}
struct SourceRuntimeSoundAc97OutputPages::CpuMemory final:SourcePhysicsContactMemory{
    SourceRuntimeSoundAc97OutputPages& owner;
    explicit CpuMemory(SourceRuntimeSoundAc97OutputPages& value):owner(value){}
    void read(std::uint32_t address,std::span<std::byte> output)const override{
        owner.require_thread();if(owner.fault_||owner.control_.generation()!=owner.generation_)throw std::runtime_error("Faulted or retired-generation AC97 source view cannot execute another CPU read");
        if(owner.fs_&&address==owner.fs_+0x24&&output.size()==1){output[0]=std::byte(owner.source_level_);return;}
        owner.memory_.read(address,output);
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override{
        owner.require_thread();if(owner.fault_||owner.control_.generation()!=owner.generation_)throw std::runtime_error("Faulted or retired-generation AC97 source view cannot execute another CPU write");
        if(owner.fs_&&address==owner.fs_+0x24)throw std::runtime_error("AC97 source cannot replace its native exclusion domain");
        owner.memory_.write(address,input);
    }
};
SourceRuntimeSoundAc97OutputPages::SourceRuntimeSoundAc97OutputPages(SourceRuntimeMemory& memory,
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma,std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep,
    std::shared_ptr<SdlSoundPlatform> output,SourceRuntimeSoundResetResources::ControlLease control)
    :memory_(memory),dma_(std::move(dma)),ep_(std::move(ep)),output_(std::move(output)),control_(std::move(control)),
     physical_(memory),generation_(control_.generation()),device_(dma_?dma_->device_id():0),controller_(control_.source_apu()+0x1c){
    control_.require_bank(memory_);require_live();cpu_memory_=std::make_unique<CpuMemory>(*this);
}
SourceRuntimeSoundAc97OutputPages::~SourceRuntimeSoundAc97OutputPages(){
    // No callback consumer is published by this page prerequisite. Its actual
    // mappings and last EP/DMA/output leases retire on the creating executor.
    if(std::this_thread::get_id()!=thread_)std::terminate();
    if(source_level_)control_.abort_transaction();
}
void SourceRuntimeSoundAc97OutputPages::require_thread()const{
    if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native AC97 output pages require their creating owner thread");
}
void SourceRuntimeSoundAc97OutputPages::require_bank(const SourceRuntimeMemory& memory)const{
    require_thread();if(&memory!=&memory_)throw std::invalid_argument("Native AC97 page owner belongs to another source bank");
}
void SourceRuntimeSoundAc97OutputPages::require_control(const SourceRuntimeMemory& memory,const SourceRuntimeSoundResetResources::ControlLease& control)const{
    require_bank(memory);if(!control_.same_owner(control))throw std::invalid_argument("Native AC97 pages belong to another actual control worker");
    control_.require_bank(memory);
}
void SourceRuntimeSoundAc97OutputPages::require_live()const{
    require_thread();
    if(fault_||!dma_||!ep_||!output_||control_.generation()!=generation_||!generation_||
        dma_->device_id()!=device_||output_->device_id()!=device_||output_->output_channels()!=2||
        SDL_GetAudioDeviceStatus(device_)!=SDL_AUDIO_PLAYING)
        throw std::runtime_error("Native AC97 page prerequisite lost its actual output/control lifetime");
    const auto ep=ep_->diagnostics();
    if(ep.faulted||ep.retired||ep.output_device!=device_||ep.active_copies||ep.callback_leases)
        throw std::runtime_error("Native AC97 source construction requires quiescent unfaulted EP byte resources");
    const auto dma=dma_->allocations();
    if(dma.size()!=16)throw std::invalid_argument("Native AC97 pages require all sixteen actual DMA bindings");
    for(const auto& row:dma){
        if(memory_.native_address(row.source,row.capacity)!=row.native||
            word(memory_,row.source_descriptor)!=row.source||word(memory_,row.source_descriptor+4)!=row.physical||
            word(memory_,row.source_descriptor+8)!=row.requested)
            throw std::invalid_argument("Native AC97 DMA resources belong to another bank/backing");
    }
    for(const auto* expected:{&ep.layout.data,&ep.layout.scatter_gather}){
        const auto at=std::ranges::find_if(dma,[&](const auto& row){return row.source_descriptor==expected->source_descriptor;});
        if(at==dma.end()||at->source!=expected->source||at->physical!=expected->physical||at->capacity!=expected->capacity||
            at->requested!=expected->requested||at->native!=expected->native||
            memory_.native_address(expected->source,expected->capacity)!=expected->native)
            throw std::invalid_argument("Native AC97 EP data/SGE resources belong to another bank/backing");
    }
    memory_.validate_mutable_view(controller_,8);
}
unsigned SourceRuntimeSoundAc97OutputPages::validate_channel(std::uint32_t channel)const{
    memory_.validate_mutable_view(channel,0x30);const auto index=word(memory_,channel);
    if(index>=2||word(memory_,controller_+index*4)!=channel||word(memory_,channel+8)!=controller_||
        memory_.native_address(channel,0x30).value<=0xffffffffu)
        throw std::invalid_argument("Native AC97 pages lack a genuine retained source channel constructor");
    return index;
}
bool SourceRuntimeSoundAc97OutputPages::dispatch(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi){
    const bool allocate=call.instruction==0x3879dc,capacity=call.instruction==0x3879e9,address=call.instruction==0x38f158;
    if(!allocate&&!capacity&&!address)return false;
    if(&bank!=&memory_)throw std::invalid_argument("Native AC97 physical request uses another source bank");
    require_live();const auto sp=abi.gp[4];
    const auto parent=allocate?word(memory_,sp+28):capacity?word(memory_,sp+12):0;
    if((allocate||capacity)&&parent!=0x38f13f&&parent!=0x38f17a)return false;
    const auto import=allocate?0x41b640u:capacity?0x41b644u:0x41b7bcu;
    const auto expected=allocate?0x800000a6u:capacity?0x800000b4u:0x800000adu;
    if(call.tail||call.target!=expected||word(memory_,import)!=expected||word(memory_,sp)!=call.instruction+6)
        throw std::invalid_argument("Native AC97 physical import/caller identity differs");
    if(allocate){
        const auto channel=abi.gp[6],index=validate_channel(channel),field=parent==0x38f13f?0x18u:0x20u;
        const SourcePhysicalAllocationRequest request{word(memory_,sp+4),word(memory_,sp+8),word(memory_,sp+12),word(memory_,sp+16),word(memory_,sp+20)};
        if(request.bytes!=(field==0x18?0x100u:0x2000u)||request.lowest||request.highest!=0xffffffffu||
            request.alignment!=0x1000||request.protection!=4||word(memory_,channel+field)||
            (field==0x20&&!(word(memory_,channel+0xc)&1))||
            std::ranges::any_of(allocations_,[&](const auto& row){return row.second.channel==channel&&row.second.field==field;}))
            throw std::invalid_argument("Native AC97 channel allocation differs from genuine38F104 request");
        // Disjoint native resource policy for this unrestricted request. This
        // is a logical namespace, never a host PCI address or original value.
        auto native_request=request;native_request.lowest=0x06000000;
        const auto source=physical_.allocate(native_request);if(!source)throw std::runtime_error("Native AC97 physical namespace exhausted");
        try{
            const auto owned=physical_.ownership_checkpoint();const auto at=std::ranges::find_if(owned.allocations,[&](const auto& row){return row.base==source;});
            if(at==owned.allocations.end())throw std::runtime_error("Native AC97 allocation lost its actual page owner");
            const auto native=memory_.native_address(source,at->bytes);
            if(native.value<=0xffffffffu||!allocations_.emplace(source,Allocation{source,source&0x7fffffffu,request.bytes,at->bytes,channel,index,field,native,memory_.native_address(channel,0x30)}).second)
                throw std::runtime_error("Native AC97 page allocation lacks a unique native64 binding");
        }catch(...){fault_=true;throw;}
        abi.gp[0]=source;abi.gp[4]+=24;return true;
    }
    const auto source=word(memory_,sp+4);const auto at=allocations_.find(source);
    if(at==allocations_.end())throw std::invalid_argument("Native AC97 page query is unowned");const auto& row=at->second;
    (void)validate_channel(row.channel);
    if(memory_.native_address(source,row.capacity)!=row.native||memory_.native_address(row.channel,0x30)!=row.native_channel)
        throw std::runtime_error("Native AC97 query lost its source/native page binding");
    memory_.validate_mutable_view(source,row.capacity);
    if(capacity){
        if(source!=abi.gp[6]||(parent==0x38f13f?0x18u:0x20u)!=row.field)
            throw std::invalid_argument("Native AC97 capacity query lost its original allocation parent");
        abi.gp[0]=row.capacity;
    }else{
        if(abi.gp[6]!=row.channel||row.field!=0x18||word(memory_,row.channel+0x18)!=source||word(memory_,row.channel+0x1c))
            throw std::invalid_argument("Native AC97 physical-address query lost its source publication");
        abi.gp[0]=row.physical;
    }
    abi.gp[4]+=8;return true;
}
SourcePhysicsContactMemory& SourceRuntimeSoundAc97OutputPages::cpu_memory(){require_live();return *cpu_memory_;}
void SourceRuntimeSoundAc97OutputPages::release_owned_page(const Allocation& expected){
    require_live();const auto at=allocations_.find(expected.source);
    if(at==allocations_.end())throw std::invalid_argument("Native AC97 page release is unowned");
    const auto& row=at->second;
    if(source_level_!=2||row.source!=expected.source||row.physical!=expected.physical||row.requested!=expected.requested||
        row.capacity!=expected.capacity||row.channel!=expected.channel||row.index!=expected.index||row.field!=expected.field||
        row.native!=expected.native||row.native_channel!=expected.native_channel||
        memory_.native_address(row.source,row.capacity)!=row.native||memory_.native_address(row.channel,0x30)!=row.native_channel||
        word(memory_,row.channel+row.field)!=row.source)
        throw std::invalid_argument("Native AC97 page release lost its exact source/native/exclusion binding");
    (void)validate_channel(row.channel);physical_.release(row.source);allocations_.erase(at);
}
bool SourceRuntimeSoundAc97OutputPages::try_exclusion(const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi){
    if(call.instruction!=0x383d88&&call.instruction!=0x383d9d)return false;
    require_live();
    if(!abi.fs_base||abi.fs_base>0xffffffdbu||(fs_&&fs_!=abi.fs_base)||memory_.view(abi.fs_base+0x24,1)[0]!=std::byte{}||
        (call.instruction==0x383d88?source_level_!=0:source_level_!=2))
        throw std::invalid_argument("AC97 source exclusion is outside its actual native passive/serialized domain");
    if(!control_.try_exclusion(memory_,call,abi))throw std::runtime_error("Actual AC97 source exclusion resource is unavailable");
    fs_=abi.fs_base;source_level_=call.instruction==0x383d88?2:0;return true;
}
SourceRuntimeSoundAc97OutputPages::State SourceRuntimeSoundAc97OutputPages::diagnostics()const{
    require_thread();std::vector<Allocation> rows;for(const auto& [source,row]:allocations_){(void)source;rows.push_back(row);}
    return {std::move(rows),generation_,device_,controller_,source_level_,fault_,false,false};
}
bool SourceRuntimeSoundAc97OutputPages::quiescent()const noexcept{
    return std::this_thread::get_id()==thread_&&!fault_&&!source_level_&&allocations_.empty();
}
SourceRuntimeSoundAc97OutputPages::CallScope::CallScope(SourceRuntimeSoundAc97OutputPages* owner)
    :owner_(owner),exceptions_(std::uncaught_exceptions()){if(owner_)owner_->require_live();}
SourceRuntimeSoundAc97OutputPages::CallScope::~CallScope(){if(owner_&&std::uncaught_exceptions()>exceptions_){owner_->fault_=true;owner_->control_.abort_transaction();}}
}
