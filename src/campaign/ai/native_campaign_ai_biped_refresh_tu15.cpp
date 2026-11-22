#include "halo2/native_campaign_ai_biped_refresh_tu15.hpp"
#include "halo2/native_campaign_ai_object_navigation_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include "halo2/native_tu15_unit_animation_keys_kernel.hpp"
#include "halo2/native_tu15_camera_query_binding.hpp"
#include "halo2/native_tu15_camera_query_leaves.hpp"
#include "halo2/native_tu15_constraint_effect_location.hpp"
#include "halo2/native_tu15_object_deletion.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"
#include <algorithm>

namespace halo2 {
struct NativeCampaignAiBipedRefreshScratchBindingTu15::State {
    SourcePhysicsContactMemory *memory;
    std::uint32_t identity;
    std::vector<NativeTu15ContactPrivateRange> retained;
    GapCheck check_gap;
    State *previous;
    bool active{};
    static thread_local State *current;
};
thread_local NativeCampaignAiBipedRefreshScratchBindingTu15::State *
    NativeCampaignAiBipedRefreshScratchBindingTu15::State::current{};
NativeCampaignAiBipedRefreshScratchBindingTu15::NativeCampaignAiBipedRefreshScratchBindingTu15(
    SourcePhysicsContactMemory &memory,std::uint32_t identity,std::span<const NativeTu15ContactPrivateRange> retained)
    :NativeCampaignAiBipedRefreshScratchBindingTu15(memory,identity,retained,nullptr) {}
NativeCampaignAiBipedRefreshScratchBindingTu15::NativeCampaignAiBipedRefreshScratchBindingTu15(
    SourcePhysicsContactMemory &memory,std::uint32_t identity,std::span<const NativeTu15ContactPrivateRange> retained,
    GapCheck check_gap) {
    if((identity&15u)||identity>0x100000000ull-bytes)
        throw std::invalid_argument("Native biped refresh requires aligned bounded query DATA");
    for(auto *prior=State::current;prior;prior=prior->previous) {
        const auto end=std::uint64_t(identity)+bytes;
        if(std::uint64_t(prior->identity)<end && std::uint64_t(identity)<std::uint64_t(prior->identity)+bytes)
            throw std::invalid_argument("Native biped refresh DATA overlaps an active binding");
    }
    if(check_gap && !check_gap(memory,identity))
        throw std::runtime_error("Native private DATA aliases retained input/output");
    state_=new State{&memory,identity,{retained.begin(),retained.end()},check_gap,State::current};
    State::current=state_;
}
NativeCampaignAiBipedRefreshScratchBindingTu15::~NativeCampaignAiBipedRefreshScratchBindingTu15() {
    State::current=state_->previous;delete state_;
}
namespace {
using namespace goal_tu15;
constexpr u32 none=0xffffffffu;
using Fp=NativeTu15PhysicsFrameFp;
void push(const Port &p,SourcePhysicsWorldRuntimeAbi &abi,u32 value) {
    abi.gp[4]-=4u;std::array<std::byte,4> raw{};
    for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(8u*n));p.memory.write(abi.gp[4],raw);
}
u32 body(const Port &p,u32 object) {
    const auto pool=p.r32(0x4ecbd0u),rows=p.r32(pool+0x44u);
    return p.r32(rows+(object&65535u)*12u+8u);
}
void al(SourcePhysicsWorldRuntimeAbi &abi,u32 value){abi.gp[0]=(abi.gp[0]&0xffffff00u)|(value&255u);}

#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
// Actor navigation ray adjustment. Its two output vectors remain the real
// caller's locals, including the original false-return branch that writes
// both vectors for action 6/7 in biped state 4.
bool actor_direction(const Port &p,SourcePhysicsWorldRuntimeAbi &abi,Fp &fp) {
    const auto entry=abi.gp[4];const auto saved=abi.gp;
    abi.gp[4]-=0x1cu;push(p,abi,saved[5]);push(p,abi,saved[6]);push(p,abi,saved[7]);
    const auto frame=abi.gp[4],origin=saved[1],direction=saved[3];
    const auto actors=p.r32(0x5020c8u),actor=p.r32(actors+0x44u)+(saved[0]&65535u)*0x888u;
    const auto unit=p.r32(actor+0x18u),biped=body(p,unit);const auto kind=p.r8(biped+0xaau);
    p.w8(0x1fd8e1u,frame+0xfu,0);bool answer=false;
    Simd simd(fp);
    if(kind==0) {
        const auto action=p.r16(biped+u32(sx16(p.r16(biped+0x346u)))+0x36u);
        p.w16(0x1fd8fcu,frame+0x10u,action);
        if(action==6u||action==7u) {
            if(p.r8(biped+0x3dcu)==4u && biped+0x3ecu) {
                const auto source=biped+0x3ecu;
                auto value=p.r32(source);p.w32(0x1fda2bu,origin,value);
                value=p.r32(source+4u);const auto down=p.r32(0x474d1cu);p.w32(0x1fda36u,origin+4u,value);
                value=p.r32(source+8u);p.w32(0x1fda3cu,origin+8u,value);
                for(unsigned n=0;n<3;++n)p.w32(std::array{0x1fda41u,0x1fda46u,0x1fda4cu}[n],direction+n*4u,p.r32(down+n*4u));
            }
        } else {
            native_unit_animation_keys_detail::lookup(p.memory,unit,
                [&](u32 site,u32 value){p.w32(site,frame+0x14u,value);},
                [&](u32 site,u32 value){p.w32(site,frame+0x18u,value);});
            if(p.r8(biped+0x3dcu)==5u || p.r16(frame+0x10u)==5u || p.r32(frame+0x14u)==0x50000cbu) {
                auto component=p.r32(biped+0x70u);const auto minus_one=p.r32(0x46a0dcu);
                p.w32(0x1fd9e8u,direction,simd.mul(component,minus_one));
                p.w32(0x1fd9f5u,direction+4u,simd.mul(p.r32(biped+0x74u),minus_one));
                component=p.r32(biped+0x78u);p.w8(0x1fda00u,frame+0xfu,1);
                p.w32(0x1fda0eu,direction+8u,simd.mul(component,minus_one));answer=true;
            } else if(p.r8(actor+0x264u)) {
                const auto time=p.r32(0x51df54u),tick=p.r32(time+8u);
                if(std::bit_cast<i32>(tick)<std::bit_cast<i32>(p.r32(actor+0x634u))) {
                    native_campaign_ai_project_point_tu15(p.memory,actor+0x638u,frame+0x1cu,fp,p.observe);
                    for(unsigned n=0;n<3;++n) {
                        const auto projected=p.r32(frame+0x1cu+n*4u);
                        p.w32(std::array{0x1fd989u,0x1fd99bu,0x1fd9b0u}[n],direction+n*4u,simd.sub(projected,p.r32(actor+0x238u+n*4u)));
                    }
                    const auto magnitude=native_campaign_ai_normalize_vector_tu15(p.memory,direction,fp,p.observe);
                    if(simd.above(magnitude,p.r32(0x46a0e8u))){p.w8(0x1fd9c8u,frame+0xfu,1);answer=true;}
                }
            }
        }
    }
    if(!answer)answer=p.r8(frame+0xfu)!=0;
    for(const auto n:{5u,6u,7u})abi.gp[n]=saved[n];abi.gp[4]=entry+4u;al(abi,answer);return answer;
}

// 16C290/16C530 initialize the same collision packet in the same order.
void initialize_packet(const Port &p,u32 result,u32 pc) {
    const auto slot=p.r16(0x474c24u),one=p.r32(0x46a0d0u);
    p.w32(pc+0x10u,result,0);p.w32(pc+0x16u,result+4u,one);p.w16(pc+0x20u,result+0x1au,slot);
    p.w32(pc+0x2cu,result+0x14u,none);p.w16(pc+0x2fu,result+0x18u,none);
    p.w32(pc+0x37u,result+0x1cu,none);p.w16(pc+0x39u,result+0x20u,none);p.w16(pc+0x3du,result+0x22u,slot);
    p.w32(pc+0x46u,result+0x38u,none);p.w32(pc+0x49u,result+0x3cu,none);p.w32(pc+0x4cu,result+0x40u,none);
}
void packet_position(const Port &p,const Simd &simd,u32 result,u32 origin,u32 delta,u32 pc,Fp &fp) {
    const auto fraction=p.r32(result+4u);
    for(unsigned n=0;n<3;++n) {
        const auto d=p.r32(delta+n*4u),value=simd.add(simd.mul(d,fraction),p.r32(origin+n*4u));
        p.w32(std::array{pc,pc+0x16u,pc+0x29u}[n],result+8u+n*4u,value);
    }
    native_tu15_constraint_effect_location(p.memory,result+8u,result+0x1cu,fp,p.observe);
}
struct Queries {
    SourcePhysicsContactMemory &memory;u32 scratch;Fp &fp;const SourcePhysicsContactRecordObserver &observe;
    void begin() {
        auto *domain=dynamic_cast<NativeTu15RetainedStoreObservationDomain *>(&memory);
        if(!domain||!domain->withhold_defined_data(scratch,0x10000u))
            throw std::runtime_error("Native biped query requires its guarded private lifetime");
    }
    bool instance(u32 mask,u32 instance,u32 origin,u32 delta,u32 output) {
        begin();
        const Port p{memory,observe};initialize_packet(p,output,0x16c290u);
        if((mask&8u)&&!(mask&0x1fff0u))mask|=0x1fff0u;
        const auto found=native_tu15_query_camera_instance(memory,{instance,mask,native_tu15_camera_bsp_query_flags(mask),origin,delta,output},
            {scratch,scratch+0x34u,scratch+0x40u,scratch+0x4cu,scratch+0x480u},fp,0,observe);
        packet_position(p,Simd(fp),output,origin,delta,0x16c329u,fp);return found;
    }
    bool object(u32 mask,u32 object,u32 origin,u32 delta,u32 ignore_first,u32 ignore_second,u32 output) {
        begin();
        const Port p{memory,observe};initialize_packet(p,output,0x16c530u);
        if((mask&8u)&&!(mask&0x1fff0u))mask|=0x1fff0u;
        const bool hidden=native_tu15_object_or_ancestor_deleting(memory,object);
        bool found=false;
        if(!hidden) {
            NativeTu15CameraDynamicServices services;
            services.acquire_frame=[&](unsigned recursion){
                if(recursion>=12u)throw std::runtime_error("Native biped refresh requires larger recursion DATA");
                const auto data=scratch+0x4000u+recursion*0x1000u;
                return NativeTu15CameraDynamicFrame{data+0x200u,data+0x220u,data+0x1fcu,{data,data+64u,data+52u,data+76u,data+116u}};
            };
            found=native_tu15_query_dynamic_camera_object(memory,{object,0,mask,native_tu15_camera_bsp_query_flags(mask),origin,delta,ignore_first,ignore_second,output},fp,0,services,observe);
        }
        packet_position(p,Simd(fp),output,origin,delta,0x16c5e2u,fp);return found;
    }
    bool scene(u32 mask,u32 origin,u32 delta,u32 output) {
        begin();
        return native_tu15_query_bound_camera_scene(memory,{mask,origin,delta,none,none,output},{scratch},fp,0,observe);
    }
};

bool point_query(const Port &p,SourcePhysicsWorldRuntimeAbi &abi,Fp &fp,Queries &queries) {
    const auto entry=abi.gp[4];const auto saved=abi.gp;abi.gp[4]-=0x74u;push(p,abi,saved[5]);
    const auto object=p.r32(abi.gp[4]+0x80u);push(p,abi,saved[7]);const auto frame=abi.gp[4],direction=saved[1];
    if(saved[0]) {
        auto x=p.r32(saved[0]),y=p.r32(saved[0]+4u),z=p.r32(saved[0]+8u);
        p.w32(0xe1deau,frame+8u,x);p.w32(0xe1deeu,frame+0xcu,y);p.w32(0xe1df2u,frame+0x10u,z);
    } else {
        const auto unit=p.r32(frame+0x80u);push(p,abi,saved[6]);push(p,abi,0xe1e09u);
        native_campaign_object_world_point_frame_tu15(p.memory,unit,frame+8u,abi.gp[4],saved[7],fp,p.observe);abi.gp[4]=frame;
    }
    const Simd simd(fp);const auto epsilon=p.r32(0x46a240u);
    auto scaled=simd.mul(p.r32(direction),epsilon);auto x=simd.sub(p.r32(frame+8u),scaled);
    scaled=simd.mul(p.r32(direction+4u),epsilon);p.w32(0xe1e30u,frame+8u,x);
    auto y=simd.sub(p.r32(frame+0xcu),scaled);scaled=simd.mul(p.r32(direction+8u),epsilon);
    auto z=simd.sub(p.r32(frame+0x10u),scaled);auto component=p.r32(direction);p.w32(0xe1e57u,frame+0x10u,z);
    const auto length=p.r32(frame+0x88u);
    p.w32(0xe1e6au,frame+0x14u,simd.mul(component,length));
    p.w32(0xe1e79u,frame+0x18u,simd.mul(p.r32(direction+4u),length));
    component=simd.mul(p.r32(direction+8u),length);p.w32(0xe1e88u,frame+0xcu,y);p.w32(0xe1e8eu,frame+0x1cu,component);
    p.w16(0xe1e94u,frame+0x44u,none);
    bool hit=false;
    if(saved[3]!=none)hit=queries.instance(0x84000du,saved[3],frame+8u,frame+0x14u,frame+0x20u);
    if(!hit && object!=none)hit=queries.object(0x84000du,object,frame+8u,frame+0x14u,none,none,frame+0x20u);
    if(!hit) {
        abi.gp[0]=p.r32(frame+0x80u);push(p,abi,0xe1ef4u);native_campaign_ai_object_navigation_call_tu15(p.memory,0xe04b0u,abi,p.observe);
        const auto mask=(abi.gp[0]&255u)?0x800001u:0x84000du;
        hit=queries.scene(mask,frame+8u,frame+0x14u,frame+0x20u);
    }
    if(hit) {
        for(const auto [arg,field,pc]:std::array<std::array<u32,3>,3>{{{0x94,0x70,0xe1f33},{0x9c,0x60,0xe1f44},{0xa0,0x68,0xe1f55}}}) {
            const auto out=p.r32(frame+arg);if(out)p.w32(pc,out,p.r32(frame+field));
        }
        for(const auto [arg,field,pc]:std::array<std::array<u32,3>,2>{{{0x8c,0x28,0xe1f66},{0x90,0x48,0xe1f85}}}) {
            const auto out=p.r32(frame+arg);if(out)for(unsigned n=0;n<3;++n)p.w32(pc+std::array{0u,6u,13u}[n],out+n*4u,p.r32(frame+field+n*4u));
        }
        const auto out=p.r32(frame+0x98u);if(out)p.w32(0xe1fa4u,out,p.r32(frame+0x5cu));
    }
    abi.gp[5]=saved[5];abi.gp[7]=saved[7];abi.gp[4]=entry+0x28u;al(abi,hit);return hit;
}

void refresh(const Port &p,SourcePhysicsWorldRuntimeAbi &abi,Fp &fp,Queries &queries) {
    const auto entry=abi.gp[4];const auto saved=abi.gp;abi.gp[4]-=0x18u;
    const auto pool=p.r32(0x4ecbd0u),rows=p.r32(pool+0x44u);push(p,abi,saved[3]);push(p,abi,saved[5]);
    const auto last_output=p.r32(abi.gp[4]+0x3cu);push(p,abi,saved[6]);
    const auto biped=p.r32(rows+(saved[7]&65535u)*12u+8u),frame=abi.gp[4];
    auto x=p.r32(biped+0x368u),y=p.r32(biped+0x36cu),z=p.r32(biped+0x370u);p.w32(0xe211fu,frame+0x14u,z);
    const auto actor=p.r32(biped+0x12cu);p.w32(0xe212cu,frame+0xcu,x);p.w32(0xe2130u,frame+0x10u,y);
    bool adjusted=false;
    if(actor!=none) {abi.gp[0]=actor;abi.gp[3]=frame+0x18u;abi.gp[1]=frame+0xcu;push(p,abi,0xe2143u);adjusted=actor_direction(p,abi,fp);}
    const Simd simd(fp);
    if(!adjusted) {
        if(p.r8(biped+0x3dcu)==5u) {
            const auto multiplier=p.r32(0x46a0dcu);
            for(unsigned n=0;n<3;++n)p.w32(std::array{0xe2165u,0xe2174u,0xe2183u}[n],frame+0x18u+n*4u,simd.mul(p.r32(biped+0x70u+n*4u),multiplier));
        } else {
            if((p.r16(biped+0x348u)>>12u)&1u) {
                const auto first=p.r32(biped+0x374u),multiplier=p.r32(0x46a0dcu);
                p.w32(0xe21b3u,frame+0x18u,simd.mul(first,multiplier));
                p.w32(0xe21c5u,frame+0x1cu,simd.mul(p.r32(biped+0x378u),multiplier));
                const auto third=p.r32(biped+0x37cu);x=p.r32(biped+0x380u);y=p.r32(biped+0x384u);z=p.r32(biped+0x388u);
                p.w32(0xe21e5u,frame+0x20u,simd.mul(third,multiplier));p.w32(0xe21ebu,frame+0xcu,x);p.w32(0xe21efu,frame+0x10u,y);p.w32(0xe21f3u,frame+0x14u,z);
            } else {
                const auto down=p.r32(0x474d1cu);x=p.r32(down);y=p.r32(down+4u);z=p.r32(down+8u);
                p.w32(0xe2207u,frame+0x18u,x);p.w32(0xe220bu,frame+0x1cu,y);p.w32(0xe220fu,frame+0x20u,z);
            }
            const auto up=p.r32(0x474d10u),offset=p.r32(0x46a12cu);
            for(unsigned n=0;n<3;++n) {
                const auto addend=simd.mul(p.r32(up+n*4u),offset);
                p.w32(std::array{0xe222eu,0xe2243u,0xe2258u}[n],frame+0xcu+n*4u,simd.add(addend,p.r32(frame+0xcu+n*4u)));
            }
        }
    }
    const auto object_output=p.r32(frame+0x3cu),bsp_output=p.r32(frame+0x38u),sector_output=p.r32(frame+0x34u),point_output=p.r32(frame+0x30u);
    abi.gp[3]=p.r32(frame+0x2cu);push(p,abi,last_output);push(p,abi,object_output);push(p,abi,bsp_output);
    const auto object=p.r32(abi.gp[4]+0x34u);push(p,abi,sector_output);push(p,abi,0);push(p,abi,point_output);
    push(p,abi,0x40000000u);push(p,abi,object);push(p,abi,saved[7]);abi.gp[1]=frame+0x18u;abi.gp[0]=frame+0xcu;
    push(p,abi,0xe2291u);
    if(!point_query(p,abi,fp,queries)) {
        const auto sector=p.r32(frame+0x34u),bsp=p.r32(frame+0x38u);
        p.w32(0xe229du,sector,none);p.w32(0xe22a3u,object_output,none);p.w32(0xe22a9u,last_output,none);p.w32(0xe22b0u,bsp,none);
    }
    for(const auto n:{3u,5u,6u})abi.gp[n]=saved[n];abi.gp[4]=entry+0x20u;
}
#endif
}

void native_campaign_ai_biped_refresh_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    if(!abi.floating_point||abi.x87_depth)throw std::invalid_argument("Native biped refresh requires retained controls and empty x87");
    Fp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};require_masked(fp,"Native biped refresh");
    struct Publish {SourcePhysicsWorldRuntimeAbi &abi;Fp &fp;~Publish(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}}publish{abi,fp};
    const Port p{memory,observe};
    if(entry==0x1fd8a0u){actor_direction(p,abi,fp);return;}
    // The binding is consulted only by owners that consume collision DATA.
    auto *binding=NativeCampaignAiBipedRefreshScratchBindingTu15::State::current;
    while(binding&&binding->memory!=&memory)binding=binding->previous;
    if(!binding)throw std::runtime_error("Native biped refresh requires owned collision DATA binding");
    if(binding->active)throw std::runtime_error("Native biped refresh cannot reenter active collision DATA");
    if(binding->check_gap && !binding->check_gap(memory,binding->identity))
        throw std::runtime_error("Native private DATA aliases retained input/output");
    struct Active {bool &active;explicit Active(bool &value):active(value){active=true;}~Active(){active=false;}}active(binding->active);
    const std::array owned{NativeTu15ContactPrivateRange{binding->identity,NativeCampaignAiBipedRefreshScratchBindingTu15::bytes}};
    NativeTu15ContactCandidatePrivateData guarded(memory,owned,binding->retained);
    Queries queries{guarded,binding->identity,fp,observe};
    if(entry==0xe20f0u)refresh(p,abi,fp,queries);
    else if(entry==0xe1dd0u)point_query(p,abi,fp,queries);
    else throw std::invalid_argument("Unknown native biped refresh entry");
#else
    (void)memory;(void)entry;(void)abi;(void)observe;throw std::runtime_error("Native biped refresh arithmetic backend unavailable");
#endif
}
}
