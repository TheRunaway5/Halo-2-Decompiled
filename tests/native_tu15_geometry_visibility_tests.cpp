#include "rendering/native_tu15_geometry_visibility.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;using J=nlohmann::json;using State=physics_world_runtime_detail::WorldState;
struct InstrumentedState:State {using Base=physics_world_runtime_detail::WorldState;using Base::Base;bool selected_observation_pcs{true};unsigned literal_calls{};void instruction_literal(std::uint32_t pc){++literal_calls;Base::instruction_literal(pc);}};
struct RetainedStop{};unsigned retained_statements{};bool stop_at_retained{};
void retained_statement(){++retained_statements;if(stop_at_retained)throw RetainedStop{};}
std::function<void(State&,std::uint32_t)> capture_child;
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "geometry_visibility_bodies.inc"
}
namespace {
struct Fault{};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>unhex(const std::string&s){std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
J packet(State&s,std::uint32_t pc){const auto a=s.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));J result{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};if(a.active_mmx){J physical=J::array();unsigned tag{};for(unsigned n=0;n<8;++n){const auto&raw=a.active_mmx->physical[n];physical.push_back(hex(raw));tag|=native_raw_x87_detail::value_tag(raw.data())<<(n*2u);}result["active_mmx"]=J{{"tag_word",tag},{"physical",physical},{"known_mask",a.active_mmx->known_mask}};}return result;}
void hostile(){const std::uint16_t unmasked=0x37e;asm volatile("fninit;fldz;fldz;fdivp;fldcw %0"::"m"(unmasked):"memory","st","st(1)");_mm_setcsr(0x8065);}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable std::uint32_t pc{};mutable J reads=J::array(),writes=J::array(),packets=J::array();
    std::set<std::uint32_t>write_sites;std::function<J()>capture;std::optional<J>fault;bool noisy{};mutable unsigned read_count{},write_count{};
    std::byte*at(std::uint32_t address,std::size_t size)const{auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unmapped visibility association data");--it;check(std::uint64_t(address-it->first)+size<=it->second.size(),"Visibility association data escaped backing");return const_cast<std::byte*>(it->second.data())+(address-it->first);}
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        const bool preimage=write_sites.contains(pc);if(!preimage){packets.push_back(capture());if(fault&&fault->at("phase")=="before_read"&&read_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}++read_count;reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});}
        std::memcpy(out.data(),at(address,out.size()),out.size());if(noisy)hostile();
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        packets.push_back(capture());if(fault&&fault->at("phase")=="before_write"&&write_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
        const auto before=hex(std::span(at(address,in.size()),in.size()));std::memcpy(at(address,in.size()),in.data(),in.size());
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",before},{"written",hex(in)}});++write_count;if(noisy)hostile();
    }
};
unsigned guards(bool forced){
    unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x167efbu,0x33b595u};constexpr std::array extra{0x38846eu,0x21f590u,0x33b595u,0x100u};
    for(unsigned route=0;route<4;++route)for(unsigned policy=0;policy<(forced?1u:25u);++policy){
        Memory memory;unsigned callbacks{},stores{};bool first{},second{};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observer;if(policy==18||policy==19)observer=[&](const auto&){++stores;};
        InstrumentedState s{memory,observer,services,!forced&&policy==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp={1,2,3,4,0x62001000,6,7,8};input.eflags=0x246;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};s.load(input);
        if(!forced){
            if(policy==1){s.active_mmx=SourcePhysicsMmxAlias{};s.fp.count=1;}if(policy==2)s.fp.count=9;if(policy==24)s.fp.count=4;if(policy==3)s.fp.top=8;
            if(policy==4){const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw));}if(policy==5)_mm_setcsr(0x1f00);
            if(policy==6||policy==19)services.instruction={};
            if(policy==7){const auto old=services.instruction;services.instruction=[old](auto pc){old(pc);};}
            if(policy==8)services.instruction_cursor.executor={};if(policy==9)services.instruction_cursor.callback_type=nullptr;
            if(policy==10)services.instruction_cursor.callback_type=&typeid(int);if(policy==11)services.instruction_cursor.position=nullptr;
            if(policy==12)s.instruction_cursor=nullptr;if(policy==13)services.instruction_cursor.literal_service_sites=false;
            if(policy==14)services.instruction_cursor.service_sites={};if(policy==15)services.instruction_cursor.service_sites=changed;
            if(policy==16)first=true;if(policy==17)second=true;if(policy==18)services.instruction_cursor.observation_only_stores=false;
            if(policy==20)s.selected_observation_pcs=false;if(policy==21)s.fp.status=0x80;if(policy==22)s.fp.status=0x8000;
            if(policy==23)services.instruction_cursor.service_sites=extra;
        }
        const auto gp=s.gp;const auto flags=s.flags.bits();const auto depth=s.fp.count,top=s.fp.top;const auto status=s.fp.status;const auto host=native_raw_x87_detail::capture();std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),s.xmm,128);memory.pc=0xdeadbeefu;
        retained_statements=0;stop_at_retained=true;bool stopped{};try{if(route==0)copies::bound::run_135970(s);else if(route==1)copies::prepared::run_135970(s);else if(route==2)copies::other_bound::run_135970(s);else copies::other_prepared::run_135970(s);}catch(const RetainedStop&){stopped=true;}stop_at_retained=false;
        check(stopped&&retained_statements==1,"Unqualified policy skipped untouched first retained statement");
        check(s.gp==gp&&s.flags.bits()==flags&&s.fp.count==depth&&s.fp.top==top&&s.fp.status==status&&!std::memcmp(xmm.data(),s.xmm,128),"Admission fallback changed CPU state");
        check(memory.pc==0xdeadbeefu&&!callbacks&&!stores&&memory.reads.empty()&&memory.writes.empty()&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Admission fallback executed source I/O/callbacks");++count;
        asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return count;
}
}

int main(int argc,char**argv)try{
 if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guards")){std::cout<<J{{"untouched_retained_fallbacks",guards(std::string_view(argv[1])=="--reference-guards")}}.dump()<<'\n';return 0;}
 check(argc==2,"Supply original geometry visibility fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);unsigned comparisons{},packets{},full{},reference{},children{};
 const bool forced=std::getenv("HALO2_GEOMETRY_VISIBILITY_REFERENCE_TEST");unsigned default_calls{};
 for(const auto&row:fixture.at("cases"))for(unsigned mode=forced?5u:0u;mode<9;++mode){
  if(row.contains("fault")&&(forced||mode==3||mode==4))continue;
  const bool after_write=row.contains("fault")&&row.at("fault").at("phase")=="after_write";if(after_write&&mode!=2&&mode<5)continue;
  Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));for(auto pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
  memory.noisy=mode==1;if(row.contains("fault"))memory.fault=row.at("fault");SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned callbacks{};
  bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
  services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
  if(mode==2||after_write)observer=[&](const auto&){if(after_write&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};hostile();};
  InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}state.load(input);retained_statements=0;
  memory.capture=[&]{return packet(state,memory.pc);};auto before=[&](auto&,auto pc){memory.pc=pc;if(mode==2)hostile();};J child_packets=J::array();
  capture_child=[&](State&child,auto entry){if(std::find(fixture.at("extra_child_entries").begin(),fixture.at("extra_child_entries").end(),entry)!=fixture.at("extra_child_entries").end())child_packets.push_back(packet(child,entry));};
  auto call=[&](auto&child,native_geometry_visibility_detail::Child which,auto&observe){
   const auto entry=which==native_geometry_visibility_detail::Child::frustum?0x167ef0u:0x166c00u;capture_child(child,entry);
   const bool accepted=which==native_geometry_visibility_detail::Child::frustum?native_cluster_frustum_abi_detail::bridge(child,observe):native_bsp_queue_find_detail::bridge(child,observe);check(accepted,"Genuine native child refused");
  };
  bool faulted{};try{if(mode==5){copies::bound::run_135970(state);++default_calls;}else if(mode==6){copies::prepared::run_135970(state);++default_calls;}else if(mode==7){copies::other_bound::run_135970(state);++default_calls;}else if(mode==8){copies::other_prepared::run_135970(state);++default_calls;}else if(mode==3)copies::retained::run_135970(state);else if(mode==4)copies::other_retained::run_135970(state);else check(native_geometry_visibility_detail::run_with(state,call,before),"Geometry candidate refused");}catch(const Fault&){faulted=true;}
  capture_child={};const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);check(faulted==row.contains("fault"),name+": completion differs");
  if(mode!=3&&mode!=4&&!forced)check(!callbacks&&!state.literal_calls&&!retained_statements,name+": native route evaluated retained instructions");else{check(retained_statements>0,name+": original parent was skipped");reference+=retained_statements;}
  const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("after_live_x87")}};
  const auto actual=packet(state,memory.pc);check(actual==expected,name+": final ABI differs actual="+actual.dump()+" expected="+expected.dump());
  check(memory.packets.size()==row.at("calls").size(),name+": I/O count differs");
  for(unsigned n=0;n<memory.packets.size();++n){const auto&a=memory.packets[n];const auto&e=row.at("calls")[n];check((mode!=3&&mode!=4&&!forced)?a==e:a.at("eflags")==e.at("eflags"),name+": I/O packet"+std::to_string(n)+" differs actual="+a.dump()+" expected="+e.dump());if(mode!=3&&mode!=4&&!forced)++full;}
  if(mode<5||forced)check(child_packets==row.at("extra_child_calls"),name+": genuine callee entry differs actual="+child_packets.dump()+" expected="+row.at("extra_child_calls").dump());children+=child_packets.size();
  check(memory.reads==row.at("reads"),name+": ordered reads differ");check(memory.writes==row.at("writes"),name+": ordered stores differ");for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backing differs");
  ++comparisons;packets+=memory.packets.size();
 }
 std::cout<<J{{"passed",true},{"comparisons",comparisons},{"observed_pre_io_packets",packets},{"fully_compared_native_pre_io_packets",full},{"genuine_callee_entries",children},{"reference_parent_statements",reference},{"source_default_calls",default_calls}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
