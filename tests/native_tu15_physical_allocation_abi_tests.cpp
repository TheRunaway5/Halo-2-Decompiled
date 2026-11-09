#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_physical_allocation_abi.hpp"
#include "physics/runtime/native_tu15_page_window_abi.hpp"
#include "physics/runtime/native_tu15_page_retire_abi.hpp"
#include "physics/runtime/native_tu15_datum_delete_abi.hpp"
#include "physics/runtime/native_tu15_stack_probe_abi.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>

namespace halo2 {
using namespace physics_world_runtime_detail;
std::uint32_t sign_extend(std::uint32_t value,unsigned width){
    if(width==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(width==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unexpected physical reference sign-extension width");
}
inline unsigned translated_fallbacks{};inline std::uint32_t required_entry{};
struct RequiredOwner{};
inline std::set<std::uint32_t> stop_owners;inline std::map<std::uint32_t,unsigned> reference_children;
inline std::function<void(WorldState&,std::uint32_t)> child_entry_observer;
void dispatch(WorldState &,std::uint32_t target,std::uint32_t,bool);
namespace production {
#include "physical_allocation_children.inc"
#include "physical_allocation_body.inc"
}
void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){
#include "physical_allocation_dispatch.inc"
}
}
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s){std::vector<std::byte>out;for(unsigned n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> file(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary);check(bool(f),"Missing owned physical-allocation bank");std::vector<char>raw((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b(raw.size());for(unsigned n=0;n<raw.size();++n)b[n]=std::byte(std::uint8_t(raw[n]));return b;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;mutable J reads=J::array();bool logging{};
    auto at(std::uint32_t p,std::size_t size)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked physical operand");--i;check(std::uint64_t(p-i->first)+size<=i->second.size(),"Unbacked physical extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{if(logging)reads.push_back(J::array({p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>in)override{const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply physical physical-allocation fixture");const std::filesystem::path path=argv[1];std::ifstream input(path);const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Physical original pin differs");
    unsigned cases{},reads{},stores{},entries{};
    for(const auto &c:fixture.at("cases"))for(bool read_pass:{false,true}) {
        const auto name=c.at("name").get<std::string>();Memory memory;memory.logging=read_pass;
        for(const auto &bank:c.at("banks"))memory.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>()));
        J written=J::array();SourcePhysicsContactRecordObserver observer;
        if(!read_pass)observer=[&](const auto &e){written.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};services.instruction=[&](auto value){
            pc=value;for(const auto &site:c.value("stop_before_sites",J::array()))if(site==pc){required_entry=pc;throw RequiredOwner{};}
        };
        const std::array<std::uint32_t,3>sites{0x38846eu,0x21f590u,0x33b595u};auto &cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.service_sites=sites;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
        const auto &before=c.at("before_abi");abi.gp=before.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=before.at("eflags");
        const auto xmm=unhex(before.at("xmm"));std::memcpy(abi.xmm.data(),xmm.data(),128);abi.x87_status=before.at("fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("fp")[0],before.at("fp")[2]};state.load(abi);
        J observed_entries=J::array();child_entry_observer=[&](auto &value,auto owner){
            if(owner!=0x13fe50u&&owner!=0x16e770u&&owner!=0x2db4cau)return;
            const auto a=value.save();observed_entries.push_back(J{{"entry",owner},{"abi",J{{"gp",a.gp},{"eflags",a.eflags&0xcd5u},
                {"xmm",hex(std::as_bytes(std::span(a.xmm)))},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})}}}});
        };
        stop_owners.clear();for(const auto &owner:c.value("required_entries",J::array()))stop_owners.insert(owner.get<std::uint32_t>());
        bool required{};required_entry=0;try{production::run_12fe00(state);}catch(const RequiredOwner&){required=true;}
        check(required==c.at("required_owner")&&required_entry==c.at("required_entry"),name+" boundary differs");
        auto expected_entries=c.at("entry_abis");for(auto &entry:expected_entries)entry["abi"]["eflags"]=entry["abi"]["eflags"].get<unsigned>()&0xcd5u;
        check(observed_entries==expected_entries,name+" full child-entry ABI differs");
        const auto after=state.save();const auto &expected=c.at("after_abi");
        check(after.gp==expected.at("gp").get<std::array<std::uint32_t,8>>(),name+" complete GP differs");
        check((after.eflags&0xcd5u)==(expected.at("eflags").get<unsigned>()&0xcd5u),name+" flags/DF differ");
        check(hex(std::as_bytes(std::span(after.xmm)))==expected.at("xmm").get<std::string>(),name+" XMM differs");
        check(J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})==expected.at("fp"),name+" FP differs");
        check(pc==c.at("last_instruction"),name+" published cursor differs");
        if(read_pass){
            J expected_reads=J::array();for(const auto &r:c.at("reads"))expected_reads.push_back(J::array({r.at("address"),r.at("bytes")}));
            if(memory.reads!=expected_reads){
                unsigned index{};while(index<memory.reads.size()&&index<expected_reads.size()&&memory.reads[index]==expected_reads[index])++index;
                throw std::runtime_error(name+" ordered reads differ at"+std::to_string(index)+" native="+
                    (index<memory.reads.size()?memory.reads[index].dump():"end")+" original="+
                    (index<expected_reads.size()?c.at("reads")[index].dump():"end"));
            }
            reads+=memory.reads.size();
        }
        else{
            if(written!=c.at("writes")){
                unsigned index{};while(index<written.size()&&index<c.at("writes").size()&&written[index]==c.at("writes")[index])++index;
                throw std::runtime_error(name+" ordered stores differ at"+std::to_string(index)+" native="+
                    (index<written.size()?written[index].dump():"end")+" original="+
                    (index<c.at("writes").size()?c.at("writes")[index].dump():"end"));
            }
            stores+=written.size();entries+=observed_entries.size();++cases;
        }
        for(const auto &bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),name+" whole bank differs");
    }
    const bool reference=std::getenv("HALO2_REFERENCE_PHYSICAL_ALLOCATION");check(reference?translated_fallbacks==2u*cases:translated_fallbacks==0,"Unexpected physical translated fallback");
    std::cout<<J{{"passed",true},{"cases",cases},{"ordered_reads",reads},{"ordered_stores",stores},{"callee_entry_packets",entries},{"translated_body_fallbacks",translated_fallbacks},{"canonical_child_calls",reference_children},
        {"qualified_child_activity",J{{"page_window",native_page_window_detail::activity().calls},{"page_retire",native_page_retire_detail::activity().calls},{"datum_delete",native_datum_delete_detail::activity().calls},{"stack_probe",native_stack_probe_detail::activity().calls}}}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
