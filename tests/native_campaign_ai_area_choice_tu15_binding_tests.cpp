#include "halo2/native_campaign_ai_area_choice_tu15.hpp"
#include "halo2/native_campaign_ai_area_choice_tu15_binding.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto value:data){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    Json timeline=Json::array();mutable Json applied=Json::array(),reads=Json::array();
    mutable std::map<std::uint32_t,unsigned> occurrences;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    bool recording{},raw_stores{},reject_writes{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t width)const {
        auto found=banks.upper_bound(address);if(found==banks.begin())throw Missing{address,width};--found;
        if(std::uint64_t(address-found->first)+width>found->second.size())throw Missing{address,width};return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
        if(recording)reads.push_back(Json::array({address,out.size()}));const auto occurrence=++occurrences[address];
        for(const auto &change:timeline)if(change.at("on_read").at("address")==address && change.at("on_read").at("occurrence")==occurrence){
            for(const auto &edit:change.at("writes")){const auto data=bytes(edit.at("bytes"));const auto target=edit.at("address").get<unsigned>();const auto region=locate(target,data.size());std::copy(data.begin(),data.end(),region->second.begin()+target-region->first);}applied.push_back(change);
        }
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        check(!reject_writes,"Private area choice attempted engine publication");
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());
        if(raw_stores)stores.push_back({0,address,{found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),found->second.begin()+address-found->first);
    }
    explicit Memory(const Json &row){for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));timeline=row.at("input_timeline");}
};
}
namespace {
SourcePhysicsWorldRuntimeAbi caller(const Json &row) {
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("callee_entry").at("gp").get<std::array<std::uint32_t,8>>();
    for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x7b000000u+n*31u+k;
    abi.eflags=0x2d7;abi.fs_base=0x514000u;abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}
    abi.x87_status=row.at("before_fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};return abi;
}
void frame(Memory &memory,const Json &row) {
    const auto sp=row.at("callee_entry").at("gp")[4].get<unsigned>();
    const auto values=bytes(row.at("callee_entry").at("frame"));
    auto found=memory.locate(sp,values.size());std::copy(values.begin(),values.end(),found->second.begin()+sp-found->first);
}
constexpr SourcePhysicsContactRuntimeCall call{0x1b0d1cu,0x1bbec0u,false};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply real original area-choice transport fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Area-choice transport authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Area-choice transport supplied an outcome");
    std::size_t count{},pending_count{},reads{},stores{},late_returns{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>() && row.at("entry_live_x87_preserved").get<bool>() && !row.at("callee_entry").is_null(),name+": original caller packet unavailable");
        const auto fault=row.value("fault",Json::object());
        for(const bool reading:std::array{false,true}) {
            if(reading && fault.value("phase",std::string{})=="after_write")continue;
            Memory memory(row);frame(memory,row);memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);memory.recording=reading;
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            SourcePhysicsContactRecordObserver observer;std::vector<SourcePhysicsContactRecordStore> events;unsigned publications{};
            if(!reading && !memory.raw_stores)observer=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write" && publications++==memory.ordinal)throw Prefix{};};
            auto abi=caller(row);const auto initial=abi;const auto activity=native_campaign_ai_area_choice_calls_tu15();
            const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool stopped{};std::optional<NativeCampaignAiAreaScorePendingTu15> pending;
            try{native_campaign_ai_area_choice_tu15_run(memory,call,abi,observer);}
            catch(const Prefix &){stopped=true;}
            catch(const NativeCampaignAiAreaScorePendingErrorTu15 &error){stopped=true;pending=error.pending();try{std::rethrow_exception(error.cause());}catch(const Prefix &){}}
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": transport host FP leaked");
            check(native_campaign_ai_area_choice_calls_tu15()==activity+1u,name+": transport activity differs");
            check(stopped==!row.at("completed").get<bool>(),name+": transport completion differs");
            check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] && abi.floating_point->mxcsr==row.at("after_fp")[2],name+": transport FP differs");
            const auto quantities=row.at("pending_extended").size();
            check(abi.x87_depth==initial.x87_depth+quantities,name+": transport pending depth differs");
            check(bool(pending)==bool(quantities),name+": transport pending result differs");
            for(unsigned n=0;n<quantities;++n)check(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10))==row.at("pending_extended")[n].get<std::string>(),name+": transport pending quantity differs");
            for(unsigned n=0;n<initial.x87_depth;++n)check(!std::memcmp(&abi.x87[n+quantities],&initial.x87[n],10),name+": transport older quantity changed");
            if(quantities)++pending_count;
            check(abi.gp[4]==initial.gp[4]+(stopped?0u:12u),name+": transport stack cleanup differs");
            const auto at_return=!row.at("boundary").is_null() && (row.at("boundary").at("instruction")==0x1bbe21u || row.at("boundary").at("instruction")==0x1bbe59u || row.at("boundary").at("instruction")==0x1bbeb2u);
            if(!stopped || at_return)check((abi.gp[0]&255u)==(row.at("boundary_gp")[0].get<unsigned>()&255u),name+": transport AL result differs");
            if(at_return)++late_returns;
            check((abi.gp[0]&0xffffff00u)==(initial.gp[0]&0xffffff00u),name+": transport invented dead EAX bits");
            for(unsigned n=1;n<8;++n)if(n!=4)check(abi.gp[n]==initial.gp[n],name+": transport changed caller-owned GP");
            check(abi.xmm==initial.xmm && abi.eflags==initial.eflags && abi.fs_base==initial.fs_base && !abi.active_mmx,name+": transport changed opaque caller state");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": transport retained bytes differ");
            check(memory.applied==row.at("applied_input_timeline"),name+": transport input timeline differs");
            if(reading){Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));if(memory.reads!=expected)std::cerr<<name<<" actual="<<memory.reads.dump()<<" expected="<<expected.dump()<<'\n';check(memory.reads==expected,name+": transport ordered reads differ");reads+=memory.reads.size();}
            else {if(memory.raw_stores)events=memory.stores;check(events.size()==row.at("writes").size(),name+": transport store count differs");for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];check((memory.raw_stores||a.instruction==b.at("instruction")) && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": transport ordered store differs");}stores+=events.size();}
        }
        ++count;
    }
    const auto &row=fixture.at("cases")[0];
    for(const auto &invalid:std::array{SourcePhysicsContactRuntimeCall{call.instruction+1u,call.target,false},SourcePhysicsContactRuntimeCall{call.instruction,call.target+1u,false},SourcePhysicsContactRuntimeCall{call.instruction,call.target,true}}) {
        Memory memory(row);frame(memory,row);memory.recording=true;auto abi=caller(row);const auto initial=abi;const auto activity=native_campaign_ai_area_choice_calls_tu15();bool rejected{};
        try{native_campaign_ai_area_choice_tu15_run(memory,invalid,abi);}catch(const std::invalid_argument &){rejected=true;}
        check(rejected && memory.reads.empty() && memory.stores.empty() && abi.gp==initial.gp && native_campaign_ai_area_choice_calls_tu15()==activity,"Transport admitted another call owner");
    }
    for(unsigned profile=0;profile<3;++profile){
        Memory memory(row);frame(memory,row);memory.recording=true;auto abi=caller(row);if(profile==0)abi.floating_point.reset();if(profile==1)abi.x87_depth=7;if(profile==2)abi.active_mmx=SourcePhysicsMmxAlias{};bool rejected{};
        try{native_campaign_ai_area_choice_tu15_run(memory,call,abi);}catch(const std::invalid_argument &){rejected=true;}
        check(rejected && memory.reads.empty(),"Transport admitted unavailable FP ownership");
    }
    {Memory memory(row);frame(memory,row);auto abi=caller(row);const auto sp=abi.gp[4];const auto at=memory.locate(sp,4);std::fill_n(at->second.begin()+sp-at->first,4,std::byte{0x5b});bool rejected{};
     try{native_campaign_ai_area_choice_tu15_run(memory,call,abi);}catch(const std::runtime_error &error){rejected=std::string_view(error.what()).find("actual1B0D1E return")!=std::string_view::npos;}
     check(rejected && abi.gp[4]==sp && (abi.gp[0]&255u)==(row.at("boundary_gp")[0].get<unsigned>()&255u),"Transport did not reject the actual return after completing its result");}
    check(count && pending_count && late_returns,"Transport qualification matrix incomplete");
    std::cout<<count<<" original caller transport cases; "<<reads<<" reads; "<<stores<<" stores; "<<pending_count<<" pending results; "<<late_returns<<" late return cuts passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
