#include "halo2/native_campaign_ai_path_distance_tu15_binding.hpp"
#include "halo2/native_campaign_ai_path_distance_tu15.hpp"
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
    bool prefix{};unsigned ordinal{};mutable unsigned read_attempts{};
    auto locate(std::uint32_t address,std::size_t width)const {
        auto found=banks.upper_bound(address);if(found==banks.begin())throw Missing{address,width};--found;
        if(std::uint64_t(address-found->first)+width>found->second.size())throw Missing{address,width};return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(prefix && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
        reads.push_back(Json::array({address,out.size()}));const auto occurrence=++occurrences[address];
        for(const auto &change:timeline)if(change.at("on_read").at("address")==address && change.at("on_read").at("occurrence")==occurrence){
            for(const auto &edit:change.at("writes")){const auto data=bytes(edit.at("bytes"));const auto target=edit.at("address").get<unsigned>();const auto region=locate(target,data.size());std::copy(data.begin(),data.end(),region->second.begin()+target-region->first);}applied.push_back(change);
        }
    }
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("Path distance attempted retained publication");}
    explicit Memory(const Json &row){
        for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));timeline=row.at("input_timeline");
        const auto sp=row.at("callee_entry").at("gp")[4].get<unsigned>();const auto frame=bytes(row.at("callee_entry").at("frame"));
        const auto found=locate(sp,frame.size());std::copy(frame.begin(),frame.end(),found->second.begin()+sp-found->first);
    }
};
SourcePhysicsWorldRuntimeAbi caller(const Json &row) {
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("callee_entry").at("gp").get<std::array<std::uint32_t,8>>();
    for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x6b000000u+row.at("id").get<unsigned>()*37u+n*4u+k;
    abi.eflags=0x2d7;abi.fs_base=0x514000u;abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}
    abi.x87_status=row.at("before_fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};return abi;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply real original path-distance transport fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Path-distance transport authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Path-distance transport supplied an outcome");
    std::size_t count{},pending_count{},reads{},faults{},late_returns{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>() && row.at("entry_live_x87_preserved").get<bool>() && !row.at("callee_entry").is_null(),name+": original caller packet unavailable");
        check(row.at("writes").empty(),name+": original path distance published retained storage");
        Memory memory(row);const auto fault=row.value("fault",Json::object());memory.prefix=fault.value("phase",std::string{})=="before_read";memory.ordinal=fault.value("ordinal",0u);
        auto abi=caller(row);const auto initial=abi;const auto activity=native_campaign_ai_path_distance_calls_tu15();
        const SourcePhysicsContactRuntimeCall call{row.at("call_site"),0x200c00u,false};
        const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        bool stopped{};std::optional<NativeCampaignAiPathDistancePendingTu15> pending;
        const auto missing=[&](const Missing &value){++faults;check(row.value("expected_fault",false),name+": unexpected missing storage");bool matched{};for(const auto &operand:row.at("boundary").at("operands"))matched|=operand.at("address")==value.address && operand.at("bytes")==value.width;check(matched,name+": source fault provenance differs");};
        try{native_campaign_ai_path_distance_tu15_run(memory,call,abi);}
        catch(const Prefix &){stopped=true;}
        catch(const Missing &value){stopped=true;missing(value);}
        catch(const NativeCampaignAiPathDistancePendingErrorTu15 &error){stopped=true;pending=error.pending();try{std::rethrow_exception(error.cause());}catch(const Prefix &){}catch(const Missing &value){missing(value);}}
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
        check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": transport host FP leaked");
        check(native_campaign_ai_path_distance_calls_tu15()==activity+1u,name+": transport activity differs");
        check(stopped==!row.at("completed").get<bool>(),name+": transport completion differs");
        check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] && abi.floating_point->mxcsr==row.at("after_fp")[2],name+": transport FP differs");
        const auto quantities=row.at("pending_extended").size();
        check(abi.x87_depth==initial.x87_depth+quantities,name+": transport pending depth differs");
        check(bool(pending)==bool(quantities),name+": transport pending result differs");
        for(unsigned n=0;n<quantities;++n)check(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10))==row.at("pending_extended")[n].get<std::string>(),name+": transport pending quantity differs");
        for(unsigned n=0;n<initial.x87_depth;++n)check(!std::memcmp(&abi.x87[n+quantities],&initial.x87[n],10),name+": transport older quantity changed");
        if(quantities)++pending_count;
        check(abi.gp[4]==initial.gp[4]+(stopped?0u:4u),name+": transport stack cleanup differs");
        check(abi.xmm[0]==row.at("boundary_xmm0").get<std::array<std::uint32_t,4>>(),name+": transport scalar publication differs");
        for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==initial.gp[n],name+": transport invented dead GP outputs");
        for(unsigned n=1;n<8;++n)check(abi.xmm[n]==initial.xmm[n],name+": transport invented dead SIMD outputs");
        check(abi.eflags==initial.eflags && abi.fs_base==initial.fs_base && !abi.active_mmx,name+": transport changed opaque caller state");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": transport retained bytes differ");
        check(memory.applied==row.at("applied_input_timeline"),name+": transport input timeline differs");
        Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
        if(memory.reads!=expected)std::cerr<<name<<" actual="<<memory.reads.dump()<<" expected="<<expected.dump()<<'\n';
        check(memory.reads==expected,name+": transport ordered reads differ");reads+=memory.reads.size();
        if(!row.at("boundary").is_null() && row.at("boundary").at("instruction")==0x200d2au)++late_returns;++count;
    }
    const auto &row=fixture.at("cases")[0];
    const SourcePhysicsContactRuntimeCall call{row.at("call_site"),0x200c00u,false};
    for(const auto &invalid:std::array{SourcePhysicsContactRuntimeCall{call.instruction+1u,call.target,false},SourcePhysicsContactRuntimeCall{call.instruction,call.target+1u,false},SourcePhysicsContactRuntimeCall{call.instruction,call.target,true}}) {
        Memory memory(row);auto abi=caller(row);const auto initial=abi;const auto activity=native_campaign_ai_path_distance_calls_tu15();bool rejected{};
        try{native_campaign_ai_path_distance_tu15_run(memory,invalid,abi);}catch(const std::invalid_argument &){rejected=true;}
        check(rejected && memory.reads.empty() && abi.gp==initial.gp && native_campaign_ai_path_distance_calls_tu15()==activity,"Transport admitted another call owner");
    }
    for(unsigned profile=0;profile<3;++profile){
        Memory memory(row);auto abi=caller(row);if(profile==0)abi.floating_point.reset();if(profile==1)abi.x87_depth=4;if(profile==2)abi.active_mmx=SourcePhysicsMmxAlias{};bool rejected{};
        try{native_campaign_ai_path_distance_tu15_run(memory,call,abi);}catch(const std::invalid_argument &){rejected=true;}
        check(rejected && memory.reads.empty(),"Transport admitted unavailable FP ownership");
    }
    {Memory memory(row);auto abi=caller(row);const auto sp=abi.gp[4];const auto at=memory.locate(sp,4);std::fill_n(at->second.begin()+sp-at->first,4,std::byte{0x5b});bool rejected{};
     try{native_campaign_ai_path_distance_tu15_run(memory,call,abi);}catch(const std::runtime_error &error){rejected=std::string_view(error.what()).find("actual return")!=std::string_view::npos;}
     check(rejected && abi.gp[4]==sp && abi.xmm[0]==row.at("boundary_xmm0").get<std::array<std::uint32_t,4>>(),"Transport did not reject the actual return after completing its result");}
    check(count && pending_count && faults==10 && late_returns==14,"Transport qualification matrix incomplete");
    std::cout<<count<<" original caller transport cases; "<<reads<<" reads; "<<faults<<" true faults; "<<pending_count<<" pending results; "<<late_returns<<" late return cuts passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
