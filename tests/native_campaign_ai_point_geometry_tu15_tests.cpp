#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
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
std::string hex(std::span<const std::byte> values) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto value:values){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;
}
struct Prefix {};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    bool recording{},raw_stores{};mutable Json reads=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Missing point storage");--found;
        check(std::uint64_t(address-found->first)+size<=found->second.size(),"Point storage extent differs");return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());
        if(recording)reads.push_back(Json::array({address,out.size()}));
        std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());
        if(raw_stores)stores.push_back({0,address,
            {found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},
            {values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),banks.at(found->first).begin()+address-found->first);
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original point fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Point original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() &&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Point reference lacks unchanged physical authority");
    std::size_t count{},reads{},stores{},prefixes{},distance{},fallback{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>() && row.at("live_x87_unchanged").get<bool>(),name+": original physical evidence missing");
        for(bool reading:{false,true}) {
            Memory memory;
            for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            const auto fault=row.value("fault",Json::object());
            memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading && memory.phase=="after_write")continue;
            memory.recording=reading;
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading && !memory.raw_stores)observe=[&](const auto &event) {
                events.push_back(event);if(memory.phase=="after_write" && observed++==memory.ordinal)throw Prefix{};
            };
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            bool stopped{};std::optional<bool> resolved;std::optional<std::uint32_t> result;
            try {
                switch(row.at("entry").get<unsigned>()) {
                case 0x145070:native_campaign_ai_transform_point_tu15(memory,row.at("point_matrix"),row.at("point_source"),row.at("destination"),fp,observe);break;
                case 0x219030:native_campaign_ai_permute_point_tu15(memory,row.at("point_source"),row.at("destination"),row.at("axis"),row.at("variant"),observe);break;
                case 0x218780:resolved=native_campaign_ai_resolve_frame_tu15(memory,row.at("frame"),row.at("point_source"),row.at("destination"),fp,observe);break;
                case 0x218b20:native_campaign_ai_project_point_tu15(memory,row.at("point_source"),row.at("destination"),fp,observe);break;
                case 0x218d00:result=native_campaign_ai_point_distance_squared_tu15(memory,row.at("point_source"),row.at("second"),fp);break;
                default:throw std::runtime_error("Unknown native point owner");
                }
            }catch(const Prefix &){stopped=true;}
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw && actual_sw==host_sw && _mm_getcsr()==host_mxcsr,name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": point completion differs");
            check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],
                name+": point FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            if(resolved) {
                check(*resolved==bool(row.at("boundary_gp")[0].get<unsigned>()&0xffu),name+": frame success differs");
                fallback+=!*resolved;
            }
            if(result){check(*result==row.at("result_bits").get<std::uint32_t>(),name+": squared-distance bits differ");++distance;}
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained point bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                check(memory.reads==expected,name+": point ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;
                check(events.size()==row.at("writes").size(),name+": point store count differs");
                for(std::size_t n=0;n<events.size();++n) {
                    const auto &a=events[n];const auto &b=row.at("writes")[n];
                    check((memory.raw_stores || a.instruction==b.at("instruction")) && a.address==b.at("address") &&
                        hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": point ordered store differs");
                }
                stores+=events.size();
            }
            prefixes+=stopped;
        }
        ++count;
    }
    check(count && prefixes && distance && fallback,"Point qualification matrix incomplete");
    std::cout<<count<<" physical original point geometry cases, "<<reads<<" ordered reads, "<<stores<<" stores, "<<prefixes<<" prefixes, "<<distance<<" distances; raw coordinates, FP, lazy reads and fallback publications passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
