#include "halo2/source_runtime_sound_event_port.hpp"
#include "audio/runtime/source_runtime_sound_interrupt_gate.hpp"
#include "platform/native_event_resources.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <exception>
#include <stdexcept>
#include <system_error>
#include <thread>

namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    const auto bytes=m.view(p,4);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
void store(SourceRuntimeMemory& m,std::uint32_t site,std::uint32_t p,unsigned n,
    std::uint32_t value,const SourcePhysicsContactRecordObserver& observe) {
    SourcePhysicsContactRecordStore row{site,p,std::vector<std::byte>(n),std::vector<std::byte>(n)};
    m.read(p,row.before);for(unsigned i=0;i<n;++i)row.written[i]=std::byte(value>>(8*i));
    m.write(p,row.written);if(observe)observe(row);
}
}
struct SourceRuntimeSoundEventPort::Impl {
    SourceRuntimeMemory& memory;
    SourceRuntimeSoundResetResources::ControlLease control;
    SourceRuntimeSoundFrontendEvents& frontend;
    const std::shared_ptr<SdlSoundPlatform> output;
    const std::thread::id thread=std::this_thread::get_id();
    const std::uint64_t generation;
    const std::uint32_t apu,device;
    struct Route {
        std::uint64_t identity;
        std::uint32_t cookie;
        std::unique_ptr<native_events::Signal> event;
        std::uint32_t routine{},context{};
        bool initialized{},connected{},pending{};std::uint64_t delivered{},cancelled{};
    };
    std::shared_ptr<Route> route;
    std::shared_ptr<SourceRuntimeSoundInterruptGate> source_policy;
    int dispatcher=-1,timer=-1;
    std::unique_ptr<native_events::Poller> poller;
    std::shared_ptr<native_events::Timer> timer_resource;
    bool timer_initialized{},retired{},fault{};
    std::shared_ptr<const TimerLease> timer_owner;
    static std::atomic<std::uint64_t> next_identity;
    Impl(SourceRuntimeMemory& m,SourceRuntimeSoundResetResources& reset,
        SourceRuntimeSoundFrontendEvents& f,std::shared_ptr<SdlSoundPlatform> o)
        :memory(m),control(reset.control_lease()),frontend(f),output(std::move(o)),
         generation(control.generation()),apu(control.source_apu()),device(reset.state().output_device) {
        control.require_bank(memory);frontend.require_binding(memory,control);
        if(!apu||!generation||!output||output->device_id()!=device||output->output_channels()!=2||frontend.state().generation!=generation)
            throw std::invalid_argument("Native sound event port lacks its actual shared output/reset/frontend generation");
        poller=std::make_unique<native_events::Poller>();dispatcher=poller->descriptor();
    }
    void require_thread()const {
        if(std::this_thread::get_id()!=thread)throw std::runtime_error("Native sound event port requires its creating thread");
    }
    void require_live()const {
        require_thread();if(fault||retired)throw std::runtime_error("Native sound event port is retired or faulted");
        control.require_bank(memory);frontend.require_binding(memory,control);
        if(control.generation()!=generation||output->device_id()!=device||output->output_channels()!=2)
            throw std::runtime_error("Native sound event port lost its actual generation/output");
    }
    void frame(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,
        const SourcePhysicsWorldRuntimeAbi& abi,std::uint32_t target,std::uint32_t cell,std::uint32_t returned,unsigned bytes)const {
        require_live();if(&bank!=&memory||call.tail||call.target!=target||word(memory,cell)!=target)
            throw std::invalid_argument("Native sound registration source bank/import differs");
        memory.view(abi.gp[4],bytes);
        if(word(memory,abi.gp[4])!=returned)throw std::invalid_argument("Native sound registration return frame differs");
    }
    void make_route() {
        if(route)throw std::invalid_argument("Native sound route is already registered");
        auto event=std::make_unique<native_events::Signal>();
        const auto identity=next_identity.fetch_add(1,std::memory_order_relaxed);
        if(identity>=0x10000ull)throw std::runtime_error("Native sound route cookie namespace exhausted");
        route=std::make_shared<Route>();route->identity=0x100000000ull+identity;
        route->cookie=0xf9060000u+std::uint32_t(identity);route->event=std::move(event);
    }
    void initialize_timer() {
        if(timer_initialized)throw std::invalid_argument("Native sound timer is already initialized");
        auto resource=std::make_shared<native_events::Timer>(native_events::Clock::Realtime);
        poller->add(resource->descriptor(),2);
        timer=resource->descriptor();timer_resource=std::move(resource);timer_initialized=true;
    }
    void cancel_native() {
        if(route&&route->connected){poller->remove(route->event->descriptor());route->connected=false;}
        if(timer_resource)(void)timer_resource->cancel();
    }
};
std::atomic<std::uint64_t> SourceRuntimeSoundEventPort::Impl::next_identity{1};
SourceRuntimeSoundEventPort::SourceRuntimeSoundEventPort(SourceRuntimeMemory& m,SourceRuntimeSoundResetResources& r,
    SourceRuntimeSoundFrontendEvents& f,std::shared_ptr<SdlSoundPlatform> output)
    :impl_(std::make_unique<Impl>(m,r,f,std::move(output))){}
SourceRuntimeSoundEventPort::~SourceRuntimeSoundEventPort(){
    if(impl_&&std::this_thread::get_id()!=impl_->thread)std::terminate();
    if(impl_&&impl_->timer_owner)std::terminate();
    if(impl_)try{retire_generation();}catch(...){}
}
bool SourceRuntimeSoundEventPort::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,
    SourcePhysicsWorldRuntimeAbi& abi,const SourcePhysicsContactRecordObserver& observe) {
    auto& p=*impl_;
    if(call.instruction==0x3893bd) {
        p.frame(bank,call,abi,0x8000006b,0x41b70c,0x3893bf,16);const auto sp=abi.gp[4];
        p.memory.validate_mutable_view(p.apu+0x4c8,28);
        if(!p.frontend.try_initialize_dpc(bank,call,abi))throw std::runtime_error("Native first sound DPC resource was not registered");
        try{store(p.memory,call.instruction,p.apu+0x4c8,2,0x13,observe);store(p.memory,call.instruction,p.apu+0x4ca,1,0,observe);
            store(p.memory,call.instruction,p.apu+0x4d4,4,0x38918c,observe);store(p.memory,call.instruction,p.apu+0x4d8,4,p.apu,observe);
        }catch(...){fault_parent_call();throw;}
        if(abi.gp[4]!=sp+16)throw std::runtime_error("Native sound DPC registration returned an invalid actual frame");return true;
    }
    if(call.instruction==0x3893c8) {
        p.frame(bank,call,abi,0x80000071,0x41b708,0x3893ce,12);const auto sp=abi.gp[4];
        if(word(p.memory,sp+4)!=p.apu+0x4e8||word(p.memory,sp+8))throw std::invalid_argument("Native sound timer initializer arguments differ");
        p.memory.validate_mutable_view(p.apu+0x4e8,40);p.initialize_timer();
        // Exact already-verified primary113 expressions, with an actual native
        // native scheduler now owning this particular initialized record.
        try{const auto at=p.apu+0x4e8;const auto put=[&](unsigned off,unsigned n,std::uint32_t v){store(p.memory,call.instruction,at+off,n,v,observe);};
            put(0,1,8);put(3,1,0);put(2,1,10);put(4,4,0);put(12,4,at+8);put(8,4,at+8);put(28,4,0);put(24,4,0);put(16,4,0);put(20,4,0);put(36,4,0);
        }catch(...){fault_parent_call();throw;}
        abi.gp[4]=sp+12;return true;
    }
    if(call.instruction==0x3893e4) {
        p.frame(bank,call,abi,0x8000002c,0x41b7a8,0x3893ea,12);const auto sp=abi.gp[4],out=word(p.memory,sp+8);
        if(word(p.memory,sp+4)!=5||out!=abi.gp[0]||out!=sp+0x1c)throw std::invalid_argument("Native sound event-route request differs from original3893E4");
        p.memory.validate_mutable_view(out,1);p.make_route();
        // Native platform policy: an actual endpoint cookie and passive source
        // callback domain. This is neither a PCI vector nor physical IRQL.
        try{store(p.memory,call.instruction,out,1,0,observe);}catch(...){fault_parent_call();throw;}
        abi.gp[0]=p.route->cookie;abi.gp[4]=sp+12;return true;
    }
    if(call.instruction==0x3893ff) {
        p.frame(bank,call,abi,0x8000006d,0x41b7b4,0x389405,32);const auto sp=abi.gp[4];
        if(!p.route||p.route->initialized||word(p.memory,sp+4)!=0x3912d8||word(p.memory,sp+8)!=0x38936c||word(p.memory,sp+12)!=p.apu||
            word(p.memory,sp+16)!=p.route->cookie||(word(p.memory,sp+20)&255)||word(p.memory,sp+24)||word(p.memory,sp+28)!=1)
            throw std::invalid_argument("Native sound interrupt initializer lost its actual endpoint/callback/domain arguments");
        p.memory.validate_mutable_view(0x3912d8,8);
        p.route->routine=0x38936c;p.route->context=p.apu;p.route->initialized=true;
        // Publish only authenticated routine/context fields used by the source
        // cleanup contract. No private PCI/IRQL/dispatch-code image is supplied.
        try{store(p.memory,call.instruction,0x3912d8,4,p.route->routine,observe);store(p.memory,call.instruction,0x3912dc,4,p.route->context,observe);}
        catch(...){fault_parent_call();throw;}
        abi.gp[4]=sp+32;return true;
    }
    if(call.instruction==0x389406) {
        p.frame(bank,call,abi,0x80000062,0x41b7b0,0x38940c,8);const auto sp=abi.gp[4];
        if(word(p.memory,sp+4)!=0x3912d8||!p.route||!p.route->initialized||p.route->connected||
            word(p.memory,0x3912d8)!=p.route->routine||word(p.memory,0x3912dc)!=p.apu)
            throw std::invalid_argument("Native sound interrupt connection lacks its actual initialized callback");
        p.poller->add(p.route->event->descriptor(),1);
        p.route->connected=true;abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;abi.gp[4]=sp+8;return true;
    }
    return false;
}
void SourceRuntimeSoundEventPort::signal_known_idle() {
    auto& p=*impl_;p.require_live();const auto frontend=p.frontend.state();
    if(!p.route||!p.route->connected||p.route->pending||frontend.generation!=p.generation||
        frontend.mode!=SourceRuntimeSoundFrontendEvents::Mode::Trapped||frontend.interrupt_status!=0x11)
        throw std::runtime_error("Native sound signal has no actual unique known idle trap/connected route");
    p.route->event->signal();
    p.route->pending=true;
}
bool SourceRuntimeSoundEventPort::dispatch_known_idle(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    auto& p=*impl_;p.require_live();if(!p.route||!p.route->connected)throw std::runtime_error("Native sound route is not connected");
    // Gate the actual APU source ISR before consuming its real native event or
    // callback frame. Kernel timer expiry is a separate delivery domain.
    if(p.source_policy&&p.source_policy->blocks_source_irq())return false;
    const auto sp=abi.gp[4];
    if(word(p.memory,sp+4)!=0x3912d8||word(p.memory,sp+8)!=p.apu)
        throw std::invalid_argument("Native registered sound callback frame differs before consuming its actual event");
    if(!p.poller->ready(1))return false;
    const auto received=p.route->event->take();
    if(!received||*received!=1||!p.route->pending)
        throw std::runtime_error("Native sound event lost its actual retained unique signal");
    try{if(!p.frontend.dispatch_registered_interrupt(abi,fp,services,observe))throw std::runtime_error("Actual registered sound source callback did not execute");
        p.route->pending=false;++p.route->delivered;return true;
    }catch(...){fault_parent_call();throw;}
}
SourceRuntimeSoundEventPort::State SourceRuntimeSoundEventPort::state()const {
    const auto& p=*impl_;p.require_thread();return {p.generation,p.route?p.route->identity:0,
        NativeRuntimeAddress{p.route?reinterpret_cast<std::uintptr_t>(p.route.get()):0},p.route?p.route->event->descriptor():-1,p.timer,p.dispatcher,
        p.timer_initialized,p.route&&p.route->initialized,p.route&&p.route->connected,p.retired,p.fault,p.route?p.route->cookie:0,p.route?p.route->delivered:0,p.route?p.route->cancelled:0,p.route&&p.route->pending};
}
std::shared_ptr<const SourceRuntimeSoundEventPort::TimerLease> SourceRuntimeSoundEventPort::claim_timer_binding(
    const SourceRuntimeMemory& memory,const SourceRuntimeSoundResetResources::ControlLease& control,
    SourceRuntimeSoundFrontendEvents& frontend) {
    auto& p=*impl_;const auto timer=require_timer_binding(memory,control);
    if(&frontend!=&p.frontend)throw std::invalid_argument("Native sound timer needs the identical retained frontend queue owner");
    frontend.require_binding(memory,control);
    if(p.timer_owner)throw std::runtime_error("Native sound timer already has its exclusive retained notification owner");
    // All fallible admission precedes publication. Allocation failure leaves
    // the original timer, queue and claim unchanged; shared_ptr assignment is
    // nonthrowing, and the borrowing constructor does no later fallible work.
    auto lease=std::make_shared<TimerLease>(TimerLease{control,p.generation,timer,p.timer_resource});p.timer_owner=lease;return lease;
}
int SourceRuntimeSoundEventPort::require_timer_lease(const SourceRuntimeMemory& memory,
    const SourceRuntimeSoundResetResources::ControlLease& control,const std::shared_ptr<const TimerLease>& lease)const {
    const auto& p=*impl_;const auto timer=require_timer_binding(memory,control);
    if(!lease||p.timer_owner!=lease||!lease->control.same_owner(control)||lease->generation!=p.generation||lease->descriptor!=timer||lease->resource!=p.timer_resource)
        throw std::invalid_argument("Native sound timer request lacks its exact exclusive retained owner lease");
    return timer;
}
void SourceRuntimeSoundEventPort::release_timer_lease(const std::shared_ptr<const TimerLease>& lease) {
    auto& p=*impl_;p.require_thread();
    if(!lease||p.timer_owner!=lease||!lease->control.same_owner(p.control)||lease->descriptor!=p.timer)
        throw std::invalid_argument("Native sound timer release belongs to another native owner");
    p.frontend.require_timer_queue_retired();lease->control.fence_retirement();
    if(!p.timer_resource->disarmed())
        throw std::runtime_error("Native sound timer owner released an armed native kernel timer");
    p.timer_owner.reset();
}
int SourceRuntimeSoundEventPort::require_timer_binding(const SourceRuntimeMemory& memory,
    const SourceRuntimeSoundResetResources::ControlLease& control)const {
    const auto& p=*impl_;p.require_thread();control.require_bank(memory);p.control.require_bank(memory);
    if(&memory!=&p.memory||!p.control.same_owner(control))
        throw std::invalid_argument("Native sound timer belongs to another actual bank/control worker");
    p.require_live();if(!p.timer_initialized||p.timer<0)
        throw std::runtime_error("Native sound timer has no actual initialized EventPort resource");
    const auto timer=p.apu+0x4e8;
    if(p.memory.view(timer,1)[0]!=std::byte{8}||p.memory.view(timer+2,1)[0]!=std::byte{10}||
        p.memory.view(timer+3,1)[0]!=std::byte{}||word(p.memory,timer+8)!=timer+8||
        word(p.memory,timer+12)!=timer+8||word(p.memory,timer+36))
        throw std::invalid_argument("Native shared sound timer record gained foreign type/waiters/period state");
    return p.timer;
}
std::shared_ptr<SourceRuntimeSoundInterruptGate> SourceRuntimeSoundEventPort::attach_disabled_source_policy(
    const SourceRuntimeMemory& memory,const SourceRuntimeSoundResetResources::ControlLease& control){
    auto& p=*impl_;p.require_thread();
    if(&memory!=&p.memory||!p.control.same_owner(control))
        throw std::invalid_argument("Native source interrupt policy belongs to another bank or actual control worker");
    p.require_live();if(p.source_policy)throw std::runtime_error("Native source interrupt dispatcher already has a policy owner");
    auto gate=std::make_shared<SourceRuntimeSoundInterruptGate>(&p.memory,p.control,p.generation,p.dispatcher);
    p.source_policy=gate;return gate;
}
void SourceRuntimeSoundEventPort::fault_parent_call()noexcept {
    auto& p=*impl_;if(std::this_thread::get_id()!=p.thread)std::terminate();p.fault=true;
    try{retire_generation();}catch(...){}
    p.control.abort_transaction();
}
SourceRuntimeSoundEventPort::CallScope::CallScope(SourceRuntimeSoundEventPort* owner)
    :owner_(owner),exceptions_(std::uncaught_exceptions()) {if(owner_)owner_->impl_->require_live();}
SourceRuntimeSoundEventPort::CallScope::~CallScope() {
    if(owner_&&std::uncaught_exceptions()>exceptions_)owner_->fault_parent_call();
}
void SourceRuntimeSoundEventPort::retire_generation() {
    auto& p=*impl_;p.require_thread();if(p.retired)return;
    if(p.source_policy){p.source_policy->retired=true;p.source_policy->dispatcher=-1;}
    p.cancel_native();
    if(p.route&&p.route->pending){
        (void)p.route->event->take();
        p.route->pending=false;++p.route->cancelled;
    }
    p.frontend.retire_generation();p.retired=true;
}
}
