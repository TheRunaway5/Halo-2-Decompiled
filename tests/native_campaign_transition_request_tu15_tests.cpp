#include "halo2/native_campaign_transition_request_tu15.hpp"
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string&s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Prefix{};
struct MissingBacking{std::uint32_t address;std::size_t bytes;};
class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    mutable Json reads=Json::array();Json writes=Json::array();
    auto locate(std::uint32_t at,std::size_t n)const{auto found=banks.upper_bound(at);if(found==banks.begin())throw MissingBacking{at,n};--found;if(std::uint64_t(at-found->first)+n>found->second.size())throw MissingBacking{at,n};return found;}
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        if(phase=="before_read"&&read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(at,out.size());std::copy_n(found->second.begin()+at-found->first,out.size(),out.begin());
        reads.push_back(Json{{"address",at},{"bytes",out.size()},{"before",hex(out)},{"stores_before",writes.size()}});
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(phase=="before_write"&&write_attempts==ordinal)throw Prefix{};
        const auto found=locate(at,in.size());const auto old=std::span(found->second).subspan(at-found->first,in.size());
        writes.push_back(Json{{"address",at},{"before",hex(old)},{"written",hex(in)}});
        std::copy(in.begin(),in.end(),banks.at(found->first).begin()+at-found->first);
        if(phase=="after_write"&&write_attempts++==ordinal)throw Prefix{};
        if(phase!="after_write")++write_attempts;
    }
};
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply independent original transition publication fixture");Json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong transition publication authority");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Supplied transition publication outcome");
    check(fixture.at("bank_format")=="physical-complete-bank-intern-v1","Missing exact complete-bank dictionary");
    std::map<std::string,std::vector<std::byte>>payloads;for(const auto&[digest,value]:fixture.at("bank_data").items())payloads.emplace(digest,bytes(value));
    check(fixture.at("event_format")=="exact-original-prefix-v1","Missing lossless original prefix traces");
    struct EventRange {const Json* values;std::size_t count;};
    auto events=[&](const Json&row,const char*field)->EventRange{
        if(row.contains(field))return {&row.at(field),row.at(field).size()};
        const auto&reference=row.at(std::string(field)+"_prefix");
        const auto&seed=fixture.at("cases").at(reference.at("seed").get<std::size_t>());
        check(seed.at("id")==reference.at("seed"),"Wrong original prefix identity");
        const auto length=reference.at("count").get<std::size_t>();
        check(length<=seed.at(field).size(),"Original event prefix is too long");
        return {&seed.at(field),length};
    };
    unsigned count{},read_count{},write_count{};
    for(const auto&row:fixture.at("cases")){
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),"Missing physical transition publication authority");Memory memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),payloads.at(bank.at("before_ref")));
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        const auto entry=row.at("entry").get<std::uint32_t>();const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};
        // Borrowing source FP controls must preserve real ambient live values,
        // even when a lazy memory provider rejects a later publication.
        asm volatile("fld1; fldpi":::"st","st(1)");
        alignas(16)std::array<std::byte,512>ambient{},after_ambient{};
        asm volatile("fxsave %0":"=m"(ambient)::"memory");
        asm volatile("fnstcw %0; fnstsw %1":"=m"(control),"=m"(status));
        bool stopped{};
        NativeTu15PhysicsFrameFp fp{row.at("before_fp").at(0),row.at("before_fp").at(1),row.at("before_fp").at(2)};bool required{};
        Json clock_events=Json::array();
        const NativeCampaignTransitionClockTu15 clock=[&](std::uint32_t cell){
            clock_events.push_back(Json{{"cell",cell},{"reads_before",memory.reads.size()},{"stores_before",memory.writes.size()}});
            if(memory.phase=="clock_refresh")throw Prefix{};
        };
        try{
            check(entry==0x191580u,"Unknown transition publication owner");
            if(row.at("private_options").get<bool>()){
                NativeCampaignTransitionOptionsTu15 options;const auto raw=bytes(row.at("options_value"));
                check(raw.size()==options.bytes.size(),"Wrong private options quantity");std::copy(raw.begin(),raw.end(),options.bytes.begin());
                native_campaign_transition_request_value_tu15(memory,options,clock);
            }else native_campaign_transition_request_tu15(memory,row.at("options"),clock);
        }catch(const Prefix&){stopped=true;}catch(const NativeCampaignTransitionRequestRequiredOwnerTu15&e){check(e.site==0x1915ebu&&e.target==0x5a080u,name+": wrong required transition owner");required=true;stopped=true;}catch(const MissingBacking&e){throw std::runtime_error(name+": unexpected transition source read "+std::to_string(e.address)+" width "+std::to_string(e.bytes));}
        std::uint16_t after_control{},after_status{};asm volatile("fnstcw %0; fnstsw %1":"=m"(after_control),"=m"(after_status));
        asm volatile("fxsave %0":"=m"(after_ambient)::"memory");
        check(control==after_control&&status==after_status&&mxcsr==_mm_getcsr(),name+": transition publication changed ambient FP");
        check(std::equal(ambient.begin(),ambient.begin()+5,after_ambient.begin())&&std::equal(ambient.begin()+32,ambient.begin()+160,after_ambient.begin()+32),name+": transition publication damaged live ambient x87");
        asm volatile("fstp %%st(0); fstp %%st(0)":::"st","st(1)");
        check(stopped==!row.at("completed").get<bool>(),name+": transition publication completion differs");
        
        
        check(required==(!row.at("boundary").is_null()&&row.at("boundary").at("phase")=="required_owner"),name+": transition required owner differs");
        check(clock_events==row.at("clock_events"),name+": clock preparation timing differs");
        if(!stopped)check(row.at("boundary_gp").at(4).get<std::uint32_t>()==row.at("gp").at(4).get<std::uint32_t>()+4u,name+": source RET differs");
        check(fp.control_word==row.at("after_fp").at(0)&&fp.mxcsr==row.at("after_fp").at(2),name+": transition publication FP controls/SIMD status differ");
        check(fp.status==row.at("after_fp").at(1),name+": transition publication x87 status differs; actual="+std::to_string(fp.status)+" expected="+row.at("after_fp").at(1).dump());
        const auto expected_reads=events(row,"reads"),expected_writes=events(row,"writes");
        // A rejected native MOVSD store already read the independently
        // decoded source operand of that same uncommitted instruction.
        const Json* pending=nullptr;
        if(memory.phase=="before_write"&&row.at("boundary").contains("uncommitted_copy_reads"))pending=&row.at("boundary").at("uncommitted_copy_reads");
        const auto pending_count=pending?pending->size():0u;
        check(memory.reads.size()==expected_reads.count+pending_count,name+": ordered transition read count differs");
        for(std::size_t n=0;n<memory.reads.size();++n){
            const auto&e=n<expected_reads.count?expected_reads.values->at(n):pending->at(n-expected_reads.count);const auto&a=memory.reads.at(n);
            check(a.at("address")==e.at("address")&&a.at("bytes")==e.at("bytes")&&a.at("before")==e.at("before")&&a.at("stores_before")==e.at("stores_before"),name+": ordered transition reads differ at "+std::to_string(n));
        }
        check(memory.writes.size()==expected_writes.count,name+": ordered transition store count differs");
        for(std::size_t n=0;n<memory.writes.size();++n){const auto&e=expected_writes.values->at(n);const auto&a=memory.writes.at(n);check(a.at("address")==e.at("address")&&a.at("before")==e.at("before")&&a.at("written")==e.at("written"),name+": ordered transition stores differ at "+std::to_string(n));}
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==payloads.at(bank.at("after_ref")),name+": retained transition publication bytes differ");
        if(!stopped)check(row.at("live_x87_unchanged").get<bool>(),name+": original transition changed live caller x87");
        if(memory.phase=="clock_refresh"&&!row.at("private_options").get<bool>()){
            Memory absent;for(const auto&bank:row.at("banks"))absent.banks.emplace(bank.at("address"),payloads.at(bank.at("before_ref")));
            bool rejected{};try{native_campaign_transition_request_tu15(absent,row.at("options"),{});}catch(const std::invalid_argument&){rejected=true;}
            check(rejected,name+": absent clock producer acknowledged");
            check(absent.reads==memory.reads&&absent.writes==memory.writes&&absent.banks==memory.banks,name+": absent clock producer changed its real failure prefix");
        }
        ++count;read_count+=memory.reads.size();write_count+=memory.writes.size();
    }
    std::cout<<count<<" physical original transition publication comparisons, "<<read_count<<" ordered reads, "<<write_count<<" stores\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
