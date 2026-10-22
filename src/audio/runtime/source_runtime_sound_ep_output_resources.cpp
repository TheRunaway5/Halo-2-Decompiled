#include "halo2/source_runtime_sound_ep_output_resources.hpp"
#include <SDL.h>
#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){
    const auto raw=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;
}
void require_fifo(unsigned fifo){if(fifo>=2)throw std::out_of_range("Native EP output owns only its two source-configured FIFO ranges");}
std::mutex bindings_mutex;
std::map<NativeRuntimeAddress,std::weak_ptr<void>> bindings;
}
struct SourceRuntimeSoundEpOutputResources::PreparedState {
    std::uint32_t offset{},bytes{};
    NativeRuntimeAddress native{};
    std::uint64_t copies{},bytes_copied{};
    bool cancelled{};
};
struct SourceRuntimeSoundEpOutputResources::Core {
    mutable std::mutex mutex;
    std::condition_variable idle;
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma;
    std::shared_ptr<SdlSoundPlatform> output;
    const std::uint32_t device;
    Layout layout;
    std::array<RingState,2> rings;
    std::span<std::byte> data;
    std::uint64_t generation=1;
    std::size_t leases{},active{};
    bool retired{},faulted{};
    std::shared_ptr<PreparedState> prepared;
    Core(std::shared_ptr<SourceRuntimeSoundDmaResources> d,std::shared_ptr<SdlSoundPlatform> o,
        Layout l,std::span<std::byte> bytes)
        :dma(std::move(d)),output(std::move(o)),device(output->device_id()),layout(l),data(bytes){
        for(unsigned i=0;i<2;++i)rings[i]={l.rings[i].pending_source_cursor,l.rings[i].pending_source_cursor,
            0,l.rings[i].end-l.rings[i].base,0,0,false};
    }
    void require_callback(std::uint64_t epoch)const{
        if(retired||faulted||epoch!=generation||data.empty())
            throw std::runtime_error("Native EP byte callback lease is cancelled, stale or lost its actual output");
    }
    void require_prepared(const std::shared_ptr<PreparedState>& state,std::uint64_t epoch)const{
        require_callback(epoch);
        if(prepared!=state||state->cancelled)
            throw std::runtime_error("Native prepared EP byte range is cancelled or superseded");
    }
    void cancel_prepared(std::unique_lock<std::mutex>& lock,const std::shared_ptr<PreparedState>& expected={}){
        if(!prepared||(expected&&prepared!=expected))return;
        const auto prior=prepared;
        if(!prior->cancelled){prior->cancelled=true;--leases;}
        idle.wait(lock,[&]{return !active;});
        if(prepared==prior)prepared.reset();
    }
    struct CallbackAccess {
        Core& owner;
        CallbackAccess(Core& core,std::uint64_t epoch):owner(core){
            std::lock_guard lock(owner.mutex);owner.require_callback(epoch);++owner.active;
        }
        ~CallbackAccess(){std::lock_guard lock(owner.mutex);--owner.active;if(!owner.active)owner.idle.notify_all();}
        CallbackAccess(const CallbackAccess&)=delete;
    };
    void reject_alias(const void* pointer,std::size_t bytes)const{
        if(!bytes)return;
        const auto begin=reinterpret_cast<std::uintptr_t>(pointer),base=reinterpret_cast<std::uintptr_t>(data.data());
        if(bytes>std::numeric_limits<std::uintptr_t>::max()-begin||
            (begin<base+data.size()&&begin+bytes>base))
            throw std::invalid_argument("Native EP byte transfer aliases its circular backing or wraps address space");
    }
    void copy(unsigned fifo,void* external,std::size_t bytes,bool write){
        auto& ring=rings[fifo];const auto& bounds=layout.rings[fifo];auto& cursor=write?ring.write_cursor:ring.read_cursor;
        const auto first=std::min<std::size_t>(bytes,bounds.end-cursor);
        // The same lock covers the complete copy and cursor/accounting commit.
        // Retirement cannot free the shared DMA pages during either segment.
        if(write){std::memcpy(data.data()+cursor,external,first);std::memcpy(data.data()+bounds.base,static_cast<std::byte*>(external)+first,bytes-first);}
        else{std::memcpy(external,data.data()+cursor,first);std::memcpy(static_cast<std::byte*>(external)+first,data.data()+bounds.base,bytes-first);}
        cursor=bounds.base+std::uint32_t((std::uint64_t(cursor-bounds.base)+bytes)%ring.capacity);
        if(write){ring.queued+=bytes;ring.native_bytes_written+=bytes;}else{ring.queued-=bytes;ring.native_bytes_read+=bytes;}
    }
};
SourceRuntimeSoundEpOutputResources::SourceRuntimeSoundEpOutputResources(SourceRuntimeMemory& memory,
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma,std::shared_ptr<SdlSoundPlatform> output):memory_(memory){
    if(!dma||!output)throw std::invalid_argument("Native EP output lacks its shared DMA/output owner");
    const auto dma_device=dma->device_id(); // Immutable executor guard precedes output state reads.
    if(!output->device_id()||output->output_channels()!=2||dma_device!=output->device_id()||
        SDL_GetAudioDeviceStatus(output->device_id())!=SDL_AUDIO_PLAYING)
        throw std::invalid_argument("Native EP output requires the actual shared live stereo DMA/output owner");
    const auto allocations=dma->allocations();if(allocations.size()!=16)
        throw std::invalid_argument("Native EP output requires all sixteen genuinely constructed DMA descriptors");
    Layout layout{};
    for(unsigned index=0;index<16;++index){
        const auto descriptor=0x391360u+index*0x10u;
        const auto found=std::ranges::find_if(allocations,[&](const auto& row){return row.source_descriptor==descriptor;});
        if(found==allocations.end()||word(memory,descriptor)!=found->source||word(memory,descriptor+4)!=found->physical||
            word(memory,descriptor+8)!=found->requested||found->native.value<=0xffffffffu||
            memory.native_address(found->source,found->capacity)!=found->native||found->protection!=4)
            throw std::invalid_argument("Native EP output lost an actual source/physical/native64 DMA binding");
        if(index==1)layout.data=*found;if(index==10)layout.scatter_gather=*found;
    }
    if(word(memory,0x390b70)!=0x2000||word(memory,0x390b74)!=0x4000||layout.data.requested!=0x6000||
        layout.data.capacity!=0x6000||layout.scatter_gather.requested!=0x30||layout.scatter_gather.capacity<0x30||
        std::uint64_t(layout.data.physical)+0x6000>0x80000000u)
        throw std::invalid_argument("Native EP output differs from the authenticated source buffer/page layout");
    layout.rings={RingLayout{0,0x2000,0x600,layout.data.native},RingLayout{0x2000,0x6000,0x3800,layout.data.native+0x2000}};
    for(unsigned i=0;i<6;++i)layout.pending_page_entries[i]={layout.data.physical+i*0x1000,0};
    auto bytes=memory.mutable_view(layout.data.source,layout.data.requested);
    core_=std::make_shared<Core>(std::move(dma),std::move(output),layout,bytes);
    std::lock_guard lock(bindings_mutex);
    const auto found=bindings.find(layout.data.native);
    if(found!=bindings.end()&&!found->second.expired())
        throw std::invalid_argument("Native EP circular backing already has a live transport owner/lease");
    bindings[layout.data.native]=core_;
}
SourceRuntimeSoundEpOutputResources::~SourceRuntimeSoundEpOutputResources(){
    // Creating-thread lifetime is part of this bank/SDL resource contract.
    // Faulting cancels all existing leases before explicit native retirement.
    fault_scope();try{retire();}catch(...){std::terminate();}
}
void SourceRuntimeSoundEpOutputResources::require_thread()const{
    if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native EP output requires its creating owner thread");
}
void SourceRuntimeSoundEpOutputResources::require_live()const{
    require_thread();std::shared_ptr<SourceRuntimeSoundDmaResources> dma;std::shared_ptr<SdlSoundPlatform> output;
    {std::lock_guard lock(core_->mutex);
        if(core_->retired||core_->faulted||!core_->dma||!core_->output)
            throw std::runtime_error("Native EP output is retired or faulted");
        dma=core_->dma;output=core_->output;}
    // SDL may lock its audio device. Never query it while holding the byte
    // transport mutex: a future audio callback may hold that device lock
    // before borrowing the transport in the opposite direction.
    if(output->device_id()!=core_->device||output->output_channels()!=2||SDL_GetAudioDeviceStatus(core_->device)!=SDL_AUDIO_PLAYING)
        throw std::runtime_error("Native EP output is retired, faulted or lost its actual stereo owner");
    // This creating-thread-only owner query also rejects a faulted SDK/DMA
    // transaction before admitting any new transport or future DSP borrower.
    (void)dma->allocations();
}
void SourceRuntimeSoundEpOutputResources::apply_pending_cursor(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
    require_live();unsigned index;
    if(pc==0x38e7c4&&address==0xfe80402c&&value==0x600)index=0;
    else if(pc==0x38e7ca&&address==0xfe80403c&&value==0x3800)index=1;
    else throw std::invalid_argument("Unowned EP cursor or unresolved scheduler request");
    std::lock_guard lock(core_->mutex);
    if(core_->rings[index].source_cursor_observed||core_->leases||core_->rings[index].queued)
        throw std::invalid_argument("Native EP cursor replacement requires its genuine source reset/lifetime owner");
    core_->rings[index].source_cursor_observed=true;
}
std::shared_ptr<SourceRuntimeSoundEpOutputResources::CallbackLease> SourceRuntimeSoundEpOutputResources::acquire_native_byte_transport(){
    require_live();std::uint64_t epoch;
    {std::lock_guard lock(core_->mutex);if(core_->prepared)
        throw std::runtime_error("Native EP circular transport conflicts with its retained prepared byte range");
        if(!core_->rings[0].source_cursor_observed||!core_->rings[1].source_cursor_observed)
        throw std::runtime_error("Native EP transport lacks its observed pending cursor requests");epoch=core_->generation;}
    auto result=std::shared_ptr<CallbackLease>(new CallbackLease(core_,epoch));
    {std::lock_guard lock(core_->mutex);core_->require_callback(epoch);result->registered_=true;++core_->leases;}
    return result;
}
std::shared_ptr<SourceRuntimeSoundEpOutputResources::PreparedRangeLease> SourceRuntimeSoundEpOutputResources::acquire_prepared_range(
    NativeRuntimeAddress native,std::uint32_t offset,std::uint32_t bytes){
    require_live();std::uint64_t epoch;
    const auto validate=[&]{
        if(native!=core_->layout.data.native||offset||bytes!=0x600||!core_->rings[0].source_cursor_observed||
            !core_->rings[1].source_cursor_observed||core_->leases||core_->active||core_->prepared)
            throw std::invalid_argument("Native prepared PCM bytes lack the exact exclusive retained EP0 range");
        for(const auto& ring:core_->rings)if(ring.queued||ring.native_bytes_written||ring.native_bytes_read)
            throw std::invalid_argument("Native prepared PCM range cannot alias an already-used circular byte transport");
    };
    {std::lock_guard lock(core_->mutex);validate();epoch=core_->generation;}
    auto state=std::make_shared<PreparedState>();state->offset=offset;state->bytes=bytes;state->native=native+offset;
    // Construct outside the Core mutex: a shared_ptr allocation failure may
    // destroy the unregistered lease, whose cleanup also locks this mutex.
    auto lease=std::shared_ptr<PreparedRangeLease>(new PreparedRangeLease(core_,state,epoch));
    {std::lock_guard lock(core_->mutex);validate();core_->require_callback(epoch);core_->prepared=state;++core_->leases;}
    return lease;
}
void SourceRuntimeSoundEpOutputResources::cancel_prepared_range(){
    require_thread();std::unique_lock lock(core_->mutex);core_->cancel_prepared(lock);
}
SourceRuntimeSoundEpOutputResources::State SourceRuntimeSoundEpOutputResources::diagnostics()const{
    require_thread();std::lock_guard lock(core_->mutex);
    return {core_->layout,core_->rings,core_->generation,core_->leases,core_->active,core_->device,core_->retired,core_->faulted,false};
}
bool SourceRuntimeSoundEpOutputResources::source_scatter_gather_constructed()const{
    require_live();std::lock_guard lock(core_->mutex);
    for(unsigned i=0;i<6;++i)for(unsigned j=0;j<2;++j)
        if(word(memory_,core_->layout.scatter_gather.source+i*8+j*4)!=core_->layout.pending_page_entries[i][j])return false;
    return true;
}
void SourceRuntimeSoundEpOutputResources::retire(){
    require_thread();std::shared_ptr<SourceRuntimeSoundDmaResources> dma;std::shared_ptr<SdlSoundPlatform> output;
    {std::unique_lock lock(core_->mutex);if(core_->retired)return;
        if(!core_->faulted&&(core_->rings[0].queued||core_->rings[1].queued))
            throw std::runtime_error("Native EP byte transport must account queued bytes before retirement");
        core_->cancel_prepared(lock);core_->retired=true;++core_->generation;
        core_->idle.wait(lock,[&]{return !core_->active;});
        core_->data={};dma=std::move(core_->dma);output=std::move(core_->output);}
    // Stale inert leases may retain Core after its actual pages are detached.
    // Only this exact retired Core relinquishes the backing registry; later
    // destruction of an old handle cannot erase a replacement owner.
    {std::lock_guard lock(bindings_mutex);const auto at=bindings.find(core_->layout.data.native);
        if(at!=bindings.end()&&at->second.lock().get()==core_.get())bindings.erase(at);}
    // Last DMA/output references, if any, are released on this creating
    // thread, never by a stale callback lease on another executor.
}
bool SourceRuntimeSoundEpOutputResources::quiescent()const noexcept{
    if(std::this_thread::get_id()!=thread_)return false;
    std::lock_guard lock(core_->mutex);return core_->retired&&!core_->leases&&!core_->active&&!core_->dma;
}
void SourceRuntimeSoundEpOutputResources::fault_scope()noexcept{
    if(std::this_thread::get_id()!=thread_)std::terminate();
    std::unique_lock lock(core_->mutex);if(!core_->retired&&!core_->faulted){core_->faulted=true;++core_->generation;}
    core_->idle.wait(lock,[&]{return !core_->active;});
}
SourceRuntimeSoundEpOutputResources::CallScope::CallScope(SourceRuntimeSoundEpOutputResources* owner)
    :owner_(owner),exceptions_(std::uncaught_exceptions()){if(owner_){owner_->require_live();owner_->cancel_prepared_range();}}
SourceRuntimeSoundEpOutputResources::CallScope::~CallScope(){if(owner_&&std::uncaught_exceptions()>exceptions_)owner_->fault_scope();}
SourceRuntimeSoundEpOutputResources::CallbackLease::CallbackLease(std::shared_ptr<Core> core,std::uint64_t epoch)
    :core_(std::move(core)),generation_(epoch){}
SourceRuntimeSoundEpOutputResources::CallbackLease::~CallbackLease(){if(registered_){std::lock_guard lock(core_->mutex);--core_->leases;}}
bool SourceRuntimeSoundEpOutputResources::CallbackLease::write(unsigned fifo,std::span<const std::byte> input){
    require_fifo(fifo);Core::CallbackAccess access(*core_,generation_);
    if(SDL_GetAudioDeviceStatus(core_->device)!=SDL_AUDIO_PLAYING)throw std::runtime_error("Native EP byte callback lost its actual output");
    std::lock_guard lock(core_->mutex);core_->require_callback(generation_);auto& ring=core_->rings[fifo];
    if(input.size()>ring.capacity-ring.queued)return false;
    if(input.size()>std::numeric_limits<std::uint64_t>::max()-ring.native_bytes_written)throw std::overflow_error("Native EP byte accounting wraps64 bits");
    core_->reject_alias(input.data(),input.size());if(!input.empty())core_->copy(fifo,const_cast<std::byte*>(input.data()),input.size(),true);return true;
}
bool SourceRuntimeSoundEpOutputResources::CallbackLease::read(unsigned fifo,std::span<std::byte> output){
    require_fifo(fifo);Core::CallbackAccess access(*core_,generation_);
    if(SDL_GetAudioDeviceStatus(core_->device)!=SDL_AUDIO_PLAYING)throw std::runtime_error("Native EP byte callback lost its actual output");
    std::lock_guard lock(core_->mutex);core_->require_callback(generation_);auto& ring=core_->rings[fifo];
    if(output.size()>ring.queued)return false;
    if(output.size()>std::numeric_limits<std::uint64_t>::max()-ring.native_bytes_read)throw std::overflow_error("Native EP byte accounting wraps64 bits");
    core_->reject_alias(output.data(),output.size());if(!output.empty())core_->copy(fifo,output.data(),output.size(),false);return true;
}
SourceRuntimeSoundEpOutputResources::RingState SourceRuntimeSoundEpOutputResources::CallbackLease::state(unsigned fifo)const{
    require_fifo(fifo);Core::CallbackAccess access(*core_,generation_);
    if(SDL_GetAudioDeviceStatus(core_->device)!=SDL_AUDIO_PLAYING)throw std::runtime_error("Native EP byte callback lost its actual output");
    std::lock_guard lock(core_->mutex);core_->require_callback(generation_);return core_->rings[fifo];
}
SourceRuntimeSoundEpOutputResources::PreparedRangeLease::PreparedRangeLease(std::shared_ptr<Core> core,
    std::shared_ptr<PreparedState> state,std::uint64_t generation):core_(std::move(core)),state_(std::move(state)),generation_(generation){}
SourceRuntimeSoundEpOutputResources::PreparedRangeLease::~PreparedRangeLease(){
    std::unique_lock lock(core_->mutex);core_->cancel_prepared(lock,state_);
}
void SourceRuntimeSoundEpOutputResources::PreparedRangeLease::copy(std::size_t offset,std::span<std::byte> output)const{
    Core::CallbackAccess access(*core_,generation_);
    {std::lock_guard lock(core_->mutex);core_->require_prepared(state_,generation_);}
    if(SDL_GetAudioDeviceStatus(core_->device)!=SDL_AUDIO_PLAYING)
        throw std::runtime_error("Native prepared EP byte range lost its actual output");
    std::lock_guard lock(core_->mutex);core_->require_prepared(state_,generation_);
    if(offset>state_->bytes||output.size()>state_->bytes-offset)
        throw std::out_of_range("Native prepared EP copy crosses its actual retained byte range");
    if(state_->copies==std::numeric_limits<std::uint64_t>::max()||output.size()>std::numeric_limits<std::uint64_t>::max()-state_->bytes_copied)
        throw std::overflow_error("Native prepared EP copy accounting wraps64 bits");
    core_->reject_alias(output.data(),output.size());
    if(!output.empty())std::memcpy(output.data(),core_->data.data()+state_->offset+offset,output.size());
    ++state_->copies;state_->bytes_copied+=output.size();
}
SourceRuntimeSoundEpOutputResources::PreparedRangeLease::State SourceRuntimeSoundEpOutputResources::PreparedRangeLease::state()const{
    std::lock_guard lock(core_->mutex);return {state_->offset,state_->bytes,state_->native,state_->copies,state_->bytes_copied,
        state_->cancelled||core_->retired||core_->faulted||generation_!=core_->generation,false,false};
}
void SourceRuntimeSoundEpOutputResources::PreparedRangeLease::cancel(){
    if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native prepared EP cancellation requires its creating owner thread");
    std::unique_lock lock(core_->mutex);core_->cancel_prepared(lock,state_);
}
}
