#include "native_tu15_sound_registered_event_fixture.hpp"
#include "halo2/source_runtime_sound_interrupt_policy.hpp"
#include <poll.h>
#include <sys/timerfd.h>
#include <unistd.h>
namespace {
using Policy=SourceRuntimeSoundInterruptPolicy;using Port=SourceRuntimeSoundEventPort;
void register_port(EventFixture& e,Port& port){
    const auto frame=[&](auto returned,std::initializer_list<std::uint32_t> args){e.frame(0,args);put(e.f.memory,stack,returned);};
    frame(0x3893bf,{apu+0x4c8,0x38918c,apu});e.f.abi.gp[3]=0x8000006b;
    check(port.try_call(e.f.memory,{0x3893bd,0x8000006b,false},e.f.abi),"Native policy lost actual source DPC registration");
    frame(0x3893ce,{apu+0x4e8,0});check(port.try_call(e.f.memory,{0x3893c8,0x80000071,false},e.f.abi),"Native policy lost actual initialized timer");
    const auto out=stack+0x1c;frame(0x3893ea,{5,out});e.f.abi.gp[0]=out;
    check(port.try_call(e.f.memory,{0x3893e4,0x8000002c,false},e.f.abi),"Native policy lost actual eventfd route allocation");
    const auto cookie=e.f.abi.gp[0];frame(0x389405,{0x3912d8,0x38936c,apu,cookie,0,0,1});
    check(port.try_call(e.f.memory,{0x3893ff,0x8000006d,false},e.f.abi),"Native policy lost actual registered source callback");
    frame(0x38940c,{0x3912d8});check(port.try_call(e.f.memory,{0x389406,0x80000062,false},e.f.abi),"Native policy did not actually connect retained native route");
}
void compare(EventFixture& e,const Json& row,const char* message){
    check(e.f.instructions==row.at("instructions")&&e.f.stores==row.at("stores")&&
        e.f.abi.gp==row.at("returned_gp").get<decltype(e.f.abi.gp)>()&&
        (e.f.abi.eflags&0xcc5)==(row.at("returned_eflags").get<unsigned>()&0xcc5),message);
}
Json run_policy(const Json& registered,const Json& original,bool early){
    EventFixture e(registered);e.frontend.reset();e.frontend=std::make_unique<Frontend>(e.f.memory,e.reset,e.dma,1);
    for(unsigned i=0;i<2;++i)e.frontend->bind(64+i,0x19010000+i*0x200,e.leases[i]);
    e.frontend->prepare_idle_trap(0x38e8eb,0xfe801504,0x8000);
    mount_native_tu15_sound_event_port_data(e.f.memory);Port port(e.f.memory,e.reset,*e.frontend,e.output);
    SourceRuntimeMemory foreign;const auto admission=e.f.memory.checkpoint();
    rejected([&]{Policy wrong(foreign,port,e.reset.control_lease());});
    Fixture foreign_fixture(registered.at("reset"));SourceRuntimeSoundResetResources foreign_reset(foreign_fixture.memory,e.output,fs);
    foreign_fixture.frame(registered.at("reset"));check(foreign_reset.try_call(foreign_fixture.memory,{0x38ec8f,0x38ec0e,false},foreign_fixture.abi,{0x27f,0x1f80},foreign_fixture.services),"Genuine foreign policy control fixture failed");
    rejected([&]{Policy wrong(e.f.memory,port,foreign_reset.control_lease());});
    check(e.f.memory.checkpoint()==admission,"Foreign disabled policy changed source storage");
    if(!early)register_port(e,port);
    Policy policy(e.f.memory,port,e.reset.control_lease());
    rejected([&]{Policy duplicate(e.f.memory,port,e.reset.control_lease());});
    rejected([&]{policy.apply_disabled_source_store(0x38e8c9,0xfe801004,0xd9);});
    rejected([&]{policy.apply_disabled_source_store(0x38e8d3,0xfe801100,0x88);});
    check(!policy.diagnostics().prepared&&!policy.diagnostics().pending_mask&&!policy.diagnostics().source_delivery_enabled,"Rejected disabled policy request fabricated preparation or source delivery");
    e.frame(apu+8,{});const auto prefix_before=e.f.memory.checkpoint();
    bool unknown_action_preimage=false;SourcePhysicsContactRecordObserver action=[&](const auto& row){
        check(row.instruction==0x38e8c9&&row.address==0xfe801004&&row.before.empty()&&row.written.size()==4,
            "Initial IEN action invented a hardware preimage read");unknown_action_preimage=true;e.f.observe(row);};
    check(policy.run_initial_prefix(e.f.abi,{0x27f,0x1f80},e.f.services,action)==0x38e8d3&&unknown_action_preimage,"Native IEN prefix skipped its next mandatory frontend policy or invented readback");
    compare(e,original.at("initial_prefix"),"Original first configuration store/GP/defined flags differ");
    check(e.f.memory.checkpoint()==prefix_before,"Typed native IEN pending policy wrote a raw source MMIO bank");
    const auto pending=policy.diagnostics();check(pending.prepared&&pending.pending_mask==0xd8&&pending.unresolved_requested_mask==0xc8&&
        !pending.source_delivery_enabled&&!pending.full_configuration_ready&&pending.native_gate.value>0xffffffffu&&pending.dispatcher==port.state().dispatcher_descriptor,
        "Disabled pending policy lost actual native dispatcher or claimed missing routes");
    if(early)register_port(e,port);
    std::ranges::fill(e.f.memory.mutable_view(stack-0x100,0x200),std::byte(original.at("fill").get<unsigned>()));
    e.frame(apu+8,{0});policy.source_global_helper(e.f.abi,{0x27f,0x1f80},e.f.services,e.f.observe);
    compare(e,original.at("disable_helper"),"Original complete global-disable helper/ABI/source stores differ");
    check(std::ranges::equal(e.f.memory.view(stack-0x100,0x200),unhex(original.at("disable_helper").at("stack_hex"))),"Original whole disable-helper stack differs");
    unsigned foreign_guards{};std::thread other([&]{auto guard=[&](auto action){try{action();}catch(const std::exception&){++foreign_guards;}};
        auto invalid=e.f.abi;invalid.gp[4]=0xffffffff;
        guard([&]{policy.apply_disabled_source_store(0x38e8c9,0xfe801004,0xd8);});guard([&]{policy.run_initial_prefix(invalid,{0x27f,0x1f80},{},{});});
        guard([&]{policy.source_global_helper(invalid,{0x27f,0x1f80},{},{});});guard([&]{policy.diagnostics();});guard([&]{policy.retire();});});other.join();
    check(foreign_guards==5,"Foreign disabled-policy operation accessed source/native mutable state");
    e.frame(apu+8,{1});e.frontend->source_mode_helper(e.f.abi,{0x27f,0x1f80},e.f.services);
    e.frontend->idle_voice(64);e.frontend->process();port.signal_known_idle();
    const auto before_dispatch=e.f.memory.checkpoint();const auto instructions=e.f.instructions,stores=e.f.stores;
    auto invalid=e.f.abi;invalid.gp[4]=0xffffffff;const auto invalid_before=invalid.gp;
    check(!port.dispatch_known_idle(invalid,{0x27f,0x1f80},e.f.services,e.f.observe)&&invalid.gp==invalid_before&&e.f.memory.checkpoint()==before_dispatch&&
        e.f.instructions==instructions&&e.f.stores==stores&&port.state().pending_irq&&!port.state().delivered_irqs,
        "Disabled native source gate consumed eventfd/source frame or dispatched its original ISR");
    pollfd irq{port.state().interrupt_descriptor,POLLIN,0};check(poll(&irq,1,0)==1&&(irq.revents&POLLIN),"Disabled source gate discarded its actual pending eventfd receipt");
    // Independent native kernel-timer readiness probe, not source KeSetTimer
    // admission or an invented D8 producer. The IEN gate never owns this route.
    itimerspec due{};due.it_value.tv_nsec=1000000;
    check(timerfd_settime(port.state().timer_descriptor,0,&due,nullptr)==0,"Native timer independence probe failed to arm its actual descriptor");
    pollfd timer{port.state().timer_descriptor,POLLIN,0};check(poll(&timer,1,100)==1&&(timer.revents&POLLIN),"APU IEN gate blocked independent native timer readiness");
    std::uint64_t expires{};check(read(timer.fd,&expires,sizeof(expires))==sizeof(expires)&&expires==1,"Native timer independence probe lost actual expiry accounting");
    e.frame(apu+8,{1});rejected([&]{policy.source_global_helper(e.f.abi,{0x27f,0x1f80},e.f.services,e.f.observe);});
    const auto rejected_state=policy.diagnostics();check(rejected_state.faulted&&rejected_state.pending_mask==0xd8&&!rejected_state.source_delivery_enabled&&
        port.state().pending_irq&&!port.state().delivered_irqs,"Unsupported original global-enable branch opened source delivery or consumed actual pending work");
    rejected([&]{e.reset.control_lease();});port.retire_generation();
    check(!port.state().pending_irq&&port.state().cancelled_irqs==1&&!port.state().delivered_irqs,"Native policy teardown lost exact pending event cancellation accounting");
    check(policy.diagnostics().retired&&policy.diagnostics().dispatcher<0,"Native port retirement left a live policy dispatcher identity");
    policy.retire();e.retire();
    return {{"passed",true},{"fill",original.at("fill")},{"policy_precedes_registration",early},{"original_prefix_instructions",original.at("initial_prefix").at("instructions").size()},
        {"original_disable_instructions",original.at("disable_helper").at("instructions").size()},{"actual_pending_eventfd_retained",true},{"source_dispatch_blocked_before_frame_access",true},
        {"native_timer_readiness_independent",true},{"foreign_owner_rejections",foreign_guards},{"genuine_enable_branch_rejected_and_control_aborted",true},
        {"first_action_preimage_unknown",true},{"unowned_next_pc",0x38e8d3},{"source_delivery_enabled",false},{"missing_routes_ready",false}};
}
void interrupted_action(const Json& reference){
    EventFixture e(reference);mount_native_tu15_sound_event_port_data(e.f.memory);Port port(e.f.memory,e.reset,*e.frontend,e.output);
    Policy policy(e.f.memory,port,e.reset.control_lease());e.frame(apu+8,{});bool actual_action=false;
    rejected([&]{policy.run_initial_prefix(e.f.abi,{0x27f,0x1f80},e.f.services,[&](const auto& row){
        check(row.before.empty()&&row.address==0xfe801004,"Interrupted action fabricated preimage");actual_action=true;throw std::runtime_error("declared observer interruption after actual native disabled store");});});
    check(actual_action&&policy.diagnostics().prepared&&policy.diagnostics().faulted&&!policy.diagnostics().source_delivery_enabled,
        "Interrupted native disabled action failed to retain its fail-closed assembly state");
    rejected([&]{e.reset.control_lease();});port.retire_generation();e.retire();
}
void foreign_destructor(const Json& reference){
    EventFixture e(reference);mount_native_tu15_sound_event_port_data(e.f.memory);Port port(e.f.memory,e.reset,*e.frontend,e.output);
    auto policy=std::make_unique<Policy>(e.f.memory,port,e.reset.control_lease());
    std::set_terminate([]{std::_Exit(77);});std::thread other([owner=std::move(policy)]()mutable{owner.reset();});other.join();
    throw std::runtime_error("Foreign policy destructor violated its creating-executor contract");
}
}
int main(int argc,char** argv){try{
    if(argc==3&&std::string(argv[1])=="--foreign-destructor-probe"){
        std::ifstream input(argv[2]);Json registered;input>>registered;foreign_destructor(registered.at("cases").at(0));return 0;
    }
    check(argc==4,"Supply registered source reference, disabled-policy original reference and report");
    std::ifstream registered_in(argv[1]),original_in(argv[2]);Json registered,original;registered_in>>registered;original_in>>original;
    check(original.at("passed")==true&&original.at("instruction_patches").empty()&&original.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original disabled-policy reference");
    Json cases=Json::array();for(std::size_t i=0;i<registered.at("cases").size();++i){cases.push_back(run_policy(registered.at("cases").at(i),original.at("cases").at(i),true));cases.push_back(run_policy(registered.at("cases").at(i),original.at("cases").at(i),false));}
    interrupted_action(registered.at("cases").at(0));
    const auto driver=SDL_GetCurrentAudioDriver();
    Json report={{"passed",true},{"native_backend",driver?Json(driver):Json(nullptr)},{"native_pcm_delivery","retained manual host mixer with genuine PCM Success receipts; no live audio device claim"},{"cases",cases},
        {"action_commit_interruption_aborts_actual_control",true},{"full_d8_enable_or_initializer45_return",false},{"frontend_policy_or_scheduler_words_admitted",false}};
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save native disabled-policy proof");std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
