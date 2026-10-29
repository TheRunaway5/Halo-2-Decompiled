#include "halo2/native_campaign_ai_jump_arc_tu15.hpp"
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
    check(argc==2,"Supply independent original jump arc fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Jump arc original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Jump arc reference lacks unchanged physical authority");
    std::size_t count{},reads{},prefixes{},pending_count{},private_count{},links{},actual_faults{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>()&&row.at("live_x87_unchanged").get<bool>(),name+": original caller values missing");
        for(bool private_inputs:{false,true}) {
            if(private_inputs&&!row.at("completed").get<bool>())continue;
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            const auto first=row.at("first").get<std::uint32_t>(),second=row.at("second").get<std::uint32_t>();
            const auto af=row.at("secondary_first").get<std::uint32_t>(),at=row.at("secondary_second").get<std::uint32_t>();
            NativeCampaignAiJumpPointMemoryTu15 first_memory(memory,first),second_memory(memory,second),af_memory(memory,af),at_memory(memory,at);
            const NativeCampaignAiJumpPointTu15 *from=&first_memory,*to=&second_memory,*alt_from=af?&af_memory:nullptr,*alt_to=at?&at_memory:nullptr;
            std::optional<NativeCampaignAiJumpPointValueTu15> first_value,second_value,af_value,at_value;
            const auto copy=[](const NativeCampaignAiJumpPointTu15 &point){std::array<std::uint32_t,3> out{};for(unsigned axis=0;axis<3;++axis)out[axis]=point.coordinate(axis);return out;};
            if(private_inputs) {
                first_value.emplace(copy(first_memory));second_value.emplace(copy(second_memory));from=&*first_value;to=&*second_value;
                if(af){af_value.emplace(copy(af_memory));alt_from=&*af_value;}if(at){at_value.emplace(copy(at_memory));alt_to=&*at_value;}
            }
            const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);memory.recording=true;
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            const auto host_mxcsr=_mm_getcsr();alignas(16) std::array<std::byte,108> host{},expected_host{},actual_host{};
            asm volatile("fnsave %0":"=m"(host)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            asm volatile("fld1\n\tfldz\n\tfldpi\n\tfnsave %0":"=m"(expected_host)::"memory","st","st(1)","st(2)");
            std::fill(expected_host.begin()+12,expected_host.begin()+24,std::byte{});asm volatile("frstor %0"::"m"(expected_host):"memory");
            bool stopped{},pending_seen{};std::optional<NativeCampaignAiJumpArcValueTu15> arc;std::optional<bool> linked;
            try {
                try {
                    if(row.at("entry")==0xff1a0u)arc=native_campaign_ai_jump_arc_profile_tu15(memory,*from,*to,row.at("speed_bits"),fp);
                    else{linked=native_campaign_ai_jump_link_tu15(memory,row.at("kind"),*from,*to,alt_from,alt_to,fp);++links;}
                }catch(const NativeCampaignAiJumpArcPendingErrorTu15 &error) {
                    Json observed=Json::array();std::visit([&](const auto &pending){
                        if constexpr(requires{pending.x_squared;}){observed.push_back(hex(pending.x_squared.bytes));observed.push_back(hex(pending.y_squared.bytes));}
                        else observed.push_back(hex(pending.value.bytes));
                    },error.pending());
                    check(row.at("pending_family")=="arc"&&error.pending().index()==row.at("pending_kind"),name+": arc pending kind differs");
                    check(observed==row.at("pending_extended"),name+": arc pending quantities differ");
                    check(bool(error.cause()),name+": arc pending cause missing");pending_seen=true;++pending_count;std::rethrow_exception(error.cause());
                }catch(const NativeCampaignAiJumpPendingErrorTu15 &error) {
                    check(row.at("pending_family")=="fallback"&&int(error.pending().kind)==row.at("pending_kind"),name+": fallback pending kind differs");
                    check(Json::array({hex(error.pending().quantity)})==row.at("pending_extended"),name+": fallback pending quantity differs");
                    check(bool(error.cause()),name+": fallback pending cause missing");pending_seen=true;++pending_count;std::rethrow_exception(error.cause());
                }
            }catch(const Prefix &){stopped=true;}
            catch(const std::runtime_error&) {
                if(!row.value("actual_read_fault",false)||!memory.failed_access){asm volatile("frstor %0"::"m"(host):"memory");throw;}
                check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": actual arc fault operand differs");stopped=true;++actual_faults;
            }catch(...){asm volatile("frstor %0"::"m"(host):"memory");throw;}
            asm volatile("fnsave %0\n\tfrstor %1":"=m"(actual_host):"m"(host):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            check(expected_host==actual_host&&_mm_getcsr()==host_mxcsr,name+": arc changed live host FP state");
            check(stopped==!row.at("completed").get<bool>(),name+": arc completion differs");
            check(pending_seen==!row.at("pending_extended").empty(),name+": arc pending presence differs");
            check(Json::array({fp.control_word,fp.status,fp.mxcsr})==row.at("after_fp"),name+": arc FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            if(arc) {
                check(arc->feasible==bool(row.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+": arc feasibility differs");
                const auto expected=bytes(row.at("direction_value"));for(unsigned axis=0;axis<3;++axis){std::uint32_t word{};for(unsigned n=0;n<4;++n)word|=std::to_integer<unsigned>(expected[axis*4u+n])<<(n*8u);check(arc->direction[axis]==word,name+": arc direction differs");}
            }
            if(linked)check(*linked==bool(row.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+": authored jump-link result differs");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained arc bytes differ");
            check(row.at("writes").empty(),name+": original profile published retained private output");
            Json expected_reads=Json::array();for(const auto &event:row.at("reads")) {
                const auto address=event.at("address").get<std::uint32_t>();bool point_read{};
                if(private_inputs)for(const auto p:{first,second,af,at})if(p&&address>=p&&std::uint64_t(address-p)<12)point_read=true;
                if(!point_read)expected_reads.push_back(Json::array({address,event.at("bytes")}));
            }
            check(memory.reads==expected_reads,name+": arc source read order differs native="+memory.reads.dump()+" original="+expected_reads.dump());
            reads+=memory.reads.size();prefixes+=stopped;private_count+=private_inputs;
        }
        ++count;
    }
    check(count&&links&&prefixes&&pending_count&&actual_faults&&private_count,"Arc proof matrix incomplete");
    std::cout<<count<<" original arc/link cases, "<<private_count<<" private point variants, "<<reads<<" ordered reads, "<<pending_count<<" pending quantities; mathematical results/FP/fault/order passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
