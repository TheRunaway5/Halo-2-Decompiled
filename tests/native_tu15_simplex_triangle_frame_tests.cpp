#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_triangle_abi.hpp"
#include "physics/runtime/native_tu15_simplex_triangle_source_owner.hpp"
#include "physics/runtime/native_tu15_simplex_helpers_handoff_internal.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
#include "reference/native_tu15_simplex_triangle_value_before.cpp"
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace { struct ActualState:WorldState{using WorldState::WorldState;bool selected_observation_pcs=true;};
}
namespace reference {
#include "simplex_triangle_frame_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,work=base+0x100u,stack=base+0x7000u;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x8000u);
    std::array<std::byte,0x1000>sign{};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(std::uint32_t)>on_read;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at>=0x461000u&&std::uint64_t(at-0x461000u)+size<=sign.size())return std::span(sign).subspan(at-0x461000u,size);
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Support DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared initialization read stop");
        if(on_read)on_read(at);
        std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(++write_count==fail_write)throw std::runtime_error("Declared initialization write stop");
        std::ranges::copy(in,view(at,in.size()).begin());
    }
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4);}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{};
    std::vector<SourcePhysicsContactRecordStore>stores;
    std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>packets;
    std::set<std::uint32_t>sites;std::uint32_t result{};std::uint64_t parent_callbacks{},child_callbacks{},child_entries{};
};
template<class State=WorldState>Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned feedback=0,unsigned fail_observer=0,bool ordered_reference=true,unsigned route=0u){
    Result out;
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        out.stores.push_back(e);
        const auto actual_pc=e.instruction>=0x309900u&&e.instruction<=0x309ad2u?e.instruction+0x3af0u:e.instruction;
        if(feedback==1u&&actual_pc==0x30d445u)m.put(e.address,0x3f800000u);
        if(feedback==2u&&actual_pc==0x30d51eu)m.put(abi.gp[4]+12u,1u);
        if(feedback==3u&&actual_pc==0x30d586u)m.put(e.address+16u,0xbf800000u);
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared triangle observer stop");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.parent_callbacks;};
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&out.pc;cursor.service_sites=service_sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);State state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{
        if(native&&route>=3u){
            if constexpr(requires{state.selected_observation_pcs;})reference::run_bound_actual(state,route-3u);
            else reference::run_bound_canonical(state,route-3u);
            out.result=state.gp[0];
        }else if(native&&route){
            if(!native_simplex_triangle_detail::bridge(state)){
                if(route==1u)reference::child_canonical(state);else reference::child_unmodified(state);
            }
            out.result=state.gp[0];
        }else if(native)out.result=native_simplex_triangle_detail::run_private_owner(state,[&](auto&s,std::uint32_t pc){out.packets.emplace_back(pc,s.save());});
        else{if(route==1u)reference::child_canonical(state);else if(ordered_reference)reference::child(state);else reference::child_unmodified(state);out.result=state.gp[0];}
    }catch(const std::runtime_error&){out.stopped=true;}
    out.abi=state.save();return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||
        a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||x.child_entries!=y.child_entries||x.result!=y.result||a.bank!=b.bank||a.sign!=b.sign||a.reads!=b.reads||x.stores!=y.stores){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";
        for(unsigned n=0;n<8;++n){if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs\n";if(x.abi.xmm[n]!=y.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";}
        throw std::runtime_error("Native simplex triangle mismatch "+name);
    }
    if(y.parent_callbacks&&!native_simplex_triangle_detail::reference())throw std::runtime_error("Native triangle used parent literal callbacks");
}
void supplemental(){
    std::mt19937 rng(0x30d3f0u);unsigned cases{},prefixes{},swaps{};std::set<std::uint32_t>sites;
    for(unsigned k=0;k<256u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[1]=work;abi.gp[4]=stack+k%16u;auto point=base+0x1000u,vertices=base+0x2000u;
        const auto aligned=(abi.gp[4]-4u)&~15u;
        if(k%16u==6u)vertices=aligned-0x30u;
        if(k%16u==10u)abi.gp[1]=aligned-0x180u;
        for(auto p:{point,vertices,abi.gp[1]})for(unsigned row=0;row<24u;++row)for(unsigned lane=0;lane<4u;++lane)
            seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,point);seed.put(abi.gp[4]+8u,vertices);seed.put(abi.gp[4]+12u,k%5u==0u?0u:1u);
        for(unsigned lane=0;lane<4u;++lane)seed.put(0x461960u+lane*4u,0x80000000u);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,7u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();
        Memory a=seed,b=seed;auto x=run(a,abi,false,k%4u,0,false),y=run(b,abi,true,k%4u);compare(a,b,x,y,"normal-"+std::to_string(k));
        if(x.stopped)throw std::runtime_error("Normal triangle fixture stopped");++cases;sites.insert(x.sites.begin(),x.sites.end());if(x.pc==0x30d5b9u)++swaps;
        if(k<8u){
            for(unsigned category=0;category<2u;++category){const auto limit=category==0?a.read_count:unsigned(x.stores.size());
                for(unsigned n=1;n<=limit;++n){Memory c=seed,d=seed;if(category==0)c.fail_read=d.fail_read=n;
                    auto u=run(c,abi,false,0,category==1?n:0),v=run(d,abi,true,0,category==1?n:0);
                    compare(c,d,u,v,"prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));
                    if(!u.stopped)throw std::runtime_error("Triangle prefix did not stop");++prefixes;
                }
            }
        }
    }
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"read_observer_prefixes\":"<<prefixes<<",\"source_sites\":"<<sites.size()<<",\"orientation_swaps\":"<<swaps<<",\"native_literal_callbacks\":0}\n";
}
void routing(){
    std::mt19937 rng(0x30d3f2u);unsigned cases{},reference_calls{};
    for(unsigned k=0;k<128u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[1]=work;abi.gp[4]=stack+k%16u;auto point=base+0x1000u,vertices=base+0x2000u;
        const auto aligned=(abi.gp[4]-4u)&~15u;
        if(k%16u==6u)vertices=aligned-0x30u;
        if(k%16u==10u)abi.gp[1]=aligned-0x180u;
        for(auto p:{point,vertices,abi.gp[1]})for(unsigned row=0;row<24u;++row)for(unsigned lane=0;lane<4u;++lane)
            seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,point);seed.put(abi.gp[4]+8u,vertices);seed.put(abi.gp[4]+12u,k%5u==0u?0u:1u);
        for(unsigned lane=0;lane<4u;++lane)seed.put(0x461960u+lane*4u,0x80000000u);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,7u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();

        for(unsigned route=1u;route<=2u;++route){
            Memory a=seed,b=seed;const auto x=run(a,abi,false,k%4u,0,false,route);
            const auto y=route==1u?run(b,abi,true,k%4u,0,false,route):run<ActualState>(b,abi,true,k%4u,0,false,route);
            compare(a,b,x,y,"bridge-"+std::to_string(route)+"-"+std::to_string(k));
            if(x.stopped)throw std::runtime_error("Normal triangle bridge stopped");
            if(native_simplex_triangle_detail::reference()){if(!y.parent_callbacks)throw std::runtime_error("Forced triangle reference did not execute literal owner");++reference_calls;}
            ++cases;
        }
    }
    auto&activity=native_simplex_triangle_detail::activity();
    if(native_simplex_triangle_detail::reference()?(activity.actual_calls||activity.canonical_calls):(activity.actual_calls!=128u||activity.canonical_calls!=128u))
        throw std::runtime_error("Triangle bridge activity differs");
    std::cerr<<"{\"bridge_cases\":"<<cases<<",\"actual_calls\":"<<activity.actual_calls<<",\"canonical_calls\":"<<activity.canonical_calls<<",\"literal_reference_calls\":"<<reference_calls<<"}\n";
}
void source_routes(){
    std::mt19937 rng(0x30d3f3u);unsigned cases{},literal_entries{};
    for(unsigned k=0;k<64u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[1]=work;abi.gp[4]=stack+k%16u;auto point=base+0x1000u,vertices=base+0x2000u;
        const auto aligned=(abi.gp[4]-4u)&~15u;
        if(k%16u==6u)vertices=aligned-0x30u;
        if(k%16u==10u)abi.gp[1]=aligned-0x180u;
        for(auto p:{point,vertices,abi.gp[1]})for(unsigned row=0;row<24u;++row)for(unsigned lane=0;lane<4u;++lane)
            seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,point);seed.put(abi.gp[4]+8u,vertices);seed.put(abi.gp[4]+12u,k%5u==0u?0u:1u);
        for(unsigned lane=0;lane<4u;++lane)seed.put(0x461960u+lane*4u,0x80000000u);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,7u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();

        for(unsigned variant=0;variant<8u;++variant){
            const bool observed_state=variant==0u||variant==3u;const bool actual_namespace=observed_state||variant>=6u;Memory a=seed,b=seed;
            const auto expected=run(a,abi,false,k%4u,0,false,actual_namespace?2u:1u);
            const auto result=observed_state?run<ActualState>(b,abi,true,k%4u,0,false,variant+3u):run(b,abi,true,k%4u,0,false,variant+3u);
            compare(a,b,expected,result,"bound-source-"+std::to_string(variant)+"-"+std::to_string(k));
            if(result.stopped)throw std::runtime_error("Normal bound triangle stopped");++cases;
        }
    }
    for(auto value:reference::bound_literal_entries)literal_entries+=value;
    if(native_simplex_triangle_detail::reference()?literal_entries!=cases:literal_entries!=0u)throw std::runtime_error("Bound source reference entry proof differs");
    auto&a=native_simplex_triangle_detail::activity();
    if(native_simplex_triangle_detail::reference()?(a.actual_calls||a.canonical_calls):(a.actual_calls!=256u||a.canonical_calls!=256u))throw std::runtime_error("Bound source native activity differs");
    std::cerr<<"{\"bound_source_cases\":"<<cases<<",\"source_variants\":8,\"actual_calls\":"<<a.actual_calls<<",\"canonical_calls\":"<<a.canonical_calls<<",\"literal_body_entries\":"<<literal_entries<<"}\n";
}
void refusals(){
    unsigned cases{};
    for(unsigned kind=0;kind<24u;++kind){
        Memory memory;SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};abi.gp[1]=work;abi.gp[4]=stack;
        SourcePhysicsContactRecordObserver observer=[](const auto&){};SourcePhysicsWorldRuntimeServices services;
        std::uint32_t pc{};services.instruction=[](std::uint32_t){};
        std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        if(kind==11u)cursor.literal_service_sites=false;
        physics_step_detail::FpScope environment(*abi.floating_point);ActualState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        if(kind!=11u&&!native_simplex_triangle_detail::admitted(state))throw std::runtime_error("Triangle guard baseline refused");
        bool trace=true;auto external=cursor;std::array extra_sites{sites[0],sites[1],sites[2],sites[2]};const auto host_mxcsr=_mm_getcsr();
        switch(kind){
        case 0:state.image=SourcePhysicsContactImage::MapPack11;break;
        case 1:state.image=SourcePhysicsContactImage(99);break;
        case 2:state.active_mmx.emplace();break;
        case 3:state.fp.count=8u;break;
        case 4:state.fp.top=8u;break;
        case 5:state.fp.status|=0x8080u;break;
        case 6:state.selected_observation_pcs=false;break;
        case 7:services.instruction={};break;
        case 8:services.instruction=[unused=17u](std::uint32_t){(void)unused;};break;
        case 9:cursor.position=nullptr;break;
        case 10:cursor.executor=std::thread::id{};break;
        case 11:cursor.literal_service_sites=true;break;
        case 12:cursor.literal_service_sites=false;break;
        case 13:state.instruction_cursor=&external;break;
        case 14:cursor.callback_type=nullptr;break;
        case 15:sites[2]^=1u;break;
        case 16:cursor.service_sites=std::span(sites).first(2);break;
        case 17:cursor.first_trace=&trace;break;
        case 18:cursor.second_trace=&trace;break;
        case 19:cursor.observation_only_stores=false;break;
        case 20:cursor.callback_type=&typeid(int);break;
        case 21:_mm_setcsr(host_mxcsr&~0x80u);break;
        case 22:services.instruction={};state.instruction_cursor=nullptr;break;
        case 23:cursor.service_sites=extra_sites;break;
        }
        const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);const auto flags=state.flags.bits(),other=state.other_flags;
        const auto count=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
        const auto accepted=native_simplex_triangle_detail::bridge(state);const auto unchanged=state.gp==gp&&std::memcmp(state.xmm,xmm.data(),128)==0&&state.flags.bits()==flags&&state.other_flags==other&&state.fp.count==count&&state.fp.top==top&&state.fp.status==status&&memory.read_count==0u&&memory.write_count==0u&&pc==0u;
        _mm_setcsr(host_mxcsr);
        if(accepted||!unchanged)throw std::runtime_error("Triangle live refusal failed "+std::to_string(kind));
        ++cases;
    }
    std::cerr<<"{\"untouched_live_refusals\":"<<cases<<"}\n";
}
void handoff_routes(){
    std::mt19937 rng(0x30eb85u);unsigned cases{};
    constexpr std::array call_sites{0x30dad9u,0x30e3b6u,0x30eb85u,0x30ece2u};
    for(unsigned k=0;k<64u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[1]=work;abi.gp[4]=stack+k%16u;auto point=base+0x1000u,vertices=base+0x2000u;
        const auto aligned=(abi.gp[4]-4u)&~15u;
        if(k%16u==6u)vertices=aligned-0x30u;
        if(k%16u==10u)abi.gp[1]=aligned-0x180u;
        for(auto p:{point,vertices,abi.gp[1]})for(unsigned row=0;row<24u;++row)for(unsigned lane=0;lane<4u;++lane)
            seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,point);seed.put(abi.gp[4]+8u,vertices);seed.put(abi.gp[4]+12u,k%5u==0u?0u:1u);
        for(unsigned lane=0;lane<4u;++lane)seed.put(0x461960u+lane*4u,0x80000000u);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,7u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();

        seed.put(abi.gp[4],call_sites[k%4u]+5u);
        Memory expected=seed,actual=seed;const auto reference_result=run<ActualState>(expected,abi,false,0,0,false);
        Result result;std::uint32_t policy_pc{};NativeTu15SimplexHelpersHandoffActivity legacy{};
        NativeTu15SimplexHelpersHandoffBinding binding{actual,{base,base+0x8000u,true,nullptr,nullptr,[&](auto pc){policy_pc=pc;}},&legacy};
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){result.pc=pc;++result.parent_callbacks;};
        constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
        cursor.position=&result.pc;cursor.service_sites=service_sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        SourcePhysicsContactRecordObserver observer=[&](const auto&e){result.stores.push_back(e);};
        physics_step_detail::FpScope environment(*abi.floating_point);ActualState state{actual,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        const auto entry=state.save();
        if(handoff_native_simplex_helpers(state,0x30d3f0u,call_sites[k%4u]))throw std::runtime_error("Full triangle owner was shadowed by scalar handoff");
        if(!same_abi(entry,state.save())||actual.read_count||actual.write_count||result.pc)throw std::runtime_error("Triangle owner deferral changed actual callee entry");
        if(!native_simplex_triangle_detail::bridge(state))reference::child_unmodified(state);
        result.abi=state.save();result.result=state.gp[0];compare(expected,actual,reference_result,result,"handoff-"+std::to_string(k));
        if(legacy.attempts||policy_pc)throw std::runtime_error("Qualified triangle entered residual scalar binding");
        if(native_simplex_triangle_detail::reference()&&!result.parent_callbacks)throw std::runtime_error("Triangle handoff reference did not execute original body");
        ++cases;
    }
    auto&a=native_simplex_triangle_detail::activity();
    if(native_simplex_triangle_detail::reference()?(a.world_reference_deferrals!=64u||a.world_owner_deferrals||a.actual_calls):(a.world_owner_deferrals!=64u||a.world_reference_deferrals||a.actual_calls!=64u))throw std::runtime_error("Triangle handoff activity differs");
    std::cerr<<"{\"actual_handoff_owner_cases\":"<<cases<<",\"real_callee_entry_unchanged\":true,\"residual_scalar_attempts\":0,\"owner_deferrals\":"<<a.world_owner_deferrals<<",\"reference_deferrals\":"<<a.world_reference_deferrals<<"}\n";
}
void value_compatibility(){
    std::mt19937 rng(0x30d3f1u);unsigned cases{};
    constexpr std::array edge{0u,0x80000000u,1u,0x80000001u,0x007fffffu,0x00800000u,0x7f7fffffu,0xff7fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0xffc23456u,0x7f812345u,0xff823456u};
    for(unsigned k=0;k<256u;++k){
        Memory a;for(auto&v:a.bank)v=std::byte(rng());const auto point=base+0x1000u,vertices=base+0x2000u;
        for(auto p:{point,vertices,work})for(unsigned row=0;row<24u;++row)for(unsigned lane=0;lane<4u;++lane)
            a.put(p+row*16u+lane*4u,k<128u?std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f):edge[(k+row*3u+lane*5u)%edge.size()]);
        for(unsigned lane=0;lane<4u;++lane)a.put(0x461960u+lane*4u,0x80000000u);Memory b=a;
        NativeTu15PhysicsFrameFp afp{std::uint16_t(0x37fu|((k%4u)<<10u)),std::uint16_t(0x4100u|((k%8u)<<11u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)},bfp=afp;
        std::vector<SourcePhysicsContactRecordStore>astores,bstores;
        auto observe=[&](auto&memory,auto&stores,const auto&e){stores.push_back(e);if(k%4u==1u&&e.instruction==0x30d51eu)memory.put(vertices+0x20u,0x3f800000u);};
        const auto expected=triangle_value_reference::native_tu15_orient_simplex_triangle(a,work,point,vertices,k%5u?1u:0u,afp,k%8u,[&](const auto&e){observe(a,astores,e);});
        const auto actual=native_tu15_orient_simplex_triangle(b,work,point,vertices,k%5u?1u:0u,bfp,k%8u,[&](const auto&e){observe(b,bstores,e);});
        if(expected!=actual||a.bank!=b.bank||a.sign!=b.sign||a.reads!=b.reads||astores!=bstores||afp.control_word!=bfp.control_word||afp.status!=bfp.status||afp.mxcsr!=bfp.mxcsr)
            throw std::runtime_error("Shared simplex triangle value mismatch "+std::to_string(k));
        ++cases;
    }
    std::cerr<<"{\"value_cases\":"<<cases<<",\"frozen_previous_data_oracle\":true}\n";
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated initialization fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){
    for(auto value:abi.gp)emit(value);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);
    for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);
    for(auto&vector:abi.xmm)for(auto value:vector)emit(value);
}

void fixture(){
    const unsigned count=input_word();for(unsigned k=0;k<count;++k){
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&value:abi.gp)value=input_word();abi.eflags=input_word();
        const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(auto&vector:abi.xmm)for(auto&value:vector)value=input_word();Memory memory;
        for(auto data:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.sign)})std::cin.read(reinterpret_cast<char*>(data.data()),data.size());
        const auto got=run(memory,abi,true);if(got.stopped)throw std::runtime_error("Native physical triangle stopped");emit_abi(got.abi);
        for(auto data:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.sign)})std::cout.write(reinterpret_cast<const char*>(data.data()),data.size());
        emit(got.stores.size());for(auto&event:got.stores){emit(event.instruction);emit(event.address);emit(event.written.size());std::cout.write(reinterpret_cast<const char*>(event.before.data()),event.before.size());std::cout.write(reinterpret_cast<const char*>(event.written.data()),event.written.size());}
        emit(got.pc);emit(got.packets.size());for(auto&[pc,state]:got.packets){emit(pc);emit_abi(state);}
    }
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else if(argc==2&&std::string(argv[1])=="--source-routes")source_routes();else if(argc==2&&std::string(argv[1])=="--handoff")handoff_routes();else if(argc==2&&std::string(argv[1])=="--refusals")refusals();else if(argc==2&&std::string(argv[1])=="--routes")routing();else if(argc==2&&std::string(argv[1])=="--value")value_compatibility();else supplemental();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
