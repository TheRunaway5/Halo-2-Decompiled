#include "halo2/native_tu15_data_array_allocate.hpp"
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
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> data) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto value:data){auto n=std::to_integer<unsigned>(value);out+=digits[n>>4];out+=digits[n&15];}
    return out;
}
struct Prefix {};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};
    mutable unsigned read_attempts{};unsigned write_attempts{};
    mutable Json reads=Json::array();Json writes=Json::array();
    auto locate(std::uint32_t at,std::size_t size)const {
        auto found=banks.upper_bound(at);check(found!=banks.begin(),"Missing allocation bank");--found;
        check(std::uint64_t(at-found->first)+size<=found->second.size(),"Allocation bank extent differs");return found;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(at,out.size());reads.push_back(Json::array({at,out.size()}));
        std::copy_n(found->second.begin()+at-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> input)override {
        if(phase=="before_write" && write_attempts==ordinal)throw Prefix{};
        const auto found=locate(at,input.size());
        const auto previous=std::span(found->second).subspan(at-found->first,input.size());
        writes.push_back(Json::object({{"address",at},{"before",hex(previous)},{"written",hex(input)}}));
        std::copy(input.begin(),input.end(),banks.at(found->first).begin()+at-found->first);
        if(phase=="after_write" && write_attempts++==ordinal)throw Prefix{};
        if(phase!="after_write")++write_attempts;
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independently captured original allocation fixture");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Different original allocation image");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Allocation fixture supplies original instructions/results");
    unsigned count{},read_count{},store_count{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>(),"Allocation input lacks physical evidence");
        Memory memory;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(control),"=m"(status));
        bool stopped{};std::uint32_t value{};
        try{value=native_tu15_data_array_allocate(memory,row.at("gp").at(2));}catch(const Prefix &){stopped=true;}
        std::uint16_t after_control{},after_status{};
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_control),"=m"(after_status));
        check(control==after_control && status==after_status && mxcsr==_mm_getcsr(),name+": allocation changed host FP");
        check(stopped==!row.at("completed").get<bool>(),name+": original allocation completion differs");
        if(!stopped) {
            check(value==row.at("boundary_gp").at(0),name+": original allocation datum differs");
            check(row.at("boundary_gp").at(4).get<std::uint32_t>()==row.at("gp").at(4).get<std::uint32_t>()+4u,name+": original RET contract differs");
            for(auto reg:{3u,5u,6u,7u})check(row.at("gp").at(reg)==row.at("boundary_gp").at(reg),name+": original preserved register contract differs");
        }
        // A source read-modify-write instruction cannot stop after its read
        // but before its write. Compare reads at true read/committed boundaries.
        if(memory.phase!="before_write") {
            Json expected=Json::array();
            for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
            if(memory.reads!=expected)throw std::runtime_error(name+": original allocation read order differs; actual="+memory.reads.dump()+" expected="+expected.dump());
        }
        Json expected=Json::array();
        for(const auto &event:row.at("writes"))expected.push_back(Json::object({{"address",event.at("address")},{"before",event.at("before")},{"written",event.at("written")}}));
        check(memory.writes==expected,name+": original allocation store order differs");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": original allocation retained bytes differ");
        check(row.at("live_x87_unchanged").get<bool>() && row.at("before_fp").at(2)==row.at("after_fp").at(2),name+": original FP contract differs");
        ++count;read_count+=memory.reads.size();store_count+=memory.writes.size();
    }
    std::cout<<count<<" original allocation comparisons, "<<read_count<<" reads, "<<store_count<<" stores\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
