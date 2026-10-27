#include "halo2/native_tu15_constraint_base.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <emmintrin.h>
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t integer(std::uint32_t p,unsigned bytes)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(bytes));std::uint32_t n{};for(unsigned k=0;k<bytes;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    std::uint32_t word(std::uint32_t p)const{return integer(p,4);}std::uint16_t half(std::uint32_t p)const{return integer(p,2);}std::uint8_t byte(std::uint32_t p)const{return integer(p,1);}
    void publish(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observer_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(event);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));publish(pc,p,b);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 constraint base requires aligned vector storage");alignas(16)std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native TU1.5 constraint base requires aligned vector storage");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);publish(pc,p,b);}
    void scalar(std::uint32_t pc,std::uint32_t p,bool pop){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observer_){event.before.resize(4);memory_.read(p,event.before);}alignas(16)std::array<std::byte,108>prior{};std::uint32_t value;asm volatile("fnsave %0\n\tfrstor %0":"=m"(prior)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");if(pop)asm volatile("fstps %0":"=m"(value)::"memory","st");else asm volatile("fsts %0":"=m"(value)::"memory","st");std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(value>>(k*8u));try{memory_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}if(observer_){event.written.assign(b.begin(),b.end());observer_(event);}}
#endif
};
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),host_mxcsr_(_mm_getcsr()){
        if(depth>1||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 constraint base requires its unsupported FP owner");asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffff;std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
};
void push(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void push_integer(std::uint32_t value){const auto n=std::bit_cast<std::int32_t>(value);asm volatile("fildl %0"::"m"(n):"memory","st");}
void pop(){asm volatile("fstp %%st(0)":::"memory","st");}
void swap(){asm volatile("fxch %%st(1)":::"memory","st","st(1)");}
void duplicate(){asm volatile("fld %%st(0)":::"memory","st","st(1)");}
void duplicate_second(){asm volatile("fld %%st(1)":::"memory","st","st(1)","st(2)");}
void duplicate_fourth(){asm volatile("fld %%st(3)":::"memory","st","st(1)","st(2)","st(3)","st(4)");}
void add(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
void subtract(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
void multiply(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
void divide(std::uint32_t bits){asm volatile("fdivs %0"::"m"(bits):"memory","st");}
void reverse_divide(std::uint32_t bits){asm volatile("fdivrs %0"::"m"(bits):"memory","st");}
void negate(){asm volatile("fchs":::"memory","st");}void absolute(){asm volatile("fabs":::"memory","st");}
void add_second(){asm volatile("fadd %%st(1),%%st(0)":::"memory","st","st(1)");}
void multiply_second(){asm volatile("fmul %%st(1),%%st(0)":::"memory","st","st(1)");}
void multiply_third(){asm volatile("fmul %%st(2),%%st(0)":::"memory","st","st(1)","st(2)");}
void multiply_fifth(){asm volatile("fmul %%st(4),%%st(0)":::"memory","st","st(1)","st(2)","st(3)","st(4)");}
void divide_second(){asm volatile("fdiv %%st(1),%%st(0)":::"memory","st","st(1)");}
void subtract_second(){asm volatile("fsub %%st(1),%%st(0)":::"memory","st","st(1)");}
void subtract_pop(){asm volatile("fsubrp %%st,%%st(1)":::"memory","st","st(1)");}
void multiply_pop(){asm volatile("fmulp %%st,%%st(1)":::"memory","st","st(1)");}
void double_top(){asm volatile("fadd %%st(0),%%st(0)":::"memory","st");}
std::uint32_t store(bool popping){std::uint32_t bits;if(popping)asm volatile("fstps %0":"=m"(bits)::"memory","st");else asm volatile("fsts %0":"=m"(bits)::"memory","st");return bits;}
std::uint16_t compare(std::uint32_t bits,bool popping){std::uint16_t sw;if(popping)asm volatile("fcomps %1\n\tfnstsw %0":"=a"(sw):"m"(bits):"memory","st");else asm volatile("fcoms %1\n\tfnstsw %0":"=a"(sw):"m"(bits):"memory","st");return sw;}
std::uint16_t compare_second_pop(){std::uint16_t sw;asm volatile("fcomp %%st(1)\n\tfnstsw %0":"=a"(sw)::"memory","st","st(1)");return sw;}
bool strict_less(std::uint16_t sw){return((sw>>8u)^(sw>>10u))&1u;}bool strict_greater(std::uint16_t sw){return!(sw&0x4100u);}
__m128 mul(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 sub(__m128 left,__m128 right){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 scalar_add(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 scalar_mul(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 scalar_sub(__m128 left,__m128 right){asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t low(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 broadcast(std::uint32_t bits){const auto v=scalar(bits);return _mm_shuffle_ps(v,v,0);}
__m128 cross(__m128 left,__m128 right){const auto high=mul(_mm_shuffle_ps(left,left,0xd2),_mm_shuffle_ps(right,right,0xc9));const auto low=mul(_mm_shuffle_ps(left,left,0xc9),_mm_shuffle_ps(right,right,0xd2));return sub(low,high);}
std::uint32_t dot3(__m128 value){const auto y=_mm_shuffle_ps(value,value,0x55);const auto yx=scalar_add(y,value);const auto z=_mm_shuffle_ps(value,value,0xaa);return low(scalar_add(z,yx));}
__m128 normalized(__m128 vector,std::uint32_t squared_length){auto length=scalar(squared_length),estimate=length;asm volatile("rsqrtss %1,%0":"+x"(estimate):"x"(length):"memory");length=scalar_mul(length,estimate);length=scalar_mul(length,estimate);const auto correction=scalar_sub(scalar(0x40400000u),length);auto scale=scalar_mul(scalar(0x3f000000u),estimate);scale=scalar_mul(scale,correction);return mul(_mm_shuffle_ps(scale,scale,0),vector);}
__m128 body_basis(Memory&m,std::uint32_t body,__m128 value){const auto z=m.vector(body+0x70u),y=m.vector(body+0x60u);const auto z_term=mul(z,_mm_shuffle_ps(value,value,0xaa));const auto y_term=mul(y,_mm_shuffle_ps(value,value,0x55));const auto x=m.vector(body+0x50u);const auto x_term=mul(x,_mm_shuffle_ps(value,value,0));return add(add(x_term,y_term),z_term);}
__m128 torsion_basis(Memory&m,std::uint32_t body,__m128 value){const auto z=m.vector(body+0x70u),y=m.vector(body+0x60u),x=m.vector(body+0x50u);const auto z_term=mul(z,_mm_shuffle_ps(value,value,0xaa));const auto y_term=mul(y,_mm_shuffle_ps(value,value,0x55));const auto x_term=mul(x,_mm_shuffle_ps(value,value,0));return add(add(x_term,y_term),z_term);}
std::array<std::uint32_t,4>words(__m128 value){std::array<std::uint32_t,4>result;std::memcpy(result.data(),&value,16);return result;}
// Source public scalar stores retain true FST/FSTP commit/rejection behavior.
void diagonal(Memory&m,std::uint32_t first,std::uint32_t second,std::uint32_t row,__m128 angular_first,bool first_aggregate,std::uint32_t mass_pc,std::uint32_t inverse_pc){
    push(m.word(first+0x3cu));__m128 angular_second;if(first_aggregate){angular_second=m.vector(row+0x20u);add(m.word(second+0x3cu));}else{add(m.word(second+0x3cu));angular_second=m.vector(row+0x20u);}const auto inertia_second=m.vector(second+0x30u);add(m.word(0x41f64cu));const auto squared_second=mul(angular_second,angular_second);const auto inertia_first=m.vector(first+0x30u);const auto weighted_second=mul(squared_second,inertia_second);const auto squared_first=mul(angular_first,angular_first);const auto weighted_first=mul(squared_first,inertia_first);add(dot3(add(weighted_first,weighted_second)));m.scalar(mass_pc,row+0x2cu,false);push(m.word(0x46a0d0u));divide_second();m.scalar(inverse_pc,row+0x1cu,true);pop();
}

std::uint32_t cross_row_mass(Memory&m,std::uint32_t first,std::uint32_t second,std::uint32_t current){
    const auto first_mass=m.word(first+0x3cu);const auto previous_direction=m.vector(current-48u),direction=m.vector(current),angular_first=m.vector(current+16u);const auto directions=mul(direction,previous_direction);const auto second_mass=m.word(second+0x3cu);const auto second_linear=mul(broadcast(second_mass),directions);const auto previous_second=m.vector(current-16u);const auto first_linear=mul(broadcast(first_mass),directions);const auto inertia_second=m.vector(second+0x30u);const auto linear=add(first_linear,second_linear);const auto angular_second=m.vector(current+32u);const auto second_product=mul(angular_second,previous_second),second_weighted=mul(second_product,inertia_second);const auto previous_first=m.vector(current-32u),inertia_first=m.vector(first+0x30u);const auto first_product=mul(angular_first,previous_first),first_weighted=mul(first_product,inertia_first);return dot3(add(add(first_weighted,second_weighted),linear));
}

void repair_axis(Memory&m,std::uint32_t frame,__m128 normal){
    pop();const auto components=words(normal);unsigned least=0,middle=1,last=2;push(components[0]);absolute();push(components[1]);absolute();const auto second=store(false);push(components[2]);absolute();const auto third=store(true);if(strict_less(compare_second_pop())){pop();middle=least;push(second);least=1;}push(third);const auto compare_third=compare_second_pop();pop();if(strict_less(compare_third)){last=least;least=2;}
    m.word(0x329d0au,frame+0x20u+least*4u,0);const auto last_value=components[last];push(components[middle]);m.word(0x329d1eu,frame+0x20u+middle*4u,last_value);negate();m.scalar(0x329d2cu,frame+0x20u+last*4u,true);push(m.word(0x46a0e8u));
}
__m128 aggregate_row(Memory&m,std::uint32_t first,std::uint32_t second,std::uint32_t frame,std::uint32_t input,std::uint32_t row,bool first_axis,__m128 position_sum,std::uint32_t reciprocal,__m128 mean){
    const auto axis=frame+(first_axis?0x20u:0x30u);const auto direction=m.vector(axis);m.vector(first_axis?0x329de8u:0x329f4fu,row,direction);const auto first_center=m.vector(first+0x40u);const auto first_kind=m.byte(first+0xcu);if(first_axis)mean=mul(broadcast(reciprocal),position_sum);const auto first_offset=sub(mean,first_center);const auto second_center=m.vector(second+0x40u);const auto second_offset=sub(mean,second_center);const auto first_direction=m.vector(axis);auto angular_first=cross(first_offset,first_direction);if(!first_kind)angular_first=body_basis(m,first,angular_first);m.vector(first_axis?0x329e65u:0x329fc6u,row+16u,angular_first);
    const auto second_direction=m.vector(axis);const auto second_kind=m.byte(second+0xcu);auto angular_second=cross(second_direction,second_offset);if(!second_kind)angular_second=body_basis(m,second,angular_second);const auto published_first=m.vector(row+16u);m.vector(first_axis?0x329ed0u:0x32a031u,row+32u,angular_second);diagonal(m,first,second,row,published_first,first_axis,first_axis?0x329f2fu:0x32a090u,first_axis?0x329f3du:0x32a09bu);push(m.word(frame+(first_axis?8u:16u)));multiply(m.word(input+0xcu));m.scalar(first_axis?0x329f48u:0x32a0a6u,row+0xcu,true);return mean;
}

#endif
}
void native_tu15_build_constraint_base(SourcePhysicsContactMemory&memory,const NativeTu15ConstraintBaseBuildRequest&request,NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintBaseStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observer);Environment environment(fp,depth);if(stats)++stats->calls;
    push(m.word(0x46a0e8u));push_integer(request.row_count);auto row=m.word(request.builder.identity);reverse_divide(m.word(0x46a0d0u));const auto first=m.word(request.input.identity),second=m.word(request.input.identity+4u);const auto header=m.word(request.builder.identity+4u);
    m.word(0x32974eu,header+8u,request.runtime);m.word(0x329751u,header,0x11801u);m.word(0x329757u,header+0xcu,first);m.word(0x32975au,header+0x10u,second);m.word(0x32975du,header+0x14u,0x1cu);m.word(0x329764u,header+4u,row);row=m.word(request.builder.identity);auto schema=header+0x18u;auto position_sum=_mm_setzero_ps(),normal_sum=_mm_setzero_ps();const auto reciprocal=store(true);push(m.word(0x46a0e8u));
    if(!less(request.row_count-1u,0u)){
        auto point=request.row_data,properties=request.runtime;
        for(auto remaining=request.row_count;remaining;--remaining,point+=32u,properties+=28u,row+=48u,schema+=4u){
            if(stats)++stats->rows;const auto direction=m.vector(point+16u);m.vector(0x3297b3u,row,direction);const auto position=m.vector(point);const auto first_center=m.vector(first+0x40u);const auto first_kind=m.byte(first+0xcu);const auto first_offset=sub(position,first_center);const auto second_center=m.vector(second+0x40u);const auto second_offset=sub(position,second_center);const auto first_direction=m.vector(point+16u);auto angular_first=cross(first_offset,first_direction);if(!first_kind)angular_first=body_basis(m,first,angular_first);m.vector(0x32982du,row+16u,angular_first);
            const auto second_direction=m.vector(point+16u);const auto second_kind=m.byte(second+0xcu);auto angular_second=cross(second_direction,second_offset);if(!second_kind)angular_second=body_basis(m,second,angular_second);const auto published_first=m.vector(row+16u);m.vector(0x329897u,row+32u,angular_second);diagonal(m,first,second,row,published_first,false,0x3298f4u,0x3298ffu);
            add(m.word(properties+8u));swap();add(m.word(properties));swap();push(m.word(properties+0x14u));push(m.word(point+0xcu));subtract_second();const auto difference=store(true);push(m.word(properties+4u));negate();subtract(difference);const auto bound=store(true);push(m.word(request.input.identity+0x10u));duplicate_second();multiply(m.word(0x41f654u));const auto scaled=store(false);const auto compare_scaled=compare_second_pop();std::uint32_t clamped;if(strict_less(compare_scaled)){pop();clamped=scaled;}else clamped=store(true);push(clamped);add_second();auto shifted=store(true);pop();push(difference);subtract(clamped);push(clamped);double_top();add(m.word(0x46a180u));if(strict_less(compare(bound,true))){push(shifted);subtract(bound);shifted=store(true);add(bound);}duplicate();absolute();if(strict_less(compare(m.word(0x41f118u),true))){pop();push(m.word(0x46a0e8u));}push(shifted);if(strict_less(compare(m.word(0x46a0e8u),true)))push(shifted);else push(m.word(0x46a0e8u));m.scalar(0x3299c6u,properties+0x14u,true);const auto current_position=m.vector(point);multiply(m.word(request.input.identity+0xcu));position_sum=add(position_sum,current_position);const auto current_normal=m.vector(point+16u);negate();normal_sum=add(normal_sum,current_normal);m.scalar(0x3299e1u,row+0xcu,true);
            if(!m.half(properties+0x18u))m.word(0x3299efu,schema,0x30402u);
            else{
                if(stats)++stats->coupled_rows;push(cross_row_mass(m,first,second,row));multiply(m.word(request.input.identity+0x18u));m.word(0x329a8eu,schema-4u,0x50803u);push(m.word(row-4u));push(m.word(row+0x2cu));duplicate_second();multiply_second();duplicate_fourth();multiply_fifth();subtract_pop();reverse_divide(m.word(0x46a0d0u));const auto inverse=store(false);multiply_second();m.scalar(0x329ab0u,row-4u,true);pop();push(inverse);multiply_second();m.scalar(0x329abau,row+0x2cu,true);pop();push(inverse);multiply_second();negate();m.scalar(0x329ac7u,schema,true);pop();
            }
        }
    }
    multiply(reciprocal);multiply_pop();if(!strict_greater(compare(m.word(0x46a0e8u),false))){pop();m.word(0x32a384u,request.builder.identity+4u,schema);m.word(0x32a387u,request.builder.identity,row);return;}
    // Choose the source aggregate direction, retaining RSQRTSS and its one
    // Newton refinement rather than replacing the original estimate.
    push(reciprocal);const auto normal_squared=mul(normal_sum,normal_sum);multiply(reciprocal);const auto squared_length=dot3(normal_squared);multiply(squared_length);__m128 normal;
    if(strict_greater(compare(m.word(0x46a188u),true)))normal=m.vector(request.row_data+16u);
    else{push(squared_length);if(strict_greater(compare(m.word(0x46a178u),true)))normal=normalized(normal_sum,squared_length);else{normal=m.vector(request.row_data+16u);const auto first_squared=dot3(mul(normal,normal));push(first_squared);if(strict_less(compare(m.word(0x46a194u),true))){normal=_mm_setzero_ps();auto bits=words(normal);bits[1]=0x3f800000u;std::memcpy(&normal,bits.data(),16);pop();push(m.word(0x46a0e8u));}}}
    // The previous retained axis is repaired only when the source cross is
    // too short. Its fourth lane is never initialized by the repair stores.
    for(;;){const auto prior_axis=m.vector(request.frame+0x20u);const auto perpendicular=cross(prior_axis,normal);m.vector(0x329c6eu,request.frame+0x30u,perpendicular);const auto length=dot3(mul(perpendicular,perpendicular));push(length);if(strict_greater(compare(m.word(0x46a180u),true)))break;if(stats)++stats->basis_repairs;repair_axis(m,request.frame,normal);}
    const auto perpendicular=m.vector(request.frame+0x30u);const auto length=dot3(mul(perpendicular,perpendicular));const auto unit_perpendicular=normalized(perpendicular,length);m.vector(0x329db0u,request.frame+0x30u,unit_perpendicular);const auto published_perpendicular=m.vector(request.frame+0x30u);const auto first_axis=cross(normal,published_perpendicular);m.vector(0x329dcfu,request.frame+0x20u,first_axis);
    if(stats)++stats->aggregate_pairs;auto mean=aggregate_row(m,first,second,request.frame,request.input.identity,row,true,position_sum,reciprocal,_mm_setzero_ps());row+=48u;aggregate_row(m,first,second,request.frame,request.input.identity,row,false,position_sum,reciprocal,mean);row+=48u;
    const auto coupling=cross_row_mass(m,first,second,row-48u);multiply(m.word(request.input.identity+0x1cu));m.word(0x32a136u,schema,0x51407u);push(m.word(row-0x34u));push(m.word(row-4u));duplicate();multiply_third();push(coupling);multiply(coupling);subtract_pop();reverse_divide(m.word(0x46a0d0u));const auto pair_inverse=store(false);multiply_second();m.scalar(0x32a163u,row-0x34u,true);pop();push(pair_inverse);multiply_second();m.scalar(0x32a16eu,row-4u,true);m.word(0x32a171u,schema+4u,request.frame+4u);auto schema_end=schema+0x14u;pop();m.word(0x32a179u,schema+0x10u,8);push(pair_inverse);multiply(coupling);negate();m.scalar(0x32a18eu,schema+8u,true);m.scalar(0x32a191u,schema+0xcu,true);
    if(!less(1u,request.row_count)){m.word(0x32a361u,request.builder.identity+4u,schema_end);m.word(0x32a364u,request.builder.identity,row);return;}
    if(m.byte(request.frame)&4u){m.word(0x32a1b5u,request.frame+0x1cu,0);if(!less(request.row_count-1u,0u)){auto point=request.row_data;for(auto remaining=request.row_count;remaining;--remaining,point+=32u){const auto delta=sub(m.vector(point),mean);auto squared=scalar(dot3(mul(delta,delta)));asm volatile("sqrtss %0,%0":"+x"(squared)::"memory");push(low(squared));add(m.word(request.frame+0x1cu));m.scalar(0x32a210u,request.frame+0x1cu,true);}}
        push(reciprocal);multiply(m.word(request.frame+0x1cu));m.scalar(0x32a224u,request.frame+0x1cu,false);if(strict_less(compare(m.word(0x41f658u),true))){m.word(0x32a370u,request.builder.identity,row);m.word(0x32a376u,request.builder.identity+4u,schema_end);return;}const auto flags=m.word(request.frame);m.word(0x32a23fu,request.frame,flags&~4u);
    }
    const auto first_kind=m.byte(first+0xcu);if(first_kind)m.vector(0x32a255u,row,normal);else m.vector(0x32a28au,row,torsion_basis(m,first,normal));const auto second_kind=m.byte(second+0xcu);const auto sign=m.vector(0x461960u);const auto negative_normal=_mm_xor_ps(normal,sign);if(second_kind)m.vector(0x32a29du,row+16u,negative_normal);else m.vector(0x32a2d3u,row+16u,torsion_basis(m,second,negative_normal));const auto second_axis=m.vector(row+16u),first_angular=m.vector(row);const auto inertia_second=m.vector(second+0x30u);const auto second_square=mul(second_axis,second_axis);const auto inertia_first=m.vector(first+0x30u);const auto second_weighted=mul(second_square,inertia_second),first_square=mul(first_angular,first_angular),first_weighted=mul(first_square,inertia_first);push(dot3(add(first_weighted,second_weighted)));add(m.word(0x41f64cu));row+=32u;reverse_divide(m.word(0x46a0d0u));m.scalar(0x32a332u,row-0x14u,true);push(m.word(0x46a0d0u));divide(m.word(request.frame+0x1cu));push(m.word(request.frame+0x1cu));m.scalar(0x32a341u,schema_end,true);m.word(0x32a343u,schema,0x61808u);schema_end=schema+0x18u;multiply(m.word(row-0x14u));m.scalar(0x32a34fu,row-0x14u,true);push(m.word(request.frame+0x18u));multiply(m.word(request.input.identity+0xcu));m.scalar(0x32a35bu,row-4u,true);if(stats)++stats->aggregate_axes;m.word(0x32a361u,request.builder.identity+4u,schema_end);m.word(0x32a364u,request.builder.identity,row);
#else
    (void)memory;(void)request;(void)fp;(void)depth;(void)observer;(void)stats;throw std::runtime_error("Exact native TU1.5 constraint base backend unavailable");
#endif
}
} // namespace halo2
