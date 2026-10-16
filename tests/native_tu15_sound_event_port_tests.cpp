// Retain the verified genuine metadata/voice/PCM fixture. No original source
// setup, native platform result or DSP-ready flag is replaced for registration.
#include "native_tu15_sound_registered_event_fixture.hpp"
#include "halo2/source_runtime_sound_event_port.hpp"
#include <fcntl.h>
#include <sys/timerfd.h>
namespace {
using Port=SourceRuntimeSoundEventPort;
void registration(const Json& reference,int interruption=0) {
    EventFixture e(reference);e.frontend.reset();e.frontend=std::make_unique<Frontend>(e.f.memory,e.reset,e.dma,1);
    for(unsigned i=0;i<2;++i)e.frontend->bind(64+i,0x19010000+i*0x200,e.leases[i]);
    e.frontend->prepare_idle_trap(0x38e8eb,0xfe801504,0x8000);
    mount_native_tu15_sound_event_port_data(e.f.memory);Port port(e.f.memory,e.reset,*e.frontend,e.output);
    const auto frame=[&](std::uint32_t returned,std::initializer_list<std::uint32_t> args){e.frame(0,args);put(e.f.memory,stack,returned);};
    frame(0x3893bf,{apu+0x4c8,0x38918c,apu});e.f.abi.gp[3]=0x8000006b;
    check(port.try_call(e.f.memory,{0x3893bd,0x8000006b,false},e.f.abi),"Actual native late first DPC was not constructed");
    frame(0x3893ce,{apu+0x4e8,0});const auto timer_before=e.f.abi.gp;
    check(port.try_call(e.f.memory,{0x3893c8,0x80000071,false},e.f.abi),"Actual native source timer was not initialized");
    for(unsigned i=0;i<8;++i)if(i!=4)check(e.f.abi.gp[i]==timer_before[i],"Native void timer registration changed GP state");
    itimerspec remaining{};check(port.state().timer_descriptor>=0&&timerfd_gettime(port.state().timer_descriptor,&remaining)==0&&!remaining.it_value.tv_sec&&!remaining.it_value.tv_nsec,
        "Native initialized timer lacks its actual unarmed kernel resource");
    check((fcntl(port.state().timer_descriptor,F_GETFD)&FD_CLOEXEC)!=0,"Actual native timer descriptor leaks across exec");
    const auto out=stack+0x1c;frame(0x3893ea,{5,out});e.f.abi.gp[0]=out;put(e.f.memory,out,0xa5a5a5a5);
    check(port.try_call(e.f.memory,{0x3893e4,0x8000002c,false},e.f.abi),"Actual native event route cookie was not created");
    const auto cookie=e.f.abi.gp[0];const auto priority_bytes=e.f.memory.view(out,4);
    check(cookie==port.state().route_cookie&&port.state().native_route.value>0xffffffffu&&port.state().route_identity>0xffffffffu&&
        priority_bytes[0]==std::byte{}&&priority_bytes[1]==std::byte{0xa5}&&priority_bytes[2]==std::byte{0xa5}&&priority_bytes[3]==std::byte{0xa5},
        "Native route truncated its owner or invented a PCI/IRQL word beyond its actual passive-policy byte");
    const auto initialize=[&](std::uint32_t value){frame(0x389405,{0x3912d8,0x38936c,apu,value,0xa5a5a500,0,1});};
    initialize(cookie+1);rejected([&]{port.try_call(e.f.memory,{0x3893ff,0x8000006d,false},e.f.abi);});
    check(!port.state().interrupt_initialized,"Wrong route cookie initialized an interrupt callback");
    initialize(cookie);check(port.try_call(e.f.memory,{0x3893ff,0x8000006d,false},e.f.abi),"Native source routine/context registration failed");
    if(interruption==1) {
        bool connected_before_stop=false;
        rejected([&]{Port::CallScope parent(&port);frame(0x38940c,{0x3912d8});
            check(port.try_call(e.f.memory,{0x389406,0x80000062,false},e.f.abi)&&port.state().connected,"Parent fixture failed actual registration");
            connected_before_stop=true;throw std::runtime_error("declared parent source stop after actual389406 connect");
        });
        check(connected_before_stop&&port.state().faulted&&port.state().retired&&!port.state().connected&&!e.reset.quiescent(),"Interrupted parent source transaction retained live event delivery or invented completion");
        rejected([&]{port.signal_known_idle();});e.retire();return;
    }
    frame(0x38940c,{0x3912d8});check(port.try_call(e.f.memory,{0x389406,0x80000062,false},e.f.abi)&&bool(e.f.abi.gp[0]&255)&&port.state().connected,
        "Native interrupt reported success without its actual registered epoll endpoint");
    frame(0x38940c,{0x3912d8});rejected([&]{port.try_call(e.f.memory,{0x389406,0x80000062,false},e.f.abi);});
    e.frame(0,{0x3912d8,apu});check(!port.dispatch_known_idle(e.f.abi,{0x27f,0x1f80},e.f.services),"Native endpoint fabricated an unproduced callback");
    e.frame(apu+8,{1});e.frontend->source_mode_helper(e.f.abi,{0x27f,0x1f80},e.f.services);
    for(unsigned i=0;i<2;++i) {
        e.frontend->idle_voice(64+i);e.frontend->process();port.signal_known_idle();rejected([&]{port.signal_known_idle();});
        check(port.state().pending_irq,"Actual eventfd signal lost its retained callback receipt");
        e.frame(0,{0x3912d8,apu+8});rejected([&]{port.dispatch_known_idle(e.f.abi,{0x27f,0x1f80},e.f.services);});
        check(port.state().pending_irq,"Rejected source callback frame consumed its real event");
        e.frame(0,{0x3912d8,apu});
        if(interruption==2) {
            bool stopped=false;auto services=e.f.services;services.instruction=[&](auto pc){if(pc==0x38b15b){stopped=true;throw std::runtime_error("declared source callback interruption before notifier publication");}};
            rejected([&]{port.dispatch_known_idle(e.f.abi,{0x27f,0x1f80},services,e.f.observe);});
            check(stopped&&port.state().faulted&&port.state().retired&&!port.state().connected&&!port.state().pending_irq&&port.state().cancelled_irqs==1&&!port.state().delivered_irqs,
                "Interrupted actual native callback remained connected or lost its cancellation receipt");
            const auto rows=e.dma.allocations();const auto n=std::ranges::find_if(rows,[](const auto& row){return row.source_descriptor==0x3913a0;});
            check(e.f.memory.view(n->source+32+64*64+0x3f,1)[0]==std::byte{0x80},"Interrupted source callback fabricated notifier completion");
            e.retire();return;
        }
        if(interruption==3) {
            port.retire_generation();check(!port.state().connected&&port.state().cancelled_irqs==1&&!port.state().delivered_irqs,"Retirement lost the pending actual native IRQ receipt");
            rejected([&]{port.dispatch_known_idle(e.f.abi,{0x27f,0x1f80},e.f.services);});e.retire();return;
        }
        check(port.dispatch_known_idle(e.f.abi,{0x27f,0x1f80},e.f.services,e.f.observe),"Actual registered original thunk was not delivered");
        const auto& expected=reference.at("case").at("events").at(i);
        check(e.f.instructions==expected.at("instructions")&&e.f.stores==expected.at("stores")&&e.f.abi.gp==expected.at("gp").get<decltype(e.f.abi.gp)>()&&
            (e.f.abi.eflags&0xcc5)==(expected.at("flags").get<unsigned>()&0xcc5),"Actual native registered callback differs from unchanged original38936C thunk/ISR");
    }
    e.frame(0,{apu+0x4c8,apu,0,0});check(e.frontend->dispatch_dpc(e.f.abi,{0x27f,0x1f80},e.f.services,e.f.observe),"Registered native route lost the real deferred notification");
    const auto& dpc=reference.at("case").at("dpc");check(e.f.instructions==dpc.at("instructions")&&e.f.stores==dpc.at("stores"),"Registered-route whole original deferred notification differs");
    check(std::ranges::equal(e.f.memory.view(apu,0x540),unhex(reference.at("case").at("apu_hex"))),"Native registered resource/source APU bytes differ");
    check(port.state().delivered_irqs==2&&!port.state().pending_irq&&!port.state().cancelled_irqs,"Actual native endpoint callback/receipt order differs");
    frame(0x38942c,{1});check(!port.try_call(e.f.memory,{0x389427,0x38e53d,false},e.f.abi),"Full source global interrupt admission was fabricated");
    port.retire_generation();rejected([&]{port.signal_known_idle();});e.retire();
    std::cout<<"Actual eventfd/epoll route delivered both unchanged registered276-instruction ISRs and original DPC; initialized timerfd remains unarmed\n";
}
void foreign_bindings(const Json& reference) {
    EventFixture e(reference);Fixture foreign(reference.at("reset"));
    SourceRuntimeSoundResetResources other(foreign.memory,e.output,fs);foreign.frame(reference.at("reset"));
    check(other.try_call(foreign.memory,{0x38ec8f,0x38ec0e,false},foreign.abi,{0x27f,0x1f80},foreign.services),"Foreign genuine reset fixture failed");
    SourceRuntimeSoundDmaResources dma(foreign.memory,e.output);Frontend frontend(foreign.memory,other,dma,1);
    const auto before=e.reset.state().completed_commands;
    rejected([&]{Port wrong(e.f.memory,e.reset,frontend,e.output);});
    rejected([&]{Port wrong(e.f.memory,other,*e.frontend,e.output);});
    check(e.reset.state().completed_commands==before,"Foreign same-output hybrid mutated the actual source worker");
    // A second genuinely reset worker can currently own the same source bank.
    // Identical bank/APU/generation/output values still must not join workers.
    SourceRuntimeSoundResetResources duplicate(foreign.memory,e.output,fs);foreign.frame(reference.at("reset"));
    check(duplicate.try_call(foreign.memory,{0x38ec8f,0x38ec0e,false},foreign.abi,{0x27f,0x1f80},foreign.services),"Second same-bank genuine reset failed");
    check(other.state().generation==duplicate.state().generation&&!other.control_lease().same_owner(duplicate.control_lease()),"Distinct actual workers lost their immutable identities");
    rejected([&]{Port wrong(foreign.memory,duplicate,frontend,e.output);});
    e.retire();
}
void foreign_destructor(const Json& reference) {
    EventFixture e(reference);auto port=std::make_unique<Port>(e.f.memory,e.reset,*e.frontend,e.output);
    std::set_terminate([]{std::_Exit(77);});
    std::thread other([owner=std::move(port)]()mutable{owner.reset();});other.join();
    throw std::runtime_error("Foreign event port destructor violated its enforced creating-thread contract");
}
}
int main(int argc,char** argv){try{
    check(argc==2||argc==3,"Supply registered original reference");std::ifstream input(argv[argc-1]);Json reference;input>>reference;
    const auto& first=reference.at("cases").at(0);
    if(argc==3&&std::string(argv[1])=="--foreign-destructor-probe"){foreign_destructor(first);return 0;}
    for(const auto& row:reference.at("cases"))registration(row);
    foreign_bindings(first);registration(first,1);registration(first,2);registration(first,3);return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
