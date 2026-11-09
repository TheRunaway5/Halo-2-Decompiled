#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "platform/native_tu15_kernel_yield.hpp"
#include "halo2/source_kernel_yield_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
namespace reference {
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}
#include "kernel_yield_reference.inc"
}
namespace {
void check(bool b,const std::string&why){if(!b)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string&v){std::vector<std::byte>r;for(std::size_t n=0;n<v.size();n+=2)r.push_back(std::byte(std::stoul(v.substr(n,2),nullptr,16)));return r;}
std::string hex(std::span<const std::byte>v){std::string r;constexpr char digits[]="0123456789abcdef";for(auto b:v){const auto n=std::to_integer<unsigned>(b);r+=digits[n>>4];r+=digits[n&15];}return r;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J writes=J::array();
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Yield input unmapped");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Yield input escaped bank");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{reads.push_back(J{{"address",p},{"bytes",out.size()}});const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>v)override{const auto i=at(p,v.size());auto&bank=banks.at(i->first);writes.push_back(J{{"address",p},{"before",hex(std::span(bank).subspan(p-i->first,v.size()))},{"written",hex(v)}});std::copy(v.begin(),v.end(),bank.begin()+p-i->first);}
    J saved()const{J result=J::array();for(const auto&[address,v]:banks)result.push_back(J{{"address",address},{"before",hex(v)}});return result;}
};
SourcePhysicsWorldRuntimeAbi abi_for(const J&r){SourcePhysicsWorldRuntimeAbi a{};a.gp=r.at("gp");a.eflags=r.at("eflags");a.xmm=r.at("xmm");const auto fp=r.at("before_fp");a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};a.x87_status=fp[1];a.x87_depth=r.at("live_x87").size();for(unsigned n=0;n<a.x87_depth;++n){const auto raw=bytes(r.at("live_x87")[n]);std::memcpy(&a.x87[n],raw.data(),10);}return a;}
J saved_abi(const SourcePhysicsWorldRuntimeAbi&a){J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));return J{{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"before_fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};}
unsigned provider_controls(const J&r){
    Memory memory;for(const auto&b:r.at("banks"))memory.banks.emplace(b.at("address"),bytes(b.at("before")));auto abi=abi_for(r);const auto incoming=saved_abi(abi);
    for(const SourcePhysicsContactRuntimeCall call:std::array{SourcePhysicsContactRuntimeCall{0x2dbf74u,0x800000eeu,true},SourcePhysicsContactRuntimeCall{0x2dbf75u,0x800000eeu,false},SourcePhysicsContactRuntimeCall{0x2dbf74u,0x800000efu,false}}){
        check(!source_try_kernel_yield_tu15(memory,call,abi),"Unowned platform yield call admitted");check(saved_abi(abi)==incoming&&memory.reads.empty()&&memory.writes.empty(),"Platform refusal changed ABI/DATA");
    }
    bool rejected=false;try{source_try_kernel_yield_tu15(memory,SourcePhysicsContactRuntimeCall{0x2dbf74u,0x800000eeu,false},abi);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected&&saved_abi(abi)==incoming&&memory.reads.size()==1&&memory.writes.empty(),"Malformed platform yield frame changed ABI");
    return 4;
}
unsigned refusals(const J&r){unsigned count{};for(unsigned v=0;v<7;++v){Memory m;for(const auto&b:r.at("banks"))m.banks.emplace(b.at("address"),bytes(b.at("before")));const auto incoming=abi_for(r);const auto banks=m.banks;std::uint32_t pc{};bool trace=v==1;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){pc=p;};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f590u,0x2dbf74u};services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};if(v==0)services.instruction_cursor.position=nullptr;if(v==2)services.instruction_cursor.executor={};if(v==3)services.instruction_cursor.callback_type=nullptr;if(v==4)services.instruction_cursor.service_sites=wrong;if(v==5)services.instruction_cursor.observation_only_stores=false;SourcePhysicsContactRecordObserver observer=[](const auto&){};WorldState s{m,observer,services,v==6?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};s.load(incoming);check(!native_kernel_yield_detail::bridge(s,[](auto&,auto,auto,bool){throw std::runtime_error("Refused yield called import");}),"Unsafe yield admitted");check(saved_abi(s.save())==saved_abi(incoming)&&m.banks==banks&&m.reads.empty()&&m.writes.empty(),"Yield refusal changed input");++count;}return count;}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply yield input");std::ifstream input(argv[1]);const auto fixture=J::parse(input);const auto refused=refusals(fixture.at("cases")[0]);const auto platform_controls=provider_controls(fixture.at("cases")[0]);J results=J::array();
    for(const auto&r:fixture.at("cases"))for(unsigned route=0;route<2;++route){
        Memory memory;for(const auto&b:r.at("banks"))memory.banks.emplace(b.at("address"),bytes(b.at("before")));std::uint32_t pc{};unsigned callbacks{},imports{};J phase;
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){pc=p;++callbacks;};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        services.required_owner=[&](auto&m,const auto&call,auto&a,const auto&){++imports;phase=saved_abi(a);phase["banks"]=memory.saved();phase["reads"]=memory.reads;phase["writes"]=memory.writes;phase["call"]={{"instruction",call.instruction},{"target",call.target},{"tail",call.tail}};const auto reads=memory.reads.size();check(source_try_kernel_yield_tu15(m,call,a),"Actual yield import refused "+r.at("name").get<std::string>()+" route="+std::to_string(route)+" "+phase["call"].dump());phase["after"]=saved_abi(a);phase["service_reads"]=J::array();for(auto n=reads;n<memory.reads.size();++n)phase["service_reads"].push_back(memory.reads[n]);};
        const SourcePhysicsContactRecordObserver observer;WorldState s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi_for(r));
        const bool forced=std::getenv("HALO2_REFERENCE_KERNEL_YIELD");
        if(route)reference::run_2dbf74(s);else if(!native_kernel_yield_detail::bridge(s,[](auto&v,auto target,auto site,bool tail){v.required(site,target,tail);})){check(forced,"Native yield declined");reference::run_2dbf74(s);}
        check(imports==1,"Yield wrapper lost actual import");check((route||forced)?callbacks==6:callbacks==0,"Yield callback route differs");
        auto result=saved_abi(s.save());result["banks"]=memory.saved();result["reads"]=memory.reads;result["writes"]=memory.writes;result["phase"]=phase;result["pc"]=pc;result["route"]=route;results.push_back(std::move(result));
    }
    std::cout<<J{{"admission_refusals",refused},{"provider_controls",platform_controls},{"cases",results}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
