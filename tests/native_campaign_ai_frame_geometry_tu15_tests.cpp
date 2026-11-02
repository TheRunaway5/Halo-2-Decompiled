#include "halo2/native_campaign_ai_frame_geometry_tu15.hpp"
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
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    bool recording{},raw_stores{};mutable Json reads=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);if(found==banks.begin())throw Missing{address,size};--found;
        if(std::uint64_t(address-found->first)+size>found->second.size())throw Missing{address,size};return found;
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
    std::size_t count{},reads{},stores{},prefixes{},distance{},fallback{},private_values{};
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
                case 0x145140:native_campaign_ai_transform_direction_tu15(memory,row.at("point_matrix"),row.at("point_source"),row.at("destination"),fp,observe);break;
                case 0x145200:native_campaign_ai_inverse_point_tu15(memory,row.at("point_matrix"),row.at("point_source"),row.at("destination"),fp,observe);break;
                case 0x218fe0:native_campaign_ai_inverse_permute_point_tu15(memory,row.at("point_source"),row.at("destination"),row.at("axis"),row.at("variant"),observe);break;
                case 0x218880:resolved=native_campaign_ai_resolve_direction_tu15(memory,row.at("frame"),row.at("point_source"),row.at("destination"),fp,observe);break;
                case 0x218960:resolved=native_campaign_ai_inverse_frame_point_tu15(memory,row.at("frame"),row.at("point_source"),row.at("destination"),fp,observe);break;
                case 0x218eb0:native_campaign_ai_framed_difference_tu15(memory,row.at("point_source"),row.at("second"),row.at("destination"),fp,observe);break;
                default:throw std::runtime_error("Unknown native point owner");
                }
            }catch(const Prefix &){stopped=true;}
            catch(const Missing &missing){stopped=true;check(row.value("expected_fault",false),name+": unexpected native missing storage");bool matched=false;for(const auto &operand:row.at("boundary").at("operands"))matched|=missing.address==operand.at("address")&&missing.width==operand.at("bytes");check(matched,name+": actual fault provenance differs");}
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
        if(row.at("entry")==0x218960u && row.at("completed").get<bool>()) {
            const auto destination=row.at("destination").get<std::uint32_t>();
            const bool disjoint=std::all_of(row.at("reads").begin(),row.at("reads").end(),[&](const auto &event){
                const auto address=event.at("address").template get<std::uint32_t>();
                const auto width=event.at("bytes").template get<unsigned>();
                return std::uint64_t(address)+width<=destination || address>=std::uint64_t(destination)+12u;
            });
            if(disjoint) {
                Memory memory;
                for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
                memory.recording=true;memory.phase="before_write";memory.ordinal=0;
                NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
                const auto host_mxcsr=_mm_getcsr();std::uint16_t host_control{},host_status{};
                asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_control),"=m"(host_status));
                const auto result=native_campaign_ai_inverse_frame_point_value_tu15(memory,row.at("frame"),row.at("point_source"),fp);
                std::uint16_t after_control{},after_status{};
                asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_control),"=m"(after_status));
                check(host_control==after_control && host_status==after_status && host_mxcsr==_mm_getcsr(),name+": private frame host FP leaked");
                check(result.resolved==bool(row.at("boundary_gp")[0].get<unsigned>()&0xffu),name+": private frame status differs");
                const auto bank=std::find_if(row.at("banks").begin(),row.at("banks").end(),[&](const auto &value){
                    const auto address=value.at("address").template get<std::uint32_t>();
                    return destination>=address && std::uint64_t(destination-address)+12u<=value.at("after").template get_ref<const std::string&>().size()/2;
                });
                check(bank!=row.at("banks").end(),name+": original private-value oracle has no retained output");
                const auto expected=bytes(bank->at("after"));const auto begin=destination-bank->at("address").get<std::uint32_t>();
                for(unsigned n=0;n<3;++n){std::uint32_t word{};for(unsigned k=0;k<4;++k)word|=std::to_integer<unsigned>(expected[begin+n*4u+k])<<(8u*k);
                    check(result.point[n]==word,name+": private frame point differs");}
                check(memory.write_attempts==0,name+": private frame attempted engine publication");
                Json expected_reads=Json::array();for(const auto &event:row.at("reads"))expected_reads.push_back(Json::array({event.at("address"),event.at("bytes")}));
                check(memory.reads==expected_reads,name+": private frame ordered reads differ");
                check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],name+": private frame FP differs");
                for(const auto &value:row.at("banks"))check(memory.banks.at(value.at("address"))==bytes(value.at("before")),name+": private frame changed retained bytes");
                ++private_values;
            }
        }
    }
    check(count && prefixes && fallback && private_values,"Frame qualification matrix incomplete");
    std::cout<<count<<" physical original frame geometry cases, "<<reads<<" ordered reads, "<<stores<<" stores, "<<prefixes<<" prefixes, "<<private_values<<" private inverse-frame values; raw coordinates, FP, lazy reads and fallback publications passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
