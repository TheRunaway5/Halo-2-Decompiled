#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_resource_touch.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
struct ChildBoundary{};struct ReadBoundary{};
namespace copies {
std::set<std::uint32_t> stops;J calls=J::array();
std::uint32_t sign_extend(std::uint32_t value,unsigned width){if(width==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));throw std::runtime_error("Unqualified resource sign extension");}
void before_owner(WorldState&s,std::uint32_t target){
    if(stops.contains(target)){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=target;throw ChildBoundary{};}
    const auto abi=s.save();J live=J::array();constexpr char digits[]="0123456789abcdef";
    for(unsigned n=0;n<abi.x87_depth;++n){std::array<unsigned char,10>raw{};std::memcpy(raw.data(),&abi.x87[n],10);std::string value;for(auto byte:raw){value+=digits[byte>>4];value+=digits[byte&15];}live.push_back(value);}
    calls.push_back(J{{"entry",target},{"arguments",J::array()},{"gp",abi.gp},{"eflags",abi.eflags},{"xmm",abi.xmm},{"live_x87",live},
        {"fp",J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})}});
}
#include "resource_touch_bodies.inc"
}
namespace {
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string&text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J writes=J::array();
    mutable unsigned ordinal{};int fault{-1};
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped resource input");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Resource access escaped bank");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(int(ordinal++)==fault)throw ReadBoundary{};
        reads.push_back(J{{"address",p},{"bytes",out.size()}});const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
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
unsigned refusals(const J&row){
    for(unsigned variant=0;variant<7;++variant){
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto banks=memory.banks;const auto incoming=abi_for(row);bool trace=variant==1;std::uint32_t pc{};
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto at){pc=at;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f590u,0x126200u};
        services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(variant==0)services.instruction_cursor.position=nullptr;
        if(variant==2)services.instruction_cursor.executor={};
        if(variant==3){const auto original=services.instruction;services.instruction=[original](auto at){original(at);};}
        if(variant==4)services.instruction_cursor.service_sites=wrong;
        if(variant==5)services.instruction_cursor.observation_only_stores=false;
        SourcePhysicsContactRecordObserver observe=[](const auto&){};
        WorldState state{memory,observe,services,variant==6?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};state.load(incoming);
        check(!native_resource_touch_detail::bridge(state,[](auto&,auto,auto,bool){throw std::runtime_error("Unqualified resource owner entered");}),"Unsafe resource protocol admitted");
        const auto actual=state.save();check(actual.gp==incoming.gp&&actual.xmm==incoming.xmm&&actual.eflags==incoming.eflags&&memory.banks==banks&&memory.reads.empty()&&memory.writes.empty(),"Resource refusal changed input");
    }return 7;
}
}
int main(int argc,char**argv)try {
    check(argc==2,"Supply physical resource fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);const auto refused=refusals(fixture.at("cases")[0]);
    unsigned comparisons{},entries{},owner_calls{},prefixes{};
    for(const auto&row:fixture.at("cases"))for(unsigned route=0;route<3;++route){
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        if(row.contains("fault"))memory.fault=row.at("fault").at("ordinal");
        copies::stops=row.at("stop_before_entries").get<std::set<std::uint32_t>>();copies::calls=J::array();
        std::uint32_t pc{};SourcePhysicsWorldRuntimeServices services;unsigned parent_callbacks{};
        services.instruction=[&](auto p){pc=p;if(p>=0x126200u&&p<=0x1262adu)++parent_callbacks;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        const auto incoming=abi_for(row);const SourcePhysicsContactRecordObserver observe;WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(incoming);
        std::string stopped="return";try{if(route==0)copies::primary::run_126200(state);else if(route==1)copies::actor::run_126200(state);else copies::frame::run_126200(state);}catch(const ChildBoundary&){stopped="child";++entries;}catch(const ReadBoundary&){stopped="before_read";++prefixes;}
        const auto actual=state.save();const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);
        check(stopped==row.at("stop_kind").get<std::string>(),name+": completion differs");
        if(actual.gp!=row.at("boundary_gp").get<std::array<std::uint32_t,8>>()||actual.eflags!=row.at("boundary_eflags").get<std::uint32_t>())
            throw std::runtime_error(name+": GP/flags differ actual="+J{{"gp",actual.gp},{"flags",actual.eflags},{"pc",pc}}.dump()+" expected="+row.at("boundary_gp").dump()+" flags="+row.at("boundary_eflags").dump());
        check(actual.xmm==row.at("boundary_xmm").get<decltype(actual.xmm)>()&&actual.x87_depth==incoming.x87_depth&&actual.x87_status==row.at("after_fp")[1]&&
            actual.floating_point->x87_control_word==row.at("after_fp")[0]&&actual.floating_point->mxcsr==row.at("after_fp")[2],name+": FP differs");
        for(unsigned n=0;n<actual.x87_depth;++n)check(!std::memcmp(&actual.x87[n],&incoming.x87[n],10),name+": live80 differs");
        check(memory.reads==row.at("reads"),name+": reads differ actual="+memory.reads.dump()+" expected="+row.at("reads").dump());
        check(memory.writes==row.at("writes"),name+": stores differ actual="+memory.writes.dump()+" expected="+row.at("writes").dump());
        check(copies::calls==row.at("calls"),name+": genuine owner entry packets differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address").get<std::uint32_t>()))==bank.at("after").get<std::string>(),name+": bank differs");
        check(pc==row.at("expected_pc").get<std::uint32_t>(),name+": cursor differs");
        const bool forced=std::getenv("HALO2_REFERENCE_RESOURCE_TOUCH");check(forced?parent_callbacks>0:parent_callbacks==0,name+": parent route differs");
        owner_calls+=copies::calls.size();++comparisons;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"genuine_owner_entry_stops",entries},{"complete_owner_entry_packets",owner_calls},{"before_parent_read_stops",prefixes},{"admission_refusals",refused}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
