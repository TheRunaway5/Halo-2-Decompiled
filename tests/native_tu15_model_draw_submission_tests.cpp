// Private 136A80 physical-original comparison harness (not a production
// target). Fixtures come from tools/validate_native_model_draw_submission_tu15.py.
#include "rendering/native_tu15_model_draw_submission.hpp"
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
#ifndef HALO2_DRAW_SUBMISSION_MUTATION_CORE_ONLY
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "model_draw_submission_bodies.inc"
}
#endif
namespace {
struct Fault{};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>unhex(const std::string&s){std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
J live(const SourcePhysicsContactRuntimeAbi&a){J result=J::array();for(unsigned n=0;n<a.x87_depth;++n)result.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));return result;}
J packet(State&s,std::uint32_t pc){const auto a=s.save();return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live(a)}};}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable std::uint32_t pc{};mutable J reads=J::array(),writes=J::array(),packets=J::array();
    std::set<std::uint32_t>write_sites,rmw_sites;mutable bool rmw_read{};std::function<J()>capture;std::optional<J>fault;mutable unsigned read_count{},write_count{};
    std::byte*at(std::uint32_t address,std::size_t size)const{auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unmapped model draw submission data");--it;check(std::uint64_t(address-it->first)+size<=it->second.size(),"Model draw submission data escaped backing");return const_cast<std::byte*>(it->second.data())+(address-it->first);}
    std::uint32_t word(std::uint32_t address)const{std::uint32_t value;std::memcpy(&value,at(address,4),4);return value;}
    void put(std::uint32_t address,std::uint32_t value){std::memcpy(at(address,4),&value,4);}
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        // A dual-access site (movsd, RMW) performs one genuine read per step;
        // an observer's preimage read at a store site is not a source read.
        const bool rmw=rmw_sites.contains(pc);const bool preimage=write_sites.contains(pc)&&(!rmw||rmw_read);if(rmw&&!preimage)rmw_read=true;
        if(!preimage){packets.push_back(capture());if(fault&&fault->at("phase")=="before_read"&&read_count==fault->at("ordinal"))throw Fault{};++read_count;reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});}
        std::memcpy(out.data(),at(address,out.size()),out.size());
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        packets.push_back(capture());if(fault&&fault->at("phase")=="before_write"&&write_count==fault->at("ordinal"))throw Fault{};
        const auto before=hex(std::span(at(address,in.size()),in.size()));std::memcpy(at(address,in.size()),in.data(),in.size());
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",before},{"written",hex(in)}});++write_count;rmw_read=false;
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
};
// Deterministic stand-in for the separately owned 2CBD0 draw unit; it must
// equal stub() in tools/validate_native_model_draw_submission_tu15.py. It
// consumes the real return address and 0x54 argument bytes, clobbers the
// volatile ABI and, per mode, an MXCSR flag, EDI, the part count or the part
// cursor. Its storage edits bypass the recorded provider, as the oracle's do.
struct Stub {
    Memory*memory{};unsigned mode{};J*entries{};unsigned calls{};
    void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&a,const SourcePhysicsContactRecordObserver&){
        check(call.instruction==0x136ce0u&&call.target==0x2cbd0u&&!call.tail,"2CBD0 required call differs");
        check(a.floating_point.has_value()&&!a.active_mmx,"2CBD0 required ABI is incomplete");
        entries->push_back(J{{"entry",0x2cbd0u},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live(a)}});
        const auto sp=a.gp[4];check(memory->word(sp)==0x136ce5u,"2CBD0 return address differs");
        const auto n=calls++;const auto seed=0x51000000u+0x101u*n+mode;
        a.gp[4]=sp+0x58u;a.gp[0]=seed;a.gp[1]=~seed;a.gp[2]=seed^0x00ff00ffu;
        a.eflags=(a.eflags&0x400u)|0x202u|((0x11u*n+0x41u*mode)&0x8d5u);
        for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)a.xmm[r][k]^=0x9e3779b9u*(n+1u)+r*4u+k;
        if(mode&1u)a.floating_point->mxcsr|=0x20u;
        if(mode==2u)a.gp[7]^=0x40u;
        if(mode==3u&&!n)memory->put(a.gp[3]+a.gp[6],memory->word(a.gp[3]+a.gp[6])-1u);
        if(mode==4u)memory->put(a.gp[4]+0x18u,memory->word(a.gp[4]+0x18u)+1u);
    }
};
const int stub_owner{};
bool stub_matches(const SourcePhysicsContactRuntimeServices::RequiredOwner&owner,const void*tag)noexcept{return owner.target<Stub>()&&tag==&stub_owner;}
void certify(SourcePhysicsWorldRuntimeServices&services){services.required_owner_policy={&services.required_owner.target_type(),std::this_thread::get_id(),&stub_owner,&stub_matches};}
#ifndef HALO2_DRAW_SUBMISSION_MUTATION_CORE_ONLY
#include "native_tu15_model_draw_submission_guards.inc"
#endif
}

// Modes: 0 ordered hook, 2 store observer, 3 retained translated parent,
// 5 bound owner, 6 prepared owner, 9 production access route (silent hook,
// committed direct pages), 10 census route (HALO2_CENSUS_NATIVE_BRIDGES:
// full callbacks without a cursor). The parent executes no x87 instruction
// and holds no host FP state across provider calls, so (unlike 135B60) no
// hostile-provider FP mode applies: both routes see the same host controls.
int main(int argc,char**argv)try{
#ifndef HALO2_DRAW_SUBMISSION_MUTATION_CORE_ONLY
 if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guards")){std::cout<<J{{"untouched_retained_fallbacks",guards(std::string_view(argv[1])=="--reference-guards")}}.dump()<<'\n';return 0;}
 if(argc==2&&std::string_view(argv[1])=="--census-guards"){setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);std::cout<<J{{"census_policies",census_guards()}}.dump()<<'\n';return 0;}
#endif
 check(argc==2,"Supply original model draw submission fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
 unsigned comparisons{},packets{},full{},reference{},callees{},default_calls{},direct_comparisons{},direct_reads{},provider_reads{},census_comparisons{};
 const bool forced=std::getenv("HALO2_DRAW_SUBMISSION_REFERENCE_TEST");constexpr unsigned direct_mode=9,census_mode=10;
 // The census route is a separate process: the switch is sampled once.
 const bool census=std::getenv("HALO2_DRAW_SUBMISSION_CENSUS_TEST");if(census)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
 for(const auto&row:fixture.at("cases"))for(unsigned mode=forced?5u:0u;mode<11u;++mode){
  if(mode==1||mode==4||mode==7||mode==8||(mode==census_mode)!=census)continue;
  if(std::getenv("HALO2_DRAW_SUBMISSION_NATIVE_ONLY")&&mode>=3&&mode!=direct_mode)continue;
#ifdef HALO2_DRAW_SUBMISSION_MUTATION_CORE_ONLY
  if(mode>=3&&mode!=direct_mode)continue;
#endif
  if(forced&&mode==direct_mode)continue;
  const bool faulting=row.contains("fault");
  if(faulting&&(mode==direct_mode||mode==census_mode||forced||mode==3))continue;
  if(faulting&&row.at("fault").at("phase")=="before_instruction"&&mode>=3)continue;
  const bool after_write=faulting&&row.at("fault").at("phase")=="after_write";if(after_write&&mode!=2&&mode<5)continue;
  const bool retained=mode==3||forced||(mode>=5&&mode<=8&&native_model_draw_submission_detail::reference());
  Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
  for(auto pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
  for(auto pc:fixture.at("rmw_sites"))memory.rmw_sites.insert(pc.get<std::uint32_t>());
  if(mode==direct_mode)memory.publish_direct();if(faulting)memory.fault=row.at("fault");
  SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned callbacks{},observed{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
  services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={mode==census_mode?nullptr:&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
  J entries=J::array();services.required_owner=Stub{&memory,row.at("stub").get<unsigned>(),&entries};certify(services);
  if(mode==2||after_write)observer=[&](const auto&){++observed;if(after_write&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};};
  InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};
  input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
  input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();
  for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}
  state.load(input);retained_statements=0;std::map<std::uint32_t,unsigned>instruction_occurrences;
  memory.capture=[&]{return packet(state,memory.pc);};
  auto before=[&](auto&,auto pc){memory.pc=pc;if(memory.fault&&memory.fault->at("phase")=="before_instruction"&&pc==memory.fault->at("instruction")&&instruction_occurrences[pc]++==memory.fault->at("ordinal"))throw Fault{};};
  bool faulted{};
  try{
#ifndef HALO2_DRAW_SUBMISSION_MUTATION_CORE_ONLY
   if(mode==5){copies::bound::run_136a80(state);++default_calls;}else if(mode==6){copies::prepared::run_136a80(state);++default_calls;}else if(mode==3)copies::retained::run_136a80(state);else
#endif
   if(mode==direct_mode||mode==census_mode)check(native_model_draw_submission_detail::bridge(state),"Model draw submission production route refused");
   else check(native_model_draw_submission_detail::bridge(state,before),"Model draw submission candidate refused");
  }catch(const Fault&){faulted=true;}
  const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);check(faulted==faulting,name+": completion differs");
  if(!retained)check(!callbacks&&!state.literal_calls&&!retained_statements,name+": native route evaluated retained instructions");
  else{check(retained_statements>0,name+": original parent was skipped");reference+=retained_statements;}
  auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("after_live_x87")}};
  auto actual=packet(state,memory.pc);
  // Without a cursor the census route publishes no instruction position:
  // every access, store and CPU packet is compared except that position.
  const auto unpositioned=[&](J value,const char*key){if(mode==census_mode)for(auto&item:value)item.erase(key);return value;};
  if(mode==census_mode){actual.erase("entry");expected.erase("entry");}
  check(actual==expected,name+": final ABI differs actual="+actual.dump()+" expected="+expected.dump());
  // Every required 2CBD0 entry sees the original's complete ABI.
  check(entries==row.at("callee_calls"),name+": 2CBD0 entry differs actual="+entries.dump()+" expected="+row.at("callee_calls").dump());callees+=entries.size();
  check(unpositioned(memory.writes,"instruction")==unpositioned(row.at("writes"),"instruction"),name+": ordered stores differ");
  if(mode==2&&!faulting)check(observed==memory.writes.size(),name+": observed store count differs");
  for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backing differs");
  if(mode==direct_mode){
   check(memory.packets.size()==memory.writes.size()+memory.reads.size(),name+": direct provider packet count differs");
   check(memory.reads.size()<=row.at("reads").size(),name+": direct route added provider reads");
   provider_reads+=memory.reads.size();direct_reads+=row.at("reads").size()-memory.reads.size();
   ++comparisons;++direct_comparisons;packets+=memory.packets.size();continue;
  }
  check(unpositioned(memory.reads,"instruction")==unpositioned(row.at("reads"),"instruction"),name+": ordered reads differ");
  check(memory.packets.size()==row.at("calls").size(),name+": I/O count differs actual="+std::to_string(memory.packets.size())+" expected="+std::to_string(row.at("calls").size()));
  // The translated fallback publishes ESP before PUSH stores, so only the
  // native route is packet-for-packet comparable; the retained route still
  // matches every access, store, 2CBD0 entry and outcome.
  const auto packets_actual=unpositioned(memory.packets,"entry"),packets_expected=unpositioned(row.at("calls"),"entry");
  if(!retained)for(unsigned n=0;n<memory.packets.size();++n){const auto&a=packets_actual[n];const auto&e=packets_expected[n];check(a==e,name+": I/O packet"+std::to_string(n)+" differs actual="+a.dump()+" expected="+e.dump());++full;}
  ++comparisons;packets+=memory.packets.size();census_comparisons+=mode==census_mode;
 }
 std::cout<<J{{"passed",true},{"comparisons",comparisons},{"observed_pre_io_packets",packets},{"fully_compared_native_pre_io_packets",full},{"required_callee_entries",callees},{"reference_parent_statements",reference},{"source_default_calls",default_calls},{"direct_route_comparisons",direct_comparisons},{"direct_page_reads",direct_reads},{"direct_route_provider_reads",provider_reads},{"census_route_comparisons",census_comparisons}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
