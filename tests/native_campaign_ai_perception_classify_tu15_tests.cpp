#include "halo2/native_campaign_ai_perception_classify_tu15.hpp"
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
class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    mutable Json reads=Json::array();Json writes=Json::array();
    auto locate(std::uint32_t at,std::size_t n)const{auto found=banks.upper_bound(at);check(found!=banks.begin(),"Missing perception classifier bank");--found;check(std::uint64_t(at-found->first)+n<=found->second.size(),"Perception classifier extent differs");return found;}
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
    check(argc==2,"Supply independent original perception classifier fixture");Json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong perception classifier authority");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Supplied perception classifier outcome");
    unsigned count{},read_count{},write_count{};
    for(const auto&row:fixture.at("cases")){
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),"Missing physical perception classifier authority");Memory memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        const auto entry=row.at("entry").get<std::uint32_t>();const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};
        // Borrowing source FP controls must preserve real ambient live values,
        // even when a lazy memory provider rejects a later publication.
        asm volatile("fld1; fldpi":::"st","st(1)");
        alignas(16)std::array<std::byte,512>ambient{},after_ambient{};
        asm volatile("fxsave %0":"=m"(ambient)::"memory");
        asm volatile("fnstcw %0; fnstsw %1":"=m"(control),"=m"(status));
        NativeTu15PhysicsFrameFp fp{row.at("before_fp").at(0),row.at("before_fp").at(1),row.at("before_fp").at(2)};
        bool stopped{};std::optional<NativeCampaignAiPerceptionClassificationTu15>result;
        try{
            result=native_campaign_ai_perception_classify_new_group_tu15(memory,row.at("object"),row.at("clump"),fp);
        }catch(const Prefix&){stopped=true;}
        std::uint16_t after_control{},after_status{};asm volatile("fnstcw %0; fnstsw %1":"=m"(after_control),"=m"(after_status));
        asm volatile("fxsave %0":"=m"(after_ambient)::"memory");
        check(control==after_control&&status==after_status&&mxcsr==_mm_getcsr(),name+": perception classifier changed ambient FP");
        check(std::equal(ambient.begin(),ambient.begin()+5,after_ambient.begin())&&std::equal(ambient.begin()+32,ambient.begin()+160,after_ambient.begin()+32),name+": perception classifier damaged live ambient x87");
        asm volatile("fstp %%st(0); fstp %%st(0)":::"st","st(1)");
        check(stopped==!row.at("completed").get<bool>(),name+": perception classifier completion differs");
        check(Json::array({fp.control_word,fp.status,fp.mxcsr})==row.at("after_fp"),name+": perception classifier FP differs; actual="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" expected="+row.at("after_fp").dump());
        if(!stopped){
            check(row.at("boundary_gp").at(4).get<std::uint32_t>()==row.at("gp").at(4).get<std::uint32_t>()+24u,"Forced classifier source RET differs");
            const auto output=bytes(row.at("private_output_after"));auto word=[&](unsigned at,unsigned width){std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(output[at+n])<<(n*8u);return value;};
            const bool produced=word(0,2)!=0xa5a5u;check(result.has_value()==produced,name+": classifier output admission differs");
            if(produced){check(result->kind==word(0,2)&&result->object==word(4,4)&&result->priority==word(8,2)&&word(12,4)==0xffffffffu,name+": classifier authored result differs");check(word(2,2)==0xa5a5u&&word(10,2)==0xa5a5u,"Original classifier initialized unowned private padding");}
            else check(std::all_of(output.begin(),output.end(),[](std::byte v){return v==std::byte{0xa5};}),"Unknown classifier type produced partial output");
        }
        Json expected_reads=Json::array();for(auto e:row.at("reads")){e.erase("instruction");expected_reads.push_back(e);}
        // A rejected source RMW instruction has no committed read. A native
        // field read may precede its rejected store; admit only that exact
        // final read of the actual faulting store operand.
        if(memory.phase=="before_write"&&memory.reads.size()==expected_reads.size()+1u){const auto&e=memory.reads.back();const auto&boundary=row.at("boundary");check(e.at("address")==boundary.at("address")&&e.at("bytes")==boundary.at("bytes"),name+": rejected RMW operand differs");memory.reads.erase(memory.reads.size()-1u);}
        check(memory.reads==expected_reads,name+": ordered perception classifier reads differ; actual="+memory.reads.dump()+" expected="+expected_reads.dump());
        Json expected_writes=Json::array();for(auto e:row.at("writes")){e.erase("instruction");expected_writes.push_back(e);}
        check(memory.writes==expected_writes,name+": ordered perception classifier stores differ");
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained perception classifier bytes differ");
        if(!stopped)check(row.at("live_x87_unchanged").get<bool>(),name+": original classifier damaged live caller x87");
        ++count;read_count+=memory.reads.size();write_count+=memory.writes.size();
    }
    std::cout<<count<<" physical original perception classifier comparisons, "<<read_count<<" ordered reads, "<<write_count<<" stores\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
