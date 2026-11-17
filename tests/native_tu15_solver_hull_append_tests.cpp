#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_append_frame.hpp"
#include <iostream>
#include <random>
#include <set>
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace append_reference {
#include "solver_hull_append_reference.inc"
}
namespace {
struct Result{J abi,packets=J::array(),clocks=J::array(),child=nullptr;bool stopped{};unsigned callbacks{},observed{};std::set<std::uint32_t>sites;};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,bool=false,unsigned observer_fault=0u,bool hostile=false,const std::vector<std::uint64_t>& = {},unsigned feedback=0u){
    Result out;WorldState*current{};
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        const auto replace=[&](std::uint32_t address,std::uint32_t value){auto i=m.at(address,4);std::memcpy(m.banks[i->first].data()+address-i->first,&value,4);};
        if(e.instruction==0x3132b8u&&feedback==1u)replace(abi.gp[1]+0x48u,0x61006000u);
        if(e.instruction==0x3132b8u&&feedback==2u)replace(0x61002000u,0x7fc34567u);
        if(e.instruction==0x3132cau&&feedback==3u)replace(abi.gp[1]+0x48u,0x61006000u);
        if(hostile)poison();if(++out.observed==observer_fault)throw Boundary{};
    };
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](std::uint32_t pc){m.pc=pc;++out.callbacks;out.sites.insert(pc);if(append_reference::io_sites.contains(pc))out.packets.push_back(packet(*current,pc));};
    auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&m.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);current=&state;
    try{if(native)native_solver_hull_append_detail::run_private_owner(state,[&](auto&s,auto pc){out.packets.push_back(packet(s,pc));});else append_reference::owner(state);}
    catch(const Boundary&){out.stopped=true;}out.abi=packet(state,m.pc);return out;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(x.abi!=y.abi||x.packets!=y.packets||x.stopped!=y.stopped||a.banks!=b.banks||a.operations!=b.operations||a.writes!=b.writes){
        std::cerr<<name<<" expected="<<x.abi.dump()<<" actual="<<y.abi.dump()<<"\n";
        for(unsigned n=0;n<std::min(x.packets.size(),y.packets.size());++n)if(x.packets[n]!=y.packets[n]){std::cerr<<"packet "<<n<<" expected="<<x.packets[n].dump()<<" actual="<<y.packets[n].dump()<<"\n";break;}
        throw std::runtime_error("Hull-append phased source differs "+name);
    }check(!y.callbacks,"Native hull append used literal callbacks");
}
void cases(){std::mt19937 random(0x313290u);unsigned count{},prefixes{},hostile{},feedback_count{};std::set<std::uint32_t>sites;constexpr std::uint32_t base=0x61000000u;
    for(unsigned k=0;k<128u;++k){Memory seed;seed.banks[base].resize(0x8000);for(auto&v:seed.banks[base])v=std::byte(random());
        const auto put=[&](std::uint32_t address,std::uint32_t value){std::memcpy(seed.banks[base].data()+address-base,&value,4);};
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=random();abi.gp[1]=base+0x1000u;abi.gp[4]=base+0x7000u+k%16u;
        const auto p=((abi.gp[4]-4u)&~15u)-0x10u,pair=k%8u==1u?base+0x4000u:base+0x2000u,differences=k%8u==3u?p:base+0x4000u;
        put(abi.gp[4],0x63000000u);put(abi.gp[4]+4u,pair);put(abi.gp[1]+0x48u,base+0x3000u);put(abi.gp[1]+0x50u,differences);
        const auto index=std::array{0u,1u,2u,3u,4u,0xffffffffu,0x80000000u,0x10000000u}[k%8u];put(base+0x3080u,index);put(base+0x6080u,2u);
        constexpr std::array<std::uint32_t,16>words{0u,0x80000000u,1u,0x80000001u,0x7fffffu,0x800000u,0x7f7fffffu,0xff7fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0xffc23456u,0x7f812345u,0xff823456u,0x3f800000u,0xbf800000u};
        for(unsigned n=0;n<12u;++n)put(pair+n*4u,k<64u?random():words[(k+n*3u)%words.size()]);
        abi.eflags=0x202u|(random()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(std::array{0x7fu,0x27fu,0x37fu}[k%3u]|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)|(k&0x3fu)};abi.x87_depth=std::array{0u,1u,4u,8u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;for(auto&v:abi.xmm)for(auto&w:v)w=random();
        Memory a=seed,b=seed;const auto x=run(a,abi,false),y=run(b,abi,true);compare(a,b,x,y,"normal-"+std::to_string(k));check(!x.stopped,"Normal append stopped");++count;sites.insert(x.sites.begin(),x.sites.end());
        if(k<8u)for(unsigned category=0;category<3u;++category){const auto limit=category==0u?a.read_count:category==1u?a.write_count:x.observed;for(unsigned n=0;n<limit;++n){Memory c=seed,d=seed;if(category==0u)c.read_fault=d.read_fault=n;if(category==1u)c.write_fault=d.write_fault=n;const auto u=run(c,abi,false,false,category==2u?n+1u:0u),v=run(d,abi,true,false,category==2u?n+1u:0u);compare(c,d,u,v,"prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));check(u.stopped,"Append prefix did not stop");++prefixes;}}
        if(k<8u){Memory c=seed,d=seed;d.hostile=true;const auto u=run(c,abi,false),v=run(d,abi,true,false,0u,true);compare(c,d,u,v,"hostile-"+std::to_string(k));++hostile;}
        if(k<4u)for(unsigned feedback=1u;feedback<4u;++feedback){Memory c=seed,d=seed;const auto u=run(c,abi,false,false,0u,false,{},feedback),v=run(d,abi,true,false,0u,false,{},feedback);compare(c,d,u,v,"feedback-"+std::to_string(k)+"-"+std::to_string(feedback));++feedback_count;}
    }
    std::cout<<J{{"source_cases",count},{"source_sites",sites.size()},{"read_write_observer_prefixes",prefixes},{"hostile_fp_cases",hostile},{"backed_header_pair_feedback_cases",feedback_count},{"native_literal_callbacks",0}}.dump()<<"\n";
}
}
int main(){try{cases();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
