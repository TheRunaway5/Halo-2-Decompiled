#include "halo2/source_runtime_sound_reset_resources.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
namespace {
using Json=nlohmann::json;
constexpr std::uint32_t stack=0x2008000,apu=0x19000000,fs=0x2001000;
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void put(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));m.write(p,b);
}
std::vector<std::byte> unhex(const std::string& s) {
    auto n=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};std::vector<std::byte> b(s.size()/2);
    for(std::size_t i=0;i<b.size();++i)b[i]=std::byte((n(s[2*i])<<4)|n(s[2*i+1]));return b;
}
template<class F>void rejected(F action) {
    bool caught=false;try{action();}catch(const std::exception&){caught=true;}
    check(caught,"Unsupported or foreign native reset request was accepted");
}
struct Fixture {
    SourceRuntimeMemory memory;
    SourcePhysicsWorldRuntimeAbi abi{};
    Json instructions=Json::array(),stores=Json::array();
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observe;
    explicit Fixture(const Json& reference) {
        memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared original reset stack and passive native context");
        memory.mount(apu,std::vector<std::byte>(0x540),SourceRuntimeMemory::Access::ReadWrite,"actual native64 zeroed constructor allocation");
        memory.mount(0x390bd0,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"genuine source voice accounting constructor cells");
        memory.mount(0x391200,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,"genuine loader-zero source context and timer cells");
        for(const auto& [slot,value]:std::array{std::pair{0x41b7acu,0x800000a0u},std::pair{0x41b6fcu,0x800000a1u}}) {
            std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));
            memory.mount(slot,raw,SourceRuntimeMemory::Access::ReadOnly,"authentic scoped exclusion import identity");
        }
        abi.gp=reference.at("constructor_input_gp").get<decltype(abi.gp)>();abi.fs_base=fs;abi.eflags=0x202;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        services.instruction=[&](auto pc){instructions.push_back(pc);};
        observe=[&](const auto& row){std::uint64_t value{};for(std::size_t i=0;i<row.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(row.written[i]))<<(8*i);
            stores.push_back({row.instruction,row.address,row.written.size(),value});};
        put(memory,stack,0x2100000);put(memory,stack+4,0);
        source_run_sound_sdk_tu15(memory,0x38871c,abi,{0x27f,0x1f80},services,observe);
        check(instructions==reference.at("constructor_instructions")&&stores==reference.at("constructor_stores"),"Original CPU APU constructor differs");
    }
    void frame(const Json& reference) {
        abi.gp=reference.at("input_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
        const auto original=reference.at("constructor_input_gp").get<decltype(abi.gp)>();
        put(memory,stack,0x38ec94);put(memory,stack+4,original[7]);put(memory,stack+8,original[6]);put(memory,stack+12,0x389389);
        instructions=Json::array();stores=Json::array();
    }
};
void run(const Json& row,const std::shared_ptr<SdlSoundPlatform>& output) {
    Fixture f(row);SourceRuntimeSoundResetResources owner(f.memory,output,fs);
    check(owner.quiescent()&&owner.state().command_capacity==64,"Native reset queue was not actually constructed");
    const SourcePhysicsContactRuntimeCall request{0x38ec8f,0x38ec0e,false};
    f.frame(row);
    const auto before=f.abi;
    auto wrong=request;wrong.tail=true;
    rejected([&]{owner.try_call(f.memory,wrong,f.abi,{0x27f,0x1f80},f.services,f.observe);});
    check(f.abi.gp==before.gp&&owner.state().reset_count==0,"Rejected reset changed source/native state");
    SourceRuntimeMemory other;
    rejected([&]{owner.try_call(other,request,f.abi,{0x27f,0x1f80},f.services,f.observe);});
    bool wrong_thread=false;std::thread thread([&]{try{owner.state();}catch(const std::exception&){wrong_thread=true;}});thread.join();
    check(wrong_thread,"Native reset accepted another owner thread");
    if(row.at("pre_reset").get<bool>()) {
        check(owner.try_call(f.memory,request,f.abi,{0x27f,0x1f80},f.services,f.observe),"First native reset was not owned");f.frame(row);
    }
    std::shared_ptr<const std::vector<float>> pcm;
    std::vector<std::shared_ptr<SourceRuntimeSoundResetResources::VoiceLease>> leases;
    if(row.at("voices").get<bool>()) {
        const SdlSoundVoiceParameters parameters{std::bit_cast<std::uint32_t>(1.0f),0,true,{{0,0,1},{1,1,1}}};
        leases.push_back(owner.bind_voice(7,0x100000007ull,parameters));leases.push_back(owner.bind_voice(8,0x100000008ull,parameters));
        pcm=std::make_shared<const std::vector<float>>(std::vector<float>(512,0.25f));
        check(owner.submit(7,{pcm,0,256,0x100000001ull})==SdlSoundPlatform::Submission::Accepted&&
              owner.submit(7,{pcm,0,256,0x100000002ull})==SdlSoundPlatform::Submission::Accepted,"Native reset did not retain real PCM");
        check(owner.submit(7,{pcm,0,256,0x100000003ull})==SdlSoundPlatform::Submission::Backpressure,"Two source packet slots did not exert real backpressure");
        rejected([&]{owner.retire_voice(7);});
    }
    check(owner.try_call(f.memory,request,f.abi,{0x27f,0x1f80},f.services,f.observe),"Reached source reset was not owned");
    check(f.instructions==row.at("instructions")&&f.stores==row.at("source_stores"),"Original native reset instruction/store order differs");
    check(f.abi.gp==row.at("returned_gp").get<decltype(f.abi.gp)>()&&
          (f.abi.eflags&0xcc5)==(row.at("returned_eflags").get<unsigned>()&0xcc5),"Original native reset GP/defined flags differ");
    check(std::ranges::equal(f.memory.view(apu,0x540),unhex(row.at("apu_hex"))),"Original native reset APU bytes differ");
    check(std::ranges::equal(f.memory.view(stack-0x100,0x110),unhex(row.at("stack_hex"))),"Original native reset complete stack bytes differ");
    const auto state=owner.state();const auto expected=1+unsigned(row.at("pre_reset").get<bool>());
    check(state.generation==expected&&state.reset_count==expected&&!state.queued_commands&&!state.faulted&&
          state.native_apu.value>0xffffffffu&&state.output_device==output->device_id()&&!state.gp_program_loaded&&!state.ep_program_loaded,
          "Native reset did not finish its actual control transaction or invented a DSP program");
    if(pcm) {
        check(!owner.quiescent()&&state.retained_voices==2,"Reset discarded unacknowledged voice/PCM lifetime");
        for(unsigned i=0;i<2;++i) {
            const auto completion=owner.completion(7);
            check(completion&&completion->status==SdlSoundCompletionStatus::Flushed&&completion->packet_context==0x100000001ull+i&&completion->pcm_frames==256,
                  "Actual native reset completion status/order/context differs");
            owner.acknowledge(7,completion->submission);
        }
        check(!owner.completion(7),"Reset retained an already accounted source packet");owner.retire_voice(7);owner.retire_voice(8);
    }
    check(owner.quiescent(),"Native reset graph did not retire all real voice resources");
    std::cout<<"Original APU reset pre="<<row.at("pre_reset")<<", voices="<<row.at("voices")<<": "<<f.instructions.size()<<" exact instructions, "<<f.stores.size()<<" ordered stores; generation "<<state.generation<<'\n';
}
void handoff(const Json& row,const std::shared_ptr<SdlSoundPlatform>& output) {
    Fixture f(row);f.frame(row);
    std::shared_ptr<SourceRuntimeSoundResetResources::VoiceLease> lease;
    const auto pcm=std::make_shared<const std::vector<float>>(std::vector<float>(2048,0.25f));
    {
        SourceRuntimeSoundResetResources owner(f.memory,output,fs);
        lease=owner.bind_voice(7,0x100000007ull,{std::bit_cast<std::uint32_t>(1.0f),0,false,{{0,0,1},{1,1,1}}});
        owner.submit(7,{pcm,0,1024,0x100000001ull});
        const auto before=output->render(64);
        check(std::ranges::all_of(before,[](float value){return value==0.25f;}),"Owned non-paused voice did not enter actual native PCM dispatch");
        check(owner.try_call(f.memory,{0x38ec8f,0x38ec0e,false},f.abi,{0x27f,0x1f80},f.services),"Real playing voice reset was not owned");
        rejected([&]{owner.submit(7,{pcm,0,1024,0x100000002ull});});
        const auto after=output->render(64);
        check(std::ranges::all_of(after,[](float value){return value==0;}),"Reset voice remained in actual PCM dispatch");
    }
    check(lease->source_index()==7&&lease->context()==0x100000007ull,"Owner teardown lost retained voice identity/context handoff");
    const auto completion=lease->completion();
    check(completion&&completion->status==SdlSoundCompletionStatus::Flushed&&completion->packet_context==0x100000001ull,"Owner teardown lost an unaccounted completion");
    lease->acknowledge(completion->submission);lease->retire();rejected([&]{lease->completion();});
}
void failure(const Json& row,const std::shared_ptr<SdlSoundPlatform>& output) {
    Fixture f(row);SourceRuntimeSoundResetResources owner(f.memory,output,fs);f.frame(row);
    auto services=f.services;
    services.instruction=[](auto pc){if(pc==0x38ec6e)throw std::runtime_error("declared interruption after real control retirement");};
    rejected([&]{owner.try_call(f.memory,{0x38ec8f,0x38ec0e,false},f.abi,{0x27f,0x1f80},services);});
    check(owner.state().faulted&&!owner.state().reset_count&&!owner.quiescent(),"Partial native reset was reported complete");
    rejected([&]{owner.try_call(f.memory,{0x38ec8f,0x38ec0e,false},f.abi,{0x27f,0x1f80},f.services);});
}
void callback_cutover(const Json& row) {
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
    Fixture f(row);f.frame(row);SourceRuntimeSoundResetResources owner(f.memory,output,fs);
    auto lease=owner.bind_voice(9,0x100000009ull,{std::bit_cast<std::uint32_t>(1.0f),0,false,{{0,0,1},{1,1,1}}});
    const auto pcm=std::make_shared<const std::vector<float>>(std::vector<float>(96000,0.125f));
    owner.submit(9,{pcm,0,48000,0x100000009ull});
    const auto first=output->rendered_frames();const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while(output->rendered_frames()==first&&std::chrono::steady_clock::now()<deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    check(output->rendered_frames()>first,"Actual native callback did not consume any application frames before reset");
    check(owner.try_call(f.memory,{0x38ec8f,0x38ec0e,false},f.abi,{0x27f,0x1f80},f.services),"Running callback reset failed");
    const auto completion=lease->completion();
    check(completion&&completion->status==SdlSoundCompletionStatus::Flushed,"Active callback did not retire the actual PCM packet as flushed");
    lease->acknowledge(completion->submission);
    rejected([&]{owner.submit(9,{pcm,0,48000,0x10000000aull});});owner.retire_voice(9);
    check(owner.quiescent(),"Active callback reset left an owned packet/voice");
    std::cout<<"Actual DeviceCallback cutover retained a Flushed packet until acknowledgement; no acoustic/capture parity claimed\n";
}
}
int main(int argc,char** argv){try {
    check(argc==2,"Supply original reset reference");std::ifstream input(argv[1]);Json reference;input>>reference;
    check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original reset reference differs");
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}});
    const auto* driver=SDL_GetCurrentAudioDriver();check(driver&&std::string(driver)!="dummy"&&std::string(driver)!="disk","Reset proof requires real native output");
    for(const auto& row:reference.at("cases"))run(row,output);
    handoff(reference.at("cases").at(0),output);
    failure(reference.at("cases").at(0),output);
    callback_cutover(reference.at("cases").at(0));
    std::cout<<"Actual retained native reset output backend: "<<driver<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
