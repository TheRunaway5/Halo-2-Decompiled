#include "halo2/native_tu15_camera_position.hpp"
#include "halo2/native_tu15_camera_basis.hpp"
#include "halo2/native_tu15_look_context.hpp"
#include "halo2/native_tu15_constraint_auxiliary_math.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include "halo2/native_campaign_object_marker_request_tu15.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned width=4){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(width);m_.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}m_.write(p,std::span(b).first(width));if(o_)o_(e);}

};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;bool active_{};
    void validate(){if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native camera adjustment requires its numerical-fault owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){
        if(depth>4)throw std::runtime_error("Native camera adjustment requires its x87 capacity owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto value=f();resume();return value;}}catch(...){resume();throw;}}
};
using F=__m128;
F scalar(std::uint32_t n){F out{};std::memcpy(&out,&n,4);return out;}
std::uint32_t bits(F n){std::uint32_t out;std::memcpy(&out,&n,4);return out;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F divide(F a,F b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Comparison {bool below,equal;bool above()const{return!below&&!equal;}};
Comparison compare(F a,F b){unsigned char below,equal;asm volatile("comiss %3,%2;setb %0;sete %1":"=qm"(below),"=qm"(equal):"x"(a),"x"(b):"cc","memory");return{below!=0,equal!=0};}
#endif
}
void native_tu15_compute_unit_camera_position(SourcePhysicsContactMemory&memory,
    std::uint32_t object,std::uint32_t destination,NativeTu15CameraPositionScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15CameraPositionServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15CameraPositionStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        Memory m(memory,observe);const auto data=scratch.parent_data;
        const auto child=[&](std::uint32_t owner,auto&&body)->decltype(auto){
            if(services.native_child)services.native_child(owner,true);
            bool exiting{};
            try{
                if constexpr(std::is_void_v<std::invoke_result_t<decltype(body)>>){body();exiting=true;if(services.native_child)services.native_child(owner,false);return;}
                else{auto value=body();exiting=true;if(services.native_child)services.native_child(owner,false);return value;}
            }catch(...){if(!exiting&&services.native_child)services.native_child(owner,false);throw;}
        };
        const auto objects=m.word(0x4ecbd0u),rows=m.word(objects+0x44u),unit=m.word(rows+(object&0xffffu)*12u+8u),definition_datum=m.word(unit),tags=m.word(0x4f0414u),definition=m.word(tags+(definition_datum&0xffffu)*16u+8u);
        m.word(0xcd4d9u,data+48u,definition);const auto parent=m.word(unit+0x14u);
        const auto marker=[&](std::uint32_t receiver,std::uint32_t name,std::uint32_t first_pc,std::uint32_t second_pc,std::uint32_t third_pc){
            if(stats)++stats->markers;
            child(0xbad60u,[&]{native_campaign_object_marker_request_tu15(memory,receiver,false,name,1,data+52u,fp);});
            const auto x=m.word(data+148u),y=m.word(data+152u),z=m.word(data+156u);
            m.word(first_pc,destination,x);m.word(second_pc,destination+4u,y);m.word(third_pc,destination+8u,z);
        };
        if(parent!=0xffffffffu){
            if(stats)++stats->origins;child(0xbbf20u,[&]{native_tu15_object_camera_origin(memory,parent,destination,fp,observe);});
            const auto seat=std::uint16_t(m.word(unit+0x1fcu,2));
            if(seat!=0xffffu){
                const auto current_parent=m.word(unit+0x14u),current_objects=m.word(0x4ecbd0u),current_rows=m.word(current_objects+0x44u),current_tags=m.word(0x4f0414u),parent_unit=m.word(current_rows+(current_parent&0xffffu)*12u+8u),parent_definition=m.word(parent_unit),parent_tag=m.word(current_tags+(parent_definition&0xffffu)*16u+8u),seats=m.word(parent_tag+0x1ccu);
                const auto kind=m.word(parent_unit+0xaau,1),selected=seats+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(seat)))*0xb0u;
                if(kind!=1u||m.word(selected+0x60u)!=0u)marker(current_parent,m.word(selected+0x60u),0xcd8f1u,0xcd8f3u,0xcd8f6u);
            }
        }else{
            const bool special=(m.word(unit+0x10au,1)&4u)!=0u;
            const bool ordinary=!special&&m.word(unit+0xaau,1)==0u;
            if(!ordinary){
                const auto related=m.word(unit+0x24cu);
                std::uint32_t name=0x4000095u;
                if(related!=0xffffffffu){
                    const auto related_unit=m.word(rows+(related&0xffffu)*12u+8u);const auto seat=std::uint16_t(m.word(related_unit+0x1fcu,2));
                    if(seat!=0xffffu){
                        const auto cached_tag=m.word(data+48u),seats=m.word(cached_tag+0x1ccu);
                        name=m.word(seats+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(seat)))*0xb0u+8u);
                    }
                }
                marker(object,name,0xcd845u,0xcd848u,0xcd84bu);
            }else{
                if(stats)++stats->on_foot;
                const auto current_definition=m.word(unit),on_foot_tag=m.word(tags+(current_definition&0xffffu)*16u+8u);
                if(stats)++stats->world_points;child(0xbbe00u,[&]{native_campaign_object_world_point_tu15(memory,object,destination,fp,observe);});
                const auto transition=child(0xe3310u,[&]{return native_tu15_unit_camera_transition(memory,object);});
                Environment environment(fp,depth);const auto read=[&](std::uint32_t at){return scalar(m.word(at));};
                auto blend=F{};if(!(transition&0xffu))blend=read(unit+0x2c4u);
                auto inverse=read(0x46a0d0u);auto height=read(on_foot_tag+0x21cu);inverse=sub(inverse,blend);inverse=mul(inverse,read(on_foot_tag+0x218u));height=mul(height,blend);inverse=add(inverse,height);inverse=mul(inverse,read(unit+0xa0u));inverse=add(inverse,read(destination+8u));m.word(0xcd565u,destination+8u,bits(inverse));
                const auto x=m.word(destination),y=m.word(destination+4u),z=m.word(destination+8u);
                m.word(0xcd57au,data,x);const auto dx=m.word(unit+0x168u);m.word(0xcd580u,data+4u,y);const auto dy=m.word(unit+0x16cu);m.word(0xcd587u,data+8u,z);const auto dz=m.word(unit+0x170u);m.word(0xcd58eu,data+12u,dx);m.word(0xcd592u,data+20u,dz);m.word(0xcd59eu,data+88u,0xffffu,2);m.word(0xcd5a5u,data+16u,dy);
                environment.dependency([&]{child(0x11fa50u,[&]{native_tu15_camera_perpendicular(memory,data+12u,data+24u,fp,depth,observe);});});
                environment.dependency([&]{child(0xe18d0u,[&]{native_tu15_adjust_unit_camera_position(memory,object,data,data+12u,data+24u,scratch.adjustment,fp,depth,observe);});});if(stats)++stats->adjustments;
                auto proposed=read(data),current=read(destination);auto delta=sub(proposed,current);m.word(0xcd5c8u,data+36u,bits(delta));proposed=read(data+4u);current=read(destination+4u);delta=sub(proposed,current);m.word(0xcd5d9u,data+40u,bits(delta));proposed=read(data+8u);current=read(destination+8u);delta=sub(proposed,current);m.word(0xcd5eeu,data+44u,bits(delta));
                const auto length=environment.dependency([&]{return child(0x30bd0u,[&]{return native_tu15_normalize_constraint_axis(memory,data+36u,fp,depth,observe);});});
                const auto relation=compare(scalar(length),read(0x46a0e8u));
                if(relation.above()){
                    const auto scale=read(0x4502c8u);auto component=mul(read(data+36u),scale);m.word(0xcd61fu,data+12u,bits(component));component=mul(read(data+40u),scale);m.word(0xcd636u,data+16u,bits(component));component=mul(read(data+44u),scale);m.word(0xcd64cu,data+20u,bits(component));
                    const auto query=[&](std::uint32_t origin,std::uint32_t displacement){
                        if(!services.segment){if(stats)++stats->unbound_segments;throw std::runtime_error("Native unit camera requires16C620 geometric query");}
                        if(stats)++stats->segment_calls;return environment.dependency([&]{return services.segment({0x4808c2du,origin,displacement,0xffffffffu,0xffffffffu,data+52u},fp);});
                    };
                    if(query(destination,data+12u)){
                        auto fraction=read(data+56u);fraction=mul(fraction,read(0x46a194u));component=mul(read(data+12u),fraction);component=add(component,read(destination));m.word(0xcd677u,data+24u,bits(component));component=mul(read(data+16u),fraction);component=add(component,read(destination+4u));m.word(0xcd68cu,data+28u,bits(component));component=mul(read(data+20u),fraction);component=add(component,read(destination+8u));m.word(0xcd6a1u,data+32u,bits(component));
                    }else{
                        const auto qx=m.word(data+60u),qy=m.word(data+64u),qz=m.word(data+68u);m.word(0xcd6b5u,data+24u,qx);m.word(0xcd6b9u,data+28u,qy);m.word(0xcd6bdu,data+32u,qz);
                    }
                    bool adjusted=false;
                    if(query(data+24u,data+36u)){
                        const auto ox=read(data+24u);auto qx=read(data+60u);const auto oy=read(data+28u),oz=read(data+32u);auto px=read(data);auto qy=read(data+64u),qz=read(data+68u),py=read(data+4u);
                        qx=sub(qx,ox);px=sub(px,ox);auto pz=read(data+8u);pz=sub(pz,oz);qz=sub(qz,oz);const auto ny=read(data+40u);qy=sub(qy,oy);py=sub(py,oy);const auto nz=read(data+44u);
                        qz=mul(qz,nz);qy=mul(qy,ny);qz=add(qz,qy);const auto nx=read(data+36u);pz=mul(pz,nz);qx=mul(qx,nx);qz=add(qz,qx);const auto threshold=read(0x46a12cu);py=mul(py,ny);pz=add(pz,py);px=mul(px,nx);pz=add(pz,px);pz=add(pz,threshold);
                        if(compare(pz,qz).above()){
                            auto value=read(data+60u);const auto bias_x=mul(nx,threshold);value=sub(value,bias_x);m.word(0xcd78du,destination,bits(value));value=read(data+64u);const auto bias_y=mul(ny,threshold);value=sub(value,bias_y);m.word(0xcd79fu,destination+4u,bits(value));value=read(data+68u);const auto bias_z=mul(nz,threshold);value=sub(value,bias_z);m.word(0xcd7b3u,destination+8u,bits(value));adjusted=true;
                        }
                    }
                    if(!adjusted){const auto px=m.word(data),py=m.word(data+4u),pz=m.word(data+8u);m.word(0xcd7ceu,destination,px);m.word(0xcd7d1u,destination+4u,py);m.word(0xcd7d4u,destination+8u,pz);}
                }
            }
        }
        if(stats)++stats->completed;
#else
        (void)memory;(void)object;(void)destination;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;throw std::runtime_error("Exact native unit camera backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
}
