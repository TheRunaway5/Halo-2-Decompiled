#include "halo2/physics_agent_scheduler.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <xmmintrin.h>
#include <stdexcept>
#include <string>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using S=SourcePhysicsAgentSchedulerServices;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
    for(unsigned k=0;k<n;++k)v|=std::to_integer<std::uint32_t>(b[k])<<(k*8);
    return v;
}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint64_t v) {
    SourcePhysicsContactRecordStore x{site,p,{},{}};x.before.resize(n);x.written.resize(n);m.read(p,x.before);
    for(unsigned k=0;k<n;++k)x.written[k]=std::byte((v>>(k*8))&255u);
    m.write(p,x.written);if(o)o(x);
}
std::int32_t sign(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t call(M&m,const S&s,std::uint32_t site,std::uint32_t entry,
    std::uint32_t receiver,std::uint32_t esp,std::vector<std::uint32_t>args,bool cdecl,const O&o) {
    if(!s.required_owner)throw std::runtime_error("Required original agent scheduler owner unavailable at "+std::to_string(entry));
    return s.required_owner(m,{site,entry,receiver,esp,std::move(args),cdecl},o);
}
struct FpScope {
    std::fenv_t saved;unsigned mx;
    explicit FpScope(SourcePhysicsFactoryFloatingPoint p):mx(_mm_getcsr()) {
        if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
            throw std::runtime_error("Unmasked original agent floating point owner unavailable");
        if(std::fegetenv(&saved)!=0)throw std::runtime_error("Original agent FP environment unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fldcw %0" ::"m"(p.x87_control_word):"memory");
#else
        throw std::runtime_error("Original agent x87 backend unavailable");
#endif
        _mm_setcsr(p.mxcsr);
    }
    ~FpScope(){std::fesetenv(&saved);_mm_setcsr(mx);}
};
float f(std::uint32_t v){return std::bit_cast<float>(v);}
std::uint32_t bits(float v){return std::bit_cast<std::uint32_t>(v);}
// These original FCOM/FCOMP predicates are x87 comparisons; widening keeps
// their input loads independent of SSE DAZ/FTZ. FP status is not an output.
bool less(std::uint32_t a,std::uint32_t b){return static_cast<long double>(f(a))<static_cast<long double>(f(b));}
bool greater(std::uint32_t a,std::uint32_t b){return static_cast<long double>(f(a))>static_cast<long double>(f(b));}
long double multiply(long double a,long double b){volatile long double x=a*b;return x;}
long double add(long double a,long double b){volatile long double x=a+b;return x;}
std::uint32_t spill(long double v){volatile float x=static_cast<float>(v);return bits(x);}
void profile(M&m,const S&s,bool finish,std::uint32_t entry,const O&o) {
    const auto p=r(m,0x487674);if(p>=r(m,0x487678))return;
    w(m,o,finish?0x31e15au:0x31df11u,p,4,finish?0x4188f4u:0x41b668u);
    if(!s.timestamp_counter)throw std::runtime_error("Required original agent RDTSC provider unavailable");
    const auto tick=std::uint32_t(s.timestamp_counter());
    w(m,o,finish?0x31e162u:0x31df19u,finish?entry+4u:entry-36u,4,tick);
    w(m,o,finish?0x31e16au:0x31df21u,p+4u,4,tick);
    w(m,o,finish?0x31e170u:0x31df27u,0x487674,4,p+12u);
}
void listeners(M&m,std::uint32_t owner,std::uint32_t packet,std::uint32_t entry,bool world,const S&s,const O&o) {
    const auto bank_off=world?0xb4u:0x60u,count_off=world?0xb8u:0x64u;
    w(m,o,world?0x31e9db:0x31c3cb,packet+0x10,4,world?0u:owner);
    auto k=r(m,owner+count_off)-1u;
    while(sign(k)>=0) {
        const auto listener=r(m,r(m,owner+bank_off)+k*4u);
        if(listener) {
            const auto target=r(m,r(m,listener)+8);
            if(target==0x27e3a0)source_deliver_physics_object_contact_listener_1_1(m,packet,entry-20u,s,o);
            else if(target!=0x27e760)call(m,s,world?0x31ea00:0x31c3e1,target,listener,entry-20u,{packet},false,o);
            // Complete actual27E760 is RET4, not a successful contact result.
        }
        --k;
    }
    k=r(m,owner+count_off)-1u;
    while(sign(k)>=0) {
        if(r(m,r(m,owner+bank_off)+k*4u)==0) {
            const auto n=r(m,owner+count_off)-1u;
            w(m,o,world?0x31ea27:0x31c401,owner+count_off,4,n);
            for(auto j=k;sign(j)<sign(r(m,owner+count_off));++j) {
                const auto bank=r(m,owner+bank_off);
                w(m,o,world?0x31ea3e:0x31c41a,bank+j*4u,4,r(m,bank+(j+1u)*4u));
            }
        }
        --k;
    }
}
}
std::uint32_t source_construct_physics_agent_output_bank_1_1(M&m,std::uint32_t d,
    std::uint32_t count,const S&s,const O&o) {
    w(m,o,0x31d610,d,4,0);w(m,o,0x31d612,d+4,4,0);w(m,o,0x31d615,d+8,4,0x80000000);
    const auto adapter=r(m,0x487638),bytes=(count*48u+15u)&~15u,available=r(m,adapter+0xc);
    std::uint32_t p;
    if(sign(bytes)>sign(available)) {
        if(!s.scratch.scratch_allocate)throw std::runtime_error("Required original agent output allocator virtual+24 unavailable");
        p=s.scratch.scratch_allocate(m,adapter,r(m,r(m,adapter)+0x24),bytes,o);
    }else {
        p=r(m,adapter+8);w(m,o,0x31d64d,adapter+8,4,p+bytes);
        w(m,o,0x31d655,adapter+0xc,4,available-bytes);
    }
    w(m,o,0x31d661,d+8,4,count|0x80000000u);w(m,o,0x31d668,d+0x10,4,count);
    w(m,o,0x31d66b,d,4,p);w(m,o,0x31d66d,d+0xc,4,p);return d;
}
void source_destroy_physics_agent_output_bank_1_1(M&m,std::uint32_t d,const S&s,const O&o) {
    const auto adapter=r(m,0x487638),p=r(m,d+0xc),bytes=(r(m,d+0x10)*48u+15u)&~15u;
    const auto alternate=r(m,adapter+0x14);
    if(bytes!=0&&(p==alternate||alternate==0)) {
        if(!s.scratch.scratch_free)throw std::runtime_error("Required original agent output allocator virtual+28 unavailable");
        s.scratch.scratch_free(m,adapter,r(m,r(m,adapter)+0x28),p,bytes,o);
    }else {
        w(m,o,0x31d6ee,adapter+8,4,r(m,adapter+8)-bytes);
        w(m,o,0x31d6f1,adapter+0xc,4,r(m,adapter+0xc)+bytes);
    }
    const auto capacity=r(m,d+8);
    if(sign(capacity)>=0) {
        const auto backend=r(m,0x480da0),block=r(m,d),n=(capacity&0x7fffffffu)*48u;
        if(r(m,r(m,adapter)+0x14)!=0x148a70u||
           (block&&(sign(block)<sign(backend+0x4340u)||sign(block)>sign(backend+0xf0340u)||r(m,r(m,backend)+0x14)!=0x22eb20u)))
            throw std::runtime_error("Required original grown agent-output free owner unavailable");
        if(block)source_free_physics_body_small_block_1_1(m,backend,block,n,o);
    }
}
void source_deliver_physics_world_contact_listeners_1_1(M&m,std::uint32_t world,std::uint32_t packet,std::uint32_t entry,const S&s,const O&o){listeners(m,world,packet,entry,true,s,o);}
void source_deliver_physics_entity_contact_listeners_1_1(M&m,std::uint32_t entity,std::uint32_t packet,std::uint32_t entry,const S&s,const O&o){listeners(m,entity,packet,entry,false,s,o);}
void source_deliver_physics_agent_output_1_1(M&m,std::uint32_t manager,
    const std::array<std::uint32_t,4>&args,std::uint32_t entry,const S&s,const O&o) {
    const auto d=r(m,args[3]),rows=r(m,d),count=r(m,d+4);
    for(std::uint32_t k=0;sign(k)<sign(count);++k) {
        const auto src=rows+k*48u,index=r(m,src+0x20,2),slot=r(m,r(m,manager+0x30)+index,1);
        const auto dst=r(m,manager+0x44)+slot*32u;
        if((src|dst)&15u)throw std::runtime_error("Original aligned contact MOVAPS backing unavailable");
        for(unsigned half=0;half<2;++half) {
            std::array<std::byte,16>b{};m.read(src+half*16u,b);
            for(unsigned lane=0;lane<2;++lane) {
                std::uint64_t v=0;for(unsigned k2=0;k2<8;++k2)v|=std::uint64_t(std::to_integer<unsigned char>(b[lane*8+k2]))<<(k2*8);
                w(m,o,half?0x31f591:0x31f586,dst+half*16u+lane*8u,8,v);
            }
        }
    }
    w(m,o,0x31f597,manager+0xc,4,r(m,manager+0xc)-1u);
    if(r(m,manager+0xc)!=0)return;
    const auto a=r(m,args[0]+0x20),b=r(m,args[1]+0x20);
    const auto rate_a=r(m,a+0x5c,2),rate_b=r(m,b+0x5c,2);
    w(m,o,0x31f5c4,manager+0xc,4,(rate_a<rate_b?rate_a:rate_b)+1u);
    const auto packet=entry-20u;
    w(m,o,0x31f5cb,packet+0xc,4,args[3]);
    w(m,o,0x31f5d4,packet,4,manager);w(m,o,0x31f5d8,packet+4,4,args[0]);w(m,o,0x31f5dc,packet+8,4,args[1]);
    source_deliver_physics_world_contact_listeners_1_1(m,r(m,manager+8),packet,entry-48u,s,o);
    if(r(m,a+0x64))source_deliver_physics_entity_contact_listeners_1_1(m,a,packet,entry-48u,s,o);
    if(r(m,b+0x64))source_deliver_physics_entity_contact_listeners_1_1(m,b,packet,entry-48u,s,o);
}
void source_refresh_physics_entity_contact_bank_1_1(M&m,std::uint32_t packet,
    const std::array<std::uint32_t,5>&a,std::uint32_t entry,const S&s,const O&o) {
    const auto component=a[0];auto offset=0u;
    auto descriptor=r(m,r(m,packet+12));
    for(auto emitted=0u;sign(emitted)<sign(r(m,descriptor+4));++emitted,offset+=48u) {
        const auto src=r(m,descriptor)+offset;
        for(auto k=0u;sign(k)<sign(r(m,component+0x8c));++k) {
            const auto row=r(m,component+0x88)+k*72u;
            const auto signed_byte=[](std::uint32_t v){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))));};
            if(r(m,row,2)!=r(m,src+0x20,2)||r(m,row+0x14)!=a[2]||
                signed_byte(r(m,row+0x45,1))!=a[3]||signed_byte(r(m,row+0x46,1))!=a[4])continue;
            const auto first=r(m,packet+4);
            if(r(m,first+0x18)!=1)continue;
            const auto body=r(m,first+0x20);if(!body)continue;
            const auto flipped=body!=r(m,packet+16);
            // Source only writes AL into this retained stack word. The
            // upper24 bits come from real caller backing, never zero-filled.
            w(m,o,0x1d41e7,entry-4u,1,flipped?1u:0u);
            if(!s.floating_point)throw std::runtime_error("Required original contact refresh FP owner unavailable");
            source_refresh_physics_contact_1_1(m,{r(m,packet),row,{0,r(m,entry-4u),a[1],a[3]},
                s.floating_point->x87_control_word,s.floating_point->mxcsr},o);
        }
        descriptor=r(m,r(m,packet+12));
    }
}
void source_deliver_physics_object_contact_listener_1_1(M&m,std::uint32_t packet,
    std::uint32_t entry,const S&s,const O&o) {
    const auto body=[&](std::uint32_t p) {
        while(const auto parent=r(m,p+12))p=parent;
        return r(m,p+0x18)==1?r(m,p+0x20):0u;
    };
    const auto a=body(r(m,packet+4)),b=body(r(m,packet+8));if(!a||!b)return;
    const auto property=[&](std::uint32_t p,std::uint32_t key) {
        const auto bank=r(m,p+0x30),n=r(m,p+0x34);
        for(auto k=0u;sign(k)<sign(n);++k)if(r(m,bank+k*8u)==key)return r(m,bank+k*8u+4);
        return 0u;
    };
    const auto ai=property(a,0x2001),bi=property(b,0x2001),as=property(a,0x2002),bs=property(b,0x2002);
    const auto base=r(m,r(m,0x526248)+0x44);const auto first=r(m,packet+16)==a;
    const auto mine=first?ai:bi,other=first?bi:ai,slot=first?as:bs,other_slot=first?bs:as;
    const auto component=base+(mine&0xffffu)*0xa0u;
    if(r(m,component+4,1)&1u)source_refresh_physics_entity_contact_bank_1_1(m,packet,{component,mine,other,slot,other_slot},entry-44u,s,o);
}
std::uint32_t source_physics_agent_minimum_distance_1_1(const M&m,std::uint32_t packet) {
    auto result=r(m,0x41b5f4);const auto d=r(m,packet),bank=r(m,d),n=r(m,d+4);
    for(std::uint32_t k=0;sign(k)<sign(n);++k) {
        const auto candidate=r(m,bank+k*48u+12u);
        if(greater(result,candidate))result=candidate;
    }
    return result;
}
void source_process_physics_island_agents_1_1(M&m,std::uint32_t island,
    std::uint32_t step,std::uint32_t entry,SourcePhysicsFactoryFloatingPoint fp,
    const S&s,const O&o) {
    FpScope scope(fp);profile(m,s,false,entry,o);
    const auto d=entry-32u,packet=entry-44u;
    source_construct_physics_agent_output_bank_1_1(m,d,64,s,o);
    w(m,o,0x31df4b,packet,4,d);
    const auto count=r(m,island+0x6c);auto row=r(m,island+0x68);
    if(sign(count)>0) {
        w(m,o,0x31df57,entry-36u,4,count);
        for(auto remaining=count;remaining!=0;--remaining,row+=16u) {
            const auto agent=r(m,row+8);
            if(agent) {
                w(m,o,0x31df74,d+4,4,0);
                const auto state=r(m,row+12),world=r(m,island+0x20);
                w(m,o,0x31df85,packet+4,4,state);
                const auto control=r(m,world+0x254);
                if(sign(state)>=0) {
                    const auto value=spill(add(multiply(static_cast<long double>(sign(state)),static_cast<long double>(f(r(m,world+0x248)))),static_cast<long double>(f(r(m,world+0x244)))));
                    w(m,o,0x31dfa9,control+12,4,2);
                    w(m,o,0x31dfb5,packet+4,4,value);w(m,o,0x31dfb9,r(m,r(m,island+0x20)+0x254)+16,4,value);
                    call(m,s,0x31dfce,r(m,r(m,agent)+16),agent,entry-80u,{r(m,row)-16u,r(m,row+4)-16u,step,packet},false,o);
                    bool refine=r(m,r(m,r(m,island+0x20)+0x254)+12)==0;
                    const auto output=r(m,packet),bank=r(m,output),n=r(m,output+4);
                    if(!refine)for(std::uint32_t k=0;sign(k)<sign(n);++k)if(less(r(m,bank+k*48u+12),r(m,packet+4))){refine=true;break;}
                    if(refine) {
                        w(m,o,0x31e059,row+12,4,r(m,row+12)+1u);
                        const auto next=spill(add(static_cast<long double>(f(r(m,packet+4))),static_cast<long double>(f(r(m,r(m,island+0x20)+0x248)))));
                        w(m,o,0x31e069,packet+4,4,next);
                        call(m,s,0x31e077,0x31ca20,island,entry-80u,{step,row,packet,next},false,o);
                    }else if(sign(r(m,row+12))>0) {
                        const auto previous=r(m,row+12)-1u;
                        w(m,o,0x31e019,packet+4,4,previous);
                        const auto value2=spill(add(multiply(static_cast<long double>(sign(previous)),static_cast<long double>(f(r(m,r(m,island+0x20)+0x248)))),static_cast<long double>(f(r(m,r(m,island+0x20)+0x244)))));
                        w(m,o,0x31e031,packet+4,4,value2);
                        if(greater(source_physics_agent_minimum_distance_1_1(m,packet),value2))w(m,o,0x31e049,row+12,4,previous);
                    }
                }else {
                    w(m,o,0x31e08a,control+12,4,2);w(m,o,0x31e096,r(m,r(m,island+0x20)+0x254)+16,4,0xc47a0000);
                    call(m,s,0x31e0af,r(m,r(m,agent)+16),agent,entry-80u,{r(m,row)-16u,r(m,row+4)-16u,step,packet},false,o);
                    if(r(m,row+12)==0xfffffffeu&&r(m,r(m,r(m,island+0x20)+0x254)+12)!=0) {
                        bool retain=false;const auto output=r(m,packet),bank=r(m,output),n=r(m,output+4);
                        for(std::uint32_t k=0;sign(k)<sign(n);++k)if(less(r(m,bank+k*48u+12),r(m,0x465048))){retain=true;break;}
                        if(!retain)w(m,o,0x31e0f7,row+12,4,0);
                    }
                }
                const auto current_agent=r(m,row+8),manager=r(m,current_agent+8);
                const auto target=r(m,r(m,manager)+12);
                const std::array args{r(m,row)-16u,r(m,row+4)-16u,step,packet};
                if(target==0x31f550)source_deliver_physics_agent_output_1_1(m,manager,args,entry-80u,s,o);
                else call(m,s,0x31e11d,target,manager,entry-80u,{args[0],args[1],args[2],args[3]},false,o);
            }
            w(m,o,0x31e12d,entry-36u,4,remaining-1u);
        }
    }
    source_destroy_physics_agent_output_bank_1_1(m,d,s,o);profile(m,s,true,entry,o);
}
void source_schedule_physics_island_agents_1_1(M&m,std::uint32_t island,std::uint32_t backend,
    std::uint32_t step,std::uint32_t entry,SourcePhysicsFactoryFloatingPoint fp,const S&s,const O&o) {
    const auto allocated=[&]() {
        auto sum=0u;
        for(const auto global:{0x480da4u,0x480da8u})if(const auto heap=r(m,global)) {
            const auto n=r(m,heap+0x20);auto p=r(m,heap+0x1c)-4u+r(m,heap+0x18);
            for(auto k=0u;sign(k)<sign(n);++k,p-=8u)sum+=r(m,p);
        }
        return sum;
    };
    w(m,o,0x27f0f4,0x5660bc,4,island);w(m,o,0x27f0fe,0x5660b8,4,allocated());
    call(m,s,0x31e2d9,0x31db70,island,entry-20u,{backend,step},false,o);
    if(r(m,r(m,island+0x20)+0x23e,1))source_process_physics_island_agents_1_1(m,island,step,entry-16u,fp,s,o);
    else call(m,s,0x31e301,0x31e190,island,entry-16u,{step},false,o);
    w(m,o,0x27f110,0x5660bc,4,0);
    if(allocated()!=r(m,0x5660b8)) {
        w(m,o,0x27f12d,island+0xae,1,1);w(m,o,0x27f133,0x5660b0,1,1);
    }
}
} // namespace halo2
