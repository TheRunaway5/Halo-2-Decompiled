#include "halo2/native_tu15_contact_features.hpp"
#include "halo2/native_tu15_polygon_features_value.hpp"
#include <bit>
#include <cstring>
#include <sstream>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observe_)observe_(event);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));bytes(pc,p,b);}
    NativeTu15ContactVirtualOwner owner(std::uint32_t object,unsigned slot)const{return{word(word(object)+slot)};}
    std::array<std::byte,16>vector_bytes(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 feature retrieval requires aligned vector DATA");std::array<std::byte,16>b{};memory_.read_vector(p,b);return b;}
    void vector_bytes(std::uint32_t pc,std::uint32_t p,const std::array<std::byte,16>&b){if(p&15u)throw std::runtime_error("Native TU1.5 feature retrieval requires aligned vector output");bytes(pc,p,b);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{const auto b=vector_bytes(p);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);vector_bytes(pc,p,b);}
#endif
};
bool nonnegative(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>=0;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_{};
    void validate()const{if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 features require their unsupported FP exception owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){if(depth>7)throw std::runtime_error("Native TU1.5 features require their overfull x87 owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();}
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
};
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
#endif
}
void native_tu15_contact_shape_features(SourcePhysicsContactMemory&memory,
    NativeTu15ContactComponent shape,NativeTu15ContactVirtualOwner owner,
    std::uint32_t ids,std::uint32_t count,std::uint32_t output,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);
    if(owner.identity==0x2e8d40u){
        const auto last=count-1u;if(!nonnegative(last))return;
        m.vector_bytes(0x2e8d63u,output,{});
        const auto words=(last&0x0fffffffu)*4u;
        for(std::uint32_t n=0;n<words;++n)m.word(0x2e8d66u,output+16u+n*4u,m.word(output+n*4u));
        return;
    }
    if(owner.identity==0x1f6690u){
        native_polygon_features_detail::ValueAccess access{m,shape.identity,ids,count,output};
        NativeTu15PolygonFeatures data;native_tu15_expand_polygon_features(access,data);return;
    }
    if(owner.identity==0x3070f0u){
        auto last=count-1u;if(!nonnegative(last))return;for(std::uint32_t n=0;n<count;++n){const auto id=m.word(ids+n*2u,2);m.vector_bytes(0x30710cu,output+n*16u,m.vector_bytes(shape.identity+id+0x10u));const auto tag=m.word(ids+n*2u,2);m.word(0x307118u,output+n*16u+0xcu,tag|0x3f000000u);}return;
    }
    if(owner.identity==0x3079c0u){
        auto last=count-1u;std::uint32_t position{},feature=ids;
        const auto point=[&](unsigned lane,bool first_group_point){std::uint32_t vertices,id;if(first_group_point){vertices=m.word(shape.identity+0x30u);id=m.word(feature,2);}else{id=m.word(feature,2);vertices=m.word(shape.identity+0x30u);}const auto source=vertices+(id>>2u)*0x30u+(id&3u)*4u;
            constexpr std::uint32_t sites[5][4]={{0x307a10u,0x307a17u,0x307a1eu,0x307a2bu},{0x307a4au,0x307a51u,0x307a58u,0x307a64u},{0x307a83u,0x307a8au,0x307a95u,0x307a9eu},{0x307abdu,0x307ac4u,0x307ad5u,0x307ad8u},{0x307b1bu,0x307b22u,0x307b29u,0x307b36u}};
            m.word(sites[lane][0],output+position,m.word(source));m.word(sites[lane][1],output+position+4u,m.word(source+0x10u));m.word(sites[lane][2],output+position+8u,m.word(source+0x20u));m.word(sites[lane][3],output+position+0xcu,id|0x3f000000u);position+=16u;feature+=2u;
        };
        if(std::bit_cast<std::int32_t>(count)>=4){const auto groups=count>>2u;for(std::uint32_t n=0;n<groups;++n)for(unsigned k=0;k<4;++k)point(k,k==0);last-=groups*4u;}
        if(nonnegative(last))for(std::uint32_t n=0;n<=last;++n)point(4,false);return;
    }
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);
    if(owner.identity==0x3067d0u){
        const auto last=count-1u;if(!nonnegative(last))return;for(std::uint32_t n=0;n<count;++n){const auto id=m.word(ids+n*2u,2);const auto extent=m.vector(shape.identity+0x10u),sign=m.vector(0x41d2f0u+id*16u);m.vector(0x306806u,output+n*16u,multiply(extent,sign));const auto tag=m.word(ids+n*2u,2);m.word(0x306812u,output+n*16u+0xcu,tag|0x3f000000u);}return;
    }
    if(owner.identity==0x2e5e20u){
        const NativeTu15ContactComponent child{m.word(shape.identity+0x30u)};const auto method=m.owner(child.identity,0x34u);environment.publish();try{native_tu15_contact_shape_features(memory,child,method,ids,count,output,fp,depth,observe);environment.resume();}catch(...){environment.resume();throw;}
        const auto last=count-1u;if(!nonnegative(last))return;for(std::uint32_t n=0;n<count;++n){const auto p=output+n*16u;const auto vertex=m.vector(p);const auto tag=m.word(p+0xcu);asm volatile("flds %0"::"m"(tag):"memory","st");const auto sweep=m.vector(shape.identity+0x10u);m.vector(0x2e5e5du,p,add(vertex,sweep));std::uint32_t restored;asm volatile("fstps %0":"=m"(restored)::"memory","st");m.word(0x2e5e60u,p+0xcu,restored);}return;
    }
#endif
    std::ostringstream message;message<<"Native TU1.5 contact feature kernel requires authored owner 0x"<<std::hex<<owner.identity
        <<" for shape 0x"<<shape.identity<<" ids 0x"<<ids<<" count 0x"<<count<<" output 0x"<<output;
    throw std::runtime_error(message.str());
}
} // namespace halo2
