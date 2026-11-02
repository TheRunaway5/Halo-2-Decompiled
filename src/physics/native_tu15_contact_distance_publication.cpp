#include "halo2/native_tu15_contact_distance_publication.hpp"
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    NativeTu15PhysicsFrameFp& fp_;
    unsigned host_mxcsr_;
    bool active_{};
    void validate() const {
        if((fp_.control_word&0x3fu)!=0x3fu || (fp_.status&0x8080u) ||
           (fp_.mxcsr&0x1f80u)!=0x1f80u || (fp_.mxcsr&0xffff0000u))
            throw std::runtime_error("Native contact distances require their FP exception owner");
    }
public:
    Environment(NativeTu15PhysicsFrameFp& fp,unsigned depth):fp_(fp),host_mxcsr_(_mm_getcsr()) {
        if(depth>6u) throw std::runtime_error("Native contact distances require their x87 capacity owner");
        validate();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment() {
        if(active_)publish();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);
    }
    void publish() {
        asm volatile("fnstcw %0":"=m"(fp_.control_word));
        asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;
    }
    void resume() {
        validate();const std::uint16_t empty=0xffff;
        std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);
        std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;
    }
    template<class F> void dependency(F&& f) {
        publish();try { f();resume(); } catch(...) { resume();throw; }
    }
};
class Memory {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& m,const SourcePhysicsContactRecordObserver& o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4) const {
        std::array<std::byte,4> b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
    }
    __m128 vector(std::uint32_t p) const {
        if(p&15u)throw std::runtime_error("Native contact distances require aligned vector DATA");
        std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 result;std::memcpy(&result,b.data(),16);return result;
    }
    void store(std::uint32_t pc,std::uint32_t p,std::span<const std::byte> b) {
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(b.size());memory_.read(p,event.before);}
        memory_.write(p,b);
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4) {
        std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(8u*n));store(pc,p,std::span(b).first(width));
    }
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value) {
        if(p&15u)throw std::runtime_error("Native contact distances require aligned output DATA");
        std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);store(pc,p,b);
    }
};
__m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
template<unsigned lane> __m128 shuffle(__m128 a){return _mm_shuffle_ps(a,a,lane);}
std::uint32_t low(__m128 a){std::uint32_t bits;std::memcpy(&bits,&a,4);return bits;}
__m128 scalar(std::uint32_t bits){__m128 a{};std::memcpy(&a,&bits,4);return a;}
std::uint32_t dot(__m128 a){auto y=add_scalar(shuffle<0x55>(a),a);return low(add_scalar(shuffle<0xaa>(a),y));}
void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void duplicate_top(){asm volatile("fld %%st(0)":::"memory","st","st(1)");}
void square_top(){asm volatile("fmul %%st(1),%%st(0)":::"memory","st");}
std::uint32_t pop_square_and_discard() {std::uint32_t bits;asm volatile("fstps %0;fstp %%st(0)":"=m"(bits)::"memory","st");return bits;}
std::uint16_t compare_pop(std::uint32_t bits){std::uint16_t sw;asm volatile("fcomps %1;fnstsw %0":"=a"(sw):"m"(bits):"memory","st");return sw;}
bool odd_compare(std::uint16_t sw){const auto bits=(sw>>8)&5u;return bits==1u||bits==4u;}
#endif
}

void native_tu15_publish_contact_distances(SourcePhysicsContactMemory& memory,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext context,std::uint32_t cutoff,std::uint32_t records,
    std::uint32_t count_field,NativeTu15ContactOutput output,std::uint32_t manager,
    NativeTu15PhysicsFrameFp& fp,unsigned depth,const NativeTu15ContactDistanceServices& services,
    const SourcePhysicsContactRecordObserver& observe,NativeTu15ContactDistanceStats* stats) {
    if(stats)++stats->attempts;
    try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        Memory m(memory,observe);Environment environment(fp,depth);
        const auto settings=m.word(context.identity+0xcu);push(m.word(settings+8u));
        auto header=m.word(output.identity);duplicate_top();const auto capacity=m.word(header+8u);square_top();
        const auto old_count=m.word(header+4u);const auto maximum_squared=pop_square_and_discard();
        const auto needed=m.word(count_field)+old_count;
        if(std::int32_t(capacity&0x7fffffffu)<std::int32_t(needed)) {
            const auto doubled=(capacity&0x7fffffffu)*2u;
            const auto desired=std::int32_t(needed)<std::int32_t(doubled)?doubled:needed;
            if(stats)++stats->reservations;
            environment.dependency([&]{native_tu15_reserve_array(memory,{header},desired,48u,fp,services.allocation,observe);});
        }
        const auto data=m.word(header);auto destination=data+old_count*48u;
        m.word(0x30cfa6u,header+4u,needed);
        const auto initial_count=m.word(count_field);
        if(std::int32_t(initial_count)>0) {
            auto record=records+0x10u;std::uint32_t accepted{};
            do {
                if(stats)++stats->tested_records;
                const auto local_first=m.vector(record-0x10u);auto first_motion=m.word(first.identity+8u);
                const auto first_translation=m.vector(first_motion+0x50u),first_z=m.vector(first_motion+0x40u);
                auto second_motion=m.word(second.identity+8u);const auto second_z=m.vector(second_motion+0x40u);
                const auto first_z_term=multiply(first_z,shuffle<0xaau>(local_first));
                const auto first_y=m.vector(first_motion+0x30u),first_tail=add(first_z_term,first_translation);
                const auto first_y_term=multiply(first_y,shuffle<0x55u>(local_first));
                const auto first_x=m.vector(first_motion+0x20u),first_x_term=multiply(first_x,shuffle<0u>(local_first));
                const auto local_second=m.vector(record);const auto first_xy=add(first_x_term,first_y_term);
                const auto second_translation=m.vector(second_motion+0x50u),world_first=add(first_xy,first_tail);
                const auto second_z_term=multiply(second_z,shuffle<0xaau>(local_second));
                const auto second_y=m.vector(second_motion+0x30u),second_tail=add(second_z_term,second_translation);
                const auto second_y_term=multiply(second_y,shuffle<0x55u>(local_second));
                const auto second_x=m.vector(second_motion+0x20u),second_x_term=multiply(second_x,shuffle<0u>(local_second));
                const auto normal=m.vector(record+0x10u),second_xy=add(second_x_term,second_y_term);
                const auto world_second=add(second_xy,second_tail),delta=subtract(world_first,world_second);
                const auto distance=dot(multiply(delta,normal));push(distance);
                const auto bound=compare_pop(m.word(context.identity+8u));
                const auto shift=multiply(shuffle<0u>(scalar(distance)),normal),on_plane=add(world_second,shift);
                const auto error=subtract(world_first,on_plane);const auto error_squared=dot(multiply(error,error));
                bool keep=odd_compare(bound);
                if(keep){push(distance);keep=(compare_pop(cutoff)&0x4100u)==0;}
                if(keep){push(error_squared);keep=odd_compare(compare_pop(maximum_squared));}
                if(keep) {
                    const auto weight=m.word(record-4u);auto blend=shuffle<0u>(scalar(weight));
                    const auto first_part=multiply(blend,world_first),one=m.vector(0x41c500u);
                    const auto other_part=multiply(subtract(one,blend),world_second);
                    m.vector(0x30d117u,destination,add(other_part,first_part));
                    const auto current_normal=m.vector(record+0x10u);m.vector(0x30d11eu,destination+0x10u,current_normal);
                    m.word(0x30d122u,destination+0xcu,distance);
                    const auto point=m.word(record+0xcu,2);m.word(0x30d129u,destination+0x20u,point,2);
                    ++accepted;record+=48u;destination+=48u;if(stats)++stats->published_records;
                } else {
                    const auto table=m.word(manager);const auto point=m.word(record+0xcu,2),owner=m.word(table+8u);
                    if(stats)++stats->removed_points;
                    environment.dependency([&]{if(!services.remove_point)throw std::runtime_error("Native contact distances require current point-removal owner");services.remove_point(manager,{owner},std::uint16_t(point),fp);});
                    const auto remaining=m.word(count_field)-1u;m.word(0x30d150u,count_field,remaining);
                    const auto last=records+remaining*48u;auto vector=m.vector(last);m.vector(0x30d163u,record-0x10u,vector);
                    vector=m.vector(last+0x10u);m.vector(0x30d16bu,record,vector);
                    vector=m.vector(last+0x20u);m.vector(0x30d175u,record+0x10u,vector);
                    header=m.word(output.identity);const auto current_count=m.word(header+4u);
                    m.word(0x30d17bu,header+4u,current_count-1u);
                }
                if(std::int32_t(accepted)>=std::int32_t(m.word(count_field)))break;
            }while(true);
        }
#else
        (void)memory;(void)first;(void)second;(void)context;(void)cutoff;(void)records;(void)count_field;
        (void)output;(void)manager;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native contact distances unavailable on this architecture");
#endif
        if(stats)++stats->completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
