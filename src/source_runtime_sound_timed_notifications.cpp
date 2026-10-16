#include "halo2/source_runtime_sound_timed_notifications.hpp"
#include "halo2/source_sound_timed_notifications_tu15.hpp"
#include <array>
#include <cerrno>
#include <chrono>
#include <stdexcept>
#include <system_error>
#include <thread>
#if defined(__linux__)
#include <sys/timerfd.h>
#include <unistd.h>
#endif
namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p,unsigned n=4) {
    const auto bytes=m.view(p,n);std::uint32_t value{};
    for(unsigned i=0;i<n;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
void put(SourceRuntimeMemory& m,std::uint32_t site,std::uint32_t p,unsigned n,std::uint32_t value,
    const SourcePhysicsContactRecordObserver& observe) {
    SourcePhysicsContactRecordStore row{site,p,std::vector<std::byte>(n),std::vector<std::byte>(n)};
    m.read(p,row.before);for(unsigned i=0;i<n;++i)row.written[i]=std::byte(value>>(8*i));
    m.write(p,row.written);if(observe)observe(row);
}
constexpr std::uint64_t filetime_epoch=116444736000000000ull;
std::uint64_t now() {
    const auto t=std::chrono::system_clock::now().time_since_epoch();
    return std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(t).count()/100)+filetime_epoch;
}
}
struct SourceRuntimeSoundTimedNotifications::Impl {
    SourceRuntimeMemory& memory;
    SourceRuntimeSoundResetResources::ControlLease control;
    SourceRuntimeSoundFrontendEvents& frontend;
    SourceRuntimeSoundEventPort& port;
    const std::thread::id thread=std::this_thread::get_id();
    const std::uint64_t generation;
    const std::uint32_t apu;
    std::shared_ptr<const SourceRuntimeSoundEventPort::TimerLease> timer_lease;
    const int timer;
    std::uint64_t due{},expirations{},completed{},cancelled{};
    bool registered{},armed{},retired{},fault{},executing{};
    unsigned exclusion_depth{};
    std::uint32_t fs{};
    PlatformObserver observe_platform;
    Impl(SourceRuntimeMemory& m,SourceRuntimeSoundResetResources& reset,
        SourceRuntimeSoundFrontendEvents& f,SourceRuntimeSoundEventPort& p)
        :memory(m),control(reset.control_lease()),frontend(f),port(p),generation(control.generation()),
         apu(control.source_apu()),timer_lease(port.claim_timer_binding(memory,control,frontend)),timer(timer_lease->descriptor) {}
    void require_thread()const {
        if(std::this_thread::get_id()!=thread)throw std::runtime_error("Native timed sound notifications require their creating thread");
    }
    void require_live()const {
        require_thread();if(fault||retired)throw std::runtime_error("Native timed sound notification generation is retired or faulted");
        control.require_bank(memory);frontend.require_binding(memory,control);
        if(control.generation()!=generation||port.require_timer_lease(memory,control,timer_lease)!=timer)
            throw std::runtime_error("Native timed sound notification lost its actual shared timer/worker generation");
    }
    void validate_list()const {
        const auto head=apu+0x4a8;auto current=word(memory,head);
        for(unsigned n=0;current!=head;++n) {
            if(n>=256||!current)throw std::invalid_argument("Native timed source notification list is malformed");
            frontend.require_timed_voice(word(memory,current+12),current);
            const auto next=word(memory,current),previous=word(memory,current+4);
            if(word(memory,next+4)!=current||word(memory,previous)!=current||!(word(memory,current+8)&2))
                throw std::invalid_argument("Native timed source notification links/admission differ");
            current=next;
        }
    }
    bool cancel_timer() {
        bool was_pending=false;
#if defined(__linux__)
        itimerspec disabled{},before{};
        if(timerfd_settime(timer,0,&disabled,&before)<0)throw std::system_error(errno,std::generic_category(),"cancel actual shared sound timer");
        was_pending=before.it_value.tv_sec||before.it_value.tv_nsec;
#endif
        if(armed)++cancelled;armed=false;due=0;return was_pending;
    }
    bool set_timer(std::uint64_t time) {
        if(time<filetime_epoch)throw std::invalid_argument("Source absolute sound timer precedes native Unix clock domain");
        bool was_pending=false;
#if defined(__linux__)
        const auto unix_time=time-filetime_epoch;itimerspec request{},before{};
        request.it_value.tv_sec=std::int64_t(unix_time/10000000ull);request.it_value.tv_nsec=long((unix_time%10000000ull)*100ull);
        if(timerfd_settime(timer,TFD_TIMER_ABSTIME,&request,&before)<0)throw std::system_error(errno,std::generic_category(),"arm actual shared sound timer");
        was_pending=before.it_value.tv_sec||before.it_value.tv_nsec;
#endif
        due=time;armed=true;return was_pending;
    }
    bool platform(const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
        const auto sp=abi.gp[4];
        if(call.instruction==0x38921a||call.instruction==0x388c6c) {
            if(call.tail||call.target!=0x80000080||word(memory,0x41b668)!=call.target||word(memory,sp)!=call.instruction+6)
                throw std::invalid_argument("Native timed system-clock source frame differs");
            const auto out=word(memory,sp+4);memory.validate_mutable_view(out,8);const auto time=now();
            std::array<std::byte,8> bytes{};for(unsigned i=0;i<8;++i)bytes[i]=std::byte(time>>(8*i));memory.write(out,bytes);
            if(observe_platform)observe_platform({call.instruction,call.target,time,false,false});abi.gp[4]=sp+8;return true;
        }
        if(call.instruction==0x3892bb||call.instruction==0x388cb8||call.instruction==0x388c3d) {
            if(call.tail||call.target!=0x80000095||word(memory,0x41b6f8)!=call.target||word(memory,sp)!=call.instruction+6||
                word(memory,sp+4)!=apu+0x4e8||word(memory,sp+16)!=apu+0x510||!registered)
                throw std::invalid_argument("Native timed timer-arm source frame differs");
            const auto time=std::uint64_t(word(memory,sp+8))|(std::uint64_t(word(memory,sp+12))<<32);
            const auto previous=set_timer(time);if(observe_platform)observe_platform({call.instruction,call.target,time,true,previous});abi.gp[0]=(abi.gp[0]&0xffffff00u)|unsigned(previous);abi.gp[4]=sp+20;return true;
        }
        if(call.instruction==0x388c10) {
            if(call.tail||call.target!=0x80000061||word(memory,0x41b788)!=call.target||word(memory,sp)!=0x388c16||word(memory,sp+4)!=apu+0x4e8)
                throw std::invalid_argument("Native timed timer-cancel source frame differs");
            const auto previous=cancel_timer();if(observe_platform)observe_platform({call.instruction,call.target,0,true,previous});abi.gp[0]=(abi.gp[0]&0xffffff00u)|unsigned(previous);abi.gp[4]=sp+8;return true;
        }
        if(call.instruction==0x388c1d) {
            if(call.tail||call.target!=0x80000089||word(memory,0x41b7f0)!=call.target||word(memory,sp)!=0x388c23||word(memory,sp+4)!=apu+0x510)
                throw std::invalid_argument("Native timed deferred-cancel source frame differs");
            const auto removed=frontend.remove_timed_dpc();if(observe_platform)observe_platform({call.instruction,call.target,0,true,removed});abi.gp[0]=(abi.gp[0]&0xffffff00u)|unsigned(removed);abi.gp[4]=sp+8;return true;
        }
        if(control.try_exclusion(memory,call,abi)) {
            if(call.instruction==0x383d88)++exclusion_depth;
            else if(exclusion_depth)--exclusion_depth;else throw std::runtime_error("Native timed exclusion completion lacks its actual scope");
            return true;
        }
        return false;
    }
    struct Memory final:SourcePhysicsContactMemory {
        Impl& p;explicit Memory(Impl& v):p(v){}
        void read(std::uint32_t address,std::span<std::byte> bytes)const override {
            if(address==p.fs+0x24&&bytes.size()==1){bytes[0]=std::byte(p.exclusion_depth?2:0);return;}
            p.memory.read(address,bytes);
        }
        void write(std::uint32_t address,std::span<const std::byte> bytes)override {p.memory.write(address,bytes);}
    };
    void run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
        const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe,const PlatformObserver& platform_observer) {
        require_live();if(executing)throw std::runtime_error("Native timed source transaction cannot reenter");
        Memory view(*this);auto scoped=services;
        scoped.required_owner=[&](auto&,const auto& call,auto& state,const auto& observer){
            if(platform(call,state))return;
            if(call.instruction==0x388c99&&call.target==0x38d1dd) {
                frontend.require_timed_voice(state.gp[1],state.gp[1]+0x108);
                source_run_sound_timed_notifications_tu15(view,call.target,state,fp,scoped,observer);return;
            }
            if(services.required_owner){services.required_owner(memory,call,state,observer);return;}
            throw std::runtime_error("Unowned native timed voice callback remains required");
        };
        observe_platform=platform_observer;fs=abi.fs_base;executing=true;
        try{source_run_sound_timed_notifications_tu15(view,entry,abi,fp,scoped,observe);
            if(exclusion_depth)throw std::runtime_error("Native timed callback returned with unfinished actual exclusion");executing=false;observe_platform={};
        }catch(...){executing=false;observe_platform={};fault=true;try{cancel_timer();}catch(...){}port.fault_parent_call();throw;}
    }
};
SourceRuntimeSoundTimedNotifications::SourceRuntimeSoundTimedNotifications(SourceRuntimeMemory& memory,
    SourceRuntimeSoundResetResources& reset,SourceRuntimeSoundFrontendEvents& frontend,SourceRuntimeSoundEventPort& port)
    :impl_(std::make_unique<Impl>(memory,reset,frontend,port)){}
SourceRuntimeSoundTimedNotifications::~SourceRuntimeSoundTimedNotifications() {
    if(impl_&&std::this_thread::get_id()!=impl_->thread)std::terminate();
    if(impl_)try{retire_generation();}catch(...){impl_->control.abort_transaction();}
}
bool SourceRuntimeSoundTimedNotifications::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,
    SourcePhysicsWorldRuntimeAbi& abi,const SourcePhysicsContactRecordObserver& observe) {
    if(call.instruction!=0x3893db)return false;auto& p=*impl_;p.require_live();
    p.memory.validate_mutable_view(p.apu+0x510,28);p.frontend.initialize_timed_dpc(bank,call,abi);p.registered=true;
    try{put(p.memory,call.instruction,p.apu+0x510,2,0x13,observe);put(p.memory,call.instruction,p.apu+0x512,1,0,observe);
        put(p.memory,call.instruction,p.apu+0x51c,4,0x38901d,observe);put(p.memory,call.instruction,p.apu+0x520,4,p.apu,observe);
    }catch(...){p.fault=true;p.port.fault_parent_call();throw;}return true;
}
void SourceRuntimeSoundTimedNotifications::source_notification(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe,const PlatformObserver& platform_observer) {
    auto& p=*impl_;p.require_live();const auto channel=abi.gp[1],sp=abi.gp[4];
    if(!p.registered||(entry!=0x38cbb9&&entry!=0x38cbe8)||word(p.memory,sp+4)!=4)
        throw std::invalid_argument("Native source timed request is not the authentic retained index4 wrapper");
    p.frontend.require_timed_voice(channel,channel+0x108);p.validate_list();p.run(entry,abi,fp,services,observe,platform_observer);
}
bool SourceRuntimeSoundTimedNotifications::collect_due() {
    auto& p=*impl_;p.require_live();if(!p.registered)throw std::runtime_error("Native timer expiry has no actual registered deferred callback");
#if defined(__linux__)
    std::uint64_t count{};const auto bytes=read(p.timer,&count,sizeof(count));
    if(bytes<0&&errno==EAGAIN)return false;
    if(bytes!=sizeof(count)||!count||!p.armed)throw std::runtime_error("Native timed notification lost its actual outstanding timer receipt");
    p.armed=false;p.due=0;p.expirations+=count;
    try{p.frontend.queue_timed_dpc();}catch(...){p.fault=true;p.port.fault_parent_call();throw;}
    return true;
#else
    return false;
#endif
}
bool SourceRuntimeSoundTimedNotifications::dispatch_dpc(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe,const PlatformObserver& platform_observer) {
    auto& p=*impl_;p.require_live();p.validate_list();
    const auto result=p.frontend.dispatch_timed_dpc(abi,[&]{p.run(0x38901d,abi,fp,services,observe,platform_observer);});
    if(result)++p.completed;return result;
}
SourceRuntimeSoundTimedNotifications::State SourceRuntimeSoundTimedNotifications::state()const {
    const auto& p=*impl_;p.require_thread();return {p.generation,p.due,p.expirations,p.completed,p.cancelled,
        NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(&p)},p.timer,p.registered,p.armed,p.retired,p.fault};
}
void SourceRuntimeSoundTimedNotifications::retire_generation() {
    auto& p=*impl_;p.require_thread();if(p.retired)return;
    if(p.executing)throw std::runtime_error("Native timed source callback is still active");
    p.cancel_timer();p.frontend.remove_timed_dpc();p.port.release_timer_lease(p.timer_lease);p.timer_lease.reset();p.retired=true;
}
}
