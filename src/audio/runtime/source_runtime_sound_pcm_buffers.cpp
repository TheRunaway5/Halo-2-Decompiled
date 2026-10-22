#include "halo2/source_runtime_sound_pcm_buffers.hpp"
#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){const auto raw=memory.view(address,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;}
std::mutex registry_mutex;
std::map<NativeRuntimeAddress,const SourceRuntimeSoundPcmBuffers*> descriptor_bindings;
}
bool source_runtime_sound_pcm_descriptor_bound(NativeRuntimeAddress descriptor_page){std::lock_guard lock(registry_mutex);return descriptor_bindings.contains(descriptor_page);}
SourceRuntimeSoundPcmBuffers::SourceRuntimeSoundPcmBuffers(SourceRuntimeMemory& memory,
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma,std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep,
    std::shared_ptr<SourceRuntimeSoundAc97OutputPages> pages,std::shared_ptr<SourceRuntimeSoundAc97ChannelReset> channels,
    SourceRuntimeSoundResetResources::ControlLease control)
    :memory_(memory),dma_(std::move(dma)),ep_(std::move(ep)),pages_(std::move(pages)),channels_(std::move(channels)),
     control_(std::move(control)),generation_(control_.generation()),device_(dma_?dma_->device_id():0),controller_(control_.source_apu()+0x1c){
    control_.require_bank(memory_);require_live();
}
SourceRuntimeSoundPcmBuffers::~SourceRuntimeSoundPcmBuffers(){if(std::this_thread::get_id()!=thread_)std::terminate();retire();}
void SourceRuntimeSoundPcmBuffers::require_thread()const{if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native PCM buffer binding requires its creating owner thread");}
void SourceRuntimeSoundPcmBuffers::require_live()const{
    require_thread();if(retired_||fault_||!dma_||!ep_||!pages_||!channels_||control_.generation()!=generation_||!generation_||dma_->device_id()!=device_)
        throw std::runtime_error("Native PCM buffer binding lost its actual owner/reset generation");
    control_.require_bank(memory_);pages_->require_control(memory_,control_);channels_->require_control(memory_,control_);
    const auto ep=ep_->diagnostics();const auto pages=pages_->diagnostics();const auto channels=channels_->diagnostics();
    if(ep.retired||ep.faulted||ep.output_device!=device_||ep.callback_leases||ep.active_copies||pages.faulted||
        pages.generation!=generation_||pages.output_device!=device_||pages.controller!=controller_||channels.faulted)
        throw std::runtime_error("Native PCM source binding requires its quiescent actual EP/page/endpoint resources");
    const auto rows=dma_->allocations();if(rows.size()!=16)throw std::invalid_argument("Native PCM binding requires sixteen genuine DMA allocations");
    for(const auto& row:rows)if(memory_.native_address(row.source,row.capacity)!=row.native||
        word(memory_,row.source_descriptor)!=row.source||word(memory_,row.source_descriptor+4)!=row.physical||word(memory_,row.source_descriptor+8)!=row.requested)
        throw std::invalid_argument("Native PCM DMA binding belongs to another bank/backing");
    for(const auto* expected:{&ep.layout.data,&ep.layout.scatter_gather}){
        const auto at=std::ranges::find_if(rows,[&](const auto& row){return row.source_descriptor==expected->source_descriptor;});
        if(at==rows.end()||at->source!=expected->source||at->physical!=expected->physical||at->requested!=expected->requested||at->capacity!=expected->capacity||at->native!=expected->native)
            throw std::invalid_argument("Native PCM EP binding belongs to another bank/backing");
    }
}
bool SourceRuntimeSoundPcmBuffers::try_bind(const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    require_thread();if(call.instruction!=0x38e4d1&&call.instruction!=0x38e524)return false;
    require_live();const auto kind=call.instruction==0x38e4d1?Kind::Analog:Kind::Spdif;const auto index=kind==Kind::Analog?0u:1u;
    const auto sp=abi.gp[4],channel=abi.gp[1];const auto layout=ep_->diagnostics().layout;
    if(call.tail||call.target!=0x38ef59||word(memory_,sp)!=call.instruction+5||bindings_.contains(kind)||
        word(memory_,sp+4)!=layout.data.physical||word(memory_,sp+8)!=0x2000||layout.data.requested!=0x6000||
        word(memory_,controller_+index*4)!=channel||word(memory_,channel)!=index||word(memory_,channel+8)!=controller_||
        word(memory_,channel+0xc)!=0||memory_.view(channel+0x24,2)[0]!=std::byte{}||memory_.view(channel+0x24,2)[1]!=std::byte{}||
        (index&&(word(memory_,0x390be4)!=0||word(memory_,channel+0x28)!=0x02000000)))
        throw std::invalid_argument("Native PCM buffer request differs from the genuine default38E484 caller");
    const auto endpoint_states=channels_->diagnostics().endpoints;
    const auto endpoint=std::ranges::find_if(endpoint_states,[&](const auto& row){return row.kind==kind&&row.source_channel==channel;});
    const auto rows=pages_->diagnostics().allocations;
    const auto table=std::ranges::find_if(rows,[&](const auto& row){return row.channel==channel&&row.field==0x18;});
    if(endpoint==endpoint_states.end()||table==rows.end()||endpoint->retired||endpoint->faulted||endpoint->resetting||
        !endpoint->descriptor_table_bound||!endpoint->completed_resets||endpoint->native_channel!=memory_.native_address(channel,0x30)||
        endpoint->native_descriptors!=table->native||table->physical!=word(memory_,channel+0x1c)||table->source!=word(memory_,channel+0x18)||
        table->requested!=0x100||memory_.native_address(table->source,table->capacity)!=table->native)
        throw std::invalid_argument("Native PCM buffer lacks its actual reset/bound32-entry source descriptor table");
    const auto lease=channels_->endpoint(kind);const auto status=lease->status();
    if(status.device_removed||!status.paused||status.pending_packets||status.completed_packets)
        throw std::runtime_error("Native PCM buffer replacement requires its paused endpoint with all receipts accounted");
    const Binding binding{kind,channel,table->source,layout.data.source,layout.data.physical,0x2000,
        endpoint->native_channel,table->native,layout.data.native,lease->native_voice(),endpoint->generation};
    // A reset clears source counts without retiring an existing native owner.
    // Reserve before executing any source/native operation so that this case
    // rejects without changing either bank or the earlier binding owner.
    {std::lock_guard lock(registry_mutex);
        if(!descriptor_bindings.emplace(binding.native_descriptors,this).second)
            throw std::runtime_error("Native PCM descriptor page already has a live binding owner");}
    try{
        // The actual original function owns descriptor data, source counts,
        // exclusion and the real native last-index publication through EDF4.
        channels_->run_cpu(call.target,abi,fp,services,observe);
        if(word(memory_,binding.descriptor_source)!=binding.buffer_physical||word(memory_,binding.descriptor_source+4)!=0xc0001000||
            memory_.view(channel+0x24,2)[0]!=std::byte{1}||memory_.view(channel+0x24,2)[1]!=std::byte{})
            throw std::runtime_error("Original PCM binding did not publish its exact single-loop descriptor/count");
        bindings_.emplace(kind,binding);endpoints_.emplace(kind,lease);
    }catch(...){
        {std::lock_guard lock(registry_mutex);const auto at=descriptor_bindings.find(binding.native_descriptors);
            if(at!=descriptor_bindings.end()&&at->second==this)descriptor_bindings.erase(at);}
        endpoints_.erase(kind);bindings_.erase(kind);fault_=true;
        // A source/native commit may already have begun. The shared control
        // assembly must not be reused after this owner rejects its result.
        control_.abort_transaction();throw;
    }
    return true;
}
const SourceRuntimeSoundPcmBuffers::Binding& SourceRuntimeSoundPcmBuffers::resolve(Kind kind)const{
    require_live();const auto at=bindings_.find(kind);if(at==bindings_.end())throw std::invalid_argument("Native PCM source buffer is not bound");
    const auto& binding=at->second;
    if(memory_.native_address(binding.channel,0x30)!=binding.native_channel||word(memory_,controller_+(kind==Kind::Analog?0:4))!=binding.channel||
        memory_.native_address(binding.descriptor_source,0x100)!=binding.native_descriptors||
        memory_.native_address(binding.buffer_source,binding.bytes)!=binding.native_buffer||
        word(memory_,binding.descriptor_source)!=binding.buffer_physical||word(memory_,binding.descriptor_source+4)!=0xc0001000||
        memory_.view(binding.channel+0x24,2)[0]!=std::byte{1}||memory_.view(binding.channel+0x24,2)[1]!=std::byte{})
        throw std::runtime_error("Native PCM source descriptor/buffer binding changed beneath its retained owner");
    const auto states=channels_->diagnostics().endpoints;const auto endpoint=std::ranges::find_if(states,[&](const auto& row){return row.kind==kind;});
    if(endpoint==states.end()||endpoint->generation!=binding.endpoint_generation||endpoint->retired||endpoint->faulted||
        endpoint->resetting||!endpoint->descriptor_table_bound)
        throw std::runtime_error("Native PCM source buffer belongs to a retired channel reset generation");
    const auto status=endpoints_.at(kind)->status();if(status.device_removed||!status.paused)
        throw std::runtime_error("Native PCM source buffer escaped its paused prerequisite endpoint");
    return binding;
}
SourceRuntimeSoundPcmBuffers::View SourceRuntimeSoundPcmBuffers::buffer(Kind kind)const{const auto& binding=resolve(kind);return {binding,memory_.view(binding.buffer_source,binding.bytes)};}
SourceRuntimeSoundPcmBuffers::State SourceRuntimeSoundPcmBuffers::diagnostics()const{require_thread();std::vector<Binding> rows;for(const auto& [kind,row]:bindings_){(void)kind;rows.push_back(row);}return {std::move(rows),generation_,retired_,fault_,false,false};}
void SourceRuntimeSoundPcmBuffers::retire(Kind kind){
    require_thread();const auto at=bindings_.find(kind);if(at==bindings_.end())return;
    channels_->cancel_native_consumer(kind);
    if(const auto borrower=prepared_borrowers_.find(kind);borrower!=prepared_borrowers_.end()){
        if(const auto lease=borrower->second.lock())lease->cancel();
        prepared_borrowers_.erase(borrower);
    }
    {std::lock_guard lock(registry_mutex);const auto registered=descriptor_bindings.find(at->second.native_descriptors);
        if(registered!=descriptor_bindings.end()&&registered->second==this)descriptor_bindings.erase(registered);}
    endpoints_.erase(kind);bindings_.erase(at);
}
bool SourceRuntimeSoundPcmBuffers::channel_quiescent(Kind kind)const noexcept{return std::this_thread::get_id()==thread_&&!bindings_.contains(kind);}
void SourceRuntimeSoundPcmBuffers::retire(){require_thread();if(retired_)return;while(!bindings_.empty())retire(bindings_.begin()->first);retired_=true;channels_.reset();pages_.reset();ep_.reset();dma_.reset();}
bool SourceRuntimeSoundPcmBuffers::quiescent()const noexcept{return std::this_thread::get_id()==thread_&&retired_;}
}
