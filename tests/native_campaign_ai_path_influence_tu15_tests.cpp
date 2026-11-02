#include "halo2/native_campaign_ai_path_influence_tu15.hpp"
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
        check(!reject_writes,"Private influence attempted engine publication");
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());
        if(raw_stores)stores.push_back({0,address,{found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),found->second.begin()+address-found->first);
    }
    explicit Memory(const Json &row){for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));timeline=row.at("input_timeline");}
};
Json engine_reads(const Json &row){Json result=Json::array();const auto argument=row.at("gp")[4].get<unsigned>()+4u;for(const auto &event:row.at("reads"))if(event.at("address")!=argument)result.push_back(Json::array({event.at("address"),event.at("bytes")}));return result;}
NativeTu15PhysicsFrameFp policy(const Json &row){return {row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};}
std::uint32_t original_distance(const Json &row){const auto output=row.at("destination").get<unsigned>();for(const auto &bank:row.at("banks")){const auto base=bank.at("address").get<unsigned>();const auto data=bytes(bank.at("after"));if(output>=base && std::uint64_t(output-base)+4<=data.size()){std::uint32_t result{};for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(data[output-base+n])<<(8*n);return result;}}throw std::runtime_error("Original influence has no retained distance");}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original influence fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Influence original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Influence reference supplied an outcome");
    std::size_t count{},pending_count{},private_count{},reads{},stores{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>() && row.at("entry_live_x87_preserved").get<bool>(),name+": original live state unqualified");
        const auto fault=row.value("fault",Json::object());
        for(const bool reading:std::array{false,true}) {
            if(reading && fault.value("phase",std::string{})=="after_write")continue;
            Memory memory(row);memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);memory.recording=reading;
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            SourcePhysicsContactRecordObserver observer;std::vector<SourcePhysicsContactRecordStore> events;unsigned publications{};
            if(!reading && !memory.raw_stores)observer=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write" && publications++==memory.ordinal)throw Prefix{};};
            auto fp=policy(row);const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool stopped{};std::optional<std::uint32_t> result;std::optional<NativeCampaignAiPathInfluenceQuantityTu15> pending;
            const auto missing=[&](const Missing &value){check(row.value("expected_fault",false),name+": unexpected influence missing storage");bool matched{};for(const auto &operand:row.at("boundary").at("operands"))matched|=operand.at("address")==value.address && operand.at("bytes")==value.width;check(matched,name+": influence fault provenance differs");};
            try {
                result=native_campaign_ai_path_influence_tu15(memory,row.at("origin"),row.at("endpoint"),row.at("influence"),row.at("destination"),fp,row.at("live_x87").size(),observer);
            }catch(const Prefix &){stopped=true;}
            catch(const Missing &value){stopped=true;missing(value);}
            catch(const NativeCampaignAiPathInfluencePendingErrorTu15 &error){stopped=true;pending=error.quantity();try{std::rethrow_exception(error.cause());}catch(const Prefix &){}catch(const Missing &value){missing(value);}}
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": influence host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": influence completion differs");
            check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],name+": influence FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            check(bool(pending)==!row.at("pending_extended").empty(),name+": influence pending quantity presence differs");
            if(pending){check(hex(pending->extended)==row.at("pending_extended")[0].get<std::string>(),name+": influence pending extended bits differ");const auto site=row.at("boundary").at("instruction").get<unsigned>();const auto kind=site==0x27c689u?NativeCampaignAiPathInfluenceQuantityTu15::Kind::DistanceRoot:site==0x27c68cu?NativeCampaignAiPathInfluenceQuantityTu15::Kind::DistanceRatio:NativeCampaignAiPathInfluenceQuantityTu15::Kind::NormalizedInfluence;check(pending->kind==kind,name+": influence pending meaning differs");++pending_count;}
            if(result)check(*result==row.at("result_bits"),name+": influence result bits differ");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": influence retained bytes differ");
            check(memory.applied==row.at("applied_input_timeline"),name+": influence input timeline differs");
            if(reading){const auto expected=engine_reads(row);if(memory.reads!=expected)std::cerr<<name<<" actual="<<memory.reads.dump()<<" expected="<<expected.dump()<<'\n';check(memory.reads==expected,name+": influence ordered reads differ");reads+=memory.reads.size();}
            else {if(memory.raw_stores)events=memory.stores;check(events.size()==row.at("writes").size(),name+": influence store count differs");for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];check((memory.raw_stores||a.instruction==b.at("instruction")) && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": influence ordered store differs");}stores+=events.size();}
        }
        if(row.at("completed").get<bool>()) {
            const auto output=row.at("destination").get<unsigned>();const auto expected=engine_reads(row);
            const bool disjoint=std::all_of(expected.begin(),expected.end(),[&](const auto &event){const auto address=event[0].template get<unsigned>(),width=event[1].template get<unsigned>();return std::uint64_t(address)+width<=output || address>=std::uint64_t(output)+4;});
            if(disjoint) {
                Memory memory(row);memory.recording=true;memory.reject_writes=true;auto fp=policy(row);
                const auto result=native_campaign_ai_path_influence_value_tu15(memory,row.at("origin"),row.at("endpoint"),row.at("influence"),fp,row.at("live_x87").size());
                check(result.influence==row.at("result_bits") && result.distance==original_distance(row),name+": private influence result differs");
                check(memory.reads==expected && memory.applied==row.at("applied_input_timeline"),name+": private influence retained reads differ");
                check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],name+": private influence FP differs");
                for(const auto &bank:row.at("banks")){auto wanted=bytes(bank.at("after"));const auto base=bank.at("address").get<unsigned>();if(output>=base && std::uint64_t(output-base)+4<=wanted.size()){const auto initial=bytes(bank.at("before"));std::copy_n(initial.begin()+output-base,4,wanted.begin()+output-base);}check(memory.banks.at(base)==wanted,name+": private influence published engine bytes");}
                ++private_count;
            }
        }
        ++count;
    }
    check(count && pending_count && private_count,"Influence qualification matrix incomplete");
    std::cout<<count<<" original influence cases; "<<reads<<" reads; "<<stores<<" stores; "<<pending_count<<" pending quantity comparisons; "<<private_count<<" private outputs passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
