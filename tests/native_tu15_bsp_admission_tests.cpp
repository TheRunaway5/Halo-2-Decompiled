#include "rendering/native_tu15_bsp_admission.hpp"
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
#ifndef HALO2_BSP_MUTATION_CORE_ONLY
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "bsp_admission_bodies.inc"
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
    struct DirectStore {
        unsigned ordinal{},width{};std::uint32_t pc{},address{};
        std::array<std::byte,4> before{},after{};
    };
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable std::uint32_t pc{};mutable J reads=J::array(),writes=J::array(),packets=J::array();
    std::set<std::uint32_t>write_sites,rmw_sites;mutable bool rmw_read{};std::function<J()>capture;std::optional<J>fault;bool noisy{};mutable unsigned read_count{},write_count{};
    std::vector<DirectStore> direct_stores;
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
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> in)noexcept override{
        if(direct_stores.size()==direct_stores.capacity() || in.size()>4u)return false;
        const auto*page=direct_page(address,in.size());if(!page || !page->writable)return false;
        auto*destination=page->bytes+(address&4095u);
        DirectStore event{write_count,static_cast<unsigned>(in.size()),pc,address};
        std::memcpy(event.before.data(),destination,in.size());
        std::memcpy(event.after.data(),in.data(),in.size());
        std::memmove(destination,in.data(),in.size());
        direct_stores.push_back(event);++write_count;rmw_read=false;return true;
    }
    J ordered_writes()const{
        J result=J::array();std::size_t scoped{},direct{};
        for(unsigned ordinal=0;ordinal<write_count;++ordinal){
            if(direct<direct_stores.size() && direct_stores[direct].ordinal==ordinal){
                const auto&s=direct_stores[direct++];
                result.push_back(J{{"instruction",s.pc},{"address",s.address},
                    {"before",hex(std::span(s.before).first(s.width))},{"written",hex(std::span(s.after).first(s.width))}});
            }else result.push_back(writes.at(scoped++));
        }
        return result;
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
#ifndef HALO2_BSP_MUTATION_CORE_ONLY
#include "native_tu15_bsp_admission_guards.inc"
#endif

}

int main(int argc,char**argv)try{
#ifndef HALO2_BSP_MUTATION_CORE_ONLY
 if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guards")){std::cout<<J{{"untouched_retained_fallbacks",guards(std::string_view(argv[1])=="--reference-guards")}}.dump()<<'\n';return 0;}
#endif
 check(argc==2,"Supply original BSP admission fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);unsigned comparisons{},packets{},full{},reference{},children{};
 const bool forced=std::getenv("HALO2_BSP_ADMISSION_REFERENCE_TEST");unsigned default_calls{};
 // Core-only mutation builds have no copied bodies. The last two modes
 // publish committed direct pages, with a preallocated store log in mode 10.
 constexpr unsigned direct_mode=9,callback_free_mode=10,modes=11;
 unsigned direct_comparisons{},direct_reads{},provider_reads{},view_reads{},callback_free_stores{},callback_free_pushes{};
 for(const auto&row:fixture.at("cases"))for(unsigned mode=forced?5u:0u;mode<modes;++mode){
  const bool direct=mode>=direct_mode;
  if(std::getenv("HALO2_BSP_ADMISSION_NATIVE_ONLY")&&mode>=3&&!direct)continue;
#ifdef HALO2_BSP_MUTATION_CORE_ONLY
  if(mode>=3&&!direct)continue;
#endif
  if(forced&&direct)continue;
  if(row.contains("fault")&&direct)continue;
  if(row.contains("fault")&&(forced||mode==3||mode==4))continue;
  if(row.contains("fault")&&row.at("fault").at("phase")=="before_instruction"&&mode>=3)continue;
  const bool after_write=row.contains("fault")&&row.at("fault").at("phase")=="after_write";if(after_write&&mode!=2&&mode<5)continue;
  Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));for(auto pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
  for(auto pc:fixture.at("rmw_sites"))memory.rmw_sites.insert(pc.get<std::uint32_t>());memory.noisy=mode==1;if(direct)memory.publish_direct();if(row.contains("fault"))memory.fault=row.at("fault");SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned callbacks{};
  if(mode==callback_free_mode)memory.direct_stores.reserve(row.at("writes").size());
  bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
  services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
  if(mode==2||after_write)observer=[&](const auto&){if(after_write&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};hostile();};
  InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}state.load(input);retained_statements=0;
  std::map<std::uint32_t,unsigned>instruction_occurrences;
  memory.capture=[&]{return packet(state,memory.pc);};auto before=[&](auto&,auto pc){memory.pc=pc;if(mode==2)hostile();if(memory.fault&&memory.fault->at("phase")=="before_instruction"&&pc==memory.fault->at("instruction")&&instruction_occurrences[pc]++==memory.fault->at("ordinal"))throw Fault{};};J child_packets=J::array();
  capture_child=[&](State&child,auto entry){if(std::find(fixture.at("extra_child_entries").begin(),fixture.at("extra_child_entries").end(),entry)!=fixture.at("extra_child_entries").end())child_packets.push_back(packet(child,entry));};
  auto call=[&](auto&child,native_bsp_admission_detail::Child which,auto&observe){
   using Child=native_bsp_admission_detail::Child;const auto entry=which==Child::geometry?0x135970u:which==Child::association?0x136e10u:0x171070u;capture_child(child,entry);
   bool accepted{};
   if(which==Child::geometry)accepted=native_geometry_visibility_detail::run_with(child,[&](auto&nested,auto kind,auto&progress){const bool frustum=kind==native_geometry_visibility_detail::Child::frustum;capture_child(nested,frustum?0x167ef0u:0x166c00u);check(frustum?native_cluster_frustum_abi_detail::bridge(nested,progress):native_bsp_queue_find_detail::bridge(nested,progress),"Genuine geometry descendant refused");},observe);
   else if(which==Child::association)accepted=native_visibility_association_detail::bridge(child,observe);
   else accepted=native_fog_membership_detail::bridge(child,observe);
   check(accepted,"Genuine native child refused");
  };
  bool faulted{};try{
#ifndef HALO2_BSP_MUTATION_CORE_ONLY
    if(mode==5){copies::bound::run_136ea0(state);++default_calls;}else if(mode==6){copies::prepared::run_136ea0(state);++default_calls;}else if(mode==7){copies::other_bound::run_136ea0(state);++default_calls;}else if(mode==8){copies::other_prepared::run_136ea0(state);++default_calls;}else if(mode==3)copies::retained::run_136ea0(state);else if(mode==4)copies::other_retained::run_136ea0(state);else
#endif
    if(direct)check(native_bsp_admission_detail::run_with(state,call),"BSP admission production route refused");
    else check(native_bsp_admission_detail::run_with(state,call,before),"BSP admission candidate refused");
  }catch(const Fault&){faulted=true;}
  capture_child={};const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);check(faulted==row.contains("fault"),name+": completion differs");
  if(mode!=3&&mode!=4&&!forced)check(!callbacks&&!state.literal_calls&&!retained_statements,name+": native route evaluated retained instructions");else{check(retained_statements>0,name+": original parent was skipped");reference+=retained_statements;}
  const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("after_live_x87")}};
  const auto actual=packet(state,memory.pc);check(actual==expected,name+": final ABI differs actual="+actual.dump()+" expected="+expected.dump());
  if(direct){
   // Only stores reach the provider; each still carries its pre-store packet.
   check(memory.packets.size()==memory.writes.size()+memory.reads.size(),name+": direct provider packet count differs");
   check(memory.reads.size()<=row.at("reads").size(),name+": direct route added provider reads");
   provider_reads+=memory.reads.size();direct_reads+=row.at("reads").size()-memory.reads.size();view_reads+=memory.view_reads;
   check(child_packets==row.at("extra_child_calls"),name+": genuine callee entry differs actual="+child_packets.dump()+" expected="+row.at("extra_child_calls").dump());children+=child_packets.size();
   check(memory.ordered_writes()==row.at("writes"),name+": ordered stores differ");for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backing differs");
   callback_free_stores+=memory.direct_stores.size();
   const auto used_pushes=std::ranges::count_if(memory.direct_stores,[](const auto&s){return s.pc==0x136ea3u;});
   const bool expected_push=mode==callback_free_mode && row.value("secondary_stack",false) &&
       !native_bsp_admission_detail::reference_callback_free_pushes() && !native_raw_x87_detail::scoped_access_reference();
   check(used_pushes==int(expected_push),name+": secondary stack transport admission differs");
   if(used_pushes)callback_free_pushes+=std::ranges::count_if(memory.direct_stores,[](const auto&s){
       return s.address>=0x60000000u && s.address<=0x600ffffcu;
   });
   ++comparisons;++direct_comparisons;packets+=memory.packets.size();continue;
  }
  check(memory.packets.size()==row.at("calls").size(),name+": I/O count differs actual="+std::to_string(memory.packets.size())+" expected="+std::to_string(row.at("calls").size()));
  for(unsigned n=0;n<memory.packets.size();++n){const auto&a=memory.packets[n];const auto&e=row.at("calls")[n];check((mode!=3&&mode!=4&&!forced)?a==e:a.at("eflags")==e.at("eflags"),name+": I/O packet"+std::to_string(n)+" differs actual="+a.dump()+" expected="+e.dump());if(mode!=3&&mode!=4&&!forced)++full;}
  if(mode<5||forced)check(child_packets==row.at("extra_child_calls"),name+": genuine callee entry differs actual="+child_packets.dump()+" expected="+row.at("extra_child_calls").dump());children+=child_packets.size();
  check(memory.reads==row.at("reads"),name+": ordered reads differ");check(memory.writes==row.at("writes"),name+": ordered stores differ");for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backing differs");
  ++comparisons;packets+=memory.packets.size();
 }
 std::cout<<J{{"passed",true},{"comparisons",comparisons},{"observed_pre_io_packets",packets},{"fully_compared_native_pre_io_packets",full},{"genuine_callee_entries",children},{"reference_parent_statements",reference},{"source_default_calls",default_calls},{"direct_route_comparisons",direct_comparisons},{"direct_page_reads",direct_reads},{"direct_route_provider_reads",provider_reads},{"direct_route_view_reads",view_reads},{"callback_free_stores",callback_free_stores},{"callback_free_secondary_stack_stores",callback_free_pushes}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
