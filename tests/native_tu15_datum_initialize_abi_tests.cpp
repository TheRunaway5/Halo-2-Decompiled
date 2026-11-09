#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_datum_specific.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace copies {
std::uint32_t sign_extend(std::uint32_t v,unsigned width){return std::uint32_t(width==1?std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))):std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
#include "datum_initialize_bodies.inc"
}
namespace {
void check(bool value,const std::string&name){if(!value)throw std::runtime_error(name);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>v;for(std::size_t n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4u];s+=d[n&15u];}return s;}
struct DeclaredStop {};
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t*pc{};mutable J reads=J::array();J writes=J::array();std::set<std::size_t>preimages;bool observer_required{true};std::string fault;unsigned ordinal{},write_count{};mutable unsigned read_count{};
    auto at(std::uint32_t p,std::size_t n)const {
        auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped polygon bank");--i;
        check(std::uint64_t(p-i->first)+n<=i->second.size(),"Polygon access escaped bank");return i;
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override {
        if(fault=="before_read"&&read_count++==ordinal)throw DeclaredStop{};
        reads.push_back(J::array({*pc,p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override {
        if(fault=="before_write"&&write_count++==ordinal)throw DeclaredStop{};
        if(observer_required)check(!reads.empty()&&reads.back()==J::array({*pc,p,in.size()}),"Native writer omitted its observable store preimage");if(observer_required)preimages.insert(reads.size()-1);
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
    check(argc==2,"Supply original datum initializer fixture");std::ifstream in(argv[1]);const auto fixture=J::parse(in);unsigned cases{},phases{},refusals{},ordinal{};const bool forced=std::getenv("HALO2_DATUM_INITIALIZE_REFERENCE_TEST");
    for(const auto&row:fixture.at("cases")){for(unsigned route=forced?1:0;route<(forced?2:(row.contains("fault")?8:(ordinal?8:18)));++route){if(forced&&ordinal)continue;if(row.contains("fault")&&route>=4&&route<=6)continue;
        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);Memory memory;std::uint32_t pc{};memory.pc=&pc;const bool prefix=row.contains("fault");
        if(prefix){memory.fault=row.at("fault").at("phase");memory.ordinal=row.at("fault").at("ordinal");memory.observer_required=memory.fault=="after_write";}
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        unsigned instructions{},parent_entries{},child_entries{};bool trace{};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto site){pc=site;++instructions;parent_entries+=site==0x16e770u;child_entries+=site==0x16e860u;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x21f590u,0x16e860u};
        services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(route==8)services.instruction_cursor={};if(route==9)trace=true;if(route==10)services.instruction_cursor.executor={};if(route==11)services.instruction_cursor.service_sites=changed;
        if(route==12)services.instruction_cursor.literal_service_sites=false;if(route==13)services.instruction_cursor.observation_only_stores=false;
        if(route==14){const auto original=services.instruction;services.instruction=[original](auto site){original(site);};}if(route==15)services.instruction={};
        J stores=J::array();SourcePhysicsContactRecordObserver observe;if(memory.observer_required)observe=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});if(memory.fault=="after_write"&&stores.size()-1==memory.ordinal)throw DeclaredStop{};};
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};const auto incoming=input(row);state.load(incoming);J packets=J::array();
        if(route==16)state.image=SourcePhysicsContactImage::MapPack11;if(route==17){state.active_mmx.emplace();state.active_mmx->known_mask=1;}
        if(forced||route>=8){const auto old=packet(pc,state.save());check(!native_datum_specific_detail::initialize(state),"Unqualified native specific admitted");check(packet(pc,state.save())==old&&memory.reads.empty()&&memory.writes.empty(),"Specific refusal changed full caller input");if(route>=8){++refusals;continue;}}
        bool stopped{};try{if(!route)check(native_datum_specific_detail::initialize(state,[&](auto&s,auto at){auto value=packet(at,s.save());value["arguments"]=J::array();packets.push_back(std::move(value));}),"Native initializer refused");
        else if(route==1)copies::bound0::run_16e860(state);else if(route==2)copies::bound1::run_16e860(state);else if(route==3)copies::bound2::run_16e860(state);else if(route==4)copies::reference0::run_16e860(state);else if(route==5)copies::reference1::run_16e860(state);else if(route==6)copies::reference2::run_16e860(state);else copies::partial::run_16e860(state);
        }catch(const DeclaredStop&){stopped=true;}check(stopped==prefix,name+": declared prefix did not stop");
        check(forced||(route>=4&&route<=6)?instructions>0:!instructions,name+": callback route differs");const auto actual=state.save();
        if(!route){const auto expected=row.at("packets");if(packets!=expected){unsigned n{};while(n<packets.size()&&n<expected.size()&&packets[n]==expected[n])++n;throw std::runtime_error(name+": packet"+std::to_string(n)+" differs actual="+(n<packets.size()?packets[n].dump():"missing")+" expected="+(n<expected.size()?expected[n].dump():"missing"));}phases+=packets.size();}
        if(actual.gp!=row.at("boundary_gp").get<std::array<unsigned,8>>()||actual.eflags!=row.at("boundary_eflags"))throw std::runtime_error(name+": full GP/flags differ actual="+packet(pc,actual).dump()+" expected_gp="+row.at("boundary_gp").dump()+" expected_flags="+row.at("boundary_eflags").dump());
        check(actual.xmm==row.at("boundary_xmm").get<std::array<std::array<unsigned,4>,8>>(),name+": SIMD differs");
        check(J::array({actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr})==row.at("after_fp"),name+": FP differs");
        check(actual.x87_depth==incoming.x87_depth,name+": live80 depth differs");for(unsigned n=0;n<actual.x87_depth;++n)check(!std::memcmp(&actual.x87[n],&incoming.x87[n],10),name+": live80 differs");
        check(pc==row.at("final_pc"),name+": final cursor differs");
        for(const auto&bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": complete backing differs");
        check(memory.writes==row.at("writes")&&(!memory.observer_required||stores==row.at("writes")),name+": stores/observers differ");
        J reads=J::array(),wanted=J::array();for(std::size_t n=0;n<memory.reads.size();++n){const auto&e=memory.reads[n];if(!memory.preimages.contains(n)&&std::ranges::find(fixture.at("explicit_read_sites"),e[0])!=fixture.at("explicit_read_sites").end())reads.push_back(e);}
        if(memory.fault=="before_write"&&!reads.empty()&&reads.back()[0]==row.at("boundary").at("instruction")&&reads.back()[1]==row.at("boundary").at("address"))reads.erase(reads.size()-1);
        for(const auto&e:row.at("reads"))wanted.push_back(J::array({e.at("instruction"),e.at("address"),e.at("bytes")}));check(reads==wanted,name+": architectural read protocol differs");
        if(forced){std::cout<<J{{"native_refused",true},{"callbacks",instructions},{"parent_entries",parent_entries},{"child_entries",child_entries},{"last_pc",pc},{"returned_eax",actual.gp[0]}}.dump()<<'\n';return 0;}++cases;
    }++ordinal;}
    std::cout<<J{{"admission_refusals",refusals},{"passed",true},{"comparisons",cases},{"pre_io_packets",phases}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
