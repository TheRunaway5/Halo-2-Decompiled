#pragma once
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_geometry_request_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_GEOMETRY_RESOURCE");
        return value&&*value;
    }();
    return enabled;
}
// Whole130980 controller. The retained frame is live DATA: policy fields,
// saved words and resource records may alias. In particular, acquisition and
// completion can change pointers and policy bytes before their next use.
// Call owns the genuine acquisition/promotion/yield/pump/publication services.
// Integer flags are published at those service boundaries and at return;
// arbitrary adapter memory-fault publication is a separate qualification.
template<class State> bool admitted(const State &s) {
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto *cursor=s.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace)||
            (s.observe&&!cursor->observation_only_stores))return false;
    }
    return true;
}
template<class State,class Call> bool run(State &s,Call call) {
    using namespace physics_step_detail;
    if(!admitted(s))return false;
    auto &gp=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t p,unsigned width=4){at(pc);return r(s.memory,p,width);};
    const auto store=[&](std::uint32_t pc,std::uint32_t p,unsigned width,std::uint32_t v){at(pc);w(s.memory,s.observe,pc,p,width,v);};
    const auto push=[&](std::uint32_t pc,std::uint32_t v){at(pc);s.push(pc,v);};
    const auto low=[&](unsigned reg,std::uint32_t value){gp[reg]=(gp[reg]&0xffffff00u)|(value&255u);};
    const auto invoke=[&](std::uint32_t pc,std::uint32_t continuation,std::uint32_t target){
        at(pc);push(pc,continuation);call(s,target,pc);
    };
    const auto finish=[&](std::uint32_t pc){
        gp[4]=s.flags.plus(gp[4],12u,4);at(pc);s.pop();
    };
    gp[4]-=12u;push(0x130983u,gp[7]);gp[7]=gp[1];
    low(2,read(0x130986u,gp[7]+0x1eu,1));low(1,0);
    if(gp[2]&255u) {
        gp[0]=read(0x13098fu,gp[7]+0x20u)&65535u;gp[1]=gp[0]*3u;
        gp[0]=read(0x13099au,0x4f2d6cu);gp[2]=read(0x13099fu,gp[0]+0x64u);
        gp[0]=read(0x1309a2u,gp[0]+0x38u);gp[2]=read(0x1309a5u,gp[2]+0x44u);
        store(0x1309a8u,gp[2]+gp[1]*8u+0x14u,4,gp[0]);
        low(0,1);gp[7]=s.pop();finish(0x1309b2u);return true;
    }
    gp[2]=read(0x1309b3u,gp[7]+8u);
    bool valid=std::bit_cast<std::int32_t>(gp[2])>0;
    if(valid){gp[2]=read(0x1309beu,gp[7]+12u);valid=std::bit_cast<std::int32_t>(gp[2])>=0;}
    if(!valid){low(0,gp[1]);gp[7]=s.pop();finish(0x130bceu);return true;}

    // Decode the request and publish its byte fields in their original frame.
    low(1,gp[0]&1u);push(0x1309ceu,gp[3]);store(0x1309cfu,gp[4]+8u,1,gp[1]);
    gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(0x1309d3u,gp[7]+0x18u,2)))));
    gp[3]=gp[0];gp[2]=gp[0];gp[0]>>=3u;low(0,gp[0]&1u);
    push(0x1309e0u,gp[6]);gp[6]=read(0x1309e1u,0x4f0414u);gp[1]<<=4u;
    gp[1]=read(0x1309eau,gp[1]+gp[6]);gp[3]>>=2u;gp[2]>>=1u;low(2,gp[2]&1u);
    store(0x1309f5u,gp[4]+14u,1,gp[0]);low(0,0);low(3,gp[3]&1u);
    store(0x130a04u,gp[4]+15u,1,gp[2]);store(0x130a08u,gp[4]+20u,1,gp[0]);
    store(0x130a0cu,gp[4]+13u,1,gp[0]);
    if(gp[1]==0x6c746d70u||gp[1]==0x73627370u)store(0x130a1au,gp[4]+13u,1,1);
    low(1,read(0x130a1fu,0x4751acu,1));
    if(gp[1]&255u)store(0x130a29u,gp[4]+12u,1,0);
    gp[6]=read(0x130a2eu,0x4f2d70u);
    bool priority_written=false,choose_flags=false;
    if(std::bit_cast<std::int32_t>(gp[6])>0&&(gp[2]&255u)&&!(gp[3]&255u)) {
        low(1,read(0x130a40u,gp[4]+12u,1));
        if(gp[1]&255u){store(0x130a89u,gp[4]+16u,4,7);priority_written=true;}
        else {
            gp[1]=read(0x130a48u,0x4f2d74u);
            bool selective=gp[1]==0;
            if(gp[1]==1){low(1,read(0x130a57u,gp[4]+13u,1));selective=(gp[1]&255u)!=0;}
            if(selective){low(0,1);store(0x130a61u,gp[4]+20u,1,gp[0]);}
            if(gp[6]==1) {
                if(gp[0]&255u)store(0x130a6eu,gp[4]+12u,1,1);
                else choose_flags=true;
            }
            if(!choose_flags&&(gp[0]&255u)){store(0x130a77u,gp[4]+16u,4,2);priority_written=true;}
        }
    }
    if(!priority_written) {
        if(!choose_flags){low(0,read(0x130a81u,gp[4]+12u,1));
            if(gp[0]&255u){store(0x130a89u,gp[4]+16u,4,7);priority_written=true;}}
        if(!priority_written) {
            low(0,read(0x130a93u,gp[4]+14u,1));
            if(gp[0]&255u)store(0x130a9bu,gp[4]+16u,4,5);
            else {gp[0]=(gp[3]&255u)?1u:2u;store(0x130aadu,gp[4]+16u,4,gp[0]);}
        }
    }
    if(read(0x130ab1u,gp[7]+0x20u)==0xffffffffu&&(gp[2]&255u)) {
        s.flags.logic(gp[2]&255u,1);
        gp[2]=read(0x130abbu,gp[4]+16u);gp[0]=read(0x130abfu,gp[4]+20u);
        push(0x130ac3u,gp[2]);push(0x130ac4u,gp[0]);low(0,read(0x130ac5u,gp[4]+20u,1));
        push(0x130ac9u,gp[7]);invoke(0x130acau,0x130acfu,0x130eb0u);
        low(2,read(0x130acfu,gp[4]+15u,1));
    }

    // Touch the linked record, promote its real job and wait on real completion.
    gp[0]=read(0x130ad3u,gp[7]+0x20u);
    if(gp[0]!=0xffffffffu) {
        gp[2]=read(0x130adfu,0x4f2d5cu);gp[2]=read(0x130ae5u,gp[2]+0x44u);
        gp[0]&=65535u;gp[1]=gp[0]*3u;push(0x130af0u,gp[5]);gp[5]=gp[2]+gp[1]*4u;
        gp[1]=read(0x130af4u,0x4f2d6cu);gp[6]=read(0x130afau,gp[1]+0x64u);
        low(2,read(0x130afdu,gp[5]+2u,1));gp[6]=read(0x130b02u,gp[6]+0x44u);
        gp[3]=read(0x130b05u,gp[1]+0x38u);gp[0]*=3u;
        store(0x130b0bu,gp[6]+gp[0]*8u+0x14u,4,gp[3]);
        if(!(gp[2]&255u)) {
            gp[2]=read(0x130b11u,gp[5]+3u,1);
            const auto priority=read(0x130b15u,gp[4]+20u);
            if(std::bit_cast<std::int32_t>(priority)>std::bit_cast<std::int32_t>(gp[2])) {
                s.flags.minus(priority,gp[2],4);gp[6]=read(0x130b1bu,gp[5]+4u);push(0x130b1eu,7);
                invoke(0x130b20u,0x130b25u,0x1236a0u);low(0,read(0x130b25u,gp[4]+20u,1));
                gp[1]=read(0x130b29u,0x4f2d6cu);store(0x130b2fu,gp[5]+3u,1,gp[0]);
            }
            low(0,read(0x130b32u,gp[4]+16u,1));
            if(gp[0]&255u) {
                low(0,read(0x130b3au,gp[5]+2u,1));
                if(!(gp[0]&255u)) {
                    low(0,read(0x130b41u,gp[5]+2u,1));
                    if(!(gp[0]&255u)) {
                        do {s.flags.logic(gp[0]&255u,1);invoke(0x130b48u,0x130b4du,0x2dbf74u);
                            invoke(0x130b4du,0x130b52u,0x128870u);low(0,read(0x130b52u,gp[5]+2u,1));
                        }while(!(gp[0]&255u));
                        gp[1]=read(0x130b59u,0x4f2d6cu);
                    }
                }
            }
        }
        low(0,read(0x130b5fu,gp[5]+2u,1));gp[5]=s.pop();
        if(gp[0]&255u) {
            low(0,read(0x130b67u,gp[7]+0x1eu,1));low(3,1);
            if(!(gp[0]&255u)) {
                gp[0]=read(0x130b70u,gp[7]+0x20u)&65535u;gp[2]=gp[0]*3u;
                gp[0]=read(0x130b7bu,gp[1]+0x64u);gp[0]=read(0x130b7eu,gp[0]+0x44u);
                gp[2]=read(0x130b81u,gp[0]+gp[2]*8u+8u);gp[1]=read(0x130b85u,gp[1]+0x34u);
                gp[2]<<=(gp[1]&31u);gp[2]=s.flags.plus(gp[2],read(0x130b8au,0x4f2d64u),4);
                push(0x130b90u,gp[2]);invoke(0x130b91u,0x130b96u,0x22a680u);
                store(0x130b96u,gp[7]+0x1eu,1,gp[3]);
            }
            gp[6]=s.pop();low(0,gp[3]);gp[3]=s.pop();gp[7]=s.pop();finish(0x130ba1u);return true;
        }
    }else if(!(gp[3]&255u)&&(gp[2]&255u))store(0x130baau,0x4f2d80u,1,1);
    low(0,read(0x130bb1u,gp[4]+13u,1));
    if(gp[0]&255u)store(0x130bb9u,0x4f2d78u,4,read(0x130bb9u,0x4f2d78u)+1u);
    gp[6]=s.pop();gp[3]=s.pop();low(0,0);gp[7]=s.pop();finish(0x130bc7u);return true;
}
} // namespace halo2::native_geometry_request_detail
