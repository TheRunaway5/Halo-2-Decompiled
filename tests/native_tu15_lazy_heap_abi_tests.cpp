#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_lazy_heap_abi.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

namespace halo2 {
using namespace physics_world_runtime_detail;
inline unsigned translated_fallbacks{};inline std::uint32_t required_entry{};
struct RequiredOwner{};
void dispatch(WorldState &,std::uint32_t target,std::uint32_t,bool){required_entry=target;throw RequiredOwner{};}
namespace production {
static void run_33b590(WorldState &s){dispatch(s,0x33b590,0,false);}
static void run_2dbf74(WorldState &s){dispatch(s,0x2dbf74,0,false);}
static void run_1cd130(WorldState &s){dispatch(s,0x1cd130,0,false);}
#include "lazy_heap_body.inc"
}
}
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s){std::vector<std::byte>out;for(unsigned n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> file(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary);check(bool(f),"Missing owned lazy-heap bank");std::vector<char>raw((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b(raw.size());for(unsigned n=0;n<raw.size();++n)b[n]=std::byte(std::uint8_t(raw[n]));return b;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;mutable J reads=J::array();bool logging{};
    auto at(std::uint32_t p,std::size_t size)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked lazy operand");--i;check(std::uint64_t(p-i->first)+size<=i->second.size(),"Unbacked lazy extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{if(logging)reads.push_back(J::array({p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>in)override{const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply physical lazy-heap fixture");const std::filesystem::path path=argv[1];std::ifstream input(path);const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Lazy original pin differs");
    unsigned cases{},reads{},stores{},clocks{};
    for(const auto &c:fixture.at("cases"))for(bool read_pass:{false,true}) {
        const auto name=c.at("name").get<std::string>();Memory memory;memory.logging=read_pass;
        for(const auto &bank:c.at("banks"))memory.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>()));
        J written=J::array();SourcePhysicsContactRecordObserver observer;
        if(!read_pass)observer=[&](const auto &e){written.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};services.instruction=[&](auto value){pc=value;};
        const std::array<std::uint32_t,3>sites{0x38846eu,0x21f590u,0x33b595u};auto &cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.service_sites=sites;
        unsigned consumed{};const auto clock_inputs=c.at("clock_inputs");
        services.timestamp_counter=[&]{
            for(const auto &site:c.value("stop_before_sites",J::array()))if(site==pc){required_entry=pc;throw RequiredOwner{};}
            check(consumed<clock_inputs.size(),name+" extra clock sample");const auto &event=clock_inputs[consumed++];
            check(event.at("instruction")==pc,name+" clock event PC/order differs");
            return (std::uint64_t(event.at("high").get<unsigned>())<<32u)|event.at("low").get<unsigned>();
        };
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
        const auto &before=c.at("before_abi");abi.gp=before.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=before.at("eflags");
        const auto xmm=unhex(before.at("xmm"));std::memcpy(abi.xmm.data(),xmm.data(),128);abi.x87_status=before.at("fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("fp")[0],before.at("fp")[2]};state.load(abi);
        bool required{};required_entry=0;try{production::run_149930(state);}catch(const RequiredOwner&){required=true;}
        check(required==c.at("required_owner")&&required_entry==c.at("required_entry"),name+" boundary differs");
        check(consumed==clock_inputs.size(),name+" missing clock input");
        const auto after=state.save();const auto &expected=c.at("after_abi");
        check(after.gp==expected.at("gp").get<std::array<std::uint32_t,8>>(),name+" complete GP differs");
        check((after.eflags&0xcd5u)==(expected.at("eflags").get<unsigned>()&0xcd5u),name+" flags/DF differ");
        check(hex(std::as_bytes(std::span(after.xmm)))==expected.at("xmm").get<std::string>(),name+" XMM differs");
        check(J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})==expected.at("fp"),name+" FP differs");
        check(pc==c.at("last_instruction"),name+" published cursor differs");
        if(read_pass){J expected_reads=J::array();for(const auto &r:c.at("reads"))expected_reads.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==expected_reads,name+" ordered reads differ");reads+=memory.reads.size();}
        else{check(written==c.at("writes"),name+" ordered stores differ");stores+=written.size();++cases;clocks+=consumed;}
        for(const auto &bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),name+" whole bank differs");
    }
    const bool reference=std::getenv("HALO2_REFERENCE_LAZY_HEAP");check(reference?translated_fallbacks==2u*cases:translated_fallbacks==0,"Unexpected lazy translated fallback");
    std::cout<<J{{"passed",true},{"cases",cases},{"ordered_reads",reads},{"ordered_stores",stores},{"clock_inputs",clocks},{"translated_body_fallbacks",translated_fallbacks}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
