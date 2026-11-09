#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_bsp_segment_setup.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
struct InstrumentedState:WorldState {
    using WorldState::WorldState;unsigned literal_calls{};
    void instruction_literal(std::uint32_t pc){++literal_calls;physics_world_runtime_detail::WorldState::instruction_literal(pc);}
};
namespace copies {
using WorldState=InstrumentedState;
std::uint32_t sign_extend(std::uint32_t v,unsigned width){return std::uint32_t(width==1?std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))):std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
#include "segment_setup_bodies.inc"
}
namespace {
void check(bool value,const std::string&name){if(!value)throw std::runtime_error(name);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>v;for(std::size_t n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4u];s+=d[n&15u];}return s;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t*pc{};mutable J reads=J::array();J writes=J::array();std::set<std::size_t>preimages;
    auto at(std::uint32_t p,std::size_t n)const {
        auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped polygon bank");--i;
        check(std::uint64_t(p-i->first)+n<=i->second.size(),"Polygon access escaped bank");return i;
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override {
        reads.push_back(J::array({*pc,p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override {
        check(!reads.empty()&&reads.back()==J::array({*pc,p,in.size()}),"Native writer omitted its observable store preimage");preimages.insert(reads.size()-1);
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
    check(argc==2,"Supply original BSP segment fixture");std::ifstream in(argv[1]);const auto fixture=J::parse(in);unsigned cases{},phases{},refusals{},ordinal{},native_calls{},native_callbacks{},native_literals{},reference_calls{},reference_literals{};
    const bool forced=std::getenv("HALO2_BSP_SEGMENT_SETUP_REFERENCE_TEST");
    const unsigned forced_route=std::getenv("HALO2_BSP_SEGMENT_SETUP_SPECIALIZED_REFERENCE_TEST")?4u:1u;
    for(const auto&row:fixture.at("cases")){for(unsigned route=forced?forced_route:0;route<(forced?forced_route+1u:22u);++route){
        if(forced?ordinal!=256:(route>=15&&ordinal))continue;

        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);Memory memory;std::uint32_t pc{};memory.pc=&pc;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        unsigned instructions{},self_entries{},surface_entries{};bool trace{};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto site){pc=site;++instructions;self_entries+=site==0x1e6980u;surface_entries+=site==0x1e6cb0u;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x21f590u,0x1e68ecu};
        auto&cursor=services.instruction_cursor;cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(route==15)cursor={};if(route==16)trace=true;if(route==17)cursor.executor={};if(route==18)cursor.service_sites=changed;
        if(route==19)cursor.literal_service_sites=false;if(route==20)cursor.observation_only_stores=false;
        if(route==21){const auto original=services.instruction;services.instruction=[original](auto site){original(site);};}
        J stores=J::array();SourcePhysicsContactRecordObserver observe=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        InstrumentedState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};const auto incoming=input(row);state.load(incoming);J packets=J::array();
        const auto child=[](auto&,auto target,auto,bool){throw std::runtime_error("Native BSP segment escaped child owner "+std::to_string(target));};
        if(forced||route>=15){check(!native_bsp_segment_setup_detail::bridge(state,child),"Unqualified setup admitted native");check(packet(0,state.save())==packet(0,incoming)&&memory.writes.empty()&&memory.reads.empty()&&stores.empty()&&!instructions&&!pc,"Setup refusal changed caller input");}
        if(!route)check(native_bsp_segment_setup_detail::bridge(state,child,[&](auto&s,auto at){
            auto value=packet(at,s.save());value["arguments"]=J::array();const auto count=(at==0x1e68e0u||at==0x1e6980u)?4u:((at==0x1e6cb0u||at==0x1e6e80u)?5u:0u);
            for(unsigned n=0;n<count;++n){const auto address=s.gp[4]+4u+n*4u;const auto bank=memory.at(address,4);std::uint32_t argument{};std::memcpy(&argument,bank->second.data()+address-bank->first,4);value["arguments"].push_back(argument);}
            packets.push_back(std::move(value));}),"Native segment refused");
        else if(route==1)copies::bound0::run_1e68e0(state);else if(route==2)copies::bound1::run_1e68e0(state);else if(route==3)copies::bound2::run_1e68e0(state);
        else if(route==4)copies::specialized0::run_1e68e0(state);else if(route==5)copies::specialized1::run_1e68e0(state);else if(route==6)copies::specialized2::run_1e68e0(state);
        else if(route==7)copies::reference0::run_1e68e0(state);else if(route==8)copies::reference1::run_1e68e0(state);else if(route==9)copies::reference2::run_1e68e0(state);else if(route==10)copies::literal_reference1::run_1e68e0(state);else if(route==11)copies::literal_reference2::run_1e68e0(state);
        else if(route==12)copies::specialized_reference0::run_1e68e0(state);else if(route==13)copies::specialized_reference1::run_1e68e0(state);else if(route==14)copies::specialized_reference2::run_1e68e0(state);else copies::bound0::run_1e68e0(state);

        check(forced||route>=7?instructions>0:(!instructions&&!state.literal_calls),name+": literal work or instruction callbacks escaped native");if(forced&&forced_route==4u)check(state.literal_calls>0,"Specialized reference route omitted its literal continuation");const auto actual=state.save();
        if(!route){const auto expected=row.at("packets");if(packets!=expected){unsigned n{};while(n<packets.size()&&n<expected.size()&&packets[n]==expected[n])++n;throw std::runtime_error(name+": packet"+std::to_string(n)+" differs actual="+(n<packets.size()?packets[n].dump():"missing")+" expected="+(n<expected.size()?expected[n].dump():"missing"));}phases+=packets.size();}
        if(actual.gp!=row.at("boundary_gp").get<std::array<unsigned,8>>()||actual.eflags!=row.at("boundary_eflags"))throw std::runtime_error(name+": full GP/flags differ actual="+packet(pc,actual).dump()+" expected_gp="+row.at("boundary_gp").dump()+" expected_flags="+row.at("boundary_eflags").dump());
        check(actual.xmm==row.at("boundary_xmm").get<std::array<std::array<unsigned,4>,8>>(),name+": SIMD differs");
        check(J::array({actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr})==row.at("after_fp"),name+": FP differs");
        check(actual.x87_depth==incoming.x87_depth,name+": live80 depth differs");for(unsigned n=0;n<actual.x87_depth;++n)check(!std::memcmp(&actual.x87[n],&incoming.x87[n],10),name+": live80 differs");
        check(pc==row.at("final_pc"),name+": final cursor differs");
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": complete backing differs");
        check(memory.writes==row.at("writes")&&stores==row.at("writes"),name+": stores/observers differ");
        J reads=J::array(),wanted=J::array();for(std::size_t n=0;n<memory.reads.size();++n){const auto&e=memory.reads[n];if(!memory.preimages.contains(n)&&std::ranges::find(fixture.at("explicit_read_sites"),e[0])!=fixture.at("explicit_read_sites").end())reads.push_back(e);}
        for(const auto&e:row.at("reads"))wanted.push_back(J::array({e.at("instruction"),e.at("address"),e.at("bytes")}));check(reads==wanted,name+": explicit read protocol differs");
        if(forced){std::cout<<J{{"native_refused",true},{"callbacks",instructions},{"literal_evaluations",state.literal_calls},{"self_entries",self_entries},{"surface_entries",surface_entries},{"last_pc",pc},{"returned_eax",actual.gp[0]}}.dump()<<'\n';return 0;}
        if(route>=15)++refusals;else{++cases;if(route<7){++native_calls;native_callbacks+=instructions;native_literals+=state.literal_calls;}else{++reference_calls;reference_literals+=state.literal_calls;}}
    }++ordinal;}
    if(!forced){
        const auto&row=fixture.at("cases")[0];Memory memory;std::uint32_t pc{};memory.pc=&pc;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeServices services;unsigned observed{};SourcePhysicsContactRecordObserver observer=[&](const auto&){++observed;};
        InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input(row));const auto before=packet(0,state.save());
        const auto child=[](auto&,auto,auto,bool){throw std::runtime_error("Unqualified observer escaped setup refusal");};
        check(!native_bsp_segment_setup_detail::bridge(state,child),"Observer without owned cursor admitted native setup");
        check(packet(0,state.save())==before&&memory.reads.empty()&&memory.writes.empty()&&!observed&&!pc,"Observer-only setup refusal changed caller state");++refusals;
    }
    std::cout<<J{{"passed",true},{"comparisons",cases},{"pre_io_packets",phases},{"admission_refusals",refusals},{"native_calls",native_calls},{"native_instruction_callbacks",native_callbacks},{"native_literal_evaluations",native_literals},{"reference_calls",reference_calls},{"reference_literal_evaluations",reference_literals}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
