#pragma once
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
}
