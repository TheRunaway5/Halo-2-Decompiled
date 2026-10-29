#include "halo2/native_campaign_ai_perception_group_initialize_tu15.hpp"
#include <algorithm>
#include <fstream>
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
    auto locate(std::uint32_t at,std::size_t n)const{auto found=banks.upper_bound(at);check(found!=banks.begin(),"Missing perception initializer bank");--found;check(std::uint64_t(at-found->first)+n<=found->second.size(),"Perception initializer extent differs");return found;}
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
    check(argc==2,"Supply independent original perception initializer fixture");Json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong perception initializer authority");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Supplied perception initializer outcome");
    unsigned count{},read_count{},write_count{};
    for(const auto&row:fixture.at("cases")){
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),"Missing physical perception initializer authority");Memory memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        const auto entry=row.at("entry").get<std::uint32_t>();const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};asm volatile("fnstcw %0; fnstsw %1":"=m"(control),"=m"(status));
        bool stopped{};std::uint32_t result{};
        try{
            if(entry==0x1e7810u)result=native_campaign_ai_perception_team_relation_tu15(memory,row.at("first"),row.at("second"));
            else if(entry==0x1e78f0u)result=native_campaign_ai_perception_team_secondary_relation_tu15(memory,row.at("first"),row.at("second"));
            else{check(entry==0x274020u,"Unknown perception initializer owner");native_campaign_ai_perception_group_initialize_tu15(memory,row.at("record"),row.at("object"),row.at("clump"),row.at("kind"),row.at("priority"));}
        }catch(const Prefix&){stopped=true;}
        std::uint16_t after_control{},after_status{};asm volatile("fnstcw %0; fnstsw %1":"=m"(after_control),"=m"(after_status));
        check(control==after_control&&status==after_status&&mxcsr==_mm_getcsr(),name+": perception initializer changed ambient FP");
        check(stopped==!row.at("completed").get<bool>(),name+": perception initializer completion differs");
        if(!stopped){if(entry==0x1e7810u||entry==0x1e78f0u)check(result==(row.at("boundary_gp").at(0).get<std::uint32_t>()&255u),name+": perception initializer result differs");check(row.at("boundary_gp").at(4).get<std::uint32_t>()==row.at("gp").at(4).get<std::uint32_t>()+(entry==0x274020u?8u:4u),"Perception initializer source RET differs");}
        Json expected_reads=Json::array();for(auto e:row.at("reads")){e.erase("instruction");expected_reads.push_back(e);}
        // A rejected source RMW instruction has no committed read. A native
        // field read may precede its rejected store; admit only that exact
        // final read of the actual faulting store operand.
        if(memory.phase=="before_write"&&memory.reads.size()==expected_reads.size()+1u){const auto&e=memory.reads.back();const auto&boundary=row.at("boundary");check(e.at("address")==boundary.at("address")&&e.at("bytes")==boundary.at("bytes"),name+": rejected RMW operand differs");memory.reads.erase(memory.reads.size()-1u);}
        check(memory.reads==expected_reads,name+": ordered perception initializer reads differ; actual="+memory.reads.dump()+" expected="+expected_reads.dump());
        Json expected_writes=Json::array();for(auto e:row.at("writes")){e.erase("instruction");expected_writes.push_back(e);}
        check(memory.writes==expected_writes,name+": ordered perception initializer stores differ");
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained perception initializer bytes differ");
        check(row.at("live_x87_unchanged").get<bool>()&&row.at("before_fp")==row.at("after_fp"),name+": original perception initializer FP changed");
        ++count;read_count+=memory.reads.size();write_count+=memory.writes.size();
    }
    // An unknown multiplayer virtual identity is a required owner, never a
    // guessed relationship. This diagnostic control authenticates its two
    // final lazy reads from the original160A79/160A7B listing, and publishes
    // no gameplay result. Those callback instructions remain unqualified by
    // the complete-original fixture until their real target closes.
    Memory unknown;unknown.banks.emplace(0x1000000u,std::vector<std::byte>(0x10000u));
    auto seed=[&](std::uint32_t p,std::uint32_t v){const auto i=unknown.locate(p,4);for(unsigned n=0;n<4;++n)unknown.banks.at(i->first)[p-i->first+n]=std::byte(v>>(n*8u));};
    for(const auto p:{0x4f3218u,0x4f63b8u,0x56dfacu})unknown.banks.emplace(p,std::vector<std::byte>(4));
    seed(0x4f3218u,0x100a000u);seed(0x100a008u,2);seed(0x4f63b8u,0x100c000u);seed(0x100cc14u,3);seed(0x56dfacu,0x100d000u);seed(0x100d000u,0x100d100u);seed(0x100d16cu,0x12345678u);
    const auto retained=unknown.banks;bool rejected{};
    try{(void)native_campaign_ai_perception_team_relation_tu15(unknown,3,2);}catch(const std::runtime_error&e){rejected=std::string(e.what()).find("160a7b -> 12345678")!=std::string::npos;}
    check(rejected&&unknown.writes.empty()&&unknown.banks==retained,"Unknown multiplayer relation callback invented a result/publication");
    std::vector<std::uint32_t>addresses;for(const auto&e:unknown.reads)addresses.push_back(e.at("address"));
    check(addresses==std::vector<std::uint32_t>{0x4f3218u,0x100a008u,0x4f63b8u,0x100cc14u,0x56dfacu,0x100d000u,0x100d16cu},"Unknown multiplayer relation callback read boundary differs");
    std::cout<<count<<" physical original perception initializer comparisons, "<<read_count<<" ordered reads, "<<write_count<<" stores\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
