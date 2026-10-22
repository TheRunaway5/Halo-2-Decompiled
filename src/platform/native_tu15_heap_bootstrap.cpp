#include "halo2/native_tu15_heap_bootstrap.hpp"
#include "halo2/source_heap_growth_tu15.hpp"
#include "physics/physics_renderer_storage_call_frames.hpp"
#include <algorithm>
#include <algorithm>

namespace halo2 {
namespace {
void put(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));memory.write(address,bytes);
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
}
NativeTu15HeapBootstrap::NativeTu15HeapBootstrap()
    :memory_(native_tu15_process_heap_memory()),virtual_memory_(memory_,0x08000000u) {
    // Explicit native caller storage, matching the existing live executor's
    // address policy. Zero initialization constructs no pool or game object.
    constexpr std::uint32_t stack=0x600f0000u,fs=0x60200000u,parameters=fs+0x1000u;
    memory_.mount(0x60000000u,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 process startup call stack");
    memory_.mount(fs,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 process startup exception chain and heap parameters");
    put(memory_,fs,0xffffffffu);put(memory_,parameters,0x30);put(memory_,stack,0x60300000u);
    // Native process allocation instrumentation has no Xbox kernel callback.
    // RtlAllocateHeap still performs its original FS:[20]+250 load/test.
    put(memory_,fs+0x20u,0x60214000u);
    memory_.mount(0x60214250u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadOnly,
        "native process has no source kernel allocation-notification callback");
    const auto arguments=native_tu15_process_heap_arguments(parameters);
    for(unsigned i=0;i<arguments.size();++i)put(memory_,stack+4+i*4,arguments[i]);
    SourcePhysicsContactRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=2;abi.fs_base=fs;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    const PhysicsPlatformLockServices unneeded{
        []()->std::uint32_t{throw std::runtime_error("Process heap construction reached a thread identity owner");},
        [](auto){throw std::runtime_error("Process heap construction reached a wait owner");},
        [](auto){throw std::runtime_error("Process heap construction reached a wake owner");}};
    source_run_process_heap_with_native_platform(memory_,0x2da83au,abi,
        SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},{},virtual_memory_,locks_,unneeded);
    if(!abi.gp[0] || abi.gp[4]!=stack+28)
        throw std::runtime_error("Native TU1.5 process heap did not return its owned allocation and complete caller frame");
    heap_=memory_.native_address(abi.gp[0],0x1000);
    // Native publication of the actual completed constructor result, matching
    // the existing source process owner. This is not a supplied callee result.
    put(memory_,0x56ee28u,memory_.source_address(heap_,0x1000));
}
std::uint32_t NativeTu15HeapBootstrap::current_thread_token() {
    std::lock_guard guard(service_mutex_);const auto id=std::this_thread::get_id();
    if(const auto at=thread_tokens_.find(id);at!=thread_tokens_.end())return at->second;
    if(next_thread_token_>UINT32_MAX)throw std::runtime_error("Native process thread-token namespace exhausted");
    const auto token=std::uint32_t(next_thread_token_++);thread_tokens_.emplace(id,token);return token;
}
PhysicsPlatformLockServices NativeTu15HeapBootstrap::lock_services() {
    const auto event=[this](std::uint32_t section) {
        std::lock_guard guard(service_mutex_);
        if(const auto at=lock_events_.find(section);at!=lock_events_.end())return at->second;
        const auto created=synchronization_.services().create_semaphore(0,1);
        if(created.error)throw std::system_error(created.error);
        lock_events_.emplace(section,created.handle);return created.handle;
    };
    return {[this]{return current_thread_token();},
        [this,event](auto section) {
            const auto result=synchronization_.services().wait(event(section),0xffffffffu);
            if(result.state!=NativeSynchronizationWaitState::Acquired)
                throw std::runtime_error("Native process heap critical-section wait failed");
        },
        [this,event](auto section) {
            const auto result=synchronization_.services().release_semaphore(event(section),1);
            if(result.error)throw std::system_error(result.error);
        }};
}
bool NativeTu15HeapBootstrap::try_crt_allocation_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsContactRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices& services,
    const SourcePhysicsContactRecordObserver& observer) {
    if(call.target!=0x2db419u && call.target!=0x2de70eu)return false;
    auto& memory=memory_;
    if(&bank!=&memory_ || call.tail || abi.fs_base!=0x60200000u ||
        call.instruction!=(call.target==0x2db419u?0x3323abu:0x3323b1u) ||
        word(memory,abi.gp[4])!=call.instruction+5u)
        throw std::invalid_argument("Native CRT allocation bank, FS, caller or return frame differs");
    source_run_physics_world_with_native_platform(memory,
        call.target==0x2db419u?0x2d7969u:0x2dac5eu,abi,
        SourcePhysicsContactImage::TitleUpdate15,fp,services,locks_,lock_services(),observer);
    return true;
}
bool NativeTu15HeapBootstrap::try_renderer_allocation_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsContactRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices& services,
    const SourcePhysicsContactRecordObserver& observer) {
    if(call.target!=0x32a3bbu)return false;
    const auto frame=std::find_if(physics_renderer_storage_frames::frames.begin(),physics_renderer_storage_frames::frames.end(),
        [&](const auto& row){return row.site==call.instruction && row.target==call.target;});
    if(&bank!=&memory_ || call.tail || abi.fs_base!=0x60200000u || frame==physics_renderer_storage_frames::frames.end() ||
        word(memory_,abi.gp[4])!=frame->return_address)
        throw std::invalid_argument("Native renderer allocation bank, FS, caller or return frame differs");
    source_run_physics_world_with_native_platform(memory_,frame->entry,abi,SourcePhysicsContactImage::TitleUpdate15,
        fp,services,locks_,lock_services(),observer);return true;
}
bool NativeTu15HeapBootstrap::try_crt_growth_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsContactRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices& services,
    const SourcePhysicsContactRecordObserver& observer,const SourceProcessHeapMemoryObserver& memory_observe) {
    if(call.target!=0x2ddeb9u && call.target!=0x2dd7c9u)return false;
    const bool extend=call.target==0x2ddeb9u;
    if(&bank!=&memory_ || call.tail || abi.fs_base!=0x60200000u ||
        call.instruction!=(extend?0x2daeb1u:0x2db217u) || word(memory_,abi.gp[4])!=(extend?0x2de966u:0x2decccu) ||
        word(memory_,abi.gp[4]+4u)!=memory_.source_address(heap_,0x1000))
        throw std::invalid_argument("Native CRT growth bank, FS, caller or return frame differs");
    auto joined=services;
    joined.required_owner=[&](auto& memory,const auto& callee,auto& state,const auto& observe) {
        if(callee.target==0x800000b8u && callee.instruction==0x2dd3d8u) {
            const auto sp=state.gp[4],bp=state.gp[5];
            if(&memory!=&memory_ || callee.tail || word(memory_,sp)!=0x2dd3deu || word(memory_,sp+4)!=bp+12u ||
                word(memory_,sp+12)!=state.gp[3] || state.gp[3]!=word(memory_,bp+16u))
                throw std::invalid_argument("Native ordinary heap commitment frame differs");
            const auto base_cell=word(memory_,sp+4),size_cell=word(memory_,sp+12);
            const SourceVirtualAllocationRequest request{word(memory_,base_cell),word(memory_,sp+8),word(memory_,size_cell),word(memory_,sp+16),word(memory_,sp+20)};
            if(request.zero_bits || request.allocation_type!=0x1000u || request.protection!=4u || !request.base)
                throw std::invalid_argument("Native ordinary heap commitment arguments differ");
            memory_.mutable_view(base_cell,4);memory_.mutable_view(size_cell,4);
            const auto result=virtual_memory_.allocate(request);
            if(memory_observe)memory_observe({callee.instruction,base_cell,size_cell,request,result});
            put(memory_,base_cell,result.base);put(memory_,size_cell,result.bytes);state.gp[0]=result.status;state.gp[4]=sp+24u;return;
        }
        if(!services.required_owner)throw std::runtime_error("Native ordinary heap growth has an unowned source dependency");
        services.required_owner(memory,callee,state,observe);
    };
    source_run_heap_growth_tu15(memory_,call.target,abi,fp,joined,observer);
    return true;
}
}
