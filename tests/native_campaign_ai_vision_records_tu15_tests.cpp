#include "halo2/native_campaign_ai_vision_records_tu15.hpp"
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
    check(argc==2,"Supply independent original vision records fixture");Json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong vision records authority");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Supplied vision records outcome");
    check(fixture.at("bank_format")=="physical-complete-bank-intern-v1","Missing exact complete-bank dictionary");
    std::map<std::string,std::vector<std::byte>>payloads;for(const auto&[digest,value]:fixture.at("bank_data").items())payloads.emplace(digest,bytes(value));
    unsigned count{},read_count{},write_count{};
    for(const auto&row:fixture.at("cases")){
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),"Missing physical vision records authority");Memory memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),payloads.at(bank.at("before_ref")));
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        const auto entry=row.at("entry").get<std::uint32_t>();const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};
        // Borrowing source FP controls must preserve real ambient live values,
        // even when a lazy memory provider rejects a later publication.
        asm volatile("fld1; fldpi":::"st","st(1)");
        alignas(16)std::array<std::byte,512>ambient{},after_ambient{};
        asm volatile("fxsave %0":"=m"(ambient)::"memory");
        asm volatile("fnstcw %0; fnstsw %1":"=m"(control),"=m"(status));
        bool stopped{};std::uint32_t result{};
        NativeTu15PhysicsFrameFp fp{row.at("before_fp").at(0),row.at("before_fp").at(1),row.at("before_fp").at(2)};bool required{},missing{};std::array<std::uint32_t,3> private_output{};
        try{
            if(entry==0x26eef0u){
                if(row.at("private_output").get<bool>()){
                    const auto value=native_campaign_ai_vision_attachment_index_tu15(memory,row.at("object"));result=value.present;private_output[0]=value.index;
                }else result=native_campaign_ai_vision_attachment_index_store_tu15(memory,row.at("object"),row.at("output"));
            }else if(entry==0x1eccc0u)result=native_campaign_ai_vision_character_block_tu15(memory,row.at("character"));
            else if(entry==0x1d0400u)result=std::uint16_t(native_campaign_ai_vision_player_slot_tu15(memory,row.at("player")));
            else{check(entry==0x272970u,"Unknown vision records owner");native_campaign_ai_vision_clump_timestamp_tu15(memory,row.at("clump"));}
        }catch(const Prefix&){stopped=true;}catch(const MissingBacking&e){throw std::runtime_error(name+": unexpected vision records source read "+std::to_string(e.address)+" width "+std::to_string(e.bytes));}
        std::uint16_t after_control{},after_status{};asm volatile("fnstcw %0; fnstsw %1":"=m"(after_control),"=m"(after_status));
        asm volatile("fxsave %0":"=m"(after_ambient)::"memory");
        check(control==after_control&&status==after_status&&mxcsr==_mm_getcsr(),name+": vision records changed ambient FP");
        check(std::equal(ambient.begin(),ambient.begin()+5,after_ambient.begin())&&std::equal(ambient.begin()+32,ambient.begin()+160,after_ambient.begin()+32),name+": vision records damaged live ambient x87");
        asm volatile("fstp %%st(0); fstp %%st(0)":::"st","st(1)");
        check(stopped==!row.at("completed").get<bool>(),name+": vision records completion differs");
        
        
        if(!stopped){
            check(row.at("boundary_gp").at(4).get<std::uint32_t>()==row.at("gp").at(4).get<std::uint32_t>()+(entry==0x26eef0u?8u:4u),name+": source RET differs");
            const auto mask=entry==0x26eef0u?0xffu:entry==0x1d0400u?0xffffu:0xffffffffu;
            if(entry!=0x272970u)check(result==(row.at("boundary_gp").at(0).get<std::uint32_t>()&mask),name+": vision records result differs");
            if(entry==0x26eef0u&&row.at("private_output").get<bool>()){
                std::array<std::byte,2> actual{std::byte(private_output[0]),std::byte(private_output[0]>>8u)};
                check(hex(actual)==row.at("iterator_after").get<std::string>(),name+": private output differs");
            }
        }
        const auto boundary=row.at("boundary");const auto pc=boundary.is_null()?0u:boundary.value("instruction",0u);
        check(fp.control_word==row.at("after_fp").at(0)&&fp.mxcsr==row.at("after_fp").at(2),name+": vision records FP controls/SIMD status differ");
        check(fp.status==row.at("after_fp").at(1),name+": vision records x87 status differs; actual="+std::to_string(fp.status)+" expected="+row.at("after_fp").at(1).dump());
        Json expected_reads=Json::array();for(auto e:row.at("reads")){e.erase("instruction");expected_reads.push_back(e);}
        // A rejected source RMW instruction has no committed read. A native
        // field read may precede its rejected store; admit only that exact
        // final read of the actual faulting store operand.
        if(memory.phase=="before_write"&&memory.reads.size()==expected_reads.size()+1u){const auto&e=memory.reads.back();const auto&boundary=row.at("boundary");check(e.at("address")==boundary.at("address")&&e.at("bytes")==boundary.at("bytes"),name+": rejected RMW operand differs");memory.reads.erase(memory.reads.size()-1u);}
        check(memory.reads==expected_reads,name+": ordered vision records reads differ; actual="+memory.reads.dump()+" expected="+expected_reads.dump());
        Json expected_writes=Json::array();for(auto e:row.at("writes")){e.erase("instruction");expected_writes.push_back(e);}
        check(memory.writes==expected_writes,name+": ordered vision records stores differ");
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==payloads.at(bank.at("after_ref")),name+": retained vision records bytes differ");
        if(!stopped)check(row.at("live_x87_unchanged").get<bool>(),name+": original actor link damaged live caller x87");
        ++count;read_count+=memory.reads.size();write_count+=memory.writes.size();
    }
    std::cout<<count<<" physical original vision records comparisons, "<<read_count<<" ordered reads, "<<write_count<<" stores\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
