#include "halo2/source_runtime_sound_table_limits.hpp"
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace halo2 {namespace {
struct Request {std::uint32_t pc,address,value,descriptor,records,width;};
constexpr std::array<Request,5> requests{{
    {0x38e90d,0xfe801104,0xff,0x391390,0x100,0x80},
    {0x38e917,0xfe801108,0x3f,0x3913e0,0x40,0x80},
    {0x38e921,0xfe80111c,0x7f,0x3913d0,0x80,0x40},
    {0x38e92b,0xfe801124,0x1fff,0x3913c0,0x2000,8},
    {0x38e935,0xfe801138,0x7ff,0x3913b0,0x800,8}
}};
struct ContextRequest {std::uint32_t pc,address,value;std::size_t context;bool length;};
constexpr std::array<ContextRequest,6> context_requests{{
    {0x38e949,0xfe80112c,0,0,false},{0x38e954,0xfe801130,0x08000000,0,true},
    {0x38e959,0xfe801140,0,1,false},{0x38e95f,0xfe801144,0x08000000,1,true},
    {0x38e964,0xfe801150,0,2,false},{0x38e96d,0xfe801154,0x08000000,2,true}
}};
struct BaseRequest {std::uint32_t pc,address;std::size_t range;};
constexpr std::array<BaseRequest,5> base_requests{{
    {0x38e104,0xfe80202c,0},{0x38e10e,0xfe802038,2},
    {0x38e118,0xfe80203c,1},{0x38e122,0xfe802030,4},
    {0x38e12c,0xfe802034,3}
}};
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){
    const auto raw=memory.view(address,4);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);return value;
}
bool same(const SourceRuntimeSoundDmaResources::Allocation& a,const SourceRuntimeSoundDmaResources::Allocation& b){
    return a.source==b.source&&a.physical==b.physical&&a.requested==b.requested&&a.capacity==b.capacity&&
        a.alignment==b.alignment&&a.protection==b.protection&&a.source_descriptor==b.source_descriptor&&a.native==b.native;
}
}
SourceRuntimeSoundTableLimits::SourceRuntimeSoundTableLimits(SourceRuntimeMemory& memory,
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma,std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep,
    SourceRuntimeSoundResetResources::ControlLease control)
    :memory_(memory),dma_(std::move(dma)),ep_(std::move(ep)),control_(std::move(control)),
     generation_(control_.generation()),device_(dma_?dma_->device_id():0){
    control_.require_bank(memory_);require_live();
    const auto allocations=dma_->allocations();
    for(std::size_t i=0;i<requests.size();++i){
        const auto& request=requests[i];
        const auto at=std::ranges::find_if(allocations,[&](const auto& row){return row.source_descriptor==request.descriptor;});
        if(at==allocations.end()||at->requested!=request.records*request.width||at->capacity<at->requested)
            throw std::invalid_argument("Native audio table extent differs from original38E996 capacity");
        ranges_[i]={static_cast<Table>(i),*at,request.records,request.width,false};
    }
    for(std::size_t i=0;i<contexts_.size();++i)contexts_[i].context=static_cast<Context>(i);
}
SourceRuntimeSoundTableLimits::~SourceRuntimeSoundTableLimits(){
    if(std::this_thread::get_id()!=thread_)std::terminate();
}
void SourceRuntimeSoundTableLimits::require_thread()const{
    if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native audio table limits require their creating owner thread");
}
void SourceRuntimeSoundTableLimits::require_live()const{
    require_thread();
    if(retired_||fault_||!dma_||!ep_||!generation_||control_.generation()!=generation_||dma_->device_id()!=device_)
        throw std::runtime_error("Native audio table limits lost their live reset/DMA generation");
    control_.require_bank(memory_);
    const auto ep=ep_->diagnostics();
    if(ep.retired||ep.faulted||ep.output_device!=device_||ep.active_copies||ep.callback_leases)
        throw std::runtime_error("Native audio table source access requires quiescent EP resources");
    const auto allocations=dma_->allocations();
    if(allocations.size()!=16)throw std::invalid_argument("Native audio table limits require all sixteen actual DMA allocations");
    for(const auto& row:allocations){
        if(row.native.value<=0xffffffffu||row.capacity<row.requested||
            memory_.native_address(row.source,row.capacity)!=row.native||
            word(memory_,row.source_descriptor)!=row.source||word(memory_,row.source_descriptor+4)!=row.physical||
            word(memory_,row.source_descriptor+8)!=row.requested)
            throw std::invalid_argument("Native audio table DMA allocation belongs to another bank/backing");
    }
    for(const auto* expected:{&ep.layout.data,&ep.layout.scatter_gather}){
        const auto at=std::ranges::find_if(allocations,[&](const auto& row){return row.source_descriptor==expected->source_descriptor;});
        if(at==allocations.end()||!same(*at,*expected)||memory_.native_address(expected->source,expected->capacity)!=expected->native)
            throw std::invalid_argument("Native audio table EP allocation belongs to another bank/backing");
    }
    for(const auto& range:ranges_)if(range.record_bytes){
        const auto at=std::ranges::find_if(allocations,[&](const auto& row){return row.source_descriptor==range.allocation.source_descriptor;});
        if(at==allocations.end()||!same(*at,range.allocation))
            throw std::runtime_error("Native audio table range lost its retained DMA allocation");
    }
}
bool SourceRuntimeSoundTableLimits::apply_limit(std::uint32_t instruction,std::uint32_t address,std::uint32_t value){
    require_thread();
    const auto at=std::ranges::find_if(requests,[&](const auto& request){return request.pc==instruction||request.address==address;});
    if(at==requests.end())return false;
    require_live();const auto i=static_cast<std::size_t>(at-requests.begin());
    if(at->pc!=instruction||at->address!=address||at->value!=value||ranges_[i].source_limit_observed)
        throw std::invalid_argument("Native audio table limit differs from its exact original source request");
    ranges_[i].source_limit_observed=true;return true;
}
bool SourceRuntimeSoundTableLimits::apply_context_bound(std::uint32_t instruction,std::uint32_t address,std::uint32_t value){
    require_thread();
    const auto at=std::ranges::find_if(context_requests,[&](const auto& request){return request.pc==instruction||request.address==address;});
    if(at==context_requests.end())return false;
    require_live();auto& window=contexts_[at->context];
    if(at->pc!=instruction||at->address!=address||at->value!=value||
        (at->length?(!window.base_observed||window.length_observed):window.base_observed))
        throw std::invalid_argument("Native audio physical context differs from its exact ordered source bounds");
    if(at->length){window.length=value;window.length_observed=true;}
    else{window.base=value;window.base_observed=true;}
    return true;
}
bool SourceRuntimeSoundTableLimits::apply_physical_base(std::uint32_t instruction,std::uint32_t address,std::uint32_t physical){
    require_thread();
    const auto at=std::ranges::find_if(base_requests,[&](const auto& request){return request.pc==instruction||request.address==address;});
    if(at==base_requests.end())return false;
    require_live();auto& range=ranges_[at->range];
    if(at->pc!=instruction||at->address!=address||physical!=range.allocation.physical||
        !range.source_limit_observed||range.source_base_observed)
        throw std::invalid_argument("Native audio table base differs from its exact original retained allocation/publication");
    range.source_base_observed=true;return true;
}
SourceRuntimeSoundTableLimits::RecordView SourceRuntimeSoundTableLimits::record(Table table,std::uint32_t index)const{
    require_live();const auto i=static_cast<std::size_t>(table);
    if(i>=ranges_.size()||!ranges_[i].source_limit_observed||index>=ranges_[i].records)
        throw std::out_of_range("Native audio table record is outside its genuinely observed source limit");
    const auto& range=ranges_[i];const auto source=range.allocation.source+index*range.record_bytes;
    const auto native=memory_.native_address(source,range.record_bytes);
    if(native.value!=range.allocation.native.value+std::uint64_t(index)*range.record_bytes)
        throw std::runtime_error("Native audio table record is not its actual contiguous native64 view");
    return {source,native,memory_.view(source,range.record_bytes)};
}
SourceRuntimeSoundTableLimits::RecordView SourceRuntimeSoundTableLimits::bound_record(Table table,std::uint32_t index)const{
    require_live();const auto i=static_cast<std::size_t>(table);
    if(i>=ranges_.size()||!ranges_[i].source_base_observed)
        throw std::runtime_error("Native audio table has no genuinely observed physical-base publication");
    return record(table,index);
}
SourceRuntimeSoundTableLimits::RecordView SourceRuntimeSoundTableLimits::resolve_physical(Context context,std::uint32_t physical,std::uint32_t bytes)const{
    require_live();const auto i=static_cast<std::size_t>(context);
    if(i>=contexts_.size())throw std::invalid_argument("Native audio physical context is unknown");
    const auto& window=contexts_[i];const auto end=std::uint64_t(physical)+bytes;
    if(!window.base_observed||!window.length_observed||!bytes||physical<window.base||end>std::uint64_t(window.base)+window.length)
        throw std::out_of_range("Native audio request is outside its genuinely observed physical context");
    const auto allocations=dma_->allocations();
    const auto at=std::ranges::find_if(allocations,[&](const auto& row){return physical>=row.physical&&end<=std::uint64_t(row.physical)+row.capacity;});
    if(at==allocations.end())throw std::out_of_range("Native audio physical context has no single actual backed allocation for this request");
    const auto offset=physical-at->physical;const auto source=at->source+offset;
    const auto native=memory_.native_address(source,bytes);
    if(native.value!=at->native.value+offset)throw std::runtime_error("Native audio physical context lost its actual native64 subrange");
    return {source,native,memory_.view(source,bytes)};
}
void SourceRuntimeSoundTableLimits::require_control(const SourceRuntimeMemory& bank,const SourceRuntimeSoundResetResources::ControlLease& control)const{
    require_thread();
    if(&bank!=&memory_||!control_.same_owner(control))
        throw std::invalid_argument("Native audio table belongs to another bank or actual control worker");
    require_live();
}
SourceRuntimeSoundTableLimits::State SourceRuntimeSoundTableLimits::diagnostics()const{
    require_thread();return {ranges_,contexts_,generation_,device_,retired_,fault_,false};
}
void SourceRuntimeSoundTableLimits::retire(){require_thread();if(retired_)return;retired_=true;ep_.reset();dma_.reset();}
bool SourceRuntimeSoundTableLimits::quiescent()const noexcept{return std::this_thread::get_id()==thread_&&retired_;}
SourceRuntimeSoundTableLimits::CallScope::CallScope(SourceRuntimeSoundTableLimits* owner)
    :owner_(owner),exceptions_(std::uncaught_exceptions()){if(owner_)owner_->require_live();}
SourceRuntimeSoundTableLimits::CallScope::~CallScope(){if(owner_&&std::uncaught_exceptions()>exceptions_)owner_->fault_=true;}
}
