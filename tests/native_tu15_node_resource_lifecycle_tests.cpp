#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_node_resource_lifecycle.hpp"
#include "physics/runtime/native_tu15_record_lookup.hpp"
#include "physics/runtime/native_tu15_node_resource.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
struct ChildBoundary {};
struct ReadBoundary {};
namespace copies {
std::uint32_t sign_extend(std::uint32_t value,unsigned width){if(width==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));throw std::runtime_error("Unqualified lifecycle sign extension");}
#include "node_resource_lifecycle_bodies.inc"
}
namespace {
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>bytes(const std::string&text){std::vector<std::byte>out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;std::uint32_t*pc{};mutable J reads=J::array();J writes=J::array();
    int fault{-1};mutable unsigned ordinal{};
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped lifecycle input");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Lifecycle access escaped bank");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(int(ordinal++)==fault)throw ReadBoundary{};
        reads.push_back(J{{"address",p},{"bytes",out.size()}});
        const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>data)override{
        const auto i=at(p,data.size());auto&bank=banks.at(i->first);writes.push_back(J{{"address",p},{"before",hex(std::span(bank).subspan(p-i->first,data.size()))},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),bank.begin()+p-i->first);
    }
};
SourcePhysicsWorldRuntimeAbi abi_for(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");const auto fp=row.at("before_fp");
    a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};a.x87_status=fp[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],v.data(),10);}return a;
}
void refusals(const J&row){
    for(unsigned variant=0;variant<7;++variant){
        std::uint32_t pc{};Memory memory;memory.pc=&pc;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto banks=memory.banks;const auto incoming=abi_for(row);bool trace=variant==1;
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto at){pc=at;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f590u,0x1e5c80u};
        services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(variant==0)services.instruction_cursor.position=nullptr;
        if(variant==2)services.instruction_cursor.executor={};
        if(variant==3){const auto original=services.instruction;services.instruction=[original](auto at){original(at);};}
        if(variant==4)services.instruction_cursor.service_sites=wrong;
        if(variant==5)services.instruction_cursor.observation_only_stores=false;
        SourcePhysicsContactRecordObserver observe=[](const auto&){};
        WorldState state{memory,observe,services,variant==6?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};state.load(incoming);
        check(!native_node_resource_lifecycle_detail::bridge(state,[](auto&,auto,auto){throw std::runtime_error("Unqualified lifecycle owner entered");}),"Unsafe lifecycle protocol admitted");
        const auto actual=state.save();check(actual.gp==incoming.gp&&actual.xmm==incoming.xmm&&actual.eflags==incoming.eflags&&memory.banks==banks&&memory.reads.empty()&&memory.writes.empty(),"Lifecycle admission refusal changed input");
    }
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply physical lifecycle fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);unsigned comparisons{},entries{};refusals(fixture.at("cases")[0]);
    for(const auto&row:fixture.at("cases"))for(unsigned route=0;route<4;++route){
        std::uint32_t pc{};Memory memory;memory.pc=&pc;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        if(row.contains("fault"))memory.fault=row.at("fault").at("ordinal");
        auto incoming=abi_for(row);SourcePhysicsWorldRuntimeServices services;unsigned parent_callbacks{},other_callbacks{};
        services.instruction=[&](auto p){pc=p;if(p>=0x1e5c80u&&p<=0x1e5d5du)++parent_callbacks;else ++other_callbacks;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observe;WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(incoming);
        std::string stopped="return";try{if(route==0)copies::primary::run_1e5c80(state);else if(route==1)copies::frame::run_1e5c80(state);else if(route==2)copies::actor::run_1e5c80(state);else copies::literal::run_1e5c80(state);}catch(const ChildBoundary&){stopped="child";++entries;}catch(const ReadBoundary&){stopped="before_read";}
        const auto actual=state.save();const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);
        check(stopped==row.at("stop_kind").get<std::string>(),name+": completion differs");
        if(actual.gp!=row.at("boundary_gp").get<std::array<std::uint32_t,8>>()||actual.eflags!=row.at("boundary_eflags").get<std::uint32_t>())
            throw std::runtime_error(name+": GP/flags differ actual="+J{{"gp",actual.gp},{"flags",actual.eflags},{"pc",pc}}.dump()+" expected="+row.at("boundary_gp").dump()+" flags="+row.at("boundary_eflags").dump());
        check(actual.xmm==incoming.xmm&&actual.x87_depth==incoming.x87_depth&&actual.x87_status==incoming.x87_status&&
            actual.floating_point->x87_control_word==incoming.floating_point->x87_control_word&&actual.floating_point->mxcsr==incoming.floating_point->mxcsr,name+": FP changed");
        for(unsigned n=0;n<actual.x87_depth;++n)check(!std::memcmp(&actual.x87[n],&incoming.x87[n],10),name+": live80 changed");
        check(memory.reads==row.at("reads"),name+": reads differ actual="+memory.reads.dump()+" expected="+row.at("reads").dump());
        check(memory.writes==row.at("writes"),name+": stores differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address").get<std::uint32_t>()))==bank.at("after").get<std::string>(),name+": bank differs");
        check(pc==row.at("expected_pc").get<std::uint32_t>(),name+": cursor differs");
        const bool forced=std::getenv("HALO2_REFERENCE_NODE_RESOURCE_LIFECYCLE");check(forced?parent_callbacks>0:parent_callbacks==0,name+": parent routing differs");
        check(other_callbacks==0,name+": native child fell back");++comparisons;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"genuine_126200_entries",entries},{"admission_refusals",7}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
