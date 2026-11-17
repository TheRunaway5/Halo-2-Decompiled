#include "halo2/native_campaign_ai_target_merge_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_clone_tu15.hpp"
#include "halo2/native_campaign_ai_target_link_tu15.hpp"
#include "halo2/native_tu15_object_body_kernel.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <bit>
#include <vector>

namespace halo2 {
namespace {
using U=std::uint32_t;constexpr U none=0xffffffffu;
std::int32_t signed32(U value){return std::bit_cast<std::int32_t>(value);}
std::int32_t signed16(U value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
struct Merge {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;
    const SourcePhysicsContactRecordObserver &observe;NativeTu15PhysicsFrameFp &fp;
    U r(U at,unsigned width=4)const {
        std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(width));U value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<U>(raw[n])<<(8u*n);return value;
    }
    void w(U pc,U at,U value,unsigned width=4)const {
        const bool observed=native_tu15_observes_retained_store(memory,observe,pc,at,width);
        SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>(width)};
        if(observed){event.before.resize(width);memory.read(at,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));
        memory.write(at,event.written);if(observed)observe(event);
    }
    // Shared BCDF0 salted lookup. The inline destination loop uses its cached
    // object pool and reloads generation after the signed-salt comparison.
    U object(U datum,U pool,bool reload_generation,U &generation)const {
        struct View {
            const Merge &p;U datum,pool;bool reload;U &generation;U selected{};
            U begin()const{return datum;}
            bool present(U value)const{return value!=none;}
            U objects()const{return pool?pool:p.r(0x4ecbd0u);}
            U limit(U owner)const{return p.r(owner+0x38u);}
            bool in_bounds(U value,U limit)const{return signed32(value&0xffffu)<signed32(limit);}
            U record(U owner)const{const auto stride=p.r(owner+0x24u);return p.r(owner+0x44u)+(datum&0xffffu)*stride;}
            U salt(U row)const{return p.r(row,2);}
            bool live(U value)const{return value!=0;}
            bool matches(U value,U salt)const {
                const bool match=signed16(salt)==(signed32(value)>>16);
                if(reload)generation=p.r(0x4eabccu);return match;
            }
            void admit(U row){selected=row;}
            U finish()const {
                if(!selected)return 0;
                const auto mask=1u<<(p.r(selected+3u,1)&31u);
                return mask?p.r(selected+8u):0;
            }
        } view{*this,datum,pool,reload_generation,generation};
        return native_object_body_detail::lookup(view);
    }
    void mark(U object,U &generation,U pc)const {
        const auto cell=0x4eabd0u+(object&0xffffu)*4u;
        if(r(cell)!=generation){w(pc,cell,generation);generation=r(0x4eabccu);}
    }
    bool above(U a,U b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native target merge requires its SIMD trap owner");
        U host{},csr=fp.mxcsr;std::uint8_t result{};
        const auto x=std::bit_cast<float>(a),y=std::bit_cast<float>(b);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[y],%[x]\n\tseta %[result]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
            :[host]"+m"(host),[csr]"+m"(csr),[result]"=&q"(result):[x]"x"(x),[y]"x"(y):"memory","cc");
        fp.mxcsr=csr;return result!=0;
#else
        throw std::runtime_error("Native target merge requires an x86 scalar FP owner");
#endif
    }
    void clone(U frame,U source,U destination) {
        abi.gp[0]=source;abi.gp[3]=destination;abi.gp[4]=frame-4u;
        w(0,abi.gp[4],0x2734ebu);
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        const auto import=[&](){if(abi.floating_point){fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;}fp.status=abi.x87_status;};
        try {native_campaign_ai_perception_group_clone_call_tu15(memory,0x274570u,abi,services,observe);}
        catch(...){import();throw;}
        import();if(abi.gp[4]!=frame)throw std::runtime_error("Native target clone returned another frame");
        if(abi.gp[0]!=none) {
            const auto group=abi.gp[0];w(0,frame-4u,group);w(0,frame-8u,0x2734f8u);
            abi.gp[0]=destination;abi.gp[4]=frame-8u;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
            try{native_campaign_ai_target_link_call_tu15(memory,0x274670u,abi,observe);}catch(...){import();throw;}
            import();if(abi.gp[4]!=frame)throw std::runtime_error("Native target association returned another frame");
        }
    }
    void run(const std::array<U,8> &saved) {
        const auto f=saved[4]-0xe8u,pool=r(0x50eef8u);auto generation=r(0x4eabccu);
        const auto rows=r(pool+0x44u),destination=rows+(saved[0]&0xffffu)*0x50u,offset=(saved[1]&0xffffu)*0x50u;
        w(0,f+0x1cu,rows);++generation;w(0x2732c6u,0x4eabccu,generation);w(0x2732ccu,0x4eabc8u,1,1);
        w(0,f+0x18u,offset);auto group=r(r(pool+0x44u)+(saved[0]&0xffffu)*0x50u+0x14u);
        const auto objects=r(0x4ecbd0u);w(0,f+0x10u,destination);
        while(group!=none) {
            const auto tracked=r(r(0x50eef4u)+0x44u)+(group&0xffffu)*0xc4u;
            group=r(tracked+0x14u);const auto datum=r(tracked+8u),body=object(datum,objects,true,generation);
            if(!body)continue;
            if(r(body+0xaau,1)==12u && r(body+0x134u)==0) {
                const auto attachment=body+U(signed16(r(body+0x13au,2)));
                if(attachment) {
                    const auto member=r(attachment+8u);
                    if(member!=none) {
                        auto unit=r(r(r(0x510fa8u)+0x44u)+(member&0xffffu)*0x34u+0x18u);
                        while(unit!=none) {
                            const auto unit_body=r(r(objects+0x44u)+(unit&0xffffu)*12u+8u);U next=none;
                            if(r(unit_body+0x134u)==0) {
                                const auto extension=unit_body+U(signed16(r(unit_body+0x13au,2)));
                                if(extension)next=r(extension+0xcu);
                            }
                            mark(unit,generation,0x2733f8u);unit=next;
                        }
                        continue;
                    }
                }
            }
            mark(datum,generation,0x273419u);
        }
        group=r(r(r(0x50eef8u)+0x44u)+r(f+0x18u)+0x14u);U count{};
        // Capture the original chain before allocating/reassociating its clones.
        while(group!=none) {
            const auto row=r(r(0x50eef4u)+0x44u)+(group&0xffffu)*0xc4u,value=group;
            group=r(row+0x14u);w(0x273465u,f+0x20u+U(signed16(count))*4u,value);++count;
        }
        if(signed16(count)>0) {
            U cursor=f+0x20u;w(0,f+0x14u,count&0xffffu);
            do {
                const auto datum=r(cursor);
                if(datum!=none) {
                    const auto groups=r(0x50eef4u),limit=r(groups+0x38u),index=datum&0xffffu;
                    if(signed32(index)<signed32(limit)) {
                        const auto stride=r(groups+0x24u),row=r(groups+0x44u)+index*stride,salt=r(row,2);
                        if(salt && signed16(salt)==(signed32(datum)>>16)) {
                            const auto target=r(row+8u);U unused=generation;
                            if(object(target,0,false,unused)) {
                                const auto cell=0x4eabd0u+(target&0xffffu)*4u;
                                if(r(cell)!=generation) {
                                    w(0x2734d9u,cell,generation);clone(f,row,r(f+0x10u));generation=r(0x4eabccu);
                                }
                            }
                        }
                    }
                }
                const auto remaining=r(f+0x14u)-1u;cursor+=4u;w(0,f+0x14u,remaining);
            }while(r(f+0x14u));
        }
        const auto source=r(f+0x18u)+r(f+0x1cu),out=r(f+0x10u);
        w(0x27351cu,0x4eabc8u,0,1);abi.xmm[0]={r(source+0x2cu),0,0,0};
        if(above(abi.xmm[0][0],r(out+0x2cu)))w(0x273533u,out+0x2cu,r(source+0x2cu));
        const auto flags=r(source+0x30u,1),old_flags=r(out+0x30u,1),old_priority=r(out+0x34u);
        w(0x273542u,out+0x30u,flags|old_flags,1);const auto priority=r(source+0x34u);
        if(signed32(priority)>signed32(old_priority))w(0x27354du,out+0x34u,priority);
        const auto rank=r(source+0x26u,2);
        if(signed16(rank)>signed16(r(out+0x26u,2))) {
            w(0x27355bu,out+0x26u,rank,2);w(0x273563u,out+0x28u,r(source+0x28u));
        }
    }
};
}
void native_campaign_ai_target_merge_call_tu15(SourcePhysicsContactMemory &memory,U entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if(entry!=0x273290u)throw std::invalid_argument("Unknown native target merge entry");
    if(!abi.floating_point || abi.active_mmx)
        throw std::invalid_argument("Native target merge requires retained x87 FP");
    const auto saved=abi.gp;const auto controls=*abi.floating_point;
    NativeTu15PhysicsFrameFp fp{controls.x87_control_word,abi.x87_status,controls.mxcsr};Merge merge{memory,abi,services,observe,fp};
    const auto publish=[&](){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
    try{merge.run(saved);}catch(...){publish();throw;}
    publish();for(auto n:{3,5,6,7})abi.gp[n]=saved[n];abi.gp[4]=saved[4]+4u;
}
}
