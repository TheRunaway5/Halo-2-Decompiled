#include "halo2/native_campaign_ai_path_result_tu15.hpp"
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
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original path-result fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Path-result original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Path-result reference supplied an outcome");
    std::size_t count{},pending_count{},reads{},stores{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>() && row.at("entry_live_x87_preserved").get<bool>(),name+": original live state unqualified");
        const auto fault=row.value("fault",Json::object());
        for(const bool reading:std::array{false,true}) {
            if(reading && fault.value("phase",std::string{})=="after_write")continue;
            Memory memory(row);memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);memory.recording=reading;
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            SourcePhysicsContactRecordObserver observer;std::vector<SourcePhysicsContactRecordStore> events;unsigned publications{};
            if(!reading && !memory.raw_stores)observer=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write" && publications++==memory.ordinal)throw Prefix{};};
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool stopped{};std::optional<bool> result;std::optional<NativeCampaignAiPathResultPendingTu15> pending;
            const auto missing=[&](const Missing &value){check(row.value("expected_fault",false),name+": unexpected missing storage");bool matched{};for(const auto &operand:row.at("boundary").at("operands"))matched|=operand.at("address")==value.address && operand.at("bytes")==value.width;check(matched,name+": source fault provenance differs");};
            try {
                result=native_campaign_ai_path_result_tu15(memory,row.at("workspace"),row.at("key"),row.at("goal"),
                    row.at("cost"),row.at("clearance"),row.at("direction"),fp,row.at("live_x87").size(),observer);
            }catch(const Prefix &){stopped=true;}
            catch(const Missing &value){stopped=true;missing(value);}
            catch(const NativeCampaignAiPathResultPendingErrorTu15 &error){stopped=true;pending=error.pending();try{std::rethrow_exception(error.cause());}catch(const Prefix &){}catch(const Missing &value){missing(value);}}
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": completion differs");
            check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],name+": FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            const auto at=row.at("boundary").is_null()?0u:row.at("boundary").at("instruction").get<unsigned>();
            const bool own_pending=!row.at("pending_extended").empty() && at>=0x27a460u && at<0x27a640u;
            check(bool(pending)==own_pending,name+": own pending quantity presence differs");
            if(pending){
                check(pending->count==row.at("pending_extended").size(),name+": pending quantity count differs");
                for(unsigned n=0;n<pending->count;++n)check(hex(pending->quantities[n])==row.at("pending_extended")[n].get<std::string>(),name+": pending extended bits differ");
                const auto site=row.at("boundary").at("instruction").get<unsigned>();using Stage=NativeCampaignAiPathResultPendingTu15::Stage;
                const auto stage=site==0x27a4d3u?Stage::ClosestX:site==0x27a4d9u?Stage::ClosestYAndX:site==0x27a4e0u?Stage::ClosestZAndYX:Stage::Distance;
                check(pending->stage==stage,name+": pending mathematical meaning differs");++pending_count;
            }
            if(result)check(unsigned(*result)==(row.at("boundary_gp")[0].get<unsigned>()&255u),name+": AL result differs");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained bytes differ");
            check(memory.applied==row.at("applied_input_timeline"),name+": input timeline differs");
            if(reading){Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));if(memory.reads!=expected)std::cerr<<name<<" actual="<<memory.reads.dump()<<" expected="<<expected.dump()<<'\n';check(memory.reads==expected,name+": ordered reads differ");reads+=memory.reads.size();}
            else {if(memory.raw_stores)events=memory.stores;check(events.size()==row.at("writes").size(),name+": store count differs");for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];check((memory.raw_stores||a.instruction==b.at("instruction")) && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": ordered store differs");}stores+=events.size();}
        }
        ++count;
    }
    check(count && pending_count,"Path-result qualification matrix incomplete");
    std::cout<<count<<" original path-result cases; "<<reads<<" reads; "<<stores<<" stores; "<<pending_count<<" pending quantity comparisons passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
