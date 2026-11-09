#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_halfword_bit_query.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
namespace copies {
unsigned fallback_calls{};
#include "halfword_bit_query_bodies.inc"
}
namespace {
struct ReadBoundary {};
void check(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&text){std::vector<std::byte>out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
J packet(const WorldState&s,std::uint32_t pc){
    const auto a=s.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
}
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;std::uint32_t*pc{};WorldState*state{};
    mutable J reads=J::array(),packets=J::array();int fault{-1};mutable unsigned ordinal{};
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped halfword input");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Halfword access escaped bank");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(state)packets.push_back(packet(*state,*pc));
        if(int(ordinal++)==fault)throw ReadBoundary{};
        reads.push_back(J{{"instruction",*pc},{"address",p},{"bytes",out.size()}});
        const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Bit query unexpectedly wrote source memory");}
};
SourcePhysicsWorldRuntimeAbi incoming(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");const auto fp=row.at("before_fp");
    a.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(),fp[2].get<std::uint32_t>()};a.x87_status=fp[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],v.data(),10);}return a;
}
void refusals(const J&row){
    for(unsigned variant=0;variant<6;++variant){
        std::uint32_t pc{};Memory memory;memory.pc=&pc;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto at){pc=at;};bool trace=variant==0;constexpr std::array ordinary{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f590u,0xfa5d7u};
        services.instruction_cursor={&pc,&trace,nullptr,ordinary,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(variant==1)services.instruction_cursor.executor={};
        if(variant==2){auto original=services.instruction;services.instruction=[original](auto at){original(at);};}
        if(variant==3)services.instruction_cursor.position=nullptr;
        if(variant==4)services.instruction_cursor.service_sites=wrong;
        WorldState state{memory,{},services,variant==5?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};const auto before=incoming(row);state.load(before);
        check(!native_halfword_bit_query_detail::bridge(state),"Unsafe halfword protocol admitted");
        const auto after=state.save();check(after.gp==before.gp&&after.eflags==before.eflags&&memory.reads.empty(),"Halfword refusal changed input");
    }
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply physical bit-query fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);refusals(fixture.at("cases")[0]);
    unsigned comparisons{},packets{};const bool forced=std::getenv("HALO2_REFERENCE_HALFWORD_BIT_QUERY");
    for(const auto&row:fixture.at("cases"))for(unsigned route=0;route<3;++route){
        std::uint32_t pc{};Memory memory;memory.pc=&pc;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        if(row.contains("fault"))memory.fault=row.at("fault").at("ordinal");
        SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};services.instruction=[&](auto at){pc=at;++callbacks;};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        bool trace=forced;services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        WorldState state{memory,{},services,SourcePhysicsContactImage::TitleUpdate15};const auto initial=incoming(row);state.load(initial);memory.state=&state;
        const auto previous_fallbacks=copies::fallback_calls;bool stopped{};try{
            if(route==0){if(!native_halfword_bit_query_detail::bridge(state)){++copies::fallback_calls;copies::reference::run_fa5d0(state);}}
            else if(route==1)copies::primary::run_fa5d0(state);
            else copies::specialized::run_fa5d0(state);
        }catch(const ReadBoundary&){stopped=true;}
        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);
        check(stopped==!row.at("completed").get<bool>(),name+": completion differs");
        const auto actual=packet(state,pc);const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")}};
        check(actual==expected,name+": full return/prefix packet differs actual="+actual.dump()+" expected="+expected.dump());
        check(memory.packets==row.at("calls"),name+": pre-I/O packets differ actual="+memory.packets.dump()+" expected="+row.at("calls").dump());
        check(memory.reads==row.at("reads"),name+": architectural reads differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address").get<std::uint32_t>()))==bank.at("after").get<std::string>(),name+": source bank changed");
        check(forced?callbacks>0:callbacks==0,name+": default/reference routing differs");++comparisons;packets+=memory.packets.size();
        check(copies::fallback_calls-previous_fallbacks==unsigned(forced),name+": actual retained continuation differs");
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"admission_refusals",6},{"native_calls",native_halfword_bit_query_detail::activity().calls}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
