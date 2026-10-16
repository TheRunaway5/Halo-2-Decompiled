#include "halo2/source_sound_configuration_tu15.hpp"
#include "halo2/source_sound_reset_tu15.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t stack=0x2008000u,stop=0x2100000u,apu=0x19000000u,fs=0x2001000u;
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string& value){
    check(value.size()%2==0,"Invalid exact request transcript hex");std::vector<std::byte> bytes;
    for(std::size_t i=0;i<value.size();i+=2)bytes.push_back(std::byte(std::stoul(value.substr(i,2),nullptr,16)));
    return bytes;
}
std::string hex(std::span<const std::byte> bytes){
    constexpr char digits[]="0123456789abcdef";std::string value;
    for(auto byte:bytes){const auto n=std::to_integer<unsigned>(byte);value+=digits[n>>4];value+=digits[n&15];}return value;
}
void put(SourceRuntimeMemory& memory,std::uint32_t p,std::uint32_t value){
    std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));memory.write(p,raw);
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p){
    const auto raw=memory.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;
}

// Test-only exact external-input replay, never a runtime MMIO implementation.
// Every hardware read and write must match the next authenticated original
// transcript item. No unknown word is accepted into native policy or hardware
// storage, and no backend/program/ready state is created by this fixture.
struct ReferenceRequestTranscript final:SourcePhysicsContactMemory {
    SourceRuntimeMemory& bank;
    const Json& expected;
    std::uint32_t pc{};
    std::size_t store{},read_index{};
    ReferenceRequestTranscript(SourceRuntimeMemory& memory,const Json& source):bank(memory),expected(source){}
    void read(std::uint32_t p,std::span<std::byte> out)const override{
        if(p<0xfe800000u){bank.read(p,out);return;}
        auto& self=const_cast<ReferenceRequestTranscript&>(*this);
        const auto& reads=expected.at("hardware_reads");
        if(self.read_index<reads.size()&&reads[self.read_index][0]==pc&&reads[self.read_index][1]==p){
            const auto& row=reads[self.read_index++];check(row[2]==out.size(),"Original hardware input width differs");
            const auto bytes=unhex(row[3]);std::ranges::copy(bytes,out.begin());return;
        }
        // The shared before/after observer reads the old destination before
        // a source store. These are transcript-before bytes, not a hardware
        // result or a second executed original hardware-read instruction.
        const auto& row=expected.at("stores").at(store);
        check(row[0]==pc&&row[1]==p&&row[2]==out.size(),"Unowned configuration read outside exact CPU transcript");
        const auto bytes=unhex(row[3]);std::ranges::copy(bytes,out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override{
        if(p<0xfe800000u){bank.write(p,in);return;}
        const auto& row=expected.at("stores").at(store);
        check(row[0]==pc&&row[1]==p&&row[2]==in.size()&&row[4]==hex(in),
            "Original configuration request differs from authenticated CPU transcript");
    }
};

void run(const Json& source,Json& reports){
    SourceRuntimeMemory bank;mount_native_tu15_sound_configuration_data(bank);
    const auto fill=source.at("fill").get<unsigned>();
    bank.mount(0x2000000u,std::vector<std::byte>(0x20000,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,
        "Declared independent source CPU caller stack/passive thread");
    bank.mount(apu,std::vector<std::byte>(0x1000,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,
        "Declared source APU CPU object; no configured native audio object");
    put(bank,fs+0x24u,0);put(bank,0x391460u,source.at("declared_initialized"));
    put(bank,0x38f848u,source.at("declared_prior_count"));put(bank,stack,stop);
    ReferenceRequestTranscript transcript(bank,source);
    SourcePhysicsWorldRuntimeAbi abi{};
    for(unsigned i=0;i<8;++i)abi.gp[i]=0x11000000u+i*0x111111u;
    abi.gp[1]=apu+8;abi.gp[4]=stack;abi.eflags=0x202;abi.fs_base=fs;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    Json instructions=Json::array(),stores=Json::array(),platform=Json::array();
    std::recursive_mutex exclusion;unsigned level=0;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){transcript.pc=pc;instructions.push_back(pc);};
    SourcePhysicsContactRecordObserver observe=[&](const auto& row){
        Json actual={row.instruction,row.address,row.written.size(),hex(row.before),hex(row.written)};
        check(actual==source.at("stores").at(transcript.store),"Unchanged configuration CPU ordered store differs");
        stores.push_back(std::move(actual));++transcript.store;
    };
    services.required_owner=[&](auto& memory,const auto& call,auto& state,const auto& observer){
        check(&memory==&transcript,"Configuration CPU crossed its declared test transcript");
        if(source_sound_reset_tu15_owns_entry(call.target)){
            source_run_sound_reset_tu15(memory,call.target,state,{0x27f,0x1f80},services,observer);return;
        }
        check(!call.tail&&word(bank,state.gp[4])==call.instruction+6u,
            "Original native exclusion caller frame differs");
        if(call.target==0x800000a0u){
            check(call.instruction==0x383d88u&&level==0&&(state.gp[1]&255)==2,
                "Original native exclusion acquisition differs");
            exclusion.lock();level=2;state.gp[0]&=0xffffff00u;
        }else{
            check(call.target==0x800000a1u&&call.instruction==0x383d9du&&level==2&&(state.gp[1]&255)==0,
                "Unowned configuration CPU external operation");
            level=0;exclusion.unlock();
        }
        put(bank,fs+0x24u,level);platform.push_back({call.target,word(bank,state.gp[4]),level});state.gp[4]+=4;
    };
    bank.mount(0x41b6fcu,std::array{std::byte{0xa1},std::byte{},std::byte{},std::byte{0x80}},
        SourceRuntimeMemory::Access::ReadOnly,"Original exclusion import identity");
    bank.mount(0x41b7acu,std::array{std::byte{0xa0},std::byte{},std::byte{},std::byte{0x80}},
        SourceRuntimeMemory::Access::ReadOnly,"Original exclusion import identity");
    source_run_sound_configuration_tu15(transcript,0x38e8c9u,abi,{0x27f,0x1f80},services,observe);
    check(!level&&instructions==source.at("instructions")&&stores==source.at("stores")&&
        platform==source.at("platform_exclusion"),"Original configuration CPU control/exclusion order differs");
    check(abi.gp==source.at("returned_gp").get<decltype(abi.gp)>()&&
        !((abi.eflags^source.at("returned_eflags").get<unsigned>())&0xcc5u),"Original configuration CPU ABI differs");
    check(transcript.store==source.at("stores").size()&&transcript.read_index==source.at("hardware_reads").size(),
        "Configuration CPU omitted an original request/input");
    check(word(bank,0x391460u)==source.at("source_initialized")&&word(bank,0x38f848u)==source.at("source_count"),
        "Configuration CPU replaced source-derived cold/retained scheduler fields");
    reports.push_back({{"fill",fill},{"initialized",source.at("declared_initialized")},
        {"instructions",instructions.size()},{"ordered_stores",stores.size()},
        {"exact_external_inputs",transcript.read_index},{"native_policy_installed",false}});
}
}

int main(int argc,char** argv)try{
    check(argc==3,"Expected configuration CPU reference report and output report");
    std::ifstream stream(argv[1]);Json original;stream>>original;
    check(original.at("schema")=="H2_TU15_CONFIGURATION_REFERENCE_ONLY_V1"&&original.at("passed")==true&&
        original.at("instruction_patches").empty()&&original.at("supplied_sdk_or_game_results").empty(),
        "Configuration CPU reference contains supplied execution results");
    Json reports=Json::array();for(const auto& source:original.at("cases"))run(source,reports);
    {
        SourceRuntimeMemory bank;mount_native_tu15_sound_configuration_data(bank);
        bank.mount(0x2000000u,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,
            "Declared absent-owner caller stack; no hardware/control region");
        put(bank,stack,stop);SourcePhysicsWorldRuntimeAbi abi{};abi.gp[1]=apu+8;abi.gp[4]=stack;
        abi.eflags=0x202;const auto before=abi.gp;bool rejected=false;std::vector<std::uint32_t> sites;
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){sites.push_back(pc);};
        try{source_run_sound_configuration_tu15(bank,0x38e8c9u,abi,{0x27f,0x1f80},services);}
        catch(const std::exception&){rejected=true;}
        check(rejected&&abi.gp==before&&sites==std::vector<std::uint32_t>{0x38e8c9u}&&
            !word(bank,0x391460u)&&!word(bank,0x38f848u),
            "Absent native policy supplied a configured result or changed the original pending ABI");
    }
    Json report={{"schema","H2_NATIVE_TU15_CONFIGURATION_CPU_TRANSCRIPT_V1"},{"passed",true},{"cases",reports},
        {"scope","Pure unchanged CPU differential against exact declared offline external inputs. No runtime MMIO bank, native configuration owner, source call skip, ready result or processing graph."},
        {"absent_native_policy_rejected_at_first_control_write",true},
        {"native_configuration_owner_implemented",false},{"initializer45_completed",false},{"native_dsp_or_pcm_parity",false}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<"12 configuration CPU request transcripts matched; no native configuration admission\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
