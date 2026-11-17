#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_context_frame.hpp"
#include <iostream>
#include <random>
#include <set>
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace context_reference {
#include "solver_hull_context_reference.inc"
}
namespace {
struct Result {J abi,packets=J::array(),clocks=J::array(),child=nullptr;bool stopped{};unsigned callbacks{},observed{};std::set<std::uint32_t>sites;};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,bool continuation,unsigned fail_observer=0u,bool hostile=false,const std::vector<std::uint64_t>&clock_inputs={},unsigned feedback=0u){
    Result out;WorldState*current{};
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        const auto replace=[&](std::uint32_t address,std::uint32_t value){auto i=m.at(address,4);std::memcpy(m.banks[i->first].data()+address-i->first,&value,4);};
        if(feedback==1u&&(e.instruction==0x314730u||e.instruction==0x3147b3u))replace(e.address,0x89abcdefu);
        if(feedback==2u&&e.instruction==0x314762u)replace(current->gp[5]+0x18u,0x61004440u);
        if(hostile)poison();if(++out.observed==fail_observer)throw Boundary{};
    };
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](std::uint32_t pc){m.pc=pc;++out.callbacks;out.sites.insert(pc);if(context_reference::io_sites.contains(pc))out.packets.push_back(packet(*current,pc));};
    unsigned clock_index{};services.timestamp_counter=[&]{out.clocks.push_back(packet(*current,m.pc));if(!clock_inputs.empty())check(clock_index<clock_inputs.size(),"Declared hull-context clock stream exhausted");const auto value=clock_inputs.empty()?0x123456789abcdef0ull+0x1020304050607080ull*clock_index:clock_inputs[clock_index];++clock_index;if(hostile)poison();return value;};
    auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&m.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);current=&state;
    const auto child=[&](auto&s){out.child=packet(s,0x313bb0u);throw Boundary{};};context_reference::child=child;
    const auto before=[&](auto&s,std::uint32_t pc){if(context_reference::io_sites.contains(pc))out.packets.push_back(packet(s,pc));};
    try{if(native){if(continuation)native_solver_hull_context_detail::run_private_continuation(state,before);else native_solver_hull_context_detail::run_private_owner(state,[&](auto&s,auto target,auto site){check(target==0x313bb0u&&site==0x314796u,"Real hull-context child identity differs");child(s);},before);}
        else if(continuation)context_reference::continuation(state);else context_reference::owner(state);}
    catch(const Boundary&){out.stopped=true;}
    context_reference::child={};out.abi=packet(state,m.pc);return out;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(x.abi!=y.abi||x.packets!=y.packets||x.clocks!=y.clocks||x.child!=y.child||x.stopped!=y.stopped||a.banks!=b.banks||a.operations!=b.operations||a.writes!=b.writes){
        std::cerr<<name<<" expected="<<x.abi.dump()<<" actual="<<y.abi.dump()<<"\n";
        for(unsigned n=0;n<std::min(x.packets.size(),y.packets.size());++n)if(x.packets[n]!=y.packets[n]){std::cerr<<"packet "<<n<<" expected="<<x.packets[n].dump()<<" actual="<<y.packets[n].dump()<<"\n";break;}
        throw std::runtime_error("Hull-context canonical seam differs "+name);
    }check(y.callbacks==0u,"Private hull-context used literal callbacks");
}
void cases(){std::mt19937 random(0x314710u);unsigned cases{},prefixes{},hostile{},children{},clock_events{},feedback_cases{};std::set<std::uint32_t>sites;
    constexpr std::uint32_t base=0x61000000u;
    for(unsigned k=0;k<128u;++k){const bool continuation=k>=64u;Memory seed;seed.banks[base].resize(0x8000u);seed.banks[0x48c000u].resize(0x1000u);for(auto&[_,bank]:seed.banks)for(auto&v:bank)v=std::byte(random());
        const auto put=[&](std::uint32_t at,std::uint32_t value){auto i=seed.at(at,4);std::memcpy(seed.banks[i->first].data()+at-i->first,&value,4);};
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=random();const auto sp=base+0x7000u+k%16u,bp=sp-4u,p=((bp)&~15u)-0x70u;
        abi.gp[4]=continuation?p:sp;if(continuation)abi.gp[5]=bp;
        put(sp,0x63000000u);put(sp+4u,base+0x1000u);put(sp+8u,base+0x1100u);put(sp+0xcu,base+0x2000u);put(sp+0x10u,base+0x3000u);put(sp+0x14u,base+0x4000u);put(sp+0x18u,base+0x5000u);
        if(continuation){put(bp,random());put(p,random());}
        for(unsigned n=0;n<16u;++n)put(base+0x2000u+n*4u,random());
        const auto profile=k%4u==3u?p+0xcu:base+0x6000u;put(0x48c974u,profile);put(0x48c978u,profile+(k%3u?12u:0u));
        abi.eflags=0x202u|(random()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(std::array{0x7fu,0x27fu,0x37fu}[k%3u]|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)|(k&0x3fu)};
        abi.x87_depth=std::array{0u,1u,4u,8u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;for(auto&v:abi.xmm)for(auto&w:v)w=random();
        Memory a=seed,b=seed;const auto x=run(a,abi,false,continuation),y=run(b,abi,true,continuation);compare(a,b,x,y,"normal-"+std::to_string(k));check(x.stopped==!continuation,"Hull-context real child stop/domain differs");++cases;children+=!x.child.is_null();clock_events+=x.clocks.size();sites.insert(x.sites.begin(),x.sites.end());
        if(k<3u||(k>=64u&&k<67u))for(unsigned category=0;category<3u;++category){const auto count=category==0u?a.read_count:category==1u?a.write_count:x.observed;for(unsigned n=0;n<count;++n){Memory c=seed,d=seed;if(category==0u)c.read_fault=d.read_fault=n;if(category==1u)c.write_fault=d.write_fault=n;const auto u=run(c,abi,false,continuation,category==2u?n+1u:0u),v=run(d,abi,true,continuation,category==2u?n+1u:0u);compare(c,d,u,v,"prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));check(u.stopped,"Hull-context prefix did not stop");++prefixes;}}
        if(k<6u||(k>=64u&&k<70u)){Memory c=seed,d=seed;d.hostile=true;const auto u=run(c,abi,false,continuation),v=run(d,abi,true,continuation,0u,true);compare(c,d,u,v,"hostile-"+std::to_string(k));++hostile;}
        if(k==1u||k==2u||k==65u||k==66u)for(unsigned feedback=1u;feedback<=(continuation?1u:2u);++feedback){Memory c=seed,d=seed;const auto u=run(c,abi,false,continuation,0u,false,{},feedback),v=run(d,abi,true,continuation,0u,false,{},feedback);compare(c,d,u,v,"feedback-"+std::to_string(k)+"-"+std::to_string(feedback));++feedback_cases;}
    }
    std::cout<<J{{"canonical_source_segments",cases},{"executed_source_sites",sites.size()},{"actual_313bb0_entry_stops",children},{"independent_declared_postchild_continuations",64},{"declared_test_clock_inputs",clock_events},{"read_write_observer_prefixes",prefixes},{"hostile_fp_segments",hostile},{"lazy_scratch_formal_observer_feedback_segments",feedback_cases},{"whole_child_outcomes_supplied",false},{"whole_owner_completion_qualified",false}}.dump()<<"\n";
}
}
int main(){try{cases();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
