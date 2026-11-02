#include "halo2/native_campaign_ai_path_distance_tu15.hpp"
#include <algorithm>
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
    explicit Memory(const Json &row){for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));timeline=row.at("input_timeline");}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original path-distance fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Path-distance original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Path-distance reference supplied an outcome");
    std::size_t count{},pending_count{},reads{},faults{},prefixes{},timelines{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>() && row.at("entry_live_x87_preserved").get<bool>(),name+": original live state unqualified");
        check(row.at("writes").empty(),name+": original path distance published retained storage");
        Memory memory(row);const auto fault=row.value("fault",Json::object());memory.prefix=fault.value("phase",std::string{})=="before_read";memory.ordinal=fault.value("ordinal",0u);
        NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
        const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        bool stopped{};std::optional<std::uint32_t> result;std::optional<NativeCampaignAiPathDistancePendingTu15> pending;
        const auto missing=[&](const Missing &value){++faults;check(row.value("expected_fault",false),name+": unexpected missing storage");bool matched{};for(const auto &operand:row.at("boundary").at("operands"))matched|=operand.at("address")==value.address && operand.at("bytes")==value.width;check(matched,name+": source fault provenance differs");};
        try {result=native_campaign_ai_path_distance_tu15(memory,row.at("actor"),fp,row.at("live_x87").size());}
        catch(const Prefix &){stopped=true;++prefixes;}
        catch(const Missing &value){stopped=true;missing(value);}
        catch(const NativeCampaignAiPathDistancePendingErrorTu15 &error){stopped=true;pending=error.pending();try{std::rethrow_exception(error.cause());}catch(const Prefix &){++prefixes;}catch(const Missing &value){missing(value);}}
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
        check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
        check(stopped==!row.at("completed").get<bool>(),name+": completion differs");
        check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],name+": FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
        check(bool(pending)==!row.at("pending_extended").empty(),name+": pending quantity presence differs");
        if(pending){check(row.at("boundary").at("instruction")==0x200cdfu && row.at("pending_extended").size()==3,name+": pending mathematical meaning differs");for(unsigned n=0;n<3;++n)check(hex(pending->previous_z_dy_dx[n])==row.at("pending_extended")[n].get<std::string>(),name+": pending extended bits differ");++pending_count;}
        if(result)check(*result==row.at("result_bits").get<unsigned>(),name+": distance result differs");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained bytes differ");
        check(memory.applied==row.at("applied_input_timeline"),name+": input timeline differs");timelines+=memory.applied.size();
        Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
        if(memory.reads!=expected)std::cerr<<name<<" actual="<<memory.reads.dump()<<" expected="<<expected.dump()<<'\n';
        check(memory.reads==expected,name+": ordered reads differ");reads+=memory.reads.size();++count;
    }
    check(count && faults==5 && prefixes && timelines && pending_count,"Path-distance qualification matrix incomplete");
    std::cout<<count<<" original path-distance cases; "<<reads<<" reads; "<<faults<<" real faults; "<<prefixes<<" rejected prefixes; "<<pending_count<<" pending quantity comparisons; "<<timelines<<" input timeline edits passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
