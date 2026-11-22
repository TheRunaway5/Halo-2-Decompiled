// Shared private physical-original comparison harness for the Shadow frame
// owners (136D50, 1362A0, 1365C0, 4BAE0; not a production target). Fixtures
// come from each owner's tools/validate_native_<name>_tu15.py through
// tools/native_shadow_frame_oracle.py.
//
// Separately owned callees are stubbed in the oracle: the original stops
// before them and the validator's deterministic stub supplies their ABI and
// storage effects. The fixture records each stub's effect (its registers,
// flags, XMM, MXCSR and byte patches) in call order; Stub replays it. The
// callee's complete entry ABI is compared first, so a replayed effect is the
// one the deterministic stub computes for that entry.
//
// Each owner's test defines an Owner type:
//   name, census_variable, reference_variable  (strings)
//   reference(), policy_admitted(state)
//   native(state, call, before), production(state, call)   the bridge
//   retained(state), bound(state), prepared(state)         translated copies
//   child(child, target, site, observe, census) -> bool     a genuine child
//                                                           (false: required)
#pragma once
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <thread>
using namespace halo2;using J=nlohmann::json;using State=physics_world_runtime_detail::WorldState;
struct InstrumentedState:State {using Base=physics_world_runtime_detail::WorldState;using Base::Base;bool selected_observation_pcs{true};unsigned literal_calls{};void instruction_literal(std::uint32_t pc){++literal_calls;Base::instruction_literal(pc);}};
struct RetainedStop{};inline unsigned retained_statements{};inline bool stop_at_retained{};
inline void retained_statement(){++retained_statements;if(stop_at_retained)throw RetainedStop{};}
namespace shadow_frame_harness {
struct Fault{};
inline void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
inline std::vector<std::byte>unhex(const std::string&s){std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
inline std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
inline J live(const SourcePhysicsContactRuntimeAbi&a){J result=J::array();for(unsigned n=0;n<a.x87_depth;++n)result.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));return result;}
inline J packet(State&s,std::uint32_t pc){const auto a=s.save();return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live(a)}};}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable std::uint32_t pc{};mutable J reads=J::array(),writes=J::array(),packets=J::array();
    std::set<std::uint32_t>write_sites,rmw_sites;mutable bool rmw_read{};std::function<J()>capture;std::optional<J>fault;mutable unsigned read_count{},write_count{};
    std::byte*at(std::uint32_t address,std::size_t size)const{auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unmapped shadow frame data "+std::to_string(address));--it;check(std::uint64_t(address-it->first)+size<=it->second.size(),"Shadow frame data escaped backing "+std::to_string(address));return const_cast<std::byte*>(it->second.data())+(address-it->first);}
    std::uint32_t word(std::uint32_t address)const{std::uint32_t value;std::memcpy(&value,at(address,4),4);return value;}
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        // A dual-access site (RMW, MOVSD) performs one genuine read per step;
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
    // committed ordinary ReadWrite pages, exactly as SourceRuntimeMemory does,
    // with SourceRuntimeMemory's read-only block for other operands.
    std::unique_ptr<SourceDirectPageIndex>index;mutable unsigned view_reads{};
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
    std::optional<std::span<const std::byte>>try_read_view(std::uint32_t address,std::size_t size)const noexcept override{
        if(!index||!size)return std::nullopt;auto it=banks.upper_bound(address);if(it==banks.begin())return std::nullopt;--it;
        if(std::uint64_t(address-it->first)+size>it->second.size())return std::nullopt;
        ++view_reads;return std::span<const std::byte>(it->second.data()+(address-it->first),size);
    }
};
// Replays the validator's deterministic stubs (see the file comment). Its
// storage edits bypass the recorded provider, as the oracle's do.
struct Stub {
    Memory*memory{};const J*effects{};J*entries{};unsigned calls{};
    void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&a,const SourcePhysicsContactRecordObserver&){
        check(a.floating_point.has_value()&&!a.active_mmx,"Required ABI is incomplete");
        entries->push_back(J{{"entry",call.target},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live(a)}});
        check(calls<effects->size(),"Unexpected required call");const auto&effect=(*effects)[calls++];
        check(call.target==effect.at("entry")&&call.instruction==effect.at("site")&&!call.tail,"Required call differs");
        check(memory->word(a.gp[4])==effect.at("site").get<std::uint32_t>()+5u,"Required return address differs");
        a.gp=effect.at("gp");a.eflags=effect.at("eflags");a.xmm=effect.at("xmm");a.floating_point->mxcsr=effect.at("mxcsr");
        a.x87_status=effect.at("x87_status");a.x87_depth=effect.at("live_x87").size();
        for(unsigned n=0;n<a.x87_depth;++n){const auto raw=unhex(effect.at("live_x87")[n]);std::memcpy(&a.x87[n],raw.data(),10);}
        for(const auto&patch:effect.at("patches")){const auto bytes=unhex(patch.at(1));std::memcpy(memory->at(patch.at(0),bytes.size()),bytes.data(),bytes.size());}
    }
};
inline const int stub_owner{};
inline bool stub_matches(const SourcePhysicsContactRuntimeServices::RequiredOwner&owner,const void*tag)noexcept{return owner.target<Stub>()&&tag==&stub_owner;}
inline void certify(SourcePhysicsWorldRuntimeServices&services){services.required_owner_policy={&services.required_owner.target_type(),std::this_thread::get_id(),&stub_owner,&stub_matches};}

// Compiled raw/prepared owner fallback checks: every unqualified policy runs
// the untouched first retained statement with no I/O or state change.
template<class Owner>unsigned guards(bool forced){
    static const J no_effects=J::array();
    unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x100u,0x33b595u};constexpr std::array extra{0x38846eu,0x21f590u,0x33b595u,0x100u};
    static const int other_owner{};
    for(unsigned route=0;route<2;++route)for(unsigned policy=0;policy<(forced?1u:23u);++policy){
        Memory memory;unsigned callbacks{},stores{};bool first{},second{};SourcePhysicsWorldRuntimeServices services;J entries=J::array();
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        services.required_owner=Stub{&memory,&no_effects,&entries};certify(services);
        SourcePhysicsContactRecordObserver observer;if(policy==18||policy==19)observer=[&](const auto&){++stores;};
        if(!forced){
            // Required-owner certification: cleared, other executor, wrapped
            // callable, foreign owner tag, no matcher.
            if(policy==3)services.required_owner_policy={};
            if(policy==4)services.required_owner_policy.executor={};
            if(policy==5){const auto old=services.required_owner;services.required_owner=[old](auto&m,const auto&c,auto&a,const auto&o){old(m,c,a,o);};}
            if(policy==21)services.required_owner_policy.owner=&other_owner;
            if(policy==22)services.required_owner_policy.matches=nullptr;
        }
        InstrumentedState s{memory,observer,services,!forced&&policy==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp={1,2,3,4,0x62001000,6,7,8};input.eflags=0x246;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};s.load(input);
        if(!forced){
            if(policy==1){s.active_mmx=SourcePhysicsMmxAlias{};s.fp.count=1;}if(policy==2)s.selected_observation_pcs=false;
            if(policy==6||policy==19)services.instruction={};
            if(policy==7){const auto old=services.instruction;services.instruction=[old](auto pc){old(pc);};}
            if(policy==8)services.instruction_cursor.executor={};if(policy==9)services.instruction_cursor.callback_type=nullptr;
            if(policy==10)services.instruction_cursor.callback_type=&typeid(int);if(policy==11)services.instruction_cursor.position=nullptr;
            if(policy==12)s.instruction_cursor=nullptr;if(policy==13)services.instruction_cursor.literal_service_sites=false;
            if(policy==14)services.instruction_cursor.service_sites={};if(policy==15)services.instruction_cursor.service_sites=changed;
            if(policy==16)first=true;if(policy==17)second=true;if(policy==18)services.instruction_cursor.observation_only_stores=false;
            if(policy==20)services.instruction_cursor.service_sites=extra;
        }
        const auto gp=s.gp;const auto flags=s.flags.bits();const auto depth=s.fp.count,top=s.fp.top;const auto status=s.fp.status;const auto host=native_raw_x87_detail::capture();std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),s.xmm,128);memory.pc=0xdeadbeefu;
        retained_statements=0;stop_at_retained=true;bool stopped{};try{if(route==0)Owner::bound(s);else Owner::prepared(s);}catch(const RetainedStop&){stopped=true;}stop_at_retained=false;
        check(stopped&&retained_statements==1,"Unqualified policy "+std::to_string(policy)+" skipped untouched first retained statement");
        check(s.gp==gp&&s.flags.bits()==flags&&s.fp.count==depth&&s.fp.top==top&&s.fp.status==status&&!std::memcmp(xmm.data(),s.xmm,128),"Admission fallback changed CPU state");
        check(memory.pc==0xdeadbeefu&&!callbacks&&!stores&&entries.empty()&&memory.reads.empty()&&memory.writes.empty()&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Admission fallback executed source I/O/callbacks");++count;
        asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return count;
}
// HALO2_CENSUS_NATIVE_BRIDGES=1 (set before the first admission query): only
// the uncursored full-callback route is newly admitted.
template<class Owner>unsigned census_guards(){
    static const J no_effects=J::array();
    unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    for(unsigned policy=0;policy<7;++policy){
        Memory memory;bool first{},second{};SourcePhysicsWorldRuntimeServices services;J entries=J::array();
        services.instruction=[&](auto){};services.instruction_cursor={nullptr,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(policy==6){const auto old=services.instruction;services.instruction=[old](auto pc){old(pc);};}
        services.required_owner=Stub{&memory,&no_effects,&entries};certify(services);if(policy==2)services.required_owner_policy={};
        SourcePhysicsContactRecordObserver observer;if(policy==5)observer=[](const auto&){};
        InstrumentedState s{memory,observer,services,policy==1?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp={1,2,3,4,0x62001000,6,7,8};input.eflags=0x246;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};s.load(input);
        if(policy==3)s.selected_observation_pcs=false;if(policy==4){s.active_mmx=SourcePhysicsMmxAlias{};s.fp.count=1;}
        const bool expected=policy==0||policy==5||policy==6;
        check(!s.instruction_cursor&&Owner::policy_admitted(s)==expected,"Census admission policy "+std::to_string(policy)+" differs");++count;
        asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return count;
}

// Modes: 0 ordered hook, 2 store observer, 3 retained translated parent,
// 5 bound owner, 6 prepared owner, 9 production access route (silent hook,
// committed direct pages, read-only views), 10 census route
// (HALO2_CENSUS_NATIVE_BRIDGES: full callbacks without a cursor).
template<class Owner>int run(int argc,char**argv)try{
 constexpr bool core_only=Owner::core_only;
 if constexpr(!core_only){
  if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guards")){std::cout<<J{{"untouched_retained_fallbacks",guards<Owner>(std::string_view(argv[1])=="--reference-guards")}}.dump()<<'\n';return 0;}
  if(argc==2&&std::string_view(argv[1])=="--census-guards"){setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);std::cout<<J{{"census_policies",census_guards<Owner>()}}.dump()<<'\n';return 0;}
 }
 check(argc==2,std::string("Supply original ")+Owner::name+" fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
 unsigned comparisons{},packets{},full{},reference{},callees{},default_calls{},direct_comparisons{},direct_reads{},provider_reads{},view_reads{},census_comparisons{};
 const bool forced=std::getenv(Owner::reference_test_variable);constexpr unsigned direct_mode=9,census_mode=10;
 // The census route is a separate process: the switch is sampled once.
 const bool census=std::getenv(Owner::census_test_variable);if(census)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
 for(const auto&row:fixture.at("cases"))for(unsigned mode=forced?5u:0u;mode<11u;++mode){
  if(mode==1||mode==4||mode==7||mode==8||(mode==census_mode)!=census)continue;
  if(core_only&&mode>=3&&mode!=direct_mode)continue;
  if(forced&&mode==direct_mode)continue;
  const bool faulting=row.contains("fault");
  if(faulting&&(mode==direct_mode||mode==census_mode||forced||mode==3))continue;
  if(faulting&&row.at("fault").at("phase")=="before_instruction"&&mode>=3)continue;
  const bool after_write=faulting&&row.at("fault").at("phase")=="after_write";if(after_write&&mode!=2&&mode<5)continue;
  const bool retained=mode==3||forced||(mode>=5&&mode<=8&&Owner::reference());
  Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
  for(auto pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
  for(auto pc:fixture.at("rmw_sites"))memory.rmw_sites.insert(pc.get<std::uint32_t>());
  if(mode==direct_mode)memory.publish_direct();if(faulting)memory.fault=row.at("fault");
  SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned callbacks{},observed{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
  services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={mode==census_mode?nullptr:&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
  J entries=J::array();services.required_owner=Stub{&memory,&row.at("stub_effects"),&entries};certify(services);
  if(mode==2||after_write)observer=[&](const auto&){++observed;if(after_write&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};};
  InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};
  input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
  input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();
  for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}
  state.load(input);retained_statements=0;std::map<std::uint32_t,unsigned>instruction_occurrences;
  memory.capture=[&]{return packet(state,memory.pc);};
  auto before=[&](auto&,auto pc){memory.pc=pc;if(memory.fault&&memory.fault->at("phase")=="before_instruction"&&pc==memory.fault->at("instruction")&&instruction_occurrences[pc]++==memory.fault->at("ordinal"))throw Fault{};};
  // Genuine children run on their own native bridges (census children with
  // the census callbacks parked); every other callee is a required owner.
  auto call=[&](auto&child,std::uint32_t target,std::uint32_t site,auto&observe){
   decltype(services.instruction)parked;if(mode==census_mode)std::swap(parked,services.instruction);
   const bool genuine=Owner::child(child,target,site,observe);
   if(mode==census_mode)std::swap(parked,services.instruction);
   if(!genuine)child.required(site,target,false);
  };
  bool faulted{};
  try{
   bool done{};
   if constexpr(!core_only){
    if(mode==5){Owner::bound(state);++default_calls;done=true;}else if(mode==6){Owner::prepared(state);++default_calls;done=true;}else if(mode==3){Owner::retained(state);done=true;}
   }
   if(done){}
   else if(mode==direct_mode||mode==census_mode)check(Owner::production(state,call),std::string(Owner::name)+" production route refused");
   else check(Owner::native(state,call,before),std::string(Owner::name)+" candidate refused");
  }catch(const Fault&){faulted=true;}
  const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);check(faulted==faulting,name+": completion differs");
  if(!retained)check(!callbacks&&!state.literal_calls&&!retained_statements,name+": native route evaluated retained instructions");
  else{check(retained_statements>0,name+": original parent was skipped");reference+=retained_statements;}
  auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("after_live_x87")}};
  auto actual=packet(state,memory.pc);
  // Without a cursor the census route publishes no instruction position:
  // every access, store and CPU packet is compared except that position.
  const auto unpositioned=[&](J value,const char*key){if(mode==census_mode)for(auto&item:value)item.erase(key);return value;};
  const auto defined_flags=[](J value){
   if constexpr(requires { Owner::undefined_flags(std::uint32_t{}); })
    for(auto& item:value)
     item["eflags"]=item.at("eflags").template get<std::uint32_t>() &
        ~Owner::undefined_flags(item.at("entry").template get<std::uint32_t>());
   return value;
  };
  if(mode==census_mode){actual.erase("entry");expected.erase("entry");}
  check(actual==expected,name+": final ABI differs actual="+actual.dump()+" expected="+expected.dump());
  // Every required callee entry sees the original's complete ABI.
  check(defined_flags(entries)==defined_flags(row.at("callee_calls")),name+": required entry differs actual="+entries.dump()+" expected="+row.at("callee_calls").dump());callees+=entries.size();
  check(unpositioned(memory.writes,"instruction")==unpositioned(row.at("writes"),"instruction"),name+": ordered stores differ actual="+memory.writes.dump().substr(0,4000)+" expected="+row.at("writes").dump().substr(0,4000));
  if(mode==2&&!faulting)check(observed==memory.writes.size(),name+": observed store count differs");
  for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backing differs");
  if(mode==direct_mode){
   check(memory.packets.size()==memory.writes.size()+memory.reads.size(),name+": direct provider packet count differs");
   check(memory.reads.size()<=row.at("reads").size(),name+": direct route added provider reads");
   provider_reads+=memory.reads.size();direct_reads+=row.at("reads").size()-memory.reads.size();view_reads+=memory.view_reads;
   ++comparisons;++direct_comparisons;packets+=memory.packets.size();continue;
  }
  check(unpositioned(memory.reads,"instruction")==unpositioned(row.at("reads"),"instruction"),name+": ordered reads differ actual="+memory.reads.dump().substr(0,4000)+" expected="+row.at("reads").dump().substr(0,4000));
  check(memory.packets.size()==row.at("calls").size(),name+": I/O count differs actual="+std::to_string(memory.packets.size())+" expected="+std::to_string(row.at("calls").size()));
  // The translated fallback publishes ESP before PUSH stores and RMW flags
  // before their store, so only the native route is packet-for-packet
  // comparable; the retained route still matches every access, store,
  // required entry and outcome.
  const auto packets_actual=unpositioned(defined_flags(memory.packets),"entry"),packets_expected=unpositioned(defined_flags(row.at("calls")),"entry");
  if(!retained)for(unsigned n=0;n<memory.packets.size();++n){const auto&a=packets_actual[n];const auto&e=packets_expected[n];check(a==e,name+": I/O packet"+std::to_string(n)+" differs actual="+a.dump()+" expected="+e.dump());++full;}
  ++comparisons;packets+=memory.packets.size();census_comparisons+=mode==census_mode;
 }
 std::cout<<J{{"passed",true},{"comparisons",comparisons},{"observed_pre_io_packets",packets},{"fully_compared_native_pre_io_packets",full},{"required_callee_entries",callees},{"reference_parent_statements",reference},{"source_default_calls",default_calls},{"direct_route_comparisons",direct_comparisons},{"direct_page_reads",direct_reads},{"direct_route_provider_reads",provider_reads},{"direct_route_view_reads",view_reads},{"census_route_comparisons",census_comparisons}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
} // namespace shadow_frame_harness
