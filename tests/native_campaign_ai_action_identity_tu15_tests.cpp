#include "halo2/native_campaign_ai_action_identity_tu15.hpp"
#include "halo2/native_tu15_data_array_lookup.hpp"
#include <algorithm>
#include <array>
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
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t width)const {
        auto found=banks.upper_bound(address);if(found==banks.begin())throw Missing{address,width};--found;
        if(std::uint64_t(address-found->first)+width>found->second.size())throw Missing{address,width};return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
        reads.push_back(Json::array({address,out.size()}));const auto occurrence=++occurrences[address];
        for(const auto &change:timeline)if(change.at("on_read").at("address")==address && change.at("on_read").at("occurrence")==occurrence){
            for(const auto &edit:change.at("writes")){const auto data=bytes(edit.at("bytes"));const auto target=edit.at("address").get<unsigned>();const auto region=locate(target,data.size());std::copy(data.begin(),data.end(),region->second.begin()+target-region->first);}applied.push_back(change);
        }
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        if(phase=="before_write" && write_attempts==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());
        stores.push_back({0,address,{found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),found->second.begin()+address-found->first);
        if(phase=="after_write" && write_attempts++==ordinal)throw Prefix{};
        if(phase!="after_write")++write_attempts;
    }
    explicit Memory(const Json &row){for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));timeline=row.at("input_timeline");}
};
std::optional<std::uint32_t> invoke(Memory &memory,const Json &row,const SourcePhysicsContactRecordObserver &observer={}) {
    const auto kind=row.at("kind").get<std::string>();
    if(kind=="history"){native_campaign_ai_action_history_tu15(memory,row.at("actor"),row.at("goal"),row.at("flag"),observer);return {};}
    if(kind=="goal")return native_campaign_ai_goal_definition_tu15(memory,row.at("goal"));
    if(kind=="dynamic")return native_campaign_ai_dynamic_goal_definition_tu15(memory,row.at("goal"));
    check(kind=="lookup","Unknown original identity owner");
    return native_tu15_data_array_try_lookup(memory,row.at("pool"),row.at("datum"));
}
void compare_stores(const std::vector<SourcePhysicsContactRecordStore> &events,const Json &row,bool sites) {
    const auto name=row.at("name").get<std::string>();check(events.size()==row.at("writes").size(),name+": store count differs");
    for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];check((!sites||a.instruction==b.at("instruction")) && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": ordered store differs");}
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original action-identity fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Action-identity original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Action-identity reference supplied an outcome");
    std::size_t count{},reads{},stores{},faults{},prefixes{},observer_cases{},timelines{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>() && row.at("entry_live_x87_preserved").get<bool>(),name+": original live state unqualified");
        check(row.at("before_fp")==row.at("after_fp") && row.at("pending_extended").empty(),name+": integer original changed FP");
        Memory memory(row);const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        bool stopped{};std::optional<std::uint32_t> result;
        try {result=invoke(memory,row);}
        catch(const Prefix &){stopped=true;++prefixes;}
        catch(const Missing &value){stopped=true;++faults;check(row.value("expected_fault",false),name+": unexpected missing storage");bool matched{};for(const auto &operand:row.at("boundary").at("operands"))matched|=operand.at("address")==value.address && operand.at("bytes")==value.width;check(matched,name+": source fault provenance differs");}
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
        check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
        check(stopped==!row.at("completed").get<bool>(),name+": completion differs");
        if(result)check(*result==row.at("boundary_gp")[0].get<unsigned>(),name+": returned identity differs");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained bytes differ");
        check(memory.applied==row.at("applied_input_timeline"),name+": input timeline differs");timelines+=memory.applied.size();
        Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
        if(memory.reads!=expected)std::cerr<<name<<" actual="<<memory.reads.dump()<<" expected="<<expected.dump()<<'\n';
        check(memory.reads==expected,name+": ordered reads differ");compare_stores(memory.stores,row,false);
        reads+=memory.reads.size();stores+=memory.stores.size();
        // Observer instrumentation is checked separately so it never adds a
        // retained read to a source timeline or a rejection boundary.
        if(row.at("completed").get<bool>() && row.at("input_timeline").empty()) {
            Memory observed(row);std::vector<SourcePhysicsContactRecordStore> events;
            invoke(observed,row,[&](const auto &event){events.push_back(event);});compare_stores(events,row,true);++observer_cases;
        }
        ++count;
    }
    check(count && faults==5 && prefixes && timelines && observer_cases,"Action-identity qualification matrix incomplete");
    std::cout<<count<<" original action-identity cases; "<<reads<<" reads; "<<stores<<" stores; "<<faults<<" real faults; "<<prefixes<<" rejected prefixes; "<<timelines<<" input timeline edits; "<<observer_cases<<" observer comparisons passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
