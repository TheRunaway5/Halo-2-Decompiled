#pragma once
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_async_order_detail {
inline bool reference() {
    static const bool enabled=[]{const auto *value=std::getenv("HALO2_REFERENCE_ASYNC_QUEUE");return value&&*value;}();
    return enabled;
}
// Whole 1AB330 ordering decision. The input task views and last-read history
// remain live bank data: publishing the other file can alias its later offset.
// site_base retains the enclosing owner's published observation namespace;
// the older Arena wrapper reports its canonical 1A5080 sites even for TU1.5.
// Complete return ABI is retained. Arbitrary failing memory-adapter prefixes
// are a separate qualification; no partial-fault ABI claim is made here.
template<class State> bool admitted(State &s) {
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto *policy=s.instruction_cursor;
        if((policy->first_trace&&*policy->first_trace)||(policy->second_trace&&*policy->second_trace)||
            (s.observe&&!policy->observation_only_stores))return false;
    }
    return true;
}
template<class State> bool compare(State &s,std::uint32_t site_base) {
    if(!admitted(s))return false;
    using namespace physics_step_detail;
    auto &g=s.gp;
    const auto at=[&](unsigned offset){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=site_base+offset;};
    const auto read=[&](unsigned offset,std::uint32_t p,unsigned width=4){at(offset);return r(s.memory,p,width);};
    const auto store=[&](unsigned offset,std::uint32_t p,unsigned width,std::uint32_t value){at(offset);w(s.memory,s.observe,site_base+offset,p,width,value);};
    const auto push=[&](unsigned offset,std::uint32_t value){at(offset);s.push(site_base+offset,value);};
    const auto low=[&](std::uint32_t value){g[0]=(g[0]&0xffffff00u)|(value&255u);};
    constexpr std::uint32_t read_callback=0x1aaa80u;
    push(0,g[5]);g[5]=read(1,g[4]+16u);low(0);
    const auto incoming_priority=std::bit_cast<std::int32_t>(g[2]);
    const auto other_priority=std::bit_cast<std::int32_t>(g[1]);
    push(9,g[6]);g[6]=read(10,g[4]+12u);
    if(incoming_priority>other_priority)low(1);
    else if(incoming_priority<other_priority)low(0);
    else {
        const auto incoming_callback=read(0x1au,g[4]+16u);
        if(incoming_callback==read_callback) {
            if(g[5]!=read_callback)low(1);
            else {
                low(read(0x3cu,g[6]+8u,1));
                if(g[0]&255u) {
                    g[2]=read(0x47u,g[6]);g[0]=read(0x49u,g[7]);g[1]=read(0x4bu,g[3]);
                    g[0]-=g[2];g[1]-=g[2];
                    if(g[0]<g[1])low(1);
                    else if(g[0]>g[1])low(0);
                    else {
                        g[0]=read(0x5fu,g[6]+4u);g[1]=read(0x62u,g[7]+12u);g[2]=read(0x65u,g[3]+12u);
                        g[1]-=g[0];g[2]-=g[0];low(g[1]<g[2]);
                    }
                }else low(0);
            }
        }else if(g[5]==read_callback)low(0);
    }
    // All successful decisions end with this source comparison. It precedes
    // the history writes, which must retain their exact read/store ordering.
    s.flags.minus(g[5],read_callback,4);
    if(g[5]==read_callback) {
        g[1]=read(0x79u,g[3]);store(0x7bu,g[6],4,g[1]);
        g[2]=read(0x7du,g[3]+12u);store(0x80u,g[6]+4u,4,g[2]);
        store(0x83u,g[6]+8u,1,1);
    }
    at(0x87u);g[6]=s.pop();at(0x88u);g[5]=s.pop();at(0x89u);s.pop();g[4]+=12u;
    return true;
}

// Insert a task into the priority/read-locality queue. The retained frame
// carries the read-history tie breaker between comparisons. It stays live
// because task data and queue links can alias that frame or one another.
// The comparator is an actual callee with its complete entry/return ABI.
template<class State,class Compare> bool insert(State &s,std::uint32_t site_base,Compare &&before) {
    if(!admitted(s))return false;
    using namespace physics_step_detail;
    auto&g=s.gp;
    const auto at=[&](unsigned offset){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=site_base+offset;};
    const auto read=[&](unsigned offset,std::uint32_t p){at(offset);return r(s.memory,p,4);};
    const auto store=[&](unsigned offset,std::uint32_t p,std::uint32_t value){at(offset);w(s.memory,s.observe,site_base+offset,p,4,value);};
    const auto push=[&](unsigned offset,std::uint32_t value){at(offset);s.push(site_base+offset,value);};
    const auto pop=[&](unsigned offset){at(offset);return s.pop();};
    const auto finish=[&](unsigned bp_site,unsigned add_site,unsigned return_site){
        g[5]=pop(bp_site);at(add_site);g[4]=s.flags.plus(g[4],0x14u,4);pop(return_site);g[4]+=4u;
    };
    g[4]-=0x14u;g[0]=0;store(5,g[4]+8u,0);push(9,g[5]);
    g[5]=read(0xau,0x4ef1bcu);store(0x12u,g[4]+0x10u,0);store(0x16u,g[4]+0x14u,0);
    g[0]=read(0x1au,g[4]+0x1cu);
    if(!g[5]) {
        store(0x20u,0x4ef1bcu,g[0]);store(0x25u,g[0]+0x3cu,g[5]);finish(0x28u,0x29u,0x2cu);return true;
    }
    g[2]=read(0x2fu,g[0]);const auto head_priority=read(0x31u,g[5]);store(0x34u,g[4]+4u,g[2]);
    if(std::bit_cast<std::int32_t>(g[2])>std::bit_cast<std::int32_t>(head_priority)) {
        store(0x3au,g[0]+0x3cu,g[5]);store(0x3du,0x4ef1bcu,g[0]);finish(0x42u,0x43u,0x46u);return true;
    }
    g[1]=read(0x49u,g[5]+0x3cu);const bool scan=g[1]!=0;
    push(0x4eu,g[3]);push(0x4fu,g[6]);push(0x50u,g[7]);
    if(scan) {
        // The head-next test is the last flag writer before the first call.
        s.flags.logic(g[1],4);
        g[1]=read(0x53u,g[0]+0x34u);g[7]=g[0]+8u;store(0x59u,g[4]+0x14u,g[1]);
        for(;;) {
            g[6]=read(0x63u,g[5]+0x3cu);g[0]=read(0x66u,g[6]+0x34u);g[1]=read(0x69u,g[4]+0x14u);
            push(0x6du,g[0]);push(0x6eu,g[1]);g[1]=read(0x6fu,g[6]);g[0]=g[4]+0x20u;g[3]=g[6]+8u;
            push(0x78u,g[0]);push(0x79u,0x12333eu);before(s);
            if(g[0]&255u) {
                g[1]=read(0xa3u,g[5]+0x3cu);g[0]=read(0xa6u,g[4]+0x28u);g[7]=pop(0xaau);g[6]=pop(0xabu);
                store(0xacu,g[0]+0x3cu,g[1]);g[3]=pop(0xafu);store(0xb0u,g[5]+0x3cu,g[0]);
                finish(0xb3u,0xb4u,0xb7u);return true;
            }
            g[5]=g[6];g[0]=read(0x84u,g[5]+0x3cu);
            if(!g[0]){g[0]=read(0x8bu,g[4]+0x28u);break;}
            s.flags.logic(g[0],4);g[2]=read(0x5fu,g[4]+0x10u);
        }
    }
    g[7]=pop(0x8fu);g[6]=pop(0x90u);store(0x91u,g[5]+0x3cu,g[0]);g[3]=pop(0x94u);
    store(0x95u,g[0]+0x3cu,0);finish(0x9cu,0x9du,0xa0u);return true;
}
} // namespace halo2::native_async_order_detail
