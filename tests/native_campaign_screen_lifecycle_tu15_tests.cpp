#include "halo2/native_campaign_screen_lifecycle_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool value,const std::string &message) {
    if(!value)throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string &text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)
        out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> data) {
    constexpr char digits[]="0123456789abcdef";
    std::string out;
    for(const auto value:data) {
        const auto n=std::to_integer<unsigned>(value);
        out+=digits[n>>4];out+=digits[n&15];
    }
    return out;
}
struct Prefix {};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;
    unsigned ordinal{};
    mutable unsigned read_attempts{};
    unsigned write_attempts{};
    bool log_reads{},log_writes{};
    mutable Json reads=Json::array();
    std::vector<SourcePhysicsContactRecordStore> writes;
    auto locate(std::uint32_t at,std::size_t size)const {
        auto found=banks.upper_bound(at);
        check(found!=banks.begin(),"Missing cinematic lifecycle storage");--found;
        check(std::uint64_t(at-found->first)+size<=found->second.size(),
              "Cinematic lifecycle storage extent differs");
        return found;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(at,out.size());
        if(log_reads)reads.push_back(Json::array({at,out.size()}));
        std::copy_n(found->second.begin()+at-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> input)override {
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(at,input.size());
        if(log_writes)writes.push_back({0,at,
            {found->second.begin()+at-found->first,found->second.begin()+at-found->first+input.size()},
            {input.begin(),input.end()}});
        std::copy(input.begin(),input.end(),banks.at(found->first).begin()+at-found->first);
    }
};
} // namespace

int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original cinematic lifecycle fixture");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Different cinematic lifecycle original image");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),
          "Cinematic reference supplied instructions or results");
    std::size_t count{},read_count{},store_count{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        for(bool reading:{false,true}) {
            Memory memory;
            for(const auto &bank:row.at("banks"))
                memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            const auto fault=row.value("fault",Json::object());
            memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading && memory.phase=="after_write")continue;
            memory.log_reads=reading;
            // A raw recording avoids adding observer reads to fault-prefix
            // runs; the separate observed path compares original store sites.
            memory.log_writes=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            std::vector<SourcePhysicsContactRecordStore> stores;
            SourcePhysicsContactRecordObserver observer;
            unsigned observed{};
            if(!reading && !memory.log_writes)observer=[&](const auto &event) {
                stores.push_back(event);
                if(memory.phase=="after_write" && observed++==memory.ordinal)throw Prefix{};
            };
            const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(control),"=m"(status));
            bool stopped{};
            try {
                if(row.at("entry")==0x1fcb0u)native_campaign_screen_stop_tu15(memory,observer);
                else {check(row.at("entry")==0x1fce0u,"Unexpected cinematic lifecycle entry");
                    native_campaign_screen_capture_complete_tu15(memory,observer);}
            }catch(const Prefix &){stopped=true;}
            std::uint16_t after_control{},after_status{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_control),"=m"(after_status));
            check(control==after_control && status==after_status && mxcsr==_mm_getcsr(),name+": native FP state changed");
            check(stopped==!row.at("completed").get<bool>(),name+": original completion differs");
            if(reading) {
                Json expected=Json::array();
                for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                check(memory.reads==expected,name+": original ordered reads differ");read_count+=memory.reads.size();
            }else {
                if(memory.log_writes)stores=memory.writes;
                check(stores.size()==row.at("writes").size(),name+": original store count differs");
                for(std::size_t n=0;n<stores.size();++n) {
                    const auto &actual=stores[n];const auto &expected=row.at("writes")[n];
                    check((memory.log_writes || actual.instruction==expected.at("instruction")) &&
                          actual.address==expected.at("address") && hex(actual.before)==expected.at("before").get<std::string>() &&
                          hex(actual.written)==expected.at("written").get<std::string>(),name+": original ordered store differs");
                }
                store_count+=stores.size();
            }
            for(const auto &bank:row.at("banks"))
                check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": original retained bank differs");
        }
        ++count;
    }
    std::cout<<count<<" original cinematic lifecycle calls/prefixes, "<<read_count
             <<" ordered reads, "<<store_count<<" ordered stores passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
