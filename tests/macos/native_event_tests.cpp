#include "platform/native_event_resources.hpp"
#include "halo2/native_tu15_startup_timer.hpp"
#include <bit>
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F &&f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}
    check(rejected,"Invalid native timer/event operation was accepted");}
bool readable(int fd,int timeout=0){pollfd value{fd,POLLIN,0};const auto n=::poll(&value,1,timeout);
    check(n>=0,"Native event poll failed");return n && (value.revents&POLLIN);}
void closed(int fd){errno=0;check(::fcntl(fd,F_GETFD)==-1 && errno==EBADF,"Retired native event descriptor is still open");}
void flags(int fd){check((::fcntl(fd,F_GETFD)&FD_CLOEXEC)!=0,"Native event descriptor leaks across exec");}
std::uint64_t unix_100ns(){return std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count()/100);}
void backend_resources(){
    using namespace native_events;
#if defined(__APPLE__)
    const auto clock_ns=[](clockid_t clock){timespec sample{};
        check(::clock_gettime(clock,&sample)==0,"Native timer clock observation failed");
        return std::uint64_t(sample.tv_sec)*1000000000ull+std::uint64_t(sample.tv_nsec);};
    const auto clocks=native_tu15_timer_clock();
    const auto before=clock_ns(CLOCK_UPTIME_RAW)/100,interrupt=clocks.interrupt_time(),after=clock_ns(CLOCK_UPTIME_RAW)/100;
    check(before<=interrupt && interrupt<=after,"Startup interrupt clock does not belong to Darwin uptime");
    constexpr auto epoch=116444736000000000ull;
    const auto wall_before=clock_ns(CLOCK_REALTIME)/100+epoch,wall=clocks.system_time(),wall_after=clock_ns(CLOCK_REALTIME)/100+epoch;
    check(wall_before<=wall && wall<=wall_after,"Startup system clock lost the native Unix/NT epoch conversion");
    const auto uptime=clock_ns(CLOCK_UPTIME_RAW),continuous=clock_ns(CLOCK_MONOTONIC_RAW);
    std::cout<<"Darwin clock observation: uptime_raw_ns="<<uptime<<" monotonic_raw_ns="<<continuous<<'\n';
#endif
    int signal_fd=-1,timer_fd=-1,dispatcher_fd=-1;
    {
        Signal signal;Timer timer(Clock::Realtime);Poller poller;
        signal_fd=signal.descriptor();timer_fd=timer.descriptor();dispatcher_fd=poller.descriptor();
        for(const auto fd:{signal_fd,timer_fd,dispatcher_fd})flags(fd);
        poller.add(signal_fd,1);poller.add(timer_fd,2);
        check(!signal.take() && !timer.take() && !poller.ready(1) && !poller.ready(2),"Unproduced native event became ready");
        rejects([&]{poller.add(signal_fd,3);});
        signal.signal();signal.signal();
        check(poller.ready(1) && poller.ready(1) && !poller.ready(2),"Polling consumed or confused a native event receipt");
        check(signal.take()==2 && !signal.take() && !poller.ready(1),"Native signal counter lost/duplicated queued receipts");
        poller.remove(signal_fd);signal.signal();
        check(!poller.ready(1),"Disconnected route remained in the dispatcher");
        poller.add(signal_fd,3);check(poller.ready(3) && !poller.ready(1),"Reconnected route kept an old dispatcher identity");
        check(signal.take()==1,"Pending native event was lost during re-registration");
        check(timer.disarmed() && !timer.cancel(),"Fresh native timer was armed");
        rejects([&]{timer.arm_relative_100ns(10000);});
        const auto future=unix_100ns()+600000000;
        check(!timer.arm_realtime_100ns(future) && timer.remaining_100ns()>590000000 && !timer.disarmed(),
              "Absolute wall timer is not a pending native resource");
        check(timer.arm_realtime_100ns(future) && timer.cancel() && !timer.cancel() && timer.disarmed() && !timer.take(),
              "Native wall timer re-arm/cancellation pending result differs");
        rejects([&]{timer.arm_realtime_100ns(0);});
        check(!timer.arm_realtime_100ns(unix_100ns()-10000),"Past wall timer reported an earlier pending arm");
        check(readable(timer_fd,2000) && poller.ready(2) && poller.ready(2),"Past absolute wall timer did not publish its actual receipt");
        check(timer.take()==1 && !timer.take() && !poller.ready(2) && !timer.cancel(),"Expired wall timer was not a unique one-shot receipt");
        // An unread expiration must be removed when cancelled or replaced.
        timer.arm_realtime_100ns(unix_100ns()+10000);check(readable(timer_fd,2000),"Wall receipt did not become readable");
        check(!timer.cancel() && !timer.take() && !readable(timer_fd),"Cancellation retained a stale timer receipt");
        timer.arm_realtime_100ns(unix_100ns()+10000);check(readable(timer_fd,2000),"Re-arm fixture did not expire");
        timer.arm_realtime_100ns(unix_100ns()+1000000);
        check(!timer.take() && !readable(timer_fd,10),"Re-arm published a previous generation's receipt before its new deadline");
        check(readable(timer_fd,2000) && timer.take()==1 && !timer.take(),"Re-armed wall timer lost its current expiration");
        poller.remove(timer_fd);
    }
    for(const auto fd:{signal_fd,timer_fd,dispatcher_fd})closed(fd);
    {
        Timer timer(Clock::Monotonic);flags(timer.descriptor());
        check(!timer.arm_relative_100ns(216000000000ull) && timer.remaining_100ns()>215990000000ull,
              "Six-hour relative timer changed its native clock/interval domain");
        check(timer.arm_relative_100ns(216000000000ull) && timer.cancel() && !timer.cancel(),"Relative timer pending/cancel result differs");
        rejects([&]{timer.arm_realtime_100ns(unix_100ns());});rejects([&]{timer.arm_relative_100ns(0);});
        timer.arm_relative_100ns(50000);check(readable(timer.descriptor(),2000) && timer.take()==1 && !timer.take(),
              "Relative native timer did not publish one real expiration");
    }
    for(unsigned i=0;i<64;++i){int fd;{Timer timer(Clock::Monotonic);fd=timer.descriptor();timer.arm_relative_100ns(1);}closed(fd);}
    std::cout<<"PASS native signal/poller registration, counts, nonconsuming readiness, wall/monotonic deadlines, cancel/re-arm stale receipt fencing and repeated retirement\n";
}
void put(SourceRuntimeMemory &memory,std::uint32_t p,std::uint32_t value){std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));memory.write(p,bytes);}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t p){const auto b=memory.view(p,4);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(b[i])<<(8*i);return value;}
void startup_owner(){
    SourceRuntimeMemory memory;
    memory.mount(0x56edc0,std::vector<std::byte>(0x70),SourceRuntimeMemory::Access::ReadWrite,"retained startup timer and DPC");
    memory.mount(0x1000,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"startup timer call frame");
    memory.write(0x56ede0,std::array{std::byte{8},std::byte{},std::byte{10},std::byte{}});
    memory.write(0x56edc0,std::array{std::byte{0x13},std::byte{},std::byte{}});
    put(memory,0x56ede8,0x56ede8);put(memory,0x56edec,0x56ede8);put(memory,0x56edcc,0x2e009d);
    constexpr std::uint32_t sp=0x1100;constexpr std::uint64_t interrupt=0x123456780000ull,system=0x0102030405060708ull;
    int descriptor=-1;
    {
        NativeTu15StartupTimer timer(memory,{[]{return interrupt;},[]{return system;}});descriptor=timer.native_descriptor();flags(descriptor);
        check(timer.backs(memory) && !timer.dispatch_due([](auto){throw std::runtime_error("Unarmed timer callback");}),
              "Fresh startup timer fabricated a callback");
        const auto frame=[&](std::int64_t duration){const auto raw=std::bit_cast<std::uint64_t>(duration);
            put(memory,sp,0x2e009c);put(memory,sp+4,0x56ede0);put(memory,sp+8,std::uint32_t(raw));
            put(memory,sp+12,std::uint32_t(raw>>32));put(memory,sp+16,0x56edc0);};
        const auto arm=[&](std::int64_t duration,bool prior){frame(duration);SourcePhysicsContactRuntimeAbi abi{};
            abi.gp={0x12345678,2,3,4,sp,6,7,8};abi.eflags=0x246;const auto before=abi;unsigned observations=0,stores=0;
            check(timer.try_call({0x2dc5e6,0x80000095,false},abi,[&](const auto&){++stores;},[&](const auto& row){
                check(row.interrupt_time==interrupt && row.due_time==interrupt+std::uint64_t(-duration) &&
                      row.interval==duration && row.previously_inserted==prior,"Startup timer due-time observation differs");++observations;
            }),"Actual startup timer call was not handled");
            auto expected=before.gp;expected[0]=0x12345600u|unsigned(prior);expected[4]+=20;
            check(abi.gp==expected && abi.xmm==before.xmm && abi.eflags==before.eflags && stores && observations==1 &&
                  (std::uint64_t(word(memory,0x56edf0))|(std::uint64_t(word(memory,0x56edf4))<<32))==interrupt+std::uint64_t(-duration),
                  "Startup timer call changed its source ABI/due record");};
        arm(-216000000000ll,false);check(timer.remaining_100ns()>215990000000ull,"Original six-hour interval was not armed");
        arm(-216000000000ll,true);check(timer.cancel() && !timer.cancel() && timer.remaining_100ns()==0,"Startup cancel/source insertion result differs");
        frame(-50000);
        for(unsigned fault=0;fault<5;++fault){frame(-50000);SourcePhysicsContactRuntimeAbi abi{};abi.gp[4]=sp;
            auto call=SourcePhysicsContactRuntimeCall{0x2dc5e6,0x80000095,false};
            if(fault==0)call.tail=true;if(fault==1)put(memory,sp,0);if(fault==2)put(memory,sp+4,0);
            if(fault==3)put(memory,sp+16,0);if(fault==4)put(memory,sp+12,0);
            const auto before=memory.checkpoint();rejects([&]{timer.try_call(call,abi);});
            check(memory.checkpoint()==before && abi.gp[4]==sp && timer.remaining_100ns()==0,"Rejected startup request changed bank/CPU/native timer");}
        arm(-50000,false);check(readable(descriptor,2000),"Real startup timer did not expire");
        rejects([&]{timer.dispatch_due({});});check(readable(descriptor),"Missing DPC executor consumed expiration");
        unsigned callbacks=0;
        check(timer.dispatch_due([&](auto dpc){++callbacks;check(dpc.address==0x56edc0 && dpc.routine==0x2e009d && !dpc.context &&
            dpc.argument1==std::uint32_t(system) && dpc.argument2==std::uint32_t(system>>32) && word(memory,0x56ede4)==1,
            "Native expiry lost its retained DPC/system-time arguments");}),"Startup expiration was not dispatched");
        check(callbacks==1 && !timer.dispatch_due([](auto){throw std::runtime_error("Duplicate startup callback");}) &&
              memory.view(0x56ede3,1)[0]==std::byte{},"One-shot startup callback or source insertion was replayed");
        arm(-10000,false);check(readable(descriptor,2000),"Pending startup cancellation fixture did not expire");
        check(timer.cancel() && !timer.dispatch_due([](auto){throw std::runtime_error("Cancelled startup callback");}),
              "Cancelling an unconsumed source insertion retained a native expiration");
        arm(-216000000000ll,false);
    }
    closed(descriptor);rejects([&]{memory.view(0x90001000,8);});
    check(memory.view(0x56ede3,1)[0]==std::byte{},"Retirement retained the startup source insertion flag");
    std::cout<<"PASS production startup timer: six-hour source request, ABI/store/due record, malformed frames, real expiry/DPC, pending cancellation and retirement\n";
}
}
int main()try{backend_resources();startup_owner();return 0;}
catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
