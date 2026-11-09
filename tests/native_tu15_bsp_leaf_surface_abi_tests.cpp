#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_bsp_leaf_surface.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace copies {
std::uint32_t sign_extend(std::uint32_t v,unsigned width){return std::uint32_t(width==1?std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))):std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
#include "bsp_leaf_surface_bodies.inc"
}
namespace {
void check(bool value,const std::string&name){if(!value)throw std::runtime_error(name);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>v;for(std::size_t n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4u];s+=d[n&15u];}return s;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t*pc{};mutable J reads=J::array();J writes=J::array();
    auto at(std::uint32_t p,std::size_t n)const {
        auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped polygon bank");--i;
        check(std::uint64_t(p-i->first)+n<=i->second.size(),"Polygon access escaped bank");return i;
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override {
        reads.push_back(J::array({*pc,p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override {
        const auto i=at(p,in.size());auto&bank=banks.at(i->first);const auto view=std::span(bank).subspan(p-i->first,in.size());
        writes.push_back(J{{"instruction",*pc},{"address",p},{"before",hex(view)},{"written",hex(in)}});std::ranges::copy(in,view.begin());
    }
};
SourcePhysicsWorldRuntimeAbi input(const J&r){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=r.at("gp");a.eflags=r.at("eflags");a.xmm=r.at("xmm");
    a.floating_point=SourcePhysicsFactoryFloatingPoint{r.at("before_fp")[0].get<std::uint16_t>(),r.at("before_fp")[2].get<std::uint32_t>()};
    a.x87_status=r.at("before_fp")[1];a.x87_depth=r.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto v=bytes(r.at("live_x87")[n]);std::memcpy(&a.x87[n],v.data(),10);}return a;
}
J packet(std::uint32_t pc,const SourcePhysicsWorldRuntimeAbi&a){
    J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"xmm",a.xmm},{"live_x87",live}};
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply original leaf-surface fixture");std::ifstream in(argv[1]);const auto fixture=J::parse(in);unsigned cases{},phases{},refusals{},ordinal{};
    const bool forced=std::getenv("HALO2_BSP_LEAF_SURFACE_REFERENCE_TEST");
    for(const auto&row:fixture.at("cases")){for(unsigned route=forced?2:0;route<(forced?3:15);++route){
        if(forced?ordinal!=72:(route>=8&&ordinal))continue;
        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);Memory memory;std::uint32_t pc{};memory.pc=&pc;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        unsigned instructions{},tree_entries{},polygon_entries{};bool trace{};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto site){pc=site;++instructions;tree_entries+=site==0x2a0230u;polygon_entries+=site==0x1e6e80u;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x21f590u,0x1e6d84u};
        auto&cursor=services.instruction_cursor;cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(route==8)cursor={};if(route==9)trace=true;if(route==10)cursor.executor={};if(route==11)cursor.service_sites=changed;
        if(route==12)cursor.literal_service_sites=false;if(route==13)cursor.observation_only_stores=false;
        if(route==14){const auto original=services.instruction;services.instruction=[original](auto site){original(site);};}
        J stores=J::array();SourcePhysicsContactRecordObserver observe=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};const auto incoming=input(row);state.load(incoming);J packets=J::array();
        const auto child=[](auto&,auto,auto,bool){throw std::runtime_error("Default leaf-surface child escaped native owner");};
        if(forced||route>=8){check(!native_bsp_leaf_surface_detail::bridge(state,child),"Unqualified parent admitted native");check(state.gp==incoming.gp&&memory.writes.empty(),"Parent refusal changed input");}
        if(!route)check(native_bsp_leaf_surface_detail::bridge(state,child,[&](auto&s,auto at){
            auto value=packet(at,s.save());value["arguments"]=J::array();
            if(at==0x1e6e80u)for(unsigned n=0;n<5;++n){const auto address=s.gp[4]+4u+n*4u;const auto bank=memory.at(address,4);std::uint32_t argument{};std::memcpy(&argument,bank->second.data()+address-bank->first,4);value["arguments"].push_back(argument);}
            packets.push_back(std::move(value));}),"Native leaf surface refused");
        else if(route==1)copies::reference::run_1e6cb0(state);
        else if(route==3)copies::bound1::run_1e6cb0(state);
        else if(route==4)copies::bound2::run_1e6cb0(state);
        else if(route==5)copies::retained1::run_1e6cb0(state);
        else if(route==6)copies::retained2::run_1e6cb0(state);
        else if(route==7)copies::fresh_reference::run_1e6cb0(state);
        else copies::bound0::run_1e6cb0(state);
        const bool reference=forced||route==1||route>=5;
        check(reference?instructions>0:!instructions,name+": instruction route differs");const auto actual=state.save();
        check(pc==row.at("final_pc"),name+": final instruction cursor differs");
        if(actual.gp!=row.at("boundary_gp").get<std::array<unsigned,8>>()||actual.eflags!=row.at("boundary_eflags"))throw std::runtime_error(name+": complete GP/flags differ actual="+packet(pc,actual).dump()+" expected_gp="+row.at("boundary_gp").dump()+" flags="+row.at("boundary_eflags").dump());
        check(actual.xmm==row.at("boundary_xmm").get<std::array<std::array<unsigned,4>,8>>(),name+": SIMD differs");
        check(J::array({actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr})==row.at("after_fp"),name+": FP differs actual="+packet(pc,actual).dump()+" expected="+row.at("after_fp").dump());
        check(actual.x87_depth==incoming.x87_depth,name+": live FP depth differs");for(unsigned n=0;n<actual.x87_depth;++n)check(!std::memcmp(&actual.x87[n],&incoming.x87[n],10),name+": live80 payload differs");
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": complete backing differs");
        check(memory.writes==row.at("writes")&&stores==row.at("writes"),name+": ordered stores/observer differ");
        J reads=J::array(),wanted=J::array();for(const auto&e:memory.reads)if(std::ranges::find(fixture.at("explicit_read_sites"),e[0])!=fixture.at("explicit_read_sites").end())reads.push_back(e);
        for(const auto&e:row.at("reads"))wanted.push_back(J::array({e.at("instruction"),e.at("address"),e.at("bytes")}));check(reads==wanted,name+": explicit read protocol differs");
        if(!route){const auto expected=row.at("packets");if(packets!=expected){unsigned n{};while(n<packets.size()&&n<expected.size()&&packets[n]==expected[n])++n;throw std::runtime_error(name+": pre-I/O packet"+std::to_string(n)+" differs actual="+(n<packets.size()?packets[n].dump():"missing")+" expected="+(n<expected.size()?expected[n].dump():"missing"));}phases+=packets.size();}
        if(forced){check(tree_entries&&polygon_entries,"Forced parent control did not execute genuine retained children");std::cout<<J{{"native_refused",true},{"callbacks",instructions},{"last_pc",pc},{"returned_eax",actual.gp[0]},{"genuine_tree_entries",tree_entries},{"genuine_polygon_entries",polygon_entries}}.dump()<<'\n';return 0;}
        if(route>=8)++refusals;else ++cases;
    }++ordinal;}
    std::cout<<J{{"passed",true},{"comparisons",cases},{"pre_io_packets",phases},{"admission_refusals",refusals}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
