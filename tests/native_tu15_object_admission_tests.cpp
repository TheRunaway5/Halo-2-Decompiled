// Private 135B60 physical-original comparison harness (not a production
// target). Fixtures come from tools/validate_native_object_admission_tu15.py.
#include "rendering/native_tu15_object_admission.hpp"
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
#ifndef HALO2_OBJECT_MUTATION_CORE_ONLY
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "object_admission_bodies.inc"
}
#endif
namespace {
struct Fault{};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>unhex(const std::string&s){std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
J packet(State&s,std::uint32_t pc){const auto a=s.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));J result{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};if(a.active_mmx){J physical=J::array();unsigned tag{};for(unsigned n=0;n<8;++n){const auto&raw=a.active_mmx->physical[n];physical.push_back(hex(raw));tag|=native_raw_x87_detail::value_tag(raw.data())<<(n*2u);}result["active_mmx"]=J{{"tag_word",tag},{"physical",physical},{"known_mask",a.active_mmx->known_mask}};}return result;}
void hostile(){const std::uint16_t unmasked=0x37e;asm volatile("fninit;fldz;fldz;fdivp;fldcw %0"::"m"(unmasked):"memory","st","st(1)");_mm_setcsr(0x8065);}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable std::uint32_t pc{};mutable J reads=J::array(),writes=J::array(),packets=J::array();
    std::set<std::uint32_t>write_sites,rmw_sites;mutable bool rmw_read{};std::function<J()>capture;std::optional<J>fault;bool noisy{};mutable unsigned read_count{},write_count{};
    std::byte*at(std::uint32_t address,std::size_t size)const{auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unmapped visibility association data");--it;check(std::uint64_t(address-it->first)+size<=it->second.size(),"Visibility association data escaped backing");return const_cast<std::byte*>(it->second.data())+(address-it->first);}
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        const bool rmw=rmw_sites.contains(pc);const bool preimage=write_sites.contains(pc)&&(!rmw||rmw_read);if(rmw&&!preimage)rmw_read=true;if(!preimage){packets.push_back(capture());if(fault&&fault->at("phase")=="before_read"&&read_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}++read_count;reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});}
        std::memcpy(out.data(),at(address,out.size()),out.size());if(noisy)hostile();
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        packets.push_back(capture());if(fault&&fault->at("phase")=="before_write"&&write_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
        const auto before=hex(std::span(at(address,in.size()),in.size()));std::memcpy(at(address,in.size()),in.data(),in.size());
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",before},{"written",hex(in)}});++write_count;rmw_read=false;if(noisy)hostile();
    }
    // Production access mode: every page-aligned bank is published as complete
    // committed ordinary ReadWrite pages, exactly as SourceRuntimeMemory does.
    std::unique_ptr<SourceDirectPageIndex>index;
    void publish_direct(){
        index=std::make_unique<SourceDirectPageIndex>();
        for(auto&[base,bytes]:banks){
            check(base%4096u==0&&bytes.size()%4096u==0,"Direct bank is not page aligned");
            for(std::size_t offset=0;offset<bytes.size();offset+=4096u){
                const auto page=(base+offset)>>12;auto&block=index->blocks[page>>10];
                if(!block)block=std::make_unique<SourceDirectPageIndex::Block>();
                (*block)[page&1023u]={bytes.data()+offset,true};
            }
        }
        direct_pages_=index.get();
    }
    // The same mode publishes SourceRuntimeMemory's read-only block capability
    // for in-bank extents that a page cannot name (page-crossing operands).
    mutable unsigned view_reads{};
    std::optional<std::span<const std::byte>>try_read_view(std::uint32_t address,std::size_t size)const noexcept override{
        if(!index||!size)return std::nullopt;auto it=banks.upper_bound(address);if(it==banks.begin())return std::nullopt;--it;
        if(std::uint64_t(address-it->first)+size>it->second.size())return std::nullopt;
        ++view_reads;return std::span<const std::byte>(it->second.data()+(address-it->first),size);
    }
};
#ifndef HALO2_OBJECT_MUTATION_CORE_ONLY
#include "native_tu15_object_admission_guards.inc"
#endif
}

// Modes: 0 ordered hook, 1 hostile provider FP, 2 hostile observer, 3 retained
// translated parent, 5 bound owner, 6 prepared owner, 9 production access
// route (silent hook, committed direct pages, read-only views), 10 census
// route (HALO2_CENSUS_NATIVE_BRIDGES: full callbacks without a cursor).
int main(int argc,char**argv)try{
#ifndef HALO2_OBJECT_MUTATION_CORE_ONLY
 if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guards")){std::cout<<J{{"untouched_retained_fallbacks",guards(std::string_view(argv[1])=="--reference-guards")}}.dump()<<'\n';return 0;}
 if(argc==2&&std::string_view(argv[1])=="--census-guards"){setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);std::cout<<J{{"census_policies",census_guards()}}.dump()<<'\n';return 0;}
#endif
 check(argc==2,"Supply original object admission fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
 unsigned comparisons{},packets{},full{},reference{},children{},default_calls{},direct_comparisons{},direct_reads{},provider_reads{},view_reads{},census_comparisons{};
 const bool forced=std::getenv("HALO2_OBJECT_ADMISSION_REFERENCE_TEST");constexpr unsigned direct_mode=9,census_mode=10;
 // The census route is a separate process: the switch is sampled once.
 const bool census=std::getenv("HALO2_OBJECT_ADMISSION_CENSUS_TEST");if(census)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
 for(const auto&row:fixture.at("cases"))for(unsigned mode=forced?5u:0u;mode<11u;++mode){
  if(mode==4||mode==7||mode==8||(mode==census_mode)!=census)continue;
  if(std::getenv("HALO2_OBJECT_ADMISSION_NATIVE_ONLY")&&mode>=3&&mode!=direct_mode)continue;
#ifdef HALO2_OBJECT_MUTATION_CORE_ONLY
  if(mode>=3&&mode!=direct_mode)continue;
#endif
  if(forced&&mode==direct_mode)continue;
  const bool faulting=row.contains("fault");
  if(faulting&&(mode==direct_mode||mode==census_mode||forced||mode==3))continue;
  if(faulting&&row.at("fault").at("phase")=="before_instruction"&&mode>=3)continue;
  const bool after_write=faulting&&row.at("fault").at("phase")=="after_write";if(after_write&&mode!=2&&mode<5)continue;
  const bool retained=mode==3||forced||(mode>=5&&mode<=8&&native_object_admission_detail::reference());
  Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
  for(auto pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
  for(auto pc:fixture.at("rmw_sites"))memory.rmw_sites.insert(pc.get<std::uint32_t>());
  memory.noisy=mode==1;if(mode==direct_mode)memory.publish_direct();if(faulting)memory.fault=row.at("fault");
  SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned callbacks{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
  services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={mode==census_mode?nullptr:&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
  if(mode==2||after_write)observer=[&](const auto&){if(after_write&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};hostile();};
  InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};
  input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
  input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();
  for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}
  state.load(input);retained_statements=0;std::map<std::uint32_t,unsigned>instruction_occurrences;
  memory.capture=[&]{return packet(state,memory.pc);};
  auto before=[&](auto&,auto pc){memory.pc=pc;if(mode==2)hostile();if(memory.fault&&memory.fault->at("phase")=="before_instruction"&&pc==memory.fault->at("instruction")&&instruction_occurrences[pc]++==memory.fault->at("ordinal"))throw Fault{};};
  J child_packets=J::array();
  capture_child=[&](State&child,auto entry){for(const auto&extra:fixture.at("extra_child_entries"))if(extra==entry){child_packets.push_back(packet(child,entry));break;}};
  auto call=[&](auto&child,native_object_admission_detail::Child which,auto&observe){
   using Child=native_object_admission_detail::Child;capture_child(child,which==Child::geometry?0x135970u:0x136e10u);bool accepted{};
   // Census children keep their own (cursor) admission; production runs them
   // translated. Here they run natively on the no-callback route.
   decltype(services.instruction)parked;if(mode==census_mode)std::swap(parked,services.instruction);
   if(which==Child::geometry)accepted=native_geometry_visibility_detail::run_with(child,[&](auto&nested,auto kind,auto&progress){const bool frustum=kind==native_geometry_visibility_detail::Child::frustum;capture_child(nested,frustum?0x167ef0u:0x166c00u);check(frustum?native_cluster_frustum_abi_detail::bridge(nested,progress):native_bsp_queue_find_detail::bridge(nested,progress),"Genuine geometry descendant refused");},observe);
   else accepted=native_visibility_association_detail::bridge(child,observe);
   if(mode==census_mode)std::swap(parked,services.instruction);
   check(accepted,"Genuine native child refused");
  };
  bool faulted{};
  try{
#ifndef HALO2_OBJECT_MUTATION_CORE_ONLY
   if(mode==5){copies::bound::run_135b60(state);++default_calls;}else if(mode==6){copies::prepared::run_135b60(state);++default_calls;}else if(mode==3)copies::retained::run_135b60(state);else
#endif
   if(mode==direct_mode||mode==census_mode)check(native_object_admission_detail::run_with(state,call),"Object admission production route refused");
   else check(native_object_admission_detail::run_with(state,call,before),"Object admission candidate refused");
  }catch(const Fault&){faulted=true;}
  capture_child={};const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);check(faulted==faulting,name+": completion differs");
  if(!retained)check(!callbacks&&!state.literal_calls&&!retained_statements,name+": native route evaluated retained instructions");
  else{check(retained_statements>0,name+": original parent was skipped");reference+=retained_statements;}
  auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("after_live_x87")}};
  auto actual=packet(state,memory.pc);
  // Without a cursor the census route publishes no instruction position:
  // every access, store and CPU packet is compared except that position.
  const auto unpositioned=[&](J value,const char*key){if(mode==census_mode)for(auto&item:value)item.erase(key);return value;};
  if(mode==census_mode){actual.erase("entry");expected.erase("entry");}
  check(actual==expected,name+": final ABI differs actual="+actual.dump()+" expected="+expected.dump());
  // Installed owners reach the genuine child bridges directly; only the
  // capturing call (and the retained helpers) publish entry witnesses.
  if(mode<5||mode==direct_mode||mode==census_mode||forced){check(child_packets==row.at("extra_child_calls"),name+": genuine callee entry differs actual="+child_packets.dump()+" expected="+row.at("extra_child_calls").dump());children+=child_packets.size();}
  check(unpositioned(memory.writes,"instruction")==unpositioned(row.at("writes"),"instruction"),name+": ordered stores differ");
  for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backing differs");
  if(mode==direct_mode){
   check(memory.packets.size()==memory.writes.size()+memory.reads.size(),name+": direct provider packet count differs");
   check(memory.reads.size()<=row.at("reads").size(),name+": direct route added provider reads");
   provider_reads+=memory.reads.size();direct_reads+=row.at("reads").size()-memory.reads.size();view_reads+=memory.view_reads;
   ++comparisons;++direct_comparisons;packets+=memory.packets.size();continue;
  }
  check(unpositioned(memory.reads,"instruction")==unpositioned(row.at("reads"),"instruction"),name+": ordered reads differ");
  check(memory.packets.size()==row.at("calls").size(),name+": I/O count differs actual="+std::to_string(memory.packets.size())+" expected="+std::to_string(row.at("calls").size()));
  // The translated fallback publishes ESP before PUSH stores and RMW flags
  // before their store, so only the native route is packet-for-packet
  // comparable; the retained route still matches every access and outcome.
  const auto packets_actual=unpositioned(memory.packets,"entry"),packets_expected=unpositioned(row.at("calls"),"entry");
  if(!retained)for(unsigned n=0;n<memory.packets.size();++n){const auto&a=packets_actual[n];const auto&e=packets_expected[n];check(a==e,name+": I/O packet"+std::to_string(n)+" differs actual="+a.dump()+" expected="+e.dump());++full;}
  ++comparisons;packets+=memory.packets.size();census_comparisons+=mode==census_mode;
 }
 std::cout<<J{{"passed",true},{"comparisons",comparisons},{"observed_pre_io_packets",packets},{"fully_compared_native_pre_io_packets",full},{"genuine_callee_entries",children},{"reference_parent_statements",reference},{"source_default_calls",default_calls},{"direct_route_comparisons",direct_comparisons},{"direct_page_reads",direct_reads},{"direct_route_provider_reads",provider_reads},{"direct_route_view_reads",view_reads},{"census_route_comparisons",census_comparisons}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
