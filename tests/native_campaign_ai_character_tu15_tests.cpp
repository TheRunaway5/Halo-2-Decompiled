#include "halo2/native_campaign_ai_character_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
struct Prefix {};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::optional<unsigned> fault;mutable unsigned attempts{};mutable Json reads=Json::array();
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(fault && attempts++==*fault)throw Prefix{};
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Missing character storage");--found;
        check(std::uint64_t(address-found->first)+out.size()<=found->second.size(),"Character storage extent differs");
        reads.push_back(Json::array({address,out.size()}));
        std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Read-only character lookup wrote source storage");}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original character fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Character original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() &&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Character reference lacks unchanged physical authority");
    std::size_t count{},read_count{},prefixes{},negative_variants{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>() && row.at("live_x87_unchanged").get<bool>(),name+": original physical evidence missing");
        check(row.at("writes").empty() && row.at("before_fp")==row.at("after_fp"),name+": original character purity differs");
        Memory memory;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        if(row.contains("fault"))memory.fault=row.at("fault").at("ordinal");
        const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
        bool stopped{};std::optional<std::uint32_t> result;
        try {
            switch(row.at("entry").get<unsigned>()) {
            case 0x1ecd00:result=native_campaign_ai_character_navigation_tu15(memory,row.at("character"));break;
            case 0x1ed700:result=native_campaign_ai_character_vehicle_navigation_tu15(memory,row.at("actor"),row.at("vehicle_character"));break;
            case 0x27c6c0:result=native_campaign_ai_order_character_tu15(memory,row.at("order_address"));break;
            case 0x27c710:result=native_campaign_ai_actor_order_character_tu15(memory,row.at("actor"));break;
            case 0x1ed100:result=native_campaign_ai_character_movement_tu15(memory,row.at("actor"));break;
            default:throw std::runtime_error("Unknown native character owner");
            }
        }catch(const Prefix &){stopped=true;}
        std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
        check(actual_cw==host_cw && actual_sw==host_sw && _mm_getcsr()==host_mxcsr,name+": character host FP leaked");
        check(stopped==!row.at("completed").get<bool>(),name+": character completion differs");
        if(result) {
            check(*result==row.at("boundary_gp")[0].get<std::uint32_t>(),name+": character block identity differs");
            negative_variants+=row.at("entry")==0x1ed100 && *result && *result<0x100a000;
        }
        Json expected=Json::array();
        for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
        check(memory.reads==expected,name+": character ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());read_count+=memory.reads.size();
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained character bytes differ");
        prefixes+=stopped;++count;
    }
    check(count && prefixes && negative_variants,"Character qualification matrix incomplete");
    std::cout<<count<<" physical original character cases, "<<read_count<<" ordered reads, "<<prefixes<<" prefixes and "<<negative_variants<<" negative movement indices; inherited identity/FP/no-write checks passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
