// Reuse the independently verified genuine cold APU fixture without modifying
// the frozen reset test or duplicating its constructors/native lifecycle.
#define main native_reset_component_main
#include "native_tu15_sound_reset_tests.cpp"
#undef main
#include "halo2/source_runtime_sound_frontend_events.hpp"
#include "halo2/source_sound_frontend_events_tu15.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
namespace {
using Frontend=SourceRuntimeSoundFrontendEvents;
struct EventFixture {
    Fixture f;
    std::shared_ptr<SdlSoundPlatform> output;
    SourceRuntimeSoundResetResources reset;
    SourceRuntimeSoundDmaResources dma;
    std::unique_ptr<Frontend> frontend;
    std::vector<std::shared_ptr<SourceRuntimeSoundResetResources::VoiceLease>> leases;
    std::array<std::uint32_t,8> initial;
    explicit EventFixture(const Json& reference)
        :f(reference.at("reset")),output(std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}})),
         reset(f.memory,output,fs),dma(f.memory,output),initial(reference.at("reset").at("constructor_input_gp").get<decltype(initial)>()) {
        f.frame(reference.at("reset"));check(reset.try_call(f.memory,{0x38ec8f,0x38ec0e,false},f.abi,{0x27f,0x1f80},f.services),"Event fixture cold reset was not owned");
        mount_native_tu15_sound_dma_imports(f.memory);
        for(const auto& [p,value]:std::array{std::pair{0x41b640u,0x800000a6u},std::pair{0x41b70cu,0x8000006bu},std::pair{0x41b7c4u,0x80000077u}}) {
            std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));f.memory.mount(p,raw,SourceRuntimeMemory::Access::ReadOnly,"authentic frontend/DMA native import identity");
        }
        f.memory.mount(0x390bc4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"genuine source DMA statistics");
        f.memory.mount(0x3912a0,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"genuine source DMA statistics pointer");
        f.memory.mount(0x391390,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,"original voice DMA descriptor");
        f.memory.mount(0x3913a0,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,"original notifier DMA descriptor");
        std::array<std::byte,36> groups{};
        for(unsigned i=0;i<9;++i){const auto value=0x2054u+i*4u;for(unsigned b=0;b<4;++b)groups[i*4+b]=std::byte(value>>(8*b));}
        f.memory.mount(0x390a7c,groups,SourceRuntimeMemory::Access::ReadOnly,"authenticated source voice group register address table");
        f.memory.mount(0x19010000,std::array<std::byte,0x200>{},SourceRuntimeMemory::Access::ReadWrite,"declared bounded one-slot source channel inputs");
        f.memory.mount(0x19020000,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadOnly,"declared original nonlooping2D source format input");
        f.instructions=Json::array();f.stores=Json::array();
        f.services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observe) {
            if(dma.dispatch(bank,call,abi))return;
            if(reset.try_call(bank,call,abi,{0x27f,0x1f80},f.services,observe))return;
            if(source_sound_frontend_events_tu15_owns_entry(call.target)) {
                source_run_sound_frontend_events_tu15(bank,call.target,abi,{0x27f,0x1f80},f.services,observe);return;
            }
            throw std::runtime_error("Unowned event fixture source dependency");
        };
        call(0x387bb5,0,{});
        for(const auto& [descriptor,size]:std::array{std::pair{0x391390u,0x8000u},std::pair{0x3913a0u,0x4020u}}) {
            put(f.memory,descriptor+8,size);call(0x38e71c,0,{descriptor,0x4000,4});
        }
        for(unsigned i=0;i<2;++i) {
            const auto channel=0x19010000+i*0x100;put(f.memory,channel+8,apu);put(f.memory,channel+0x64,1);put(f.memory,channel+0x70,0x19020000);
            f.memory.write(channel+0x12,std::array{std::byte{1},std::byte{}});
            call(0x388817,apu,{channel});call(0x38bb72,channel+0x68,{64+i});
        }
        check(f.instructions==reference.at("case").at("setup").at("instructions")&&f.stores==reference.at("case").at("setup").at("stores"),"Original event prerequisite CPU construction differs");
        // Explicit declared source active-list input, not a playback result.
        const auto head=apu+0x488;put(f.memory,head,0x1901004c);put(f.memory,head+4,0x1901014c);
        put(f.memory,0x1901004c,0x1901014c);put(f.memory,0x19010050,head);put(f.memory,0x1901014c,head);put(f.memory,0x19010150,0x1901004c);
        const SdlSoundVoiceParameters parameters{std::bit_cast<std::uint32_t>(1.0f),0,false,{{0,0,1},{1,1,1}}};
        for(unsigned i=0;i<2;++i) {
            auto lease=reset.bind_voice(64+i,0x100000040ull+i,parameters);
            const auto pcm=std::make_shared<const std::vector<float>>(std::vector<float>(128,0.125f));
            reset.submit(64+i,{pcm,0,64,0x100000040ull+i});leases.push_back(std::move(lease));
        }
        (void)output->render(64);
        frontend=std::make_unique<Frontend>(f.memory,reset,dma,1);
        frame(0,{});f.abi.gp[3]=0x8000006b;put(f.memory,stack,0x3893bf);put(f.memory,stack+4,apu+0x4c8);put(f.memory,stack+8,0x38918c);put(f.memory,stack+12,apu);
        check(frontend->try_initialize_dpc(f.memory,{0x3893bd,0x8000006b,false},f.abi),"Actual native deferred callback resource registration failed");
        frontend->prepare_idle_trap(0x38e8eb,0xfe801504,0x8000);
        for(unsigned i=0;i<2;++i)frontend->bind(64+i,0x19010000+i*0x100,leases[i]);
    }
    void frame(std::uint32_t ecx,std::initializer_list<std::uint32_t> args) {
        f.abi.gp=initial;f.abi.gp[1]=ecx;f.abi.gp[4]=stack;f.abi.eflags=0x202;put(f.memory,stack,0x2100000);
        unsigned p=stack+4;for(const auto value:args){put(f.memory,p,value);p+=4;}
        f.instructions=Json::array();f.stores=Json::array();
    }
    void call(std::uint32_t entry,std::uint32_t ecx,std::initializer_list<std::uint32_t> args) {
        // Keep the complete prerequisite trace across individual real calls.
        auto instructions=std::move(f.instructions),stores=std::move(f.stores);frame(ecx,args);f.instructions=std::move(instructions);f.stores=std::move(stores);
        if(source_sound_dma_storage_tu15_owns_entry(entry))source_run_sound_dma_storage_tu15(f.memory,entry,f.abi,{0x27f,0x1f80},f.services,f.observe);
        else if(source_sound_frontend_events_tu15_owns_entry(entry))source_run_sound_frontend_events_tu15(f.memory,entry,f.abi,{0x27f,0x1f80},f.services,f.observe);
        else source_run_sound_sdk_tu15(f.memory,entry,f.abi,{0x27f,0x1f80},f.services,f.observe);
    }
    void retire() {
        frontend.reset();
        for(unsigned i=0;i<2;++i){const auto receipt=leases[i]->completion();check(bool(receipt),"Actual event packet lost its source-accounting receipt");leases[i]->acknowledge(receipt->submission);reset.retire_voice(64+i);}
    }
};
void events(const Json& reference) {
    EventFixture e(reference);auto& f=e.f;auto& frontend=*e.frontend;
    SourceRuntimeMemory mirrored;mirrored.mount(apu,f.memory.view(apu,0x540),SourceRuntimeMemory::Access::ReadWrite,"foreign mirrored APU bytes");
    const auto commands_before=e.reset.state().completed_commands;
    rejected([&]{Frontend foreign(mirrored,e.reset,e.dma,1);});
    check(e.reset.state().completed_commands==commands_before,"Foreign mirrored bank mutated the actual control worker");
    check(frontend.idle_voice(64)==Frontend::Admission::Accepted&&frontend.idle_voice(65)==Frontend::Admission::Backpressure,"Actual bounded frontend queue lacks backpressure");
    frontend.process();check(frontend.state().mode==Frontend::Mode::Halted&&frontend.state().queued_commands==1,"Halted frontend consumed a command");
    e.frame(apu+8,{1});frontend.source_mode_helper(f.abi,{0x27f,0x1f80},f.services,f.observe);
    const auto& mode=reference.at("case").at("mode");
    check(f.instructions==mode.at("instructions")&&f.stores==mode.at("stores")&&f.abi.gp==mode.at("gp").get<decltype(f.abi.gp)>(),"Original frontend running-mode CPU behavior differs");
    for(unsigned i=0;i<2;++i) {
        frontend.process();check(frontend.state().mode==Frontend::Mode::Trapped&&frontend.state().interrupt_status==0x11,"Actual idle command did not trap/pause its frontend");
        if(i==0){check(frontend.idle_voice(65)==Frontend::Admission::Accepted,"Trapped frontend lost bounded command admission");frontend.process();check(frontend.state().queued_commands==1,"Trapped frontend dispatched another command");}
        e.frame(apu,{});check(frontend.dispatch_interrupt(f.abi,{0x27f,0x1f80},f.services,f.observe),"Actual native idle interrupt was not dispatched");
        const auto& expected=reference.at("case").at("events").at(i);
        check(f.instructions==expected.at("instructions")&&f.stores==expected.at("stores"),"Original idle interrupt instruction/ordered-store trace differs");
        check(f.abi.gp==expected.at("gp").get<decltype(f.abi.gp)>()&&(f.abi.eflags&0xcc5)==(expected.at("flags").get<unsigned>()&0xcc5),"Original idle interrupt GP/defined flags differ");
        const auto dpc=frontend.pending_dpc();check(dpc&&dpc->identity>0xffffffffu&&dpc->native.value>0xffffffffu&&dpc->source_dpc==apu+0x4c8&&dpc->routine==0x38918c&&dpc->context==apu,
            "Real native deferred callback ownership was lost or truncated");
        check(frontend.state().queued_dpcs==1&&frontend.state().mode==Frontend::Mode::Running&&!frontend.state().interrupt_status,"Original idle callback did not resume the frontend/coalesce actual deferred work");
    }
    const auto& expected=reference.at("case");
    check(std::ranges::equal(f.memory.view(apu,0x540),unhex(expected.at("apu_hex"))),"Original final source APU bytes differ");
    for(unsigned i=0;i<2;++i)check(std::ranges::equal(f.memory.view(0x19010000+i*0x100,0x100),unhex(expected.at("channels_hex").at(i))),"Original final source channel bytes differ");
    for(const auto& row:e.dma.allocations()) {
        const auto key=row.source_descriptor==0x391390?"voice_hex":"notifier_hex";
        check(std::ranges::equal(f.memory.view(row.source,row.capacity),unhex(expected.at(key))),"Original final real DMA voice/notifier bytes differ");
    }
    check(std::ranges::equal(f.memory.view(stack-0x200,0x210),unhex(expected.at("stack_hex"))),"Original final source stack bytes differ");
    rejected([&]{frontend.prepare_idle_trap(0x38e8c9,0xfe801004,0xd8);});
    frontend.retire_generation();check(frontend.state().cancelled_dpcs==1,"Generation retirement lost unexecuted deferred work diagnostic");
    rejected([&]{frontend.process();});e.retire();
    std::cout<<"Actual native idle commands matched both original interrupt/unlink/notifier passes; real deferred work remains unexecuted\n";
}
void interruption(const Json& reference) {
    EventFixture e(reference);auto& frontend=*e.frontend;
    frontend.idle_voice(64);e.frame(apu+8,{1});frontend.source_mode_helper(e.f.abi,{0x27f,0x1f80},e.f.services);frontend.process();e.frame(apu,{});
    auto services=e.f.services;services.instruction=[](auto pc){if(pc==0x38b15b)throw std::runtime_error("declared interruption before authentic source notifier publication");};
    rejected([&]{frontend.dispatch_interrupt(e.f.abi,{0x27f,0x1f80},services);});
    check(frontend.state().faulted&&!e.reset.quiescent(),"Partial source idle callback was claimed complete");
    const auto raw=e.f.memory.view(0x8500905f,1);check(raw[0]==std::byte{0x80},"Interrupted source idle callback invented notifier publication");
    rejected([&]{frontend.process();});frontend.retire_generation();e.retire();
}
void generation(const Json& reference) {
    EventFixture e(reference);auto& frontend=*e.frontend;
    frontend.idle_voice(64);
    e.f.frame(reference.at("reset"));
    check(e.reset.try_call(e.f.memory,{0x38ec8f,0x38ec0e,false},e.f.abi,{0x27f,0x1f80},e.f.services),"Next actual control generation reset failed");
    rejected([&]{frontend.process();});frontend.retire_generation();
    check(frontend.state().cancelled_commands==1&&!frontend.state().queued_commands&&!frontend.state().interrupt_status,
          "Retired native generation delivered or lost its admitted command diagnostic");
    e.retire();
}
void after_enqueue_failure(const Json& reference) {
    EventFixture e(reference);auto& frontend=*e.frontend;
    frontend.idle_voice(64);e.frame(apu+8,{1});frontend.source_mode_helper(e.f.abi,{0x27f,0x1f80},e.f.services);frontend.process();e.frame(apu,{});
    auto services=e.f.services;services.instruction=[](auto pc){if(pc==0x3891e2)throw std::runtime_error("declared interruption after actual deferred-call insertion");};
    rejected([&]{frontend.dispatch_interrupt(e.f.abi,{0x27f,0x1f80},services);});
    check(frontend.state().faulted&&frontend.state().queued_dpcs==1,"Interrupted callback lost actual unexecuted deferred work");
    frontend.retire_generation();check(frontend.state().cancelled_dpcs==1,"Fault retirement manufactured deferred-call completion");e.retire();
}
}
int main(int argc,char** argv){try {
    check(argc==2,"Supply original native frontend reference");std::ifstream input(argv[1]);Json reference;input>>reference;
    check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original frontend reference differs");
    events(reference);interruption(reference);generation(reference);after_enqueue_failure(reference);return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
