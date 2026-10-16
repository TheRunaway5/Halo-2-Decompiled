#include "halo2/source_runtime_sound_pio_resources.hpp"
#include "halo2/source_sound_pio_storage_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <mutex>
#include <stdexcept>

namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){const auto b=m.view(p,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;}
std::mutex registry_mutex;
std::map<std::uint64_t,const SourceRuntimeSoundPioResources*> registry;
struct Boundary{};
bool same(const SourceRuntimeSoundDmaResources::Allocation& a,const SourceRuntimeSoundDmaResources::Allocation& b){return a.source==b.source&&a.physical==b.physical&&a.requested==b.requested&&a.capacity==b.capacity&&a.native==b.native&&a.source_descriptor==b.source_descriptor;}
}
struct SourceRuntimeSoundPioResources::Queue {
    struct Command {bool selection{};std::uint32_t payload{};NativeRuntimeAddress destination{};};
    // Each real slot owns one source DWORD payload plus typed metadata. FREE
    // counts payload bytes, never sizeof(metadata) or invented hardware space.
    std::array<Command,8> slots{};
    std::size_t head{},count{};
};
struct SourceRuntimeSoundPioResources::CpuMemory final:SourceSoundPioActionMemory {
    SourceRuntimeSoundPioResources& p;
    explicit CpuMemory(SourceRuntimeSoundPioResources& owner):p(owner){}
    void read(std::uint32_t address,std::span<std::byte> bytes)const override{
        p.require_live();
        if(address==0xfe820010&&bytes.size()==4&&p.running_&&p.pc_==0x38e0ce){const auto value=p.available_payload_bytes();for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));return;}
        if(std::uint64_t(address)+bytes.size()>0xfe800000u)throw std::runtime_error("Unowned native PIO read remains required");
        p.memory_.read(address,bytes);
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes)override{
        p.require_live();if(std::uint64_t(address)+bytes.size()>0xfe800000u)throw std::runtime_error("Native PIO actions require their exact action-store path");p.memory_.write(address,bytes);
    }
    void write_pio_action(std::uint32_t pc,std::uint32_t address,std::span<const std::byte> bytes)override{p.write_action(pc,address,bytes);}
};
SourceRuntimeSoundPioResources::SourceRuntimeSoundPioResources(SourceRuntimeMemory& memory,
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma,std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep,
    std::shared_ptr<SourceRuntimeSoundTableLimits> tables,std::shared_ptr<SdlSoundPlatform> output,
    SourceRuntimeSoundResetResources::ControlLease control):memory_(memory),dma_(std::move(dma)),ep_(std::move(ep)),
    tables_(std::move(tables)),output_(std::move(output)),control_(std::move(control)),generation_(control_.generation()){
    require_live();
    const auto table_state=tables_->diagnostics();
    if(std::ranges::any_of(table_state.ranges,[](const auto& r){return !r.source_limit_observed;})||
        std::ranges::any_of(table_state.contexts,[](const auto& c){return !c.base_observed||!c.length_observed;}))
        throw std::invalid_argument("Native PIO lacks the genuinely observed table capacities/physical contexts");
    const auto rows=dma_->allocations();
    auto allocation=[&](std::uint32_t descriptor){const auto at=std::ranges::find_if(rows,[&](const auto& r){return r.source_descriptor==descriptor;});if(at==rows.end())throw std::invalid_argument("Native PIO allocation descriptor is absent");return *at;};
    notifiers_=allocation(0x3913a0);input_page_=allocation(0x391410);
    if(notifiers_.requested!=0x4020||input_page_.requested!=0x1000||(input_page_.physical&0xfffu))throw std::invalid_argument("Native PIO notifier/input page extent differs from authentic resources");
    memory_.validate_mutable_view(notifiers_.source,notifiers_.requested);
    // Allocate all local metadata before publishing exclusive ownership.
    queue_=std::make_unique<Queue>();cpu_=std::make_unique<CpuMemory>(*this);
    if(reinterpret_cast<std::uintptr_t>(queue_.get())<=0xffffffffu)throw std::runtime_error("Native PIO queue lacks actual native64 storage");
    {std::lock_guard lock(registry_mutex);if(!registry.emplace(notifiers_.native.value,this).second)throw std::invalid_argument("Actual native PIO notifier/controller domain already has an owner");claimed_=true;}
}
SourceRuntimeSoundPioResources::~SourceRuntimeSoundPioResources(){if(std::this_thread::get_id()!=thread_)std::terminate();retire();}
void SourceRuntimeSoundPioResources::require_thread()const{if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native PIO requires its creating owner thread");}
void SourceRuntimeSoundPioResources::require_live()const{
    require_thread();if(retired_||fault_||!dma_||!ep_||!tables_||!output_||!generation_)throw std::runtime_error("Native PIO retained resources are not live");
    control_.require_bank(memory_);control_.require_output(*output_);
    if(control_.generation()!=generation_||dma_->device_id()!=output_->device_id())throw std::runtime_error("Native PIO actual worker/output generation changed");
    tables_->require_control(memory_,control_);
    const auto ep=ep_->diagnostics();const auto rows=dma_->allocations();
    if(rows.size()!=16||ep.output_device!=output_->device_id()||ep.retired||ep.faulted||ep.callback_leases||ep.active_copies)throw std::runtime_error("Native PIO source transactions require the same quiescent EP resource domain");
    for(const auto& r:rows)if(memory_.native_address(r.source,r.capacity)!=r.native||word(memory_,r.source_descriptor)!=r.source||word(memory_,r.source_descriptor+4)!=r.physical||word(memory_,r.source_descriptor+8)!=r.requested)throw std::invalid_argument("Native PIO DMA page belongs to another actual source bank");
    for(const auto* r:{&ep.layout.data,&ep.layout.scatter_gather})if(std::ranges::none_of(rows,[&](const auto& own){return same(*r,own);}))throw std::invalid_argument("Native PIO EP/DMA page identity differs");
    if(claimed_){std::lock_guard lock(registry_mutex);const auto at=registry.find(notifiers_.native.value);if(at==registry.end()||at->second!=this)throw std::runtime_error("Native PIO lost its exact exclusive domain claim");}
}
void SourceRuntimeSoundPioResources::require_control(const SourceRuntimeMemory& bank,const SourceRuntimeSoundResetResources::ControlLease& control)const{require_thread();if(&bank!=&memory_||!control_.same_owner(control))throw std::invalid_argument("Native PIO belongs to another bank or actual worker");require_live();}
std::uint32_t SourceRuntimeSoundPioResources::available_payload_bytes()const{require_live();return std::uint32_t((queue_->slots.size()-queue_->count)*4);}
bool SourceRuntimeSoundPioResources::enqueue(bool selection){
    require_live();if(queue_->count==queue_->slots.size())return false;
    const auto view=tables_->bound_record(SourceRuntimeSoundTableLimits::Table::InputScatterGather,0x7ff);
    const auto page=tables_->resolve_physical(SourceRuntimeSoundTableLimits::Context::InputScatterGather,input_page_.physical,0x1000);
    if(view.bytes.size()!=8||page.source!=input_page_.source||page.native!=input_page_.native)throw std::invalid_argument("Native PIO input SGE/page is not the actual retained context");
    memory_.validate_mutable_view(view.source,8);
    control_.run(generation_,[&]{auto& slot=queue_->slots[(queue_->head+queue_->count)%queue_->slots.size()];slot={selection,selection?0x7ffu:input_page_.physical,view.native};++queue_->count;});return true;
}
void SourceRuntimeSoundPioResources::write_action(std::uint32_t pc,std::uint32_t address,std::span<const std::byte> bytes){
    require_live();if(!running_||pc!=pc_||bytes.size()!=4)throw std::invalid_argument("Native PIO source action lacks its authentic CPU context");
    std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);
    if(tables_->apply_physical_base(pc,address,value))return;
    if(pc==0x38e136&&address==0xfe80115c&&value==notifiers_.physical&&!notifier_published_){
        control_.run(generation_,[&]{notifier_published_=true;});return;
    }
    if(pc==0x38e159&&address==0xfe820804&&value==0x7ff&&notifier_published_&&!selection_queued_&&!input_bound_){
        if(!enqueue(true))throw std::runtime_error("Actual native PIO queue has no selection payload slot");selection_queued_=true;return;
    }
    if(pc==0x38e168&&address==0xfe820808&&value==input_page_.physical&&selection_queued_&&!input_queued_){
        if(!enqueue(false))throw std::runtime_error("Actual native PIO queue has no input-page payload slot");input_queued_=true;return;
    }
    throw std::runtime_error("Unsupported native PIO method remains required");
}
std::uint32_t SourceRuntimeSoundPioResources::run(std::uint32_t entry,std::uint32_t stop,SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    require_live();if(running_)throw std::runtime_error("Native PIO source execution cannot reenter");running_=true;auto scoped=services;
    scoped.instruction=[&](auto pc){pc_=pc;if(pc==stop)throw Boundary{};if(services.instruction)services.instruction(pc);};
    scoped.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Unsupported native PIO CPU dependency remains required");};
    try{source_run_sound_pio_storage_tu15(*cpu_,entry,abi,fp,scoped,observe);throw std::runtime_error("Native PIO crossed its suspended source boundary");}
    catch(const Boundary&){running_=false;return stop;}
    catch(...){running_=false;abort();throw;}
}
std::uint32_t SourceRuntimeSoundPioResources::run_prefix(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){require_live();const auto state=tables_->diagnostics();if(notifier_published_||queue_->count||std::ranges::any_of(state.ranges,[](const auto& r){return r.source_base_observed;})||abi.gp[1]!=control_.source_apu()+8||word(memory_,abi.gp[4])!=0x38ecb5)throw std::invalid_argument("Native PIO prefix was already prepared or lacks the genuine38ECB0 caller frame");return run(0x38e0ce,0x38e140,abi,fp,services,observe);}
std::uint32_t SourceRuntimeSoundPioResources::run_input_component(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    require_live();const auto voice=tables_->bound_record(SourceRuntimeSoundTableLimits::Table::Voice,0);if(!notifier_published_||selection_queued_||queue_->count||abi.gp[0]!=0xfff||abi.gp[1]!=256||abi.gp[2]!=255||abi.gp[6]!=voice.source||word(memory_,abi.gp[4])!=control_.source_apu()+8||word(memory_,abi.gp[4]+4)!=0x38ecb5)throw std::invalid_argument("Native PIO input component lacks its exact prepared resource/caller state");const auto end=run(0x38e159,0x38e16d,abi,fp,services,observe);try{drain();return end;}catch(...){abort();throw;}
}
bool SourceRuntimeSoundPioResources::queue_native_input_page(){require_live();if(!input_bound_||selected_!=0x7ff)throw std::runtime_error("Native input page queue lacks its executed source selection/binding");return enqueue(false);}
SourceRuntimeSoundPioResources::NotifierView SourceRuntimeSoundPioResources::notifier(std::uint16_t voice)const{
    require_live();if(!notifier_published_||voice>=256)throw std::out_of_range("Native PIO notifier row lacks its actual published voice domain");
    const auto source=notifiers_.source+32+std::uint32_t(voice)*64;const auto native=memory_.native_address(source,64);
    if(native.value!=notifiers_.native.value+32+std::uint64_t(voice)*64)throw std::runtime_error("Native PIO notifier row lost its actual contiguous backing");return {source,native,memory_.view(source,64)};
}
void SourceRuntimeSoundPioResources::drain(){
    require_live();const auto view=tables_->bound_record(SourceRuntimeSoundTableLimits::Table::InputScatterGather,0x7ff);
    try{while(queue_->count){const auto command=queue_->slots[queue_->head];if(command.destination!=view.native)throw std::runtime_error("Native PIO queued destination changed");
        // Real mutable borrowing (with permission and mutation observers)
        // belongs to the creating executor immediately before the synchronous
        // worker job. Selection-only commands do not borrow writable bytes.
        const auto destination=command.selection?std::span<std::byte>{}:memory_.mutable_view(view.native,8);
        control_.run(generation_,[&]{
        if(command.selection){if(command.payload!=0x7ff)throw std::runtime_error("Native PIO input selection changed");selected_=command.payload;input_record_=view.native;}
        else{if(selected_!=0x7ff||command.payload!=input_page_.physical)throw std::runtime_error("Native PIO input bind lacks its actual selection/page");std::array<std::byte,8> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(command.payload>>(8*i));std::copy(raw.begin(),raw.end(),destination.begin());input_bound_=true;}
        queue_->head=(queue_->head+1)%queue_->slots.size();--queue_->count;++completed_;
    });}}catch(...){abort();throw;}
}
SourceRuntimeSoundPioResources::State SourceRuntimeSoundPioResources::diagnostics()const{require_thread();return {NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(queue_.get())},notifiers_.native,input_record_,input_page_.native,32u,queue_?std::uint32_t((queue_->slots.size()-queue_->count)*4):0u,queue_?std::uint32_t(queue_->count):0u,selected_,generation_,completed_,cancelled_,notifier_published_,input_bound_,retired_,fault_,false};}
void SourceRuntimeSoundPioResources::release_claim()noexcept{std::lock_guard lock(registry_mutex);if(!claimed_)return;const auto at=registry.find(notifiers_.native.value);if(at!=registry.end()&&at->second==this)registry.erase(at);claimed_=false;}
void SourceRuntimeSoundPioResources::abort()noexcept{if(std::this_thread::get_id()!=thread_)std::terminate();if(fault_)return;fault_=true;try{control_.abort_transaction();control_.fence_retirement();}catch(...){std::terminate();}if(queue_){cancelled_+=queue_->count;queue_->count=0;}release_claim();}
void SourceRuntimeSoundPioResources::retire(){require_thread();if(retired_)return;if(running_)throw std::runtime_error("Cannot retire executing native PIO CPU work");control_.fence_retirement();if(queue_){cancelled_+=queue_->count;queue_->count=0;}release_claim();retired_=true;}
}
