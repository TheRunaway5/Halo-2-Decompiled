// Reuse the independently verified genuine cold APU fixture without modifying
// the frozen reset test or duplicating its constructors/native lifecycle.
#define main native_reset_component_main
#include "native_tu15_sound_reset_tests.cpp"
#undef main
#include "halo2/source_runtime_sound_frontend_events.hpp"
#include "halo2/source_sound_frontend_dpc_tu15.hpp"
#include "halo2/native_tu15_sound_frontend_dpc_data.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/source_sound_pending_aggregate_tu15.hpp"
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
        f.memory.write(0x2000000,std::vector<std::byte>(0x20000,std::byte(reference.at("case").at("fill").get<unsigned>())));
        f.memory.write(stack-0x100,unhex(reference.at("reset").at("stack_hex")));put(f.memory,fs+0x24,0);put(f.memory,fs+0x58,0);
        mount_native_tu15_sound_dma_imports(f.memory);
        mount_native_tu15_sound_frontend_dpc_data(f.memory);
        f.memory.mount(0x3911e4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"original fence counter loader zero");
        f.memory.mount(0x391380,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,"original fence DMA descriptor");
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
        f.memory.mount(0x19010000,std::array<std::byte,0x400>{},SourceRuntimeMemory::Access::ReadWrite,"declared bounded one-slot source channel inputs");
        f.memory.mount(0x19020000,std::array<std::byte,0xe0>{},SourceRuntimeMemory::Access::ReadWrite,"actual zeroed native metadata object");
        std::array<std::byte,24> descriptor{};
        descriptor[0]=std::byte{24};for(unsigned i=0;i<4;++i)descriptor[12+i]=std::byte(0x19022000u>>(8*i));
        f.memory.mount(0x19021000,descriptor,SourceRuntimeMemory::Access::ReadOnly,"declared source nonloop PCM descriptor input");
        const std::array<unsigned char,18> wave{1,0,2,0,0x80,0xbb,0,0,0,0xee,2,0,4,0,16,0,0,0};
        f.memory.mount(0x19022000,std::as_bytes(std::span(wave)),SourceRuntimeMemory::Access::ReadOnly,"declared original stereo48k PCM input");
        f.instructions=Json::array();f.stores=Json::array();
        f.services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observe) {
            if(dma.dispatch(bank,call,abi))return;
            if(reset.try_call(bank,call,abi,{0x27f,0x1f80},f.services,observe))return;
            if(source_sound_frontend_dpc_tu15_owns_entry(call.target)) {
                source_run_sound_frontend_dpc_tu15(bank,call.target,abi,{0x27f,0x1f80},f.services,observe);return;
            }
            if(source_sound_frontend_events_tu15_owns_entry(call.target)) {
                source_run_sound_frontend_events_tu15(bank,call.target,abi,{0x27f,0x1f80},f.services,observe);return;
            }
            throw std::runtime_error("Unowned event fixture source dependency");
        };
        call(0x387bb5,0,{});
        for(const auto& [descriptor,size]:std::array{std::pair{0x391390u,0x8000u},std::pair{0x3913a0u,0x4020u},std::pair{0x391380u,4u}}) {
            put(f.memory,descriptor+8,size);call(0x38e71c,0,{descriptor,0x4000,4});
        }
        f.abi.gp[0]=0x19020000;source_construct_sound_pcm_metadata_header_tu15(f.memory,f.abi,{0x27f,0x1f80},f.services,f.observe);
        call(0x386fd1,0x19020000,{0x19021000});
        for(unsigned i=0;i<2;++i) {
            const auto channel=0x19010000+i*0x200;call(0x38cc02,channel,{apu,0x19020000});put(f.memory,channel+0x64,1);
            f.memory.write(channel+0x12,std::array{std::byte{1},std::byte{}});
            call(0x388817,apu,{channel});call(0x38bb72,channel+0x68,{64+i});
        }
        check(f.instructions==reference.at("case").at("setup").at("instructions")&&f.stores==reference.at("case").at("setup").at("stores"),"Original event prerequisite CPU construction differs");
        // Explicit declared source active-list input, not a playback result.
        const auto head=apu+0x488;put(f.memory,head,0x1901004c);put(f.memory,head+4,0x1901024c);
        put(f.memory,0x1901004c,0x1901024c);put(f.memory,0x19010050,head);put(f.memory,0x1901024c,head);put(f.memory,0x19010250,0x1901004c);
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
        for(unsigned i=0;i<2;++i)frontend->bind(64+i,0x19010000+i*0x200,leases[i]);
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
        else if(source_sound_frontend_dpc_tu15_owns_entry(entry))source_run_sound_frontend_dpc_tu15(f.memory,entry,f.abi,{0x27f,0x1f80},f.services,f.observe);
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
    e.frame(0,{apu+0x4c8,apu,0,0});
    check(frontend.dispatch_dpc(f.abi,{0x27f,0x1f80},f.services,f.observe),"Real deferred callback was not executed");
    const auto& dpc=reference.at("case").at("dpc");
    if(f.instructions!=dpc.at("instructions")||f.stores!=dpc.at("stores")) {
        std::ofstream detail("/tmp/native-halo2-dpc-mismatch.json");detail<<Json{{"instructions",f.instructions},{"stores",f.stores},{"expected",dpc}}.dump(2);
        throw std::runtime_error("Original whole deferred callback instruction/ordered-store trace differs");
    }
    check(f.abi.gp==dpc.at("gp").get<decltype(f.abi.gp)>()&&(f.abi.eflags&0xcc5)==(dpc.at("flags").get<unsigned>()&0xcc5),"Original DPC GP/defined flags differ");
    check(!frontend.pending_dpc()&&frontend.state().completed_dpcs==1&&frontend.state().completed_fences==1,"Real deferred work/fence lifetime was not completed exactly once");
    check(!frontend.dispatch_dpc(f.abi,{0x27f,0x1f80},f.services),"Completed deferred callback was executed twice");
    const auto& expected=reference.at("case");
    check(std::ranges::equal(f.memory.view(apu,0x540),unhex(expected.at("apu_hex"))),"Original final source APU bytes differ");
    for(unsigned i=0;i<2;++i)check(std::ranges::equal(f.memory.view(0x19010000+i*0x200,0x158),unhex(expected.at("channels_hex").at(i))),"Original final source channel bytes differ");
    for(const auto& row:e.dma.allocations()) {
        const auto key=row.source_descriptor==0x391390?"voice_hex":row.source_descriptor==0x3913a0?"notifier_hex":"fence_hex";
        check(std::ranges::equal(f.memory.view(row.source,row.capacity),unhex(expected.at(key))),"Original final real DMA voice/notifier bytes differ");
    }
    check(std::ranges::equal(f.memory.view(stack-0x200,0x210),unhex(expected.at("stack_hex"))),"Original final source stack bytes differ");
    rejected([&]{frontend.prepare_idle_trap(0x38e8c9,0xfe801004,0xd8);});
    check(std::ranges::equal(f.memory.view(0x19020000,0xe0),unhex(expected.at("metadata_hex"))),"Original PCM metadata bytes differ");
    for(const auto& lease:e.leases)check(lease->completion()->status==SdlSoundCompletionStatus::Success,"Source notification consumed or rewrote actual retained PCM receipt");
    frontend.retire_generation();check(frontend.state().cancelled_dpcs==0,"Completed DPC was fabricated as cancelled");
    rejected([&]{frontend.process();});e.retire();
    std::cout<<"Genuine PCM constructors, both idle ISRs and whole598-instruction/203-store deferred notification matched; actual DMA fence completed\n";
}
void admitted_idle(EventFixture& e) {
    e.frontend->idle_voice(64);e.frame(apu+8,{1});e.frontend->source_mode_helper(e.f.abi,{0x27f,0x1f80},e.f.services);
    e.frontend->process();e.frame(apu,{});e.frontend->dispatch_interrupt(e.f.abi,{0x27f,0x1f80},e.f.services);
    e.frame(0,{apu+0x4c8,apu,0,0});
}
void deferred_failure(const Json& reference,bool after_fence) {
    EventFixture e(reference);admitted_idle(e);auto& frontend=*e.frontend;
    const auto before=e.reset.state().completed_commands;
    const auto idle=e.f.memory.view(apu+0x4a0,8);const std::vector<std::byte> saved_idle(idle.begin(),idle.end());
    put(e.f.memory,apu+0x4a0,0x19010354);put(e.f.memory,apu+0x4a4,0x19010354);
    put(e.f.memory,0x19010354,apu+0x4a0);put(e.f.memory,0x19010358,apu+0x4a0);
    rejected([&]{frontend.dispatch_dpc(e.f.abi,{0x27f,0x1f80},e.f.services);});e.f.memory.write(apu+0x4a0,saved_idle);
    check(!frontend.state().faulted&&frontend.state().queued_dpcs==1&&e.reset.state().completed_commands==before,
        "Unowned source list voice was executed or touched actual native worker before rejection");
    auto foreign=e.f.abi;foreign.gp[4]=0xfffffff0;bool wrong_thread=false;
    std::thread other([&]{try{frontend.dispatch_dpc(foreign,{0x27f,0x1f80},e.f.services);}catch(const std::exception& error){wrong_thread=std::string(error.what()).find("foreign owner thread")!=std::string::npos;}});other.join();
    check(wrong_thread&&e.reset.state().completed_commands==before,"Foreign DPC touched invalid source stack or native worker before thread rejection");
    put(e.f.memory,stack+8,apu+8);rejected([&]{frontend.dispatch_dpc(e.f.abi,{0x27f,0x1f80},e.f.services);});put(e.f.memory,stack+8,apu);
    check(!frontend.state().faulted&&frontend.state().queued_dpcs==1,"Rejected DPC frame completed or lost its retained record");
    auto services=e.f.services;services.instruction=[&](auto pc){if(pc==(after_fence?0x38cad7u:0x38894fu))throw std::runtime_error("declared source interruption during actual deferred drain");};
    rejected([&]{frontend.dispatch_dpc(e.f.abi,{0x27f,0x1f80},services);});
    check(frontend.state().faulted&&!e.reset.quiescent()&&frontend.state().queued_dpcs==1&&!frontend.state().completed_dpcs&&frontend.state().completed_fences==unsigned(after_fence),
        "Partial genuine DPC was reported complete or discarded its actual fence/record diagnostics");
    const auto rows=e.dma.allocations();const auto at=std::ranges::find_if(rows,[](const auto& row){return row.source_descriptor==0x391380;});
    check(at!=rows.end(),"Native fence DMA owner disappeared");const auto raw=e.f.memory.view(at->native,4);
    check(raw[0]==std::byte(after_fence?1:0xfe)&&raw[1]==std::byte(after_fence?0:0xff),"Interrupted native DMA fence invented completion or rolled back actual transfer");
    frontend.retire_generation();check(frontend.state().cancelled_dpcs==1,"Faulted DPC was manufactured as completed");e.retire();
}
void unsupported_arm(const Json& reference) {
    EventFixture e(reference);admitted_idle(e);e.f.memory.write(0x1902000a,std::array{std::byte{4}});
    auto services=e.f.services;bool reached=false;
    services.required_owner=[&](auto&,const auto& call,auto&,const auto&){reached=call.instruction==0x389346&&call.target==0x388bc2;throw std::runtime_error("Required native immediate notification/timer owner is unavailable");};
    rejected([&]{e.frontend->dispatch_dpc(e.f.abi,{0x27f,0x1f80},services);});
    check(reached&&e.frontend->state().faulted&&e.frontend->state().queued_dpcs==1&&!e.frontend->state().completed_dpcs,
        "Unowned derived notification arm was skipped or claimed completed");
    check(e.leases[0]->completion()->status==SdlSoundCompletionStatus::Success,"Unowned source notification invented native packet acknowledgement");
    e.frontend->retire_generation();e.retire();
}
void data_composition() {
    SourceRuntimeMemory m;mount_native_tu15_sound_sdk_prefix_data(m);
    const auto old=m.native_address(0x4212b0,12);const auto before=m.view(old,12);const std::vector<std::byte> original(before.begin(),before.end());
    mount_native_tu15_sound_frontend_dpc_data(m);mount_native_tu15_sound_frontend_dpc_data(m);
    check(m.native_address(0x4212b0,12)==old&&std::ranges::equal(m.view(old,12),original),"DPC literal composition rebound the actual SDK vtable");
    const auto literal=m.view(0x4212b0,8);const std::vector<std::byte> good(literal.begin(),literal.end());
    for(unsigned kind=0;kind<4;++kind) {
        SourceRuntimeMemory foreign;
        if(kind==3) {
            foreign.reserve(0x41b000,0x1000,0x1000,"foreign uncommitted reservation covering late DPC import operands");
        }else {
            auto bytes=good;if(kind==0)bytes[0]^=std::byte{1};if(kind==2)bytes.resize(4);
            foreign.mount(0x4212b0,bytes,kind==1?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,"foreign changed/mutable/partial literal");
        }
        const auto saved=foreign.checkpoint();
        rejected([&]{mount_native_tu15_sound_frontend_dpc_data(foreign);});
        check(foreign.checkpoint()==saved,"Rejected DPC literal composition changed an existing bank or leaked new rows");
    }
    // Import operands can be fully covered by adjacent earlier immutable
    // subregions; their actual backing must also remain untouched.
    SourceRuntimeMemory split;const auto import=m.view(0x41b7b8,4);
    split.mount(0x41b7b8,import.first(2),SourceRuntimeMemory::Access::ReadOnly,"shared import low half");
    split.mount(0x41b7ba,import.last(2),SourceRuntimeMemory::Access::ReadOnly,"shared import high half");
    const auto low=split.native_address(0x41b7b8,2),high=split.native_address(0x41b7ba,2);
    mount_native_tu15_sound_frontend_dpc_data(split);
    check(split.native_address(0x41b7b8,2)==low&&split.native_address(0x41b7ba,2)==high,"Composed immutable import operands were replaced");
}
void aggregate(const Json& report) {
    for(const auto& row:report.at("bit7_cases")) {
        SourceRuntimeMemory memory;const auto fill=std::byte(row.at("fill").get<unsigned>());
        memory.mount(0x2000000,std::vector<std::byte>(0x20000,fill),SourceRuntimeMemory::Access::ReadWrite,"declared aggregate source stack");
        memory.mount(apu,std::vector<std::byte>(0x1000,fill),SourceRuntimeMemory::Access::ReadWrite,"actual aligned aggregate backing");
        // Apply the independent original ISR's genuine output as declared
        // input. This supplies no invented bit7 hardware producer.
        for(const auto& store:row.at("irq").at("stores")) {
            const auto p=store.at(1).get<unsigned>();if(p<0xfe800000)memory.write(p,unhex(store.at(4)));
        }
        put(memory,apu+0x4c0,0);put(memory,stack,0x2100000);put(memory,stack+4,apu);
        SourcePhysicsWorldRuntimeAbi abi{};for(unsigned i=0;i<8;++i)abi.gp[i]=0x11000000+i*0x111111;abi.gp[1]=0;abi.gp[4]=stack;abi.fs_base=fs;abi.eflags=0x202;
        Json instructions=Json::array(),stores=Json::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){instructions.push_back(pc);};
        const auto hex=[](std::span<const std::byte> bytes){const char* digits="0123456789abcdef";std::string s;for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);s+=digits[v>>4];s+=digits[v&15];}return s;};
        std::mutex domain;std::lock_guard lock(domain);
        source_run_sound_pending_aggregate_tu15(memory,abi,{0x27f,0x1f80},services,[&](const auto& s){stores.push_back({s.instruction,s.address,s.written.size(),hex(s.before),hex(s.written)});});
        const auto& expected=row.at("aggregate");
        check(instructions==expected.at("instructions")&&stores==expected.at("stores")&&abi.gp==expected.at("returned_gp").get<decltype(abi.gp)>()&&
            (abi.eflags&0xcc5)==(expected.at("returned_eflags").get<unsigned>()&0xcc5),"Original atomic aggregate discarded opaque status bits or changed ABI/stores");
        check(memory.view(apu+0x4c0,4)[0]==std::byte{0x81}&&memory.view(apu+0x4b8,4)[0]==std::byte{},"Actual atomic pending backing lost raw81 status");
    }
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
    data_composition();for(const auto& row:reference.at("cases"))events(row);aggregate(reference);
    const auto& first=reference.at("cases").at(0);interruption(first);after_enqueue_failure(first);deferred_failure(first,false);deferred_failure(first,true);unsupported_arm(first);return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
