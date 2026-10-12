#include "halo2/native_tu15_startup_timer.hpp"
#include "halo2/source_tu15_settings.hpp"
#include <bit>
#include <cerrno>
#include <chrono>
#include <system_error>
#if defined(__linux__)
#include <sys/timerfd.h>
#include <unistd.h>
#endif

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {
    const auto b=m.view(p,4);std::uint32_t v=0;
    for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
void put(SourceRuntimeMemory &m,std::uint32_t site,std::uint32_t p,unsigned width,
         std::uint32_t v,const SourcePhysicsContactRecordObserver &observe) {
    const auto old=m.view(p,width);
    SourcePhysicsContactRecordStore change{site,p,{old.begin(),old.end()},std::vector<std::byte>(width)};
    for(unsigned i=0;i<width;++i)change.written[i]=std::byte(v>>(8*i));
    m.write(p,change.written);if(observe)observe(change);
}
}
NativeTu15TimerClock native_tu15_timer_clock() {
    return {
        [] {
            const auto now=std::chrono::steady_clock::now().time_since_epoch();
            return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count()/100);
        },
        [] {
            const auto now=std::chrono::system_clock::now().time_since_epoch();
            // NT's1601 epoch relative to Unix1970, in100ns units.
            return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count()/100)+116444736000000000ull;
        }};
}
struct NativeTu15StartupTimer::State {
    SourceRuntimeMemory &memory;
    NativeTu15TimerClock clock;
    std::uint32_t head;
    int descriptor=-1;
    bool armed=false;
    State(SourceRuntimeMemory &m,NativeTu15TimerClock c,std::uint32_t h):memory(m),clock(std::move(c)),head(h) {}
    void validate_timer(bool inserted) const {
        memory.mutable_view(0x56ede0,40);memory.view(head,8);memory.view(0x56edc0,28);
        const auto timer=memory.view(0x56ede0,40),dpc=memory.view(0x56edc0,28);
        if(timer[0]!=std::byte{8} || timer[2]!=std::byte{10} || timer[3]!=std::byte(inserted) ||
            word(memory,0x56ede8)!=0x56ede8 || word(memory,0x56edec)!=0x56ede8 ||
            dpc[0]!=std::byte{0x13} || dpc[1]!=std::byte{0} || dpc[2]!=std::byte{0} ||
            word(memory,0x56edcc)!=0x2e009d || word(memory,0x56edd0)!=0)
            throw std::runtime_error("Native startup timer lost its initialized timer/DPC or has foreign waiters");
        const auto expected=inserted?0x56edf8u:head;
        if(word(memory,head)!=expected || word(memory,head+4)!=expected ||
            (inserted && (word(memory,0x56edf8)!=head || word(memory,0x56edfc)!=head)))
            throw std::runtime_error("Native startup timer lost its retained insertion links");
    }
    void detach(std::uint32_t site,const SourcePhysicsContactRecordObserver &observe) {
        put(memory,site,head,4,head,observe);put(memory,site,head+4,4,head,observe);
        put(memory,site,0x56ede3,1,0,observe);armed=false;
    }
};
NativeTu15StartupTimer::NativeTu15StartupTimer(SourceRuntimeMemory &m,NativeTu15TimerClock clock,std::uint32_t head)
    :state_(std::make_unique<State>(m,std::move(clock),head)) {
    if(!state_->clock.interrupt_time || !state_->clock.system_time || head>0xfffffff8u || (head&3))
        throw std::invalid_argument("Native timer requires owned clocks and an aligned retained list identity");
#if defined(__linux__)
    const int fd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC);
    if(fd<0)throw std::system_error(errno,std::generic_category(),"create native startup timer");
    try {
        std::array<std::byte,8> initial{};
        for(unsigned i=0;i<4;++i)initial[i]=initial[i+4]=std::byte(head>>(8*i));
        m.mount(head,initial,SourceRuntimeMemory::Access::ReadWrite,"native startup timer owned list head");
    }catch(...) {close(fd);throw;}
    state_->descriptor=fd;
#else
    throw std::runtime_error("Native startup timer resource is not implemented on this platform");
#endif
}
NativeTu15StartupTimer::~NativeTu15StartupTimer() {
    if(!state_)return;
#if defined(__linux__)
    if(state_->descriptor>=0)close(state_->descriptor);
#endif
    // Executor lifetime requires the retained bank to outlive this owner.
    // Retirement cancels this real resource and removes its owned links; it
    // never signals a source timer or invokes a shutdown callback.
    try {if(state_->armed)state_->detach(0,{});}catch(...) {}
    try {state_->memory.release(state_->head);}catch(...) {}
}
bool NativeTu15StartupTimer::backs(const SourceRuntimeMemory &m) const {return &state_->memory==&m;}
int NativeTu15StartupTimer::native_descriptor() const {return state_->descriptor;}
bool NativeTu15StartupTimer::try_call(const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe,const NativeTu15TimerObserver &timer_observe) {
    if(call.instruction!=0x2dc5e6u || call.target!=0x80000095u)return false;
    auto &s=*state_;const auto sp=abi.gp[4];s.memory.view(sp,20);
    if(call.tail || word(s.memory,sp)!=0x2e009cu || word(s.memory,sp+4)!=0x56ede0u || word(s.memory,sp+16)!=0x56edc0u)
        throw std::runtime_error("Native startup timer requires its actual complete source call frame");
    const auto raw=std::uint64_t(word(s.memory,sp+8))|(std::uint64_t(word(s.memory,sp+12))<<32);
    const auto interval=std::bit_cast<std::int64_t>(raw);
    if(interval>=0)throw std::runtime_error("Native startup timer requires the original relative one-shot domain");
    s.validate_timer(s.armed);s.memory.mutable_view(s.head,8);
    const auto now=s.clock.interrupt_time(),due=now-raw;
    const auto duration=std::uint64_t(0)-raw;
#if defined(__linux__)
    itimerspec request{};request.it_value.tv_sec=static_cast<time_t>(duration/10000000ull);
    request.it_value.tv_nsec=static_cast<long>((duration%10000000ull)*100);
    if(timerfd_settime(s.descriptor,0,&request,nullptr)<0)
        throw std::system_error(errno,std::generic_category(),"arm native startup timer");
#endif
    const bool previous=s.armed;
    if(previous)s.detach(call.instruction,observe);
    put(s.memory,call.instruction,0x56ee00,4,0x56edc0,observe);
    put(s.memory,call.instruction,0x56ee04,4,0,observe);
    put(s.memory,call.instruction,0x56ede1,1,0,observe);
    put(s.memory,call.instruction,0x56edf0,4,static_cast<std::uint32_t>(due),observe);
    put(s.memory,call.instruction,0x56edf4,4,static_cast<std::uint32_t>(due>>32),observe);
    put(s.memory,call.instruction,0x56ede3,1,1,observe);
    put(s.memory,call.instruction,0x56ede4,4,0,observe);
    put(s.memory,call.instruction,0x56edf8,4,s.head,observe);
    put(s.memory,call.instruction,0x56edfc,4,s.head,observe);
    put(s.memory,call.instruction,s.head,4,0x56edf8,observe);
    put(s.memory,call.instruction,s.head+4,4,0x56edf8,observe);
    s.armed=true;
    if(timer_observe)timer_observe({now,due,interval,previous});
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|(previous?1u:0u);abi.gp[4]=sp+20;return true;
}
bool NativeTu15StartupTimer::cancel(const SourcePhysicsContactRecordObserver &observe) {
    auto &s=*state_;s.validate_timer(s.armed);
#if defined(__linux__)
    itimerspec request{};
    if(timerfd_settime(s.descriptor,0,&request,nullptr)<0)
        throw std::system_error(errno,std::generic_category(),"cancel native startup timer");
#endif
    const auto previous=s.armed;if(previous)s.detach(0x80000060u,observe);return previous;
}
bool NativeTu15StartupTimer::dispatch_due(const std::function<void(NativeTu15TimerDpc)> &execute,
    const SourcePhysicsContactRecordObserver &observe) {
    auto &s=*state_;if(!execute)throw std::invalid_argument("Native timer expiry requires its original DPC executor");
    s.validate_timer(s.armed);
#if defined(__linux__)
    std::uint64_t expirations=0;const auto n=read(s.descriptor,&expirations,sizeof(expirations));
    if(n<0 && errno==EAGAIN)return false;
    if(n<0)throw std::system_error(errno,std::generic_category(),"read native timer expiration");
    if(n!=sizeof(expirations) || expirations!=1 || !s.armed)
        throw std::runtime_error("Native one-shot timer expiration lost its actual resource owner");
#endif
    const auto now=s.clock.system_time();s.detach(0x80000095u,observe);
    put(s.memory,0x80000095u,0x56ede4,4,1,observe);
    // Expiry executes the actual retained DPC with system-time arguments.
    // It does not substitute a Boolean setting test for original game code.
    execute({0x56edc0,word(s.memory,0x56edcc),word(s.memory,0x56edd0),static_cast<std::uint32_t>(now),static_cast<std::uint32_t>(now>>32)});
    return true;
}
std::uint64_t NativeTu15StartupTimer::remaining_100ns() const {
#if defined(__linux__)
    itimerspec remaining{};
    if(timerfd_gettime(state_->descriptor,&remaining)<0)
        throw std::system_error(errno,std::generic_category(),"query native timer expiration");
    return static_cast<std::uint64_t>(remaining.it_value.tv_sec)*10000000ull+static_cast<std::uint64_t>(remaining.it_value.tv_nsec)/100;
#else
    throw std::runtime_error("Native startup timer resource unavailable");
#endif
}
void source_run_native_tu15_startup_with_timer(const NativeTu15StartupPlatform &platform,
    NativeTu15StartupTimer &timer,SourceRuntimeMemory &memory,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &lock_platform,SourceProcessShutdownRegistry &shutdown,
    const SourcePhysicsContactRecordObserver &observe,const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,const SourcePhysicsContactRecordObserver &query_observe,
    const NativeTu15TimerObserver &timer_observe,const SourceProcessHeapMemoryObserver &memory_observe,
    const PhysicsPlatformLockObserver &lock_observe) {
    if(!timer.backs(memory))throw std::runtime_error("Native startup timer bank owner mismatch");
    if(entry==0x2d7f8eu && (abi.eflags&0x400u))
        throw std::invalid_argument("Complete process startup requires its incoming clear-direction ABI");
    auto joined=services;
    std::uint32_t instruction=0;
    joined.instruction=[&](auto p) {instruction=p;if(services.instruction)services.instruction(p);};
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&memory)throw std::runtime_error("Native startup timer call bank owner mismatch");
        if(timer.try_call(call,state,observer,timer_observe))return;
        if(platform.try_call(memory,call,state,query_observe))return;
        if(call.instruction==0x2dc64cu && call.target==0x2e0fe2u && !call.tail) {
            if(word(memory,state.gp[4])!=0x2e0101u)
                throw std::runtime_error("Native startup settings wrapper return mismatch");
            source_run_tu15_settings_runtime(bank,state,fp,joined,observer);return;
        }
        if(!services.required_owner)throw std::runtime_error("Native startup still requires its actual heap/world owners");
        services.required_owner(bank,call,state,observer);
    };
    const auto records=[&](const SourceProcessStartupRecordStore &change) {
        if(instruction!=0x2dc611u && instruction!=0x2dc61eu)
            throw std::runtime_error("Native timer startup record lost its source caller");
        if(record_observe)record_observe({instruction,change.address,change.before,change.written});
    };
    source_run_process_heap_with_native_platform(memory,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
        fp,joined,vm,locks,lock_platform,observe,memory_observe,lock_observe,&shutdown,shutdown_observe,records,platform.data_owner());
}
} // namespace halo2
