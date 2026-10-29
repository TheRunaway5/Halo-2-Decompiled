#include "halo2/native_campaign_ai_jump_fallback_tu15.hpp"
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
    mutable std::optional<std::pair<std::uint32_t,std::size_t>> failed_access;
    bool recording{},raw_stores{};mutable Json reads=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing jump fallback storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Jump fallback storage extent differs");}return found;
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
    check(argc==2,"Supply independent original jump fallback fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Jump fallback original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() &&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Jump fallback reference lacks unchanged physical authority");
    std::size_t count{},reads{},stores{},prefixes{},pending_count{},faults{},value_count{};
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
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write" || row.value("actual_write_fault",false));
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading && !memory.raw_stores)observe=[&](const auto &event) {
                events.push_back(event);if(memory.phase=="after_write" && observed++==memory.ordinal)throw Prefix{};
            };
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            alignas(16) std::array<std::byte,108> host{},expected_host{},actual_host{};
            asm volatile("fnsave %0":"=m"(host)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            asm volatile("fld1\n\tfldz\n\tfldpi\n\tfnsave %0":"=m"(expected_host)::"memory","st","st(1)","st(2)");
            std::fill(expected_host.begin()+12,expected_host.begin()+24,std::byte{});
            asm volatile("frstor %0"::"m"(expected_host):"memory");
            bool stopped{},pending_seen{};
            try {
                try {
                    native_campaign_ai_jump_fallback_tu15(memory,row.at("first"),row.at("second"),row.at("speed_bits"),
                        {row.at("direction"),row.at("distance"),row.at("speed_out"),row.at("duration")},fp,observe);
                }catch(const NativeCampaignAiJumpPendingErrorTu15 &error) {
                    check(hex(error.pending().quantity)==row.at("pending_extended").get<std::string>(),name+": pending jump quantity differs");
                    check(int(error.pending().kind)==row.at("pending_kind").get<int>(),name+": pending jump kind differs");
                    check(bool(error.cause()),name+": pending jump cause missing");pending_seen=true;++pending_count;
                    std::rethrow_exception(error.cause());
                }
            }catch(const Prefix &){stopped=true;}
            catch(const std::runtime_error&) {
                if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)throw;
                check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": actual jump fallback fault operand differs");stopped=true;++faults;
            }
            catch(...){asm volatile("frstor %0"::"m"(host):"memory");throw;}
            asm volatile("fnsave %0\n\tfrstor %1":"=m"(actual_host):"m"(host):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            check(expected_host==actual_host,name+": jump kernel changed live host x87 values/environment");
            check(pending_seen==!row.at("pending_extended").get<std::string>().empty(),name+": pending jump presence differs");
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw && actual_sw==host_sw && _mm_getcsr()==host_mxcsr,name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": jump fallback completion differs");
            check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],
                name+": jump fallback FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            if(!stopped)check((row.at("boundary_gp")[0].get<std::uint32_t>()&255u)==1u,name+": original fallback return contract differs");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained jump fallback bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                check(memory.reads==expected,name+": jump fallback ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;
                check(events.size()==row.at("writes").size(),name+": jump fallback store count differs");
                for(std::size_t n=0;n<events.size();++n) {
                    const auto &a=events[n];const auto &b=row.at("writes")[n];
                    check((memory.raw_stores || a.instruction==b.at("instruction")) && a.address==b.at("address") &&
                        hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": jump fallback ordered store differs");
                }
                stores+=events.size();
            }
            prefixes+=stopped;
        }
        if(row.at("completed").get<bool>()&&row.at("distance").get<std::uint32_t>()&&row.at("duration").get<std::uint32_t>()) {
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};memory.recording=true;
            const auto value=native_campaign_ai_jump_fallback_value_tu15(memory,row.at("first"),row.at("second"),row.at("speed_bits"),fp);
            const auto word=[](const Json &event){const auto raw=bytes(event.at("written"));std::uint32_t out{};for(unsigned n=0;n<4;++n)out|=std::to_integer<unsigned>(raw[n])<<(n*8u);return out;};
            for(unsigned axis=0;axis<3;++axis)check(value.direction[axis]==word(row.at("writes")[axis]),name+": private trajectory direction differs");
            for(const auto &event:row.at("writes")) {
                if(event.at("instruction")==0xff676u)check(value.distance_bits==word(event),name+": private trajectory distance differs");
                if(event.at("instruction")==0xff688u)check(value.duration_bits==word(event),name+": private trajectory duration differs");
            }
            check(value.speed_bits==row.at("speed_bits"),name+": private trajectory speed differs");
            check(Json::array({fp.control_word,fp.status,fp.mxcsr})==row.at("after_fp"),name+": private trajectory FP differs");
            Json expected_reads=Json::array();for(const auto &event:row.at("reads"))expected_reads.push_back(Json::array({event.at("address"),event.at("bytes")}));
            check(memory.reads==expected_reads,name+": private trajectory read order differs");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("before")),name+": private trajectory changed retained memory");
            memory.recording=false;
            NativeCampaignAiJumpPointMemoryTu15 first_memory(memory,row.at("first")),second_memory(memory,row.at("second"));
            std::array<std::uint32_t,3> first_words{},second_words{};
            for(unsigned axis=0;axis<3;++axis){first_words[axis]=first_memory.coordinate(axis);second_words[axis]=second_memory.coordinate(axis);}
            NativeCampaignAiJumpPointValueTu15 first_value(first_words),second_value(second_words);
            memory.reads=Json::array();memory.recording=true;
            fp={row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            const auto private_inputs=native_campaign_ai_jump_fallback_value_tu15(memory,first_value,second_value,row.at("speed_bits"),fp);
            check(private_inputs.direction==value.direction&&private_inputs.distance_bits==value.distance_bits&&
                private_inputs.speed_bits==value.speed_bits&&private_inputs.duration_bits==value.duration_bits,
                name+": private point trajectory differs");
            check(Json::array({fp.control_word,fp.status,fp.mxcsr})==row.at("after_fp"),name+": private point FP differs");
            Json constants=Json::array();for(const auto &event:row.at("reads")) {
                const auto address=event.at("address").get<std::uint32_t>();
                const auto first=row.at("first").get<std::uint32_t>(),second=row.at("second").get<std::uint32_t>();
                if(!((address>=first&&address<first+12u)||(address>=second&&address<second+12u)))
                    constants.push_back(Json::array({address,event.at("bytes")}));
            }
            check(memory.reads==constants,name+": private point constant read order differs");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("before")),name+": private points changed retained memory");
            ++value_count;
        }
        ++count;
    }
    check(count && prefixes && pending_count && faults && value_count,"Jump fallback qualification matrix incomplete");
    std::cout<<count<<" physical original jump fallback cases, "<<reads<<" ordered reads, "<<stores<<" stores, "<<prefixes<<" prefixes, "<<pending_count<<" pending quantities, "<<value_count<<" private values and private point pairs; exact mixed-precision trajectory outputs, FP and fault publications passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
