#include "halo2/native_campaign_ai_path_arena_tu15.hpp"
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
    mutable std::optional<std::pair<std::uint32_t,std::size_t>> failed_read;
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_read={{address,size}};throw std::runtime_error("Missing path-arena storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){
            failed_read={{address,size}};throw std::runtime_error("Path-arena storage extent differs");
        }
        return found;
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
    check(argc==2,"Supply independent original path-arena fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Path-arena original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() &&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Path-arena reference lacks unchanged physical authority");
    std::size_t count{},reads{},stores{},prefixes{},owner_boundaries{};
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
            const auto initial_fp=row.at("before_fp");
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            bool stopped{};std::optional<std::uint32_t> acquired;
            try {
                switch(row.at("entry").get<std::uint32_t>()) {
                case 0x149680:native_campaign_ai_path_arena_restore_tu15(memory,observe);break;
                case 0x1498e0:native_campaign_ai_path_arena_retire_tu15(memory,observe);break;
                case 0x1d2d90:acquired=native_campaign_ai_path_workspace_acquire_tu15(memory,observe);break;
                case 0x1d2e30:native_campaign_ai_path_workspace_release_tu15(memory,row.at("workspace"),observe);break;
                case 0x233120:native_campaign_ai_path_allocator_retire_tu15(memory,row.at("gp")[1],observe);break;
                default:throw std::runtime_error("Unknown path arena owner");
                }
            }catch(const Prefix &){stopped=true;}
            catch(const NativeCampaignAiPathArenaRequiredOwnerTu15 &required) {
                check(row.contains("required_owner") && required.entry()==row.at("required_owner") &&
                    required.identity()==row.at("boundary_gp")[row.at("identity_gp").get<unsigned>()].get<std::uint32_t>(),
                    name+": real required-owner boundary differs");stopped=true;++owner_boundaries;
            }
            catch(const std::runtime_error &) {
                if(!row.value("actual_read_fault",false) || !memory.failed_read)throw;
                check(memory.failed_read->first==row.at("boundary").at("address") &&
                    memory.failed_read->second==row.at("boundary").at("bytes"),name+": actual missing-source operand differs");
                stopped=true;
            }
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw && actual_sw==host_sw && _mm_getcsr()==host_mxcsr,name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": path-arena completion differs");
            check(initial_fp==row.at("after_fp"),name+": original path-arena FP changed");
            if(acquired)check(*acquired==row.at("boundary_gp")[0].get<std::uint32_t>(),name+": actual leased workspace identity differs");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained path-arena bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                check(memory.reads==expected,name+": path-arena ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;
                check(events.size()==row.at("writes").size(),name+": path-arena store count differs");
                for(std::size_t n=0;n<events.size();++n) {
                    const auto &a=events[n];const auto &b=row.at("writes")[n];
                    check((memory.raw_stores || a.instruction==b.at("instruction")) && a.address==b.at("address") &&
                        hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": path-arena ordered store differs");
                }
                stores+=events.size();
            }
            prefixes+=stopped;
        }
        ++count;
    }
    check(count && prefixes && owner_boundaries,"Path-arena qualification matrix incomplete");
    std::cout<<count<<" physical original path-arena geometry cases, "<<reads<<" ordered reads, "<<stores<<" stores, "<<prefixes<<" prefixes; exact lease/arena fields, required-owner boundaries, FP and lazy publications passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
