#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_bsp_surface_polygon.hpp"
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace copies {
std::uint32_t sign_extend(std::uint32_t v,unsigned width){return std::uint32_t(width==1?std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))):std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
#include "bsp_surface_polygon_bodies.inc"
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
int main(int argc,char**argv)try {
    check(argc==2,"Supply polygon hardware fixture");std::ifstream in(argv[1]);const auto fixture=J::parse(in);unsigned count{},phases{},refusals{},ordinal{};
    const bool forced=std::getenv("HALO2_BSP_POLYGON_REFERENCE_TEST");
    for(const auto&row:fixture.at("cases")){
      for(unsigned route=forced?2:0;route<(forced?3:11);++route){
        if((forced||route>=4)&&ordinal)continue;
        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);Memory memory;std::uint32_t pc{};memory.pc=&pc;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        unsigned instructions{};bool trace{};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto site){pc=site;++instructions;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x21f590u,0x1e6f13u};
        auto&cursor=services.instruction_cursor;cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(route==4)cursor={};if(route==5)trace=true;if(route==6)cursor.executor={};if(route==7)cursor.service_sites=changed;
        if(route==8)cursor.literal_service_sites=false;if(route==9)cursor.observation_only_stores=false;
        if(route==10){const auto original=services.instruction;services.instruction=[original](auto site){original(site);};}
        J stores=J::array();SourcePhysicsContactRecordObserver observe=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};const auto incoming=input(row);state.load(incoming);J packets=J::array();
        const auto child=[](auto&,auto,auto,bool){throw std::runtime_error("Polygon requested a child owner");};
        if(forced||route>=4){
            check(!native_bsp_surface_polygon_detail::bridge(state),"Unqualified polygon admitted native");
            check(state.gp==incoming.gp&&memory.writes.empty(),"Polygon refusal changed input");
            if(forced)check(!native_allocator_owner_detail::dispatch(state,0x1e6e80u,child)&&native_allocator_owner_detail::activity().bsp_polygon_entries.empty(),"Forced polygon reference entered actual owner");
        }
        if(!route)check(native_bsp_surface_polygon_detail::bridge(state,[&](auto&s,auto at){packets.push_back(packet(at,s.save()));}),"Native polygon unexpectedly refused");
        else if(route==1)copies::reference::run_1e6e80(state);
        else if(route==3){
            auto&a=native_allocator_owner_detail::activity();const auto previous=a.bsp_polygon_entries[0x1e6e80u];
            check(native_allocator_owner_detail::dispatch(state,0x1e6e80u,child),"Actual polygon owner refused");
            check(a.bsp_polygon_entries[0x1e6e80u]==previous+1&&a.entries.empty()&&a.tick_entries.empty()&&a.epoch_entries.empty()&&a.bsp_entries.empty()&&a.delete_entries.empty(),"Actual polygon activity transport changed");
        }else copies::bound::run_1e6e80(state);
        const bool reference=forced||route==1||route>=4;
        check(reference?instructions>0:instructions==0,name+": wrong native/reference instruction route");const auto actual=state.save();
        if(actual.gp!=row.at("boundary_gp").get<std::array<std::uint32_t,8>>()||actual.eflags!=row.at("boundary_eflags"))throw std::runtime_error(name+": GP/flags mismatch "+packet(pc,actual).dump()+" expected_gp="+row.at("boundary_gp").dump()+" expected_flags="+row.at("boundary_eflags").dump());
        check(actual.xmm==row.at("boundary_xmm").get<std::array<std::array<std::uint32_t,4>,8>>(),name+": SIMD mismatch");
        check(J::array({actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr})==row.at("after_fp"),name+": FP controls/status mismatch");
        check(actual.x87_depth==incoming.x87_depth,name+": live FP depth mismatch");for(unsigned n=0;n<actual.x87_depth;++n)check(!std::memcmp(&actual.x87[n],&incoming.x87[n],10),name+": live80 payload mismatch");
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": complete backing mismatch");
        check(memory.writes==row.at("writes")&&stores==row.at("writes"),name+": ordered stores/observers mismatch");
        J explicit_reads=J::array(),wanted_reads=J::array();
        for(const auto&e:memory.reads)if(std::ranges::find(fixture.at("explicit_read_sites"),e[0])!=fixture.at("explicit_read_sites").end())explicit_reads.push_back(e);
        for(const auto&e:row.at("reads"))wanted_reads.push_back(J::array({e.at("instruction"),e.at("address"),e.at("bytes")}));
        check(explicit_reads==wanted_reads,name+": physical explicit operand read order mismatch");
        if(!route){
            const auto expected=row.at("packets");if(packets!=expected){unsigned first{};while(first<packets.size()&&first<expected.size()&&packets[first]==expected[first])++first;
                throw std::runtime_error(name+": pre-I/O packet mismatch "+std::to_string(first)+" actual="+(first<packets.size()?packets[first].dump():"missing")+" expected="+(first<expected.size()?expected[first].dump():"missing"));}
            phases+=packets.size();
        }
        if(forced){std::cout<<J{{"native_refused",true},{"callbacks",instructions},{"last_pc",pc},{"returned_eax",actual.gp[0]}}.dump()<<'\n';return 0;}
        if(route>=4)++refusals;else ++count;
      }
      ++ordinal;
    }
    std::cout<<J{{"passed",true},{"comparisons",count},{"pre_io_full_abi_packets",phases},{"admission_refusals",refusals}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
