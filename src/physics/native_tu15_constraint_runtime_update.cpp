#include "halo2/native_tu15_constraint_runtime_update.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){_mm_setcsr(fp.mxcsr);}
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(8u*n));SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(event);
}
std::int32_t signed_byte(std::uint32_t value){return std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value)));}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(__m128 value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 convert(std::int32_t value){__m128 result=_mm_setzero_ps();asm volatile("cvtsi2ssl %1,%0":"+x"(result):"r"(value):"memory");return result;}
bool greater(__m128 a,__m128 b){unsigned char value;asm volatile("comiss %2,%1; seta %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value;}
bool state_kind(std::int32_t value){return value>=2&&value<=3;}
}
void native_tu15_update_constraint_runtime(SourcePhysicsContactMemory&m,std::uint32_t instance,
    std::uint32_t datum,NativeTu15ConstraintBodySlots slots,NativeTu15ConstraintRuntimeUpdateScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConstraintRuntimeUpdateServices&services,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintRuntimeUpdateStats*stats){
    if(stats)++stats->calls;
    if(depth>3u||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
       (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
        if(stats)++stats->unsupported_profiles;throw std::runtime_error("Native TU1.5 constraint runtime update requires its unsupported numerical-fault owner");}
    const auto data=scratch.storage;
    if(std::uint64_t(data)+224u>0x100000000ull)throw std::runtime_error("Native TU1.5 constraint runtime DATA overflows its owner");
    const auto private_store=[&](std::uint32_t offset,std::uint32_t value,unsigned width=4){store(m,{},0,data+offset,value,width);};
    const auto private_read=[&](std::uint32_t offset){return read(m,data+offset);};
    const auto first=read(m,instance+0x24u),manager=read(m,0x52be5cu),components=read(m,manager+0x44u),object_pool=read(m,0x4ecbd0u);
    const auto object_datum=read(m,components+(first&0xffffu)*160u+8u),objects=read(m,object_pool+0x44u);
    const auto half_bits=read(m,0x46a0ccu),object=read(m,objects+(object_datum&0xffffu)*12u+8u);
    for(unsigned n=0;n<3;++n)private_store(0xbcu+n*4u,read(m,instance+0x68u+n*4u));
    const auto normal_x=read(m,instance+0x5cu);private_store(0xc8u,normal_x);const auto normal_y=read(m,instance+0x60u),normal_z=read(m,instance+0x64u);
    private_store(0xccu,normal_y);const auto transient_flag=read(m,instance+0x11u,1);private_store(0xb0u,0xffffffffu);private_store(0x9cu,0xffffffffu);
    private_store(0xd4u,0x0600022bu);private_store(0xd0u,normal_z);
    auto scale_bits=std::uint32_t{};
    const auto clamp=[&](std::uint32_t address){Environment environment(fp);const auto zero=_mm_setzero_ps();
        if(greater(zero,scalar(read(m,address))))return std::uint32_t(0);
        const auto value=scalar(read(m,address)),one=scalar(read(m,0x46a0d0u));return bits(greater(value,one)?one:value);};
    const auto select=[&](std::uint16_t first_key,std::uint16_t second_key,std::int32_t mode){
        if(!services.select_effects){if(stats)++stats->unbound_effect_selections;
            throw std::runtime_error("Native TU1.5 constraint runtime update requires actual22EEC0 effect selection");}
        if(stats)++stats->effect_selections;
        services.select_effects({instance,instance+0x68u,first_key,second_key,mode,
            {data+0xb8u,data+0x1cu,data+0xb4u,data+0x18u,data+0x10u,data+0x14u}},fp);
    };
    const auto create=[&](std::uint32_t effect,std::uint32_t scale,bool primary){
        // Exact ordinary96-byte request written by the source caller. The
        // alternate request preserves its different post-push field stores.
        for(unsigned n=0;n<24;++n)private_store(0x30u+n*4u,0u);
        private_store(0x84u,0xff808080u);private_store(0x88u,0xff808080u);
        private_store(0x34u,effect);private_store(0x48u,0xffffu,2);private_store(0x38u,0xffffffffu);
        private_store(0x40u,0xffffffffu);private_store(0x3cu,0xffffffffu);private_store(0x44u,0xffffu,2);
        private_store(0x50u,1u);private_store(0x4cu,data+0xbcu);private_store(0x70u,scale);private_store(0x74u,scale);
        if(!primary){private_store(0x88u,0xff808080u);}
        if(!services.create_handle){if(stats)++stats->unbound_handle_creations;
            throw std::runtime_error("Native TU1.5 constraint runtime update requires actual178E30 handle creation");}
        if(stats)++stats->handle_creations;return services.create_handle(data+0x30u,fp);
    };
    bool transient=false;
    if(transient_flag&&read(m,0x48b892u,1)&&signed_byte(read(m,instance+0x1cu,1))>2){
        const auto state=signed_byte(read(m,instance+0x1du,1));bool trigger=!state_kind(state);
        if(!trigger){const auto value=read(m,instance+(state==3?0x48u:0x4cu));
            if(state==2){Environment environment(fp);trigger=greater(scalar(half_bits),scalar(value));
                if(!trigger)trigger=greater(scalar(read(m,instance+0x18u)),scalar(read(m,0x46a130u)));}}
        if(trigger){
            transient=true;if(stats)++stats->transient_updates;scale_bits=clamp(instance+0x18u);private_store(8u,scale_bits);
            for(unsigned n=0;n<3;++n)private_store(0x30u+n*4u,read(m,instance+0x68u+n*4u));
            for(const auto offset:{0xb8u,0x1cu,0xb4u,0x18u,0x10u,0x14u})private_store(offset,0xffffffffu);
            private_store(0x3cu,native_tu15_pack_constraint_direction(m,instance+0x50u,fp,depth,stats?&stats->math:nullptr));
            const auto zero=read(m,0x474d04u),zero_x=read(m,zero),zero_y=read(m,zero+4u),zero_z=read(m,zero+8u);
            private_store(0x40u,zero_x);private_store(0x48u,zero_z);const auto object_second=read(m,object+0x2cu);
            private_store(0x44u,zero_y);const auto object_first=read(m,object+0x28u);
            const auto second_key=read(m,instance+0x2eu,2),first_key=read(m,instance+0x2cu,2);
            private_store(0x50u,object_second);private_store(0x4cu,object_first);private_store(0x94u,second_key,2);private_store(0x20u,first_key,2);
            select(std::uint16_t(first_key),std::uint16_t(second_key),0);
            for(const auto offset:{0xb8u,0xb4u,0x10u}){const auto effect=private_read(offset);if(effect==0xffffffffu)continue;
                private_store(0xa4u,effect);private_store(0xa0u,0xffffffffu);private_store(0xa8u,scale_bits);private_store(0xacu,0u);
                if(!services.publish_effect){if(stats)++stats->unbound_effect_publications;
                    throw std::runtime_error("Native TU1.5 constraint runtime update requires actual18C440 effect publication");}
                if(stats)++stats->effect_publications;services.publish_effect(data+0x30u,data+0xa0u,fp);}
            if(read(m,instance+0x9cu,2)==0xffffu){
                auto effect=private_read(0x1cu);if(effect!=0xffffffffu)private_store(0xb0u,create(effect,scale_bits,true));
                effect=private_read(0x18u);if(effect!=0xffffffffu)private_store(0x9cu,create(effect,scale_bits,false));
                effect=private_read(0x14u);if(effect!=0xffffffffu)private_store(0x9cu,create(effect,scale_bits,false));
            }
            const auto clock=read(m,0x51df54u),tick=read(m,clock+8u);store(m,observer,0x23033au,instance+0x94u,tick);
            store(m,observer,0x230340u,instance+0x98u,scale_bits);store(m,observer,0x230348u,instance+0x1cu,0u,1);
            store(m,observer,0x23034cu,instance+0x10u,0u,1);
        }
    }
    if(!transient){
        auto kind=read(m,instance+0xcu,1);const auto clock=read(m,0x51df54u);bool cleared=false;
        if(kind==1u){
            const auto ticks=signed_byte(read(m,instance+0x1eu,1));bool expired{};
            {Environment environment(fp);auto time=convert(ticks);time=mul(time,scalar(read(m,clock+4u)));expired=greater(time,scalar(half_bits));}
            if(expired){bool any=false;for(unsigned n=0;n<4;++n)if(read(m,instance+0x30u+n*4u)!=0xffffffffu){any=true;break;}
                if(any){native_tu15_clear_constraint_runtime_handles(m,instance,true,true,fp,services.handles,observer,stats?&stats->handles:nullptr);
                    store(m,observer,0x230393u,instance+0x1du,0xffu,1);cleared=true;}}
        }
        if(!cleared&&state_kind(signed_byte(kind))&&signed_byte(read(m,instance+0x91u,1))>2){
            const auto old_state=read(m,instance+0x1du,1);bool change=kind!=old_state;
            if(change){const auto ticks=signed_byte(read(m,instance+0x1eu,1));
                Environment environment(fp);auto time=convert(ticks);time=mul(time,scalar(read(m,clock+4u)));change=greater(time,scalar(half_bits));}
            if(change){
                if(stats)++stats->state_updates;bool retain=false;
                if(read(m,instance+0x1fu,1)!=kind){store(m,observer,0x230401u,instance+0x20u,0u,1);store(m,observer,0x230405u,instance+0x1fu,kind,1);}
                bool delay{};{Environment environment(fp);auto time=convert(signed_byte(read(m,instance+0x20u,1)));
                    time=mul(time,scalar(read(m,clock+4u)));delay=!greater(time,scalar(read(m,0x456918u)));}
                bool proceed=!delay||!native_tu15_constraint_has_runtime_handles(m,instance,stats?&stats->handles:nullptr);
                if(proceed){
                    if(native_tu15_constraint_has_runtime_handles(m,instance,stats?&stats->handles:nullptr)){
                        retain=state_kind(signed_byte(old_state));if(retain)retain=state_kind(signed_byte(read(m,instance+0x1fu,1)));
                        native_tu15_clear_constraint_runtime_handles(m,instance,!retain,true,fp,services.handles,observer,stats?&stats->handles:nullptr);
                    }
                    store(m,observer,0x2304a0u,instance+0x1du,0xffu,1);
                }
                if(read(m,0x48b892u,1)&&(!native_tu15_constraint_has_runtime_handles(m,instance,stats?&stats->handles:nullptr)||retain)){
                    const auto marker=read(m,instance+0xdu,1);bool links=marker==0xffu;if(!links)links=read(m,instance+0xeu,1)!=0;
                    bool handles=false;if(marker==0xffu||read(m,instance+0xeu,1)==0)handles=read(m,instance+0x9cu,2)==0xffffu;
                    const auto state=read(m,instance+0xcu,1);scale_bits=clamp(instance+(state==3u?0x48u:0x4cu));private_store(8u,scale_bits);
                    const auto second_key=read(m,instance+0x2eu,2),first_key=read(m,instance+0x2cu,2);
                    private_store(0x20u,second_key,2);private_store(0x94u,first_key,2);
                    for(const auto offset:{0x14u,0x18u,0x1cu,0x10u})private_store(offset,0xffffffffu);
                    // The selector's first request switches the key order and
                    // uses four outputs; its final two outputs are null.
                    if(!services.select_effects){if(stats)++stats->unbound_effect_selections;
                        throw std::runtime_error("Native TU1.5 constraint runtime update requires actual22EEC0 effect selection");}
                    if(stats)++stats->effect_selections;
                    services.select_effects({instance,instance+0x68u,std::uint16_t(first_key),std::uint16_t(second_key),signed_byte(state),
                        {data+0x14u,data+0x1cu,data+0x18u,data+0x10u,0u,0u}},fp);
                    if(links){
                        if(retain){for(unsigned n=0;n<2;++n){const auto link=read(m,instance+0x30u+n*4u);if(link==0xffffffffu)continue;
                            native_tu15_set_constraint_runtime_link_flag(m,link,read(m,instance+0xcu,1)!=2u,observer,stats?&stats->parameters:nullptr);}}
                        else for(unsigned n=0;n<2;++n){const auto effect=private_read(0x14u+n*4u);if(effect==0xffffffffu)continue;
                            const auto link=native_tu15_create_constraint_runtime_link(m,effect,datum,observer,stats?&stats->parameters:nullptr);
                            const bool enabled=read(m,instance+0xcu,1)!=2u;store(m,observer,n?0x230619u:0x2305f3u,instance+0x30u+n*4u,link);
                            native_tu15_set_constraint_runtime_link_flag(m,link,enabled,observer,stats?&stats->parameters:nullptr);}
                    }
                    if(handles){auto effect=private_read(0x1cu);if(effect!=0xffffffffu)store(m,observer,0x2306c2u,instance+0x38u,create(effect,scale_bits,true));
                        effect=private_read(0x10u);if(effect!=0xffffffffu)store(m,observer,0x230756u,instance+0x3cu,create(effect,scale_bits,false));}
                    if(native_tu15_constraint_has_runtime_handles(m,instance,stats?&stats->handles:nullptr)){
                        const auto next=read(m,instance+0xcu,1);store(m,observer,0x230767u,instance+0x1eu,0u,1);store(m,observer,0x23076bu,instance+0x1du,next,1);}
                }
            }
        }
    }
    for(const auto offset:{0x20u,0x1cu,0x1eu}){const auto value=signed_byte(read(m,instance+offset,1))+1;
        store(m,observer,offset==0x20u?0x23077du:offset==0x1cu?0x23078fu:0x2307a1u,instance+offset,std::uint32_t(value>126?126:value),1);
        if(stats)++stats->counter_updates;}
    bool frame_needed=read(m,instance+0x38u)!=0xffffffffu;
    if(!frame_needed)frame_needed=read(m,instance+0x3cu)!=0xffffffffu;
    if(!frame_needed)frame_needed=private_read(0xb0u)!=0xffffffffu;
    if(!frame_needed)frame_needed=private_read(0x9cu)!=0xffffffffu;
    if(frame_needed){
        scale_bits=clamp(instance+(read(m,instance+0x1du,1)==3u?0x48u:0x4cu));private_store(8u,scale_bits);
        const auto component_datum=read(m,instance+0x24u),pool=read(m,0x52be5cu),rows=read(m,pool+0x44u),component=rows+(component_datum&0xffffu)*160u;
        if(stats)++stats->geometry_updates;
        native_tu15_constraint_geometry_frame(m,instance,datum,slots.first,
            {data+0x34u,data+0x58u,data+0x4cu,data+0x40u,data+0x94u,data+0x98u},false,scratch.geometry,fp,depth,observer,stats?&stats->geometry:nullptr);
        std::array<std::uint32_t,3>velocity{};
        {Environment environment(fp);const auto negate=scalar(read(m,0x46a0dcu));auto value=scalar(private_read(0x40u));const auto bodies=read(m,component+0x70u);
            value=mul(value,negate);private_store(0x40u,bits(value));value=scalar(private_read(0x44u));const auto body=read(m,bodies+slots.first*0x60u+0x40u),inactive=read(m,body+0x40u,1);
            value=mul(value,negate);private_store(0x44u,bits(value));value=mul(scalar(private_read(0x48u)),negate);private_store(0x48u,bits(value));
            if(!inactive){const auto motion=read(m,body+0x3cu);for(unsigned n=0;n<3;++n)velocity[n]=read(m,motion+0x40u+n*4u);}
            else{const auto zero=read(m,0x474d04u);for(unsigned n=0;n<3;++n)velocity[n]=read(m,zero+n*4u);}
            for(unsigned n=0;n<3;++n)private_store(0x24u+n*4u,velocity[n]);}
        if(read(m,instance+8u,2)!=0){const auto second=read(m,instance+0x28u);if(second!=0xffffffffu){
            const auto next=read(m,0x52be5cu),next_rows=read(m,next+0x44u);
            const auto other=native_tu15_constraint_body_linear_velocity(m,{next_rows+(second&0xffffu)*160u},slots.second,stats?&stats->math:nullptr);
            Environment environment(fp);for(unsigned n=0;n<3;++n){velocity[n]=bits(sub(scalar(velocity[n]),scalar(other[n])));private_store(0x24u+n*4u,velocity[n]);}}}
        for(unsigned n=0;n<2;++n){const auto p=instance+0x38u+n*4u,handle=read(m,p);if(handle==0xffffffffu)continue;
            const bool retained=native_tu15_update_constraint_runtime_parameters(m,handle,{data+0x24u,data+0x94u,data+0x30u,scale_bits,0u},observer,stats?&stats->parameters:nullptr);
            if(!retained)store(m,observer,n?0x2309b6u:0x23098au,p,0xffffffffu);}
        const auto first_handle=private_read(0xb0u),saved_scale=private_read(8u);
        const auto one_bits=read(m,0x46a0d0u);
        const auto publish_transient=[&](std::uint32_t handle){
            std::uint32_t primary{};{Environment environment(fp);const auto zero=_mm_setzero_ps();
                if(greater(zero,scalar(read(m,instance+0x18u))))primary=0;
                else{const auto value=scalar(read(m,instance+0x18u));if(greater(value,scalar(one_bits)))primary=one_bits;
                    else primary=read(m,instance+0x18u);}}
            native_tu15_update_constraint_runtime_parameters(m,handle,{data+0x24u,data+0x94u,data+0x30u,primary,saved_scale},observer,stats?&stats->parameters:nullptr);
        };
        if(first_handle!=0xffffffffu)publish_transient(first_handle);const auto second_handle=private_read(0x9cu);if(second_handle!=0xffffffffu)publish_transient(second_handle);
    }
    store(m,observer,0x230a47u,instance+0x11u,0u,1);
}
} // namespace halo2
