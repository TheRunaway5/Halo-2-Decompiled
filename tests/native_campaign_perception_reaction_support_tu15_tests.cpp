#include "halo2/native_campaign_perception_reaction_support_tu15.hpp"
#include <algorithm>
#include <bit>
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
struct ActualMissing{};
class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t missing_address=0xffffffffu;mutable bool actual_missing{};
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    mutable Json reads=Json::array();Json writes=Json::array();
    auto locate(std::uint32_t at,std::size_t n)const{auto found=banks.upper_bound(at);check(found!=banks.begin(),"Missing reaction support bank");--found;check(std::uint64_t(at-found->first)+n<=found->second.size(),"Whole perception extent differs");return found;}
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        if(at==missing_address){actual_missing=true;throw ActualMissing{};}
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
    check(argc==2,"Supply independent original reaction support fixture");Json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong reaction support authority");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Supplied reaction support outcome");
    check(fixture.at("bank_format")=="physical-complete-bank-intern-v1","Missing exact complete-bank dictionary");
    std::map<std::string,std::vector<std::byte>>payloads;for(const auto&[digest,value]:fixture.at("bank_data").items())payloads.emplace(digest,bytes(value));
    unsigned count{},read_count{},write_count{};
    for(const auto&row:fixture.at("cases")){
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),"Missing physical reaction support authority");Memory memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),payloads.at(bank.at("before_ref")));
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);memory.missing_address=row.value("actual_missing_read",0xffffffffu);
        const auto entry=row.at("entry").get<std::uint32_t>();const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};
        // Borrowing source FP controls must preserve real ambient live values,
        // even when a lazy memory provider rejects a later publication.
        asm volatile("fld1; fldpi":::"st","st(1)");
        alignas(16)std::array<std::byte,512>ambient{},after_ambient{};
        asm volatile("fxsave %0":"=m"(ambient)::"memory");
        asm volatile("fnstcw %0; fnstsw %1":"=m"(control),"=m"(status));
        NativeTu15PhysicsFrameFp fp{row.at("before_fp").at(0),row.at("before_fp").at(1),row.at("before_fp").at(2)};
        bool stopped{};std::uint32_t result{};
        try{
            if(entry==0x11e850u)result=native_campaign_perception_transition_tu15(memory,row.at("location"),row.at("point"),row.at("sector_destination"),fp);
            else if(entry==0xbcf70u)result=native_campaign_perception_flagged_root_tu15(memory,row.at("object"));
            else if(entry==0x1d2cc0u)result=native_campaign_perception_vehicle_root_tu15(memory,row.at("object"));
            else if(entry==0xbcfb0u)result=native_campaign_perception_parent_root_tu15(memory,row.at("object"));
            else if(entry==0xf34f0u)result=native_campaign_perception_definition_bit7_tu15(memory,row.at("object"));
            else if(entry==0x275c20u)result=native_campaign_perception_root_state_tu15(memory,row.at("object"));
            else{check(entry==0x275ba0u,"Unknown native reaction support owner");result=native_campaign_perception_root_cacheable_tu15(memory,row.at("object"));}
        }catch(const Prefix&){stopped=true;}catch(const ActualMissing&){check(row.contains("actual_missing_read")&&memory.actual_missing,"Unexpected missing reaction data");stopped=true;}
        std::uint16_t after_control{},after_status{};asm volatile("fnstcw %0; fnstsw %1":"=m"(after_control),"=m"(after_status));
        asm volatile("fxsave %0":"=m"(after_ambient)::"memory");
        check(control==after_control&&status==after_status&&mxcsr==_mm_getcsr(),name+": reaction support changed ambient FP");
        check(std::equal(ambient.begin(),ambient.begin()+5,after_ambient.begin())&&std::equal(ambient.begin()+32,ambient.begin()+160,after_ambient.begin()+32),name+": reaction support damaged live ambient x87");
        asm volatile("fstp %%st(0); fstp %%st(0)":::"st","st(1)");
        check(stopped==!row.at("completed").get<bool>(),name+": reaction support completion differs");
        if(!stopped){check(row.at("boundary_gp").at(4).get<std::uint32_t>()==row.at("gp").at(4).get<std::uint32_t>()+4u+row.at("arguments").size()*4u,"Perception source RET differs");check(result==(row.at("boundary_gp").at(0).get<std::uint32_t>()&((entry==0xbcf70u||entry==0x1d2cc0u||entry==0xbcfb0u)?0xffffffffu:0xffu)),name+": reaction support result differs");}
        check(Json::array({fp.control_word,fp.status,fp.mxcsr})==row.at("after_fp"),name+": completed perception distance FP differs; actual="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" expected="+row.at("after_fp").dump());
        Json expected_reads=Json::array();for(auto e:row.at("reads")){e.erase("instruction");expected_reads.push_back(e);}
        check(memory.reads==expected_reads,name+": ordered reaction support reads differ; actual="+memory.reads.dump()+" expected="+expected_reads.dump());
        Json expected_writes=Json::array();for(auto e:row.at("writes")){e.erase("instruction");expected_writes.push_back(e);}
        check(memory.writes==expected_writes,name+": ordered reaction support stores differ");
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==payloads.at(bank.at("after_ref")),name+": retained reaction support bytes differ");
        if(!stopped)check(row.at("live_x87_unchanged").get<bool>(),name+": original actor link damaged live caller x87");
        if(entry==0x11e850u&&!stopped){
            Memory private_memory;for(const auto&bank:row.at("banks"))private_memory.banks.emplace(bank.at("address"),payloads.at(bank.at("before_ref")));
            const auto before=private_memory.banks;
            NativeTu15PhysicsFrameFp private_fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            const auto value=native_campaign_perception_transition_value_tu15(private_memory,row.at("location"),row.at("point"),private_fp);
            check(value.accepted==bool(result)&&value.sector.has_value()==value.accepted,name+": private transition admission differs");
            if(value.accepted){bool matched{};for(const auto&e:row.at("reads"))if(e.at("instruction")==0x11e88fu){const auto raw=bytes(e.at("before"));const auto sector=std::to_integer<unsigned>(raw[0])|(std::to_integer<unsigned>(raw[1])<<8u);check(*value.sector==sector,name+": private transition sector differs");matched=true;}check(matched,"Missing physical transition sector read");}
            check(private_memory.reads==expected_reads&&private_memory.writes.empty()&&private_memory.banks==before,name+": private transition engine reads/state differ");
            check(Json::array({private_fp.control_word,private_fp.status,private_fp.mxcsr})==row.at("after_fp"),name+": private transition FP differs");
        }
        ++count;read_count+=memory.reads.size();write_count+=memory.writes.size();
    }
    std::cout<<count<<" physical original reaction support comparisons, "<<read_count<<" ordered reads, "<<write_count<<" stores\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
