#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_physical_callbacks_abi.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>

namespace halo2 {
using namespace physics_world_runtime_detail;
inline unsigned translated_fallbacks{};inline std::uint32_t required_entry{};
struct RequiredOwner{};
inline std::function<void(WorldState&,std::uint32_t)> child_entry_observer;
inline std::function<void(WorldState&,std::uint32_t)> test_provider;
void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){
    if(child_entry_observer)child_entry_observer(s,target);
    if(test_provider){test_provider(s,target);return;}
    required_entry=target;throw RequiredOwner{};
}
namespace reference_owner {
#include "physical_callbacks_reference.inc"
}
namespace production {
#include "physical_callbacks_body.inc"
}
}

namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s){std::vector<std::byte>out;for(unsigned n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> file(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary);check(bool(f),"Missing owned physical-callback bank");std::vector<char>raw((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b(raw.size());for(unsigned n=0;n<raw.size();++n)b[n]=std::byte(std::uint8_t(raw[n]));return b;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;mutable J reads=J::array();bool logging{};
    auto at(std::uint32_t p,std::size_t size)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked physical operand");--i;check(std::uint64_t(p-i->first)+size<=i->second.size(),"Unbacked physical extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{if(logging)reads.push_back(J::array({p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>in)override{const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);}
};
J snapshot(WorldState &state){const auto a=state.save();return J{{"gp",a.gp},{"eflags",a.eflags&0xcd5u},
    {"xmm",hex(std::as_bytes(std::span(a.xmm)))},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})}};}
struct SeamResult {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    J reads,writes,entries,abi;std::uint32_t pc{};std::set<std::uint32_t> sites;
};
SeamResult seam(bool canonical,bool read_pass,unsigned alignment,unsigned policy){
    Memory memory;memory.banks.emplace(0x2000000u,std::vector<std::byte>(0x10000));
    memory.banks.emplace(0x4000000u,std::vector<std::byte>(0x10000));memory.banks.emplace(0x4f2d2cu,std::vector<std::byte>(4));
    const auto put=[&](auto at,auto value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));memory.write(at,b);};
    const auto count=alignment%4u,sp=0x200f000u+alignment;
    put(sp,0x5000000u);put(0x4f2d2cu,count?0x4001000u:0u);
    for(unsigned index=0;index<count;++index){const auto node=0x4001000u+index*64u;
        put(node+8u,0x4003000u+index*32u);put(node+16u,0x100000u+index);put(node+20u,0x12f910u);put(node+32u,index+1u<count?node+64u:0u);}
    memory.logging=read_pass;J writes=J::array(),entries=J::array();SourcePhysicsContactRecordObserver observe;
    if(!read_pass)observe=[&](const auto &e){writes.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
    SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};std::set<std::uint32_t> sites;
    services.instruction=[&](auto value){pc=value;sites.insert(value);};const std::array<std::uint32_t,3>service_sites{0x38846eu,0x21f590u,0x33b595u};auto &cursor=services.instruction_cursor;
    cursor.position=&pc;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.service_sites=service_sites;
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp={0x12345678,0x87654321,0xa5a5a5a5,0x11223344,sp,0x55667788,0x99887766,0x66554433};
    abi.eflags=(alignment&1u)?0x402u:2u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x27fu|((alignment%4u)<<10u)),0x1f80u|((alignment%4u)<<13u)};state.load(abi);
    child_entry_observer=[&](auto &value,auto owner){entries.push_back(J{{"entry",owner},{"abi",snapshot(value)}});};
    test_provider=[&](auto &value,auto owner){
        check(owner==0x12f910u,"Unexpected declared callback test owner");auto &g=value.gp;
        const auto address=physics_step_detail::r(value.memory,g[4]+4u,4),context=physics_step_detail::r(value.memory,g[4]+8u,4);
        const auto current=g[6];const auto read=[&](auto at){return physics_step_detail::r(value.memory,at,4);};
        const auto store=[&](auto at,auto v){physics_step_detail::w(value.memory,value.observe,0xfe0000u+policy,at,4,v);};
        if(policy==1u){const auto next=read(current+32u),head=read(0x4f2d2cu);
            if(head==current)store(0x4f2d2cu,next);
            else for(auto at=head;at;at=read(at+32u))if(read(at+32u)==current){store(at+32u,next);break;}}
        if(policy==2u)store(0x4f2d2cu,0u);
        if(policy==3u){const auto head=read(0x4f2d2cu);store(0x4f2d2cu,head?read(head+32u):0u);}
        if(policy==4u){const auto next=read(current+32u);store(0x4f2d2cu,next?read(next+32u):0u);}
        if(policy==5u)store(g[4]+12u,current^context); // Real saved-ESI caller-cell alias.
        g[0]=value.flags.plus(address,context,4);g[1]=value.flags.logic(address^context,4);g[2]=value.flags.minus(address,context,4);
        value.xmm[0]=_mm_castsi128_ps(_mm_set_epi32(context,address,current,policy));value.pop();g[4]+=8u;
    };
    if(canonical)reference_owner::run_12ef10(state);else production::run_12ef10(state);
    test_provider={};child_entry_observer={};return{memory.banks,memory.reads,writes,entries,snapshot(state),pc,sites};
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply physical-callback fixture");const std::filesystem::path path=argv[1];std::ifstream input(path);const auto fixture=J::parse(input);
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
            const auto a=value.save();observed_entries.push_back(J{{"entry",owner},{"abi",J{{"gp",a.gp},{"eflags",a.eflags&0xcd5u},
                {"xmm",hex(std::as_bytes(std::span(a.xmm)))},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})}}}});
        };
        bool required{};required_entry=0;try{production::run_12ef10(state);}catch(const RequiredOwner&){required=true;}
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
    unsigned seam_transactions{},seam_entries{},seam_reads{},seam_stores{};std::set<std::uint32_t> seam_sites;
    for(unsigned alignment=0;alignment<16;++alignment)for(unsigned policy=0;policy<6;++policy)for(bool read_pass:{false,true}){
        const auto expected=seam(true,read_pass,alignment,policy),actual=seam(false,read_pass,alignment,policy);
        check(actual.banks==expected.banks,"Callback seam whole banks differ");check(actual.abi==expected.abi,"Callback seam complete ABI differs");
        check(actual.entries==expected.entries,"Callback seam complete child-entry ABI differs");check(actual.pc==expected.pc,"Callback seam cursor differs");
        check(actual.reads==expected.reads,"Callback seam ordered reads differ");check(actual.writes==expected.writes,"Callback seam ordered stores differ");
        ++seam_transactions;seam_entries+=actual.entries.size();seam_reads+=actual.reads.size();seam_stores+=actual.writes.size();seam_sites.insert(expected.sites.begin(),expected.sites.end());
    }
    check(seam_sites.size()==31u,"Callback canonical seam did not execute the whole31-site source body");
    const bool reference=std::getenv("HALO2_REFERENCE_PHYSICAL_CALLBACKS");check(reference?translated_fallbacks==2u*cases+seam_transactions:translated_fallbacks==0,"Unexpected physical translated fallback");
    std::cout<<J{{"passed",true},{"cases",cases},{"ordered_reads",reads},{"ordered_stores",stores},{"callee_entry_packets",entries},{"translated_body_fallbacks",translated_fallbacks},
        {"canonical_seam",J{{"transactions",seam_transactions},{"callee_entry_packets",seam_entries},{"ordered_reads",seam_reads},{"ordered_stores",seam_stores},{"source_sites",seam_sites.size()}}}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
