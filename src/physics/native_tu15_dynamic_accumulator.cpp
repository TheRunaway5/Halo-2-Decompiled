#include "halo2/native_tu15_dynamic_accumulator.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
using Storage=NativeTu15DynamicAccumulatorStorage;using Vector=NativeTu15PhysicsVector;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>5||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 accumulator requires its unsupported FP trap owner");
        mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    void publish(){asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();}
    // The closed angular helper owns a nested native numeric scope. Keep this
    // root's real extended dt value/register tags intact while adopting the
    // helper's actual FP flags and controls after its scope has restored dt.
    void resume(){std::array<std::byte,28>environment{};asm volatile("fnstenv %0":"=m"(environment)::"memory");std::memcpy(environment.data(),&fp_.control_word,2);std::memcpy(environment.data()+4,&fp_.status,2);asm volatile("fldenv %0"::"m"(environment):"memory");_mm_setcsr(fp_.mxcsr);}
    ~Environment(){publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);}
};
__m128 simd(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
Vector words(__m128 value){Vector v;_mm_storeu_si128(reinterpret_cast<__m128i*>(v.data()),_mm_castps_si128(value));return v;}
__m128 broadcast(std::uint32_t value){return _mm_castsi128_ps(_mm_set1_epi32(std::bit_cast<std::int32_t>(value)));}
__m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void duplicate(){asm volatile("fld %%st(0)":::"memory","st");}
void multiply_scalar(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
void reverse_subtract(std::uint32_t bits){asm volatile("fsubrs %0"::"m"(bits):"memory","st");}
std::uint32_t pop(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
std::uint32_t stored(){std::uint32_t value;asm volatile("fsts %0":"=m"(value)::"memory");return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value)::"memory");return value;}
void clamp_candidate(Storage&s){
    reverse_subtract(s.one());push(s.zero());asm volatile("fcomp %%st(1)":::"memory","st");
    if((status()&0x4100u)==0){asm volatile("fstp %%st(0)":::"memory","st");push(s.zero());}
}
bool torque_required(){const auto flags=status()&0x4400u;return flags==0||flags==0x4400u;}
void transpose_rotation(Storage&s){
    s.accumulator_rotation_component(0,s.rotation_component(0),0x2e4826u);
    s.accumulator_rotation_component(5,s.rotation_component(5),0x2e482bu);
    s.accumulator_rotation_component(10,s.rotation_component(10),0x2e4831u);
    s.accumulator_rotation_component(3,0,0x2e4836u);s.accumulator_rotation_component(7,0,0x2e4839u);s.accumulator_rotation_component(11,0,0x2e483cu);
    s.accumulator_rotation_component(1,s.rotation_component(4),0x2e4842u);s.accumulator_rotation_component(4,s.rotation_component(1),0x2e4848u);
    s.accumulator_rotation_component(2,s.rotation_component(8),0x2e484eu);s.accumulator_rotation_component(8,s.rotation_component(2),0x2e4854u);
    s.accumulator_rotation_component(6,s.rotation_component(9),0x2e485au);s.accumulator_rotation_component(9,s.rotation_component(6),0x2e4860u);
}
#endif
}
NativeTu15ConstraintAccumulator native_tu15_setup_dynamic_motion_accumulator(Storage&s,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);push(s.seconds());const auto force=simd(s.accumulated_force());duplicate();multiply_scalar(s.inverse_mass());const auto impulse=pop();const auto seconds=stored();duplicate();
    const auto linear=s.linear_velocity();s.linear_velocity(words(add(simd(linear),multiply(broadcast(impulse),force))),0);
    const auto gravity=simd(s.gravity());const auto current_linear=simd(s.linear_velocity());const auto gravity_impulse=multiply(broadcast(seconds),gravity);s.linear_velocity(words(add(current_linear,gravity_impulse)),1);
    multiply_scalar(s.linear_damping());clamp_candidate(s);const auto damped_linear=simd(s.linear_velocity());const auto linear_scale=pop();push(s.zero());
    s.linear_velocity(words(multiply(broadcast(linear_scale),damped_linear)),2);
    push(s.torque_present());asm volatile("fucompp":::"memory","st");s.clear_force();
    if(torque_required()){
        const auto step=s.seconds();const auto torque=simd(s.accumulated_torque());const auto angular_impulse=words(multiply(broadcast(step),torque));
        environment.publish();try{native_tu15_apply_motion_angular_impulse(s,angular_impulse,fp,depth+1u);}catch(...){environment.resume();throw;}environment.resume();s.clear_torque();
    }
    multiply_scalar(s.angular_damping());clamp_candidate(s);const auto angular_scale=pop();const auto angular=simd(s.angular_velocity());s.angular_velocity(words(multiply(broadcast(angular_scale),angular)),false);
    transpose_rotation(s);s.accumulator_type();s.accumulator_inverse_inertia(s.inverse_inertia());s.accumulator_linear_velocity(s.linear_velocity());s.accumulator_position(s.position());
    const auto velocity=simd(s.angular_velocity()),z=simd(s.accumulator_rotation(2)),y=simd(s.accumulator_rotation(1));
    const auto scaled_z=multiply(z,_mm_shuffle_ps(velocity,velocity,0xaa));const auto scaled_y=multiply(y,_mm_shuffle_ps(velocity,velocity,0x55));const auto x=simd(s.accumulator_rotation(0));
    const auto scaled_x=multiply(x,_mm_shuffle_ps(velocity,velocity,0));s.accumulator_angular_velocity(words(add(add(scaled_x,scaled_y),scaled_z)));s.accumulator_dynamic_flag();return{s.output().identity+0x80u};
#else
    (void)s;(void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 dynamic accumulator backend unavailable");
#endif
}
NativeTu15DynamicAccumulatorMemory::NativeTu15DynamicAccumulatorMemory(SourcePhysicsContactMemory&m,NativeTu15PhysicsMotion motion,NativeTu15PhysicsStepPacket step,NativeTu15PhysicsGravity gravity,NativeTu15ConstraintAccumulator output,const SourcePhysicsContactRecordObserver&o):memory_(m),motion_(motion.identity),step_(step.identity),gravity_(gravity.identity),output_(output.identity),observer_(o){}
std::uint32_t NativeTu15DynamicAccumulatorMemory::scalar(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
Vector NativeTu15DynamicAccumulatorMemory::vector(std::uint32_t p)const{std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector v{};for(unsigned n=0;n<4;++n)for(unsigned i=0;i<4;++i)v[n]|=std::to_integer<unsigned>(b[n*4u+i])<<(8u*i);return v;}
void NativeTu15DynamicAccumulatorMemory::scalar(std::uint32_t site,std::uint32_t p,std::uint32_t v,unsigned n){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8u*i));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(n);memory_.read(p,store.before);store.written.assign(b.begin(),b.begin()+n);}memory_.write(p,std::span(b).first(n));if(observer_)observer_(store);}
void NativeTu15DynamicAccumulatorMemory::vector(std::uint32_t site,std::uint32_t p,const Vector&v){std::array<std::byte,16>b{};for(unsigned n=0;n<4;++n)for(unsigned i=0;i<4;++i)b[n*4u+i]=std::byte(v[n]>>(8u*i));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(16);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
std::uint32_t NativeTu15DynamicAccumulatorMemory::seconds()const{return scalar(step_);}
std::uint32_t NativeTu15DynamicAccumulatorMemory::inverse_mass()const{return scalar(motion_+0x2cu);}
std::uint32_t NativeTu15DynamicAccumulatorMemory::linear_damping()const{return scalar(motion_+0x14u);}
std::uint32_t NativeTu15DynamicAccumulatorMemory::angular_damping()const{return scalar(motion_+0x18u);}
std::uint32_t NativeTu15DynamicAccumulatorMemory::one()const{return scalar(0x46a0d0u);}
std::uint32_t NativeTu15DynamicAccumulatorMemory::zero()const{return scalar(0x46a0e8u);}
std::uint32_t NativeTu15DynamicAccumulatorMemory::torque_present()const{return scalar(motion_+0x10cu);}
Vector NativeTu15DynamicAccumulatorMemory::gravity()const{return vector(gravity_);}
Vector NativeTu15DynamicAccumulatorMemory::linear_velocity()const{return vector(motion_+0x40u);}
Vector NativeTu15DynamicAccumulatorMemory::angular_velocity()const{return vector(motion_+0x50u);}
Vector NativeTu15DynamicAccumulatorMemory::accumulated_force()const{return vector(motion_+0xf0u);}
Vector NativeTu15DynamicAccumulatorMemory::accumulated_torque()const{return vector(motion_+0x100u);}
Vector NativeTu15DynamicAccumulatorMemory::rotation(unsigned axis)const{if(axis>2)throw std::runtime_error("Invalid accumulator rotation axis");return vector(motion_+0x80u+axis*16u);}
Vector NativeTu15DynamicAccumulatorMemory::inverse_inertia()const{return vector(motion_+0xe0u);}
void NativeTu15DynamicAccumulatorMemory::linear_velocity(Vector v,unsigned phase){constexpr std::array sites{0x325453u,0x325471u,0x3254b3u};vector(sites.at(phase),motion_+0x40u,v);}
void NativeTu15DynamicAccumulatorMemory::angular_velocity(Vector v,bool impulse){vector(impulse?0x32515au:0x325556u,motion_+0x50u,v);}
void NativeTu15DynamicAccumulatorMemory::clear_force(){vector(0x3254c7u,motion_+0xf0u,{});}
void NativeTu15DynamicAccumulatorMemory::clear_torque(){vector(0x325508u,motion_+0x100u,{});}
std::uint32_t NativeTu15DynamicAccumulatorMemory::rotation_component(unsigned n)const{if(n>11)throw std::runtime_error("Invalid accumulator rotation component");return scalar(motion_+0x80u+n*4u);}
void NativeTu15DynamicAccumulatorMemory::accumulator_rotation_component(unsigned n,std::uint32_t value,std::uint32_t site){if(n>11)throw std::runtime_error("Invalid accumulator output rotation component");scalar(site,output_+0x50u+n*4u,value);}
Vector NativeTu15DynamicAccumulatorMemory::accumulator_rotation(unsigned axis)const{if(axis>2)throw std::runtime_error("Invalid accumulator output rotation axis");return vector(output_+0x50u+axis*16u);}
Vector NativeTu15DynamicAccumulatorMemory::position()const{return vector(motion_+0x70u);}
void NativeTu15DynamicAccumulatorMemory::accumulator_type(){scalar(0x32555fu,output_,2,1);}
void NativeTu15DynamicAccumulatorMemory::accumulator_inverse_inertia(Vector v){vector(0x325569u,output_+0x30u,v);}
void NativeTu15DynamicAccumulatorMemory::accumulator_linear_velocity(Vector v){vector(0x325571u,output_+0x10u,v);}
void NativeTu15DynamicAccumulatorMemory::accumulator_position(Vector v){vector(0x325579u,output_+0x40u,v);}
void NativeTu15DynamicAccumulatorMemory::accumulator_angular_velocity(Vector v){vector(0x3255b0u,output_+0x20u,v);}
void NativeTu15DynamicAccumulatorMemory::accumulator_dynamic_flag(){scalar(0x3255b4u,output_+0xcu,0,1);}
NativeTu15ConstraintAccumulator NativeTu15DynamicAccumulatorMemory::output()const{return{output_};}
} // namespace halo2
