#include "native_tu15_constraint_solver_internal.hpp"
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2::native_tu15_solver_detail {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
__m128 simd(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
Vector words(__m128 v){Vector out;_mm_storeu_si128(reinterpret_cast<__m128i*>(out.data()),_mm_castps_si128(v));return out;}
__m128 broadcast(std::uint32_t n){return _mm_castsi128_ps(_mm_set1_epi32(std::bit_cast<std::int32_t>(n)));}
__m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint32_t projection(__m128 value){const auto xy=add_scalar(_mm_shuffle_ps(value,value,0x55),value);return words(add_scalar(_mm_shuffle_ps(value,value,0xaa),xy))[0];}
// Real native extended intermediates stay on the hardware x87 stack. These
// expressions have no register array, guest flag evaluator or CPU dispatcher.
void begin_scalar(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void scale_current(std::uint32_t value){asm volatile("fmuls %0"::"m"(value):"memory","st");}
void subtract_current(std::uint32_t value){asm volatile("fsubs %0"::"m"(value):"memory","st");}
// GNU AT&T's two-register spelling is reversed: FSUBRP below encodes the
// original Intel FSUBP ST(1),ST (bias minus projected velocity).
void subtract_projected_velocity(){asm volatile("fsubrp %%st,%%st(1)":::"memory","st");}
void multiply_effective_mass(){asm volatile("fmulp %%st,%%st(1)":::"memory","st");}
void discard_scalar(){asm volatile("fstp %%st(0)":::"memory","st");}
std::uint32_t impulse_for_simd(){std::uint32_t value;asm volatile("fsts %0":"=m"(value)::"memory","st");return value;}
std::uint32_t take_scalar(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
std::uint16_t compare_current(std::uint32_t value,bool pop=false){std::uint16_t status;if(pop)asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(value):"memory","st");else asm volatile("fcoms %1\n\tfnstsw %0":"=a"(status):"m"(value):"memory","st");return status;}
bool positive(std::uint16_t status){return(status&0x4100u)==0;}
bool negative(std::uint16_t status){return(status&0x0500u)==0x0100u;}
bool greater_or_unordered(std::uint16_t status){const auto pair=status&0x4100u;return pair==0u||pair==0x4100u;}
void multiply_by_effective_mass(){asm volatile("fmul %%st(1)":::"memory","st");}
void retain_scaled_impulse(){asm volatile("fstp %%st(1)":::"memory","st");}
void subtract_retained_projection(){asm volatile("fsub %%st(1)":::"memory","st");}
void finish_upper_limit(){asm volatile("fsubp %%st,%%st(1)":::"memory","st");} // Intel FSUBRP.
void replace_with_bound_ratio(std::uint32_t bound){begin_scalar(bound);asm volatile("fdiv %%st(1)\n\tfmulp %%st,%%st(1)":::"memory","st");}
void absolute_copy(){asm volatile("fld %%st(0)\n\tfabs":::"memory","st");}
void cap_ratio(std::uint32_t cap){asm volatile("fdivrs %0\n\tfld %%st(0)\n\tfmulp %%st,%%st(2)"::"m"(cap):"memory","st");}
void exchange_scalars(){asm volatile("fxch %%st(1)":::"memory","st");}
void add_scalar_products(){asm volatile("faddp %%st,%%st(1)":::"memory","st");}
void copy_first_pair_impulse(){asm volatile("fld %%st(1)":::"memory","st");}
void coupled_pair(Access&m,std::uint32_t row,std::uint32_t schema,std::uint32_t first_residual,std::uint32_t second_residual){
  begin_scalar(m.word(row+0x2cu));begin_scalar(second_residual);scale_current(m.word(schema+8u));exchange_scalars();scale_current(first_residual);add_scalar_products();
  begin_scalar(m.word(row+0x5cu));begin_scalar(first_residual);scale_current(m.word(schema+8u));exchange_scalars();scale_current(second_residual);add_scalar_products();
}
void triple_coupled_pair(Access&m,std::uint32_t row,std::uint32_t schema,std::uint32_t first_residual,std::uint32_t second_residual){
  // Actual32742E/32743E push the retained residual BEFORE reading mass.
  // This also preserves FMUL destination priority for distinct NaN payloads.
  begin_scalar(first_residual);scale_current(m.word(row+0x2cu));begin_scalar(second_residual);scale_current(m.word(schema+8u));add_scalar_products();
  begin_scalar(second_residual);scale_current(m.word(row+0x5cu));begin_scalar(first_residual);scale_current(m.word(schema+8u));add_scalar_products();
}
struct FullImpulseStores {std::uint32_t first_linear,second_linear,first_angular,second_angular,result;};
void accumulate_result(Access&,std::uint32_t,std::uint32_t);
void apply_full_weights(Access&m,const PrimaryState&s,std::uint32_t row,__m128 axis,__m128 first_weight,__m128 second_weight,std::uint32_t result,const FullImpulseStores&pc){
  const auto first_delta=multiply(_mm_shuffle_ps(first_weight,first_weight,0xff),axis);
  const auto second_delta=multiply(_mm_shuffle_ps(second_weight,second_weight,0xff),axis);
  m.vector(pc.first_linear,s.first_accumulator+0x10u,words(add(simd(m.vector(s.first_accumulator+0x10u)),first_delta)));
  m.vector(pc.second_linear,s.second_accumulator+0x10u,words(subtract(simd(m.vector(s.second_accumulator+0x10u)),second_delta)));
  const auto first_angular_delta=multiply(first_weight,simd(m.vector(row+0x10u)));
  const auto second_angular_delta=multiply(second_weight,simd(m.vector(row+0x20u)));
  m.vector(pc.first_angular,s.first_accumulator+0x20u,words(add(simd(m.vector(s.first_accumulator+0x20u)),first_angular_delta)));
  m.vector(pc.second_angular,s.second_accumulator+0x20u,words(add(simd(m.vector(s.second_accumulator+0x20u)),second_angular_delta)));
  accumulate_result(m,result,pc.result);
}
void clamp_impulse(Access&m,std::uint32_t schema){
  if(positive(compare_current(m.word(schema+4u))))replace_with_bound_ratio(m.word(schema+4u));
  else if(negative(compare_current(m.word(schema+8u))))replace_with_bound_ratio(m.word(schema+8u));
}
void accumulate_result(Access&m,std::uint32_t address,std::uint32_t site){const auto previous=m.word(address);std::uint32_t value;asm volatile("fadds %1\n\tfstps %0":"=m"(value):"m"(previous):"memory","st");m.word(site,address,value);}

void positive_full_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian;
    const auto second_linear=simd(m.vector(second+0x10u));begin_scalar(m.word(row+0xcu));
    const auto first_linear=simd(m.vector(first+0x10u)),first_axis=simd(m.vector(row+0x10u));
    const auto second_angular=simd(m.vector(second+0x20u)),linear_axis=simd(m.vector(row));
    const auto difference=subtract(first_linear,second_linear);
    const auto angular=multiply(simd(m.vector(first+0x20u)),first_axis);
    const auto second_axis=simd(m.vector(row+0x20u));const auto other_angular=multiply(second_angular,second_axis);
    const auto linear=multiply(difference,linear_axis);const auto dot=projection(add(add(angular,other_angular),linear));
    scale_current(m.word(s.configuration+4u));begin_scalar(dot);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();
    if(positive(compare_current(m.word(0x46a0e8u)))){
      scale_current(m.word(row+0x1cu));const auto first_mass=simd(m.vector(first+0x30u));
      const auto impulse=broadcast(impulse_for_simd());const auto first_weight=multiply(impulse,first_mass);
      const auto second_mass=simd(m.vector(second+0x30u));const auto second_weight=multiply(impulse,second_mass);
      const auto first_linear_delta=multiply(_mm_shuffle_ps(first_weight,first_weight,0xff),linear_axis);
      const auto second_linear_delta=multiply(_mm_shuffle_ps(second_weight,second_weight,0xff),linear_axis);
      m.vector(0x32715eu,first+0x10u,words(add(simd(m.vector(first+0x10u)),first_linear_delta)));
      m.vector(0x327168u,second+0x10u,words(subtract(simd(m.vector(second+0x10u)),second_linear_delta)));
      const auto first_angular_delta=multiply(first_weight,simd(m.vector(row+0x10u)));
      const auto second_angular_delta=multiply(second_weight,simd(m.vector(row+0x20u)));
      m.vector(0x327180u,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_angular_delta)));
      m.vector(0x32718au,second+0x20u,words(add(simd(m.vector(second+0x20u)),second_angular_delta)));
      accumulate_result(m,s.result,0x32718fu);
    }else discard_scalar();
    const auto next=m.byte(s.schema+4u);s.schema+=4u;s.jacobian+=0x30u;s.result+=4u;
    if(next!=2u)return;
  }while(true);
}
void bilateral_angular_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian,result=s.result;
    const auto first_axis=simd(m.vector(row));begin_scalar(m.word(row+0xcu));
    const auto second_axis=simd(m.vector(row+0x10u));begin_scalar(m.word(row+0x1cu));
    const auto first_angular=simd(m.vector(first+0x20u)),second_angular=simd(m.vector(second+0x20u));
    const auto dot=projection(add(multiply(first_angular,first_axis),multiply(second_angular,second_axis)));
    scale_current(m.word(s.configuration+4u));const auto first_mass=simd(m.vector(first+0x30u));
    begin_scalar(dot);s.schema+=4u;scale_current(m.word(s.configuration+8u));s.jacobian+=0x20u;s.result+=4u;
    subtract_projected_velocity();multiply_effective_mass();
    const auto impulse=broadcast(impulse_for_simd());const auto first_weight=multiply(impulse,first_mass);
    const auto second_mass=simd(m.vector(second+0x30u));const auto first_delta=multiply(first_weight,first_axis);
    m.vector(0x327bf5u,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_delta)));
    const auto second_current=simd(m.vector(second+0x20u));const auto second_weight=multiply(impulse,second_mass);
    m.vector(0x327c08u,second+0x20u,words(add(second_current,multiply(second_weight,second_axis))));
    accumulate_result(m,result,0x327c0eu);if(m.byte(s.schema)!=12u)return;
  }while(true);
}
void bilateral_full_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian,result=s.result;
    const auto first_linear=simd(m.vector(first+0x10u));begin_scalar(m.word(row+0x1cu));
    const auto first_axis=simd(m.vector(row+0x10u));begin_scalar(m.word(row+0xcu));
    const auto second_axis=simd(m.vector(row+0x20u)),second_linear=simd(m.vector(second+0x10u));
    const auto second_angular=simd(m.vector(second+0x20u));const auto difference=subtract(first_linear,second_linear);
    const auto first_product=multiply(simd(m.vector(first+0x20u)),first_axis);
    const auto linear_axis=simd(m.vector(row));const auto linear_product=multiply(difference,linear_axis);
    const auto second_product=multiply(second_angular,second_axis);const auto dot=projection(add(add(first_product,second_product),linear_product));
    scale_current(m.word(s.configuration+4u));const auto first_mass=simd(m.vector(first+0x30u));
    begin_scalar(dot);s.schema+=4u;scale_current(m.word(s.configuration+8u));s.jacobian+=0x30u;s.result+=4u;
    subtract_projected_velocity();multiply_effective_mass();
    const auto impulse=broadcast(impulse_for_simd());const auto first_weight=multiply(impulse,first_mass);
    const auto second_weight=multiply(impulse,simd(m.vector(second+0x30u)));
    const auto first_linear_delta=multiply(_mm_shuffle_ps(first_weight,first_weight,0xff),linear_axis);
    const auto second_linear_delta=multiply(_mm_shuffle_ps(second_weight,second_weight,0xff),linear_axis);
    m.vector(0x327cf6u,first+0x10u,words(add(simd(m.vector(first+0x10u)),first_linear_delta)));
    m.vector(0x327cffu,second+0x10u,words(subtract(simd(m.vector(second+0x10u)),second_linear_delta)));
    const auto first_angular_delta=multiply(first_weight,simd(m.vector(row+0x10u)));
    const auto second_angular_delta=multiply(second_weight,simd(m.vector(row+0x20u)));
    m.vector(0x327d19u,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_angular_delta)));
    m.vector(0x327d26u,second+0x20u,words(add(simd(m.vector(second+0x20u)),second_angular_delta)));
    accumulate_result(m,result,0x327d2cu);if(m.byte(s.schema)!=13u)return;
  }while(true);
}
void capped_angular_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian;
    const auto first_axis=simd(m.vector(row));begin_scalar(m.word(row+0x1cu));
    const auto first_angular=simd(m.vector(first+0x20u)),second_angular=simd(m.vector(second+0x20u));
    const auto cap=m.word(s.schema+4u);const auto first_product=multiply(first_angular,first_axis);
    const auto second_axis=simd(m.vector(row+0x10u));const auto second_product=multiply(second_angular,second_axis);
    const auto dot=projection(add(first_product,second_product));
    scale_current(m.word(s.configuration+0xcu));begin_scalar(dot);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();scale_current(m.word(row+0xcu));
    absolute_copy();if(positive(compare_current(cap))){cap_ratio(cap);scale_current(m.word(row+0x1cu));m.word(0x327924u,row+0x1cu,take_scalar());}else discard_scalar();
    const auto first_mass=simd(m.vector(first+0x30u));const auto impulse=broadcast(impulse_for_simd());
    const auto first_weight=multiply(impulse,first_mass),second_weight=multiply(impulse,simd(m.vector(second+0x30u)));
    const auto first_delta=multiply(first_weight,simd(m.vector(row)));
    const auto second_delta=multiply(second_weight,simd(m.vector(row+0x10u)));
    m.vector(0x327965u,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_delta)));
    m.vector(0x32796fu,second+0x20u,words(add(simd(m.vector(second+0x20u)),second_delta)));
    accumulate_result(m,s.result,0x32797au);s.schema+=8u;s.jacobian+=0x20u;
    const auto next=m.byte(s.schema);s.result+=4u;if(next!=9u)return;
  }while(true);
}
void capped_full_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian;
    const auto second_linear=simd(m.vector(second+0x10u));begin_scalar(m.word(row+0xcu));const auto cap=m.word(s.schema+4u);
    const auto first_linear=simd(m.vector(first+0x10u)),first_axis=simd(m.vector(row+0x10u));
    const auto difference=subtract(first_linear,second_linear);const auto first_product=multiply(simd(m.vector(first+0x20u)),first_axis);
    const auto linear_axis=simd(m.vector(row)),second_angular=simd(m.vector(second+0x20u));
    const auto linear_product=multiply(difference,linear_axis);const auto second_axis=simd(m.vector(row+0x20u));
    const auto second_product=multiply(second_angular,second_axis);const auto dot=projection(add(add(first_product,second_product),linear_product));
    scale_current(m.word(s.configuration+0xcu));begin_scalar(dot);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();scale_current(m.word(row+0x1cu));
    absolute_copy();if(positive(compare_current(cap))){cap_ratio(cap);scale_current(m.word(row+0xcu));m.word(0x327de6u,row+0xcu,take_scalar());}else discard_scalar();
    const auto first_mass=simd(m.vector(first+0x30u));const auto impulse=broadcast(impulse_for_simd());
    const auto current_axis=simd(m.vector(row));const auto first_weight=multiply(impulse,first_mass);
    const auto second_weight=multiply(impulse,simd(m.vector(second+0x30u)));
    const auto first_linear_delta=multiply(_mm_shuffle_ps(first_weight,first_weight,0xff),current_axis);
    const auto second_linear_delta=multiply(_mm_shuffle_ps(second_weight,second_weight,0xff),current_axis);
    m.vector(0x327e35u,first+0x10u,words(add(simd(m.vector(first+0x10u)),first_linear_delta)));
    m.vector(0x327e3fu,second+0x10u,words(subtract(simd(m.vector(second+0x10u)),second_linear_delta)));
    const auto first_delta=multiply(first_weight,simd(m.vector(row+0x10u)));
    const auto second_delta=multiply(second_weight,simd(m.vector(row+0x20u)));
    m.vector(0x327e57u,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_delta)));
    m.vector(0x327e61u,second+0x20u,words(add(simd(m.vector(second+0x20u)),second_delta)));
    accumulate_result(m,s.result,0x327e6cu);s.schema+=8u;s.jacobian+=0x30u;
    const auto next=m.byte(s.schema);s.result+=4u;if(next!=6u)return;
  }while(true);
}
void clamped_angular_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian;
    const auto first_angular=simd(m.vector(first+0x20u));begin_scalar(m.word(row+0xcu));
    const auto second_angular=simd(m.vector(second+0x20u));begin_scalar(m.word(row+0x1cu));
    const auto first_axis=simd(m.vector(row)),second_axis=simd(m.vector(row+0x10u));
    const auto second_product=multiply(second_angular,second_axis);const auto first_product=multiply(first_angular,first_axis);
    const auto dot=projection(add(first_product,second_product));subtract_current(dot);multiply_by_effective_mass();scale_current(m.word(s.configuration+4u));retain_scaled_impulse();clamp_impulse(m,s.schema);
    const auto first_mass=simd(m.vector(first+0x30u));const auto impulse=broadcast(impulse_for_simd());
    const auto first_weight=multiply(impulse,first_mass);const auto second_mass=simd(m.vector(second+0x30u));
    const auto first_delta=multiply(first_weight,first_axis);
    m.vector(0x327b22u,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_delta)));
    const auto second_current=simd(m.vector(second+0x20u));const auto second_weight=multiply(impulse,second_mass);
    m.vector(0x327b36u,second+0x20u,words(add(second_current,multiply(second_weight,second_axis))));
    accumulate_result(m,s.result,0x327b41u);s.schema+=0x10u;s.jacobian+=0x20u;
    const auto next=m.byte(s.schema);s.result+=4u;if(next!=4u)return;
  }while(true);
}
void clamped_full_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian;begin_scalar(m.word(row+0x1cu));
    const auto second_linear=simd(m.vector(second+0x10u));begin_scalar(m.word(row+0xcu));
    const auto first_linear=simd(m.vector(first+0x10u)),first_axis=simd(m.vector(row+0x10u));
    const auto second_angular=simd(m.vector(second+0x20u)),linear_axis=simd(m.vector(row));
    const auto difference=subtract(first_linear,second_linear);const auto first_product=multiply(simd(m.vector(first+0x20u)),first_axis);
    const auto second_axis=simd(m.vector(row+0x20u));const auto second_product=multiply(second_angular,second_axis);
    const auto linear_product=multiply(difference,linear_axis);const auto dot=projection(add(add(first_product,second_product),linear_product));
    subtract_current(dot);multiply_by_effective_mass();scale_current(m.word(s.configuration+4u));retain_scaled_impulse();clamp_impulse(m,s.schema);
    const auto first_mass=simd(m.vector(first+0x30u));const auto impulse=broadcast(impulse_for_simd());
    const auto first_weight=multiply(impulse,first_mass),second_weight=multiply(impulse,simd(m.vector(second+0x30u)));
    const auto first_linear_delta=multiply(_mm_shuffle_ps(first_weight,first_weight,0xff),linear_axis);
    const auto second_linear_delta=multiply(_mm_shuffle_ps(second_weight,second_weight,0xff),linear_axis);
    m.vector(0x32810du,first+0x10u,words(add(simd(m.vector(first+0x10u)),first_linear_delta)));
    m.vector(0x328118u,second+0x10u,words(subtract(simd(m.vector(second+0x10u)),second_linear_delta)));
    const auto first_delta=multiply(first_weight,simd(m.vector(row+0x10u)));
    const auto second_delta=multiply(second_weight,simd(m.vector(row+0x20u)));
    m.vector(0x328131u,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_delta)));
    m.vector(0x32813cu,second+0x20u,words(add(simd(m.vector(second+0x20u)),second_delta)));
    accumulate_result(m,s.result,0x328146u);s.jacobian+=0x30u;s.schema+=0x10u;
    const auto next=m.byte(s.schema);s.result+=4u;if(next!=5u)return;
  }while(true);
}
void angular_limit_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian;
    const auto first_angular=simd(m.vector(first+0x20u)),second_angular=simd(m.vector(second+0x20u));
    const auto first_axis=simd(m.vector(row)),second_axis=simd(m.vector(row+0x10u));
    const auto second_product=multiply(second_angular,second_axis);const auto first_product=multiply(first_angular,first_axis);
    const auto dot=projection(add(first_product,second_product));begin_scalar(dot);scale_current(m.word(s.configuration+8u));
    begin_scalar(m.word(row+0x1cu));subtract_current(m.word(s.schema+4u));scale_current(m.word(s.schema+0xcu));subtract_retained_projection();
    const auto lower_difference=impulse_for_simd();bool apply{};std::uint32_t impulse_bits{};
    if(positive(compare_current(m.word(0x46a0e8u),true))){
      discard_scalar();begin_scalar(lower_difference);scale_current(m.word(row+0xcu));impulse_bits=impulse_for_simd();apply=true;
    }else{
      begin_scalar(m.word(row+0x1cu));subtract_current(m.word(s.schema+8u));scale_current(m.word(s.schema+0xcu));finish_upper_limit();
      if(negative(compare_current(m.word(0x46a0e8u)))){scale_current(m.word(row+0xcu));impulse_bits=impulse_for_simd();apply=true;}
      else discard_scalar();
    }
    if(apply){
      const auto first_mass=simd(m.vector(first+0x30u));const auto impulse=broadcast(impulse_bits);
      const auto first_weight=multiply(impulse,first_mass);const auto second_mass=simd(m.vector(second+0x30u));
      const auto first_delta=multiply(first_weight,first_axis);
      m.vector(0x327a4au,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_delta)));
      const auto second_current=simd(m.vector(second+0x20u));const auto second_weight=multiply(impulse,second_mass);
      m.vector(0x327a5eu,second+0x20u,words(add(second_current,multiply(second_weight,second_axis))));
      accumulate_result(m,s.result,0x327a63u);
    }
    const auto next=m.byte(s.schema+0x10u);s.schema+=0x10u;s.jacobian+=0x20u;s.result+=4u;if(next!=10u)return;
  }while(true);
}
void full_limit_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator;
  do{
    const auto row=s.jacobian;
    const auto second_linear=simd(m.vector(second+0x10u)),first_linear=simd(m.vector(first+0x10u));
    const auto first_axis=simd(m.vector(row+0x10u)),second_angular=simd(m.vector(second+0x20u));
    const auto linear_axis=simd(m.vector(row));const auto difference=subtract(first_linear,second_linear);
    const auto first_product=multiply(simd(m.vector(first+0x20u)),first_axis);
    const auto second_axis=simd(m.vector(row+0x20u));const auto second_product=multiply(second_angular,second_axis);
    const auto linear_product=multiply(difference,linear_axis);const auto dot=projection(add(add(first_product,second_product),linear_product));
    begin_scalar(dot);scale_current(m.word(s.configuration+8u));
    auto apply_limit=[&](bool upper){
      const auto first_mass=simd(m.vector(first+0x30u));
      const auto current_axis=upper?simd(m.vector(row)):linear_axis;const auto impulse=broadcast(impulse_for_simd());
      const auto first_weight=multiply(impulse,first_mass),second_weight=multiply(impulse,simd(m.vector(second+0x30u)));
      const auto first_linear_delta=multiply(_mm_shuffle_ps(first_weight,first_weight,0xff),current_axis);
      const auto second_linear_delta=multiply(_mm_shuffle_ps(second_weight,second_weight,0xff),current_axis);
      m.vector(upper?0x327feeu:0x327f4cu,first+0x10u,words(add(simd(m.vector(first+0x10u)),first_linear_delta)));
      m.vector(upper?0x327ff8u:0x327f56u,second+0x10u,words(subtract(simd(m.vector(second+0x10u)),second_linear_delta)));
      const auto first_delta=multiply(first_weight,simd(m.vector(row+0x10u)));
      const auto second_delta=multiply(second_weight,simd(m.vector(row+0x20u)));
      m.vector(upper?0x328010u:0x327f6eu,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_delta)));
      m.vector(upper?0x32801au:0x327f78u,second+0x20u,words(add(simd(m.vector(second+0x20u)),second_delta)));
      accumulate_result(m,s.result,upper?0x32801fu:0x327f7du);
    };
    begin_scalar(m.word(row+0xcu));subtract_current(m.word(s.schema+4u));scale_current(m.word(s.configuration+4u));subtract_retained_projection();
    if(positive(compare_current(m.word(0x46a0e8u)))){scale_current(m.word(row+0x1cu));apply_limit(false);}else discard_scalar();
    begin_scalar(m.word(row+0xcu));subtract_current(m.word(s.schema+8u));scale_current(m.word(s.configuration+4u));finish_upper_limit();
    if(negative(compare_current(m.word(0x46a0e8u)))){scale_current(m.word(row+0x1cu));apply_limit(true);}else discard_scalar();
    const auto next=m.byte(s.schema+0xcu);s.schema+=0xcu;s.jacobian+=0x30u;s.result+=4u;if(next!=11u)return;
  }while(true);
}
void radial_pair_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator,row=s.jacobian;
  begin_scalar(m.word(row+0xcu));
  const auto second_linear=simd(m.vector(second+0x10u)),first_linear=simd(m.vector(first+0x10u));
  const auto second_angular=simd(m.vector(second+0x20u)),first_axis=simd(m.vector(row+0x10u));
  const auto difference=subtract(first_linear,second_linear);const auto first_product=multiply(simd(m.vector(first+0x20u)),first_axis);
  const auto linear_axis=simd(m.vector(row));const auto linear_product=multiply(difference,linear_axis);
  const auto second_axis=simd(m.vector(row+0x20u));const auto second_product=multiply(second_angular,second_axis);
  const auto first_projection=projection(add(add(first_product,second_product),linear_product));
  const auto other_second_linear=simd(m.vector(second+0x10u)),other_first_linear=simd(m.vector(first+0x10u));
  const auto other_first_axis=simd(m.vector(row+0x40u)),other_second_axis=simd(m.vector(row+0x50u));
  const auto other_difference=subtract(other_first_linear,other_second_linear);
  const auto other_first_product=multiply(simd(m.vector(first+0x20u)),other_first_axis);
  const auto other_second_angular=simd(m.vector(second+0x20u));
  const auto other_second_product=multiply(other_second_angular,other_second_axis);const auto other_linear_axis=simd(m.vector(row+0x30u));
  const auto other_linear_product=multiply(other_difference,other_linear_axis);
  const auto second_projection=projection(add(add(other_first_product,other_second_product),other_linear_product));
  scale_current(m.word(s.configuration+0xcu));begin_scalar(first_projection);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();const auto first_residual=take_scalar();
  begin_scalar(m.word(row+0x3cu));scale_current(m.word(s.configuration+0xcu));begin_scalar(second_projection);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();const auto second_residual=take_scalar();
  begin_scalar(m.word(s.schema+0xcu));asm volatile("fld %%st(0)\n\tfmul %%st(1)":::"memory","st");const auto cap_square=take_scalar();discard_scalar();
  coupled_pair(m,row,s.schema,first_residual,second_residual);
  asm volatile("fld %%st(1)\n\tfmul %%st(2)\n\tfld %%st(1)\n\tfmul %%st(2)\n\tfaddp %%st,%%st(1)":::"memory","st");
  if(positive(compare_current(cap_square))){
    asm volatile("fdivrs %0\n\tfsqrt\n\tfld %%st(0)\n\tfmulp %%st,%%st(3)\n\tfld %%st(0)\n\tfmulp %%st,%%st(2)\n\tfld %%st(0)"::"m"(cap_square):"memory","st");
    scale_current(m.word(row+0xcu));m.word(0x327761u,row+0xcu,take_scalar());
    scale_current(m.word(row+0x3cu));m.word(0x327767u,row+0x3cu,take_scalar());
  }else discard_scalar();
  const auto first_mass=simd(m.vector(first+0x30u));copy_first_pair_impulse();const auto current_axis=simd(m.vector(row));
  const auto first_impulse=broadcast(take_scalar());exchange_scalars();const auto first_weight=multiply(first_impulse,first_mass);
  const auto second_weight=multiply(first_impulse,simd(m.vector(second+0x30u)));
  apply_full_weights(m,s,row,current_axis,first_weight,second_weight,s.result,{0x3277bau,0x3277c4u,0x3277dcu,0x3277eeu,0x3277feu});
  const auto other_first_mass=simd(m.vector(first+0x30u));const auto other_impulse=broadcast(impulse_for_simd());s.jacobian+=0x60u;
  const auto other_first_weight=multiply(other_impulse,other_first_mass);const auto other_second_mass=simd(m.vector(second+0x30u));
  const auto current_other_axis=simd(m.vector(row+0x30u));const auto other_second_weight=multiply(other_impulse,other_second_mass);
  apply_full_weights(m,s,row+0x30u,current_other_axis,other_first_weight,other_second_weight,s.result+4u,{0x327847u,0x327851u,0x32786du,0x327877u,0x327880u});
  s.result+=8u;const auto next=m.byte(s.schema+0x14u);s.schema+=0x14u;if(next==1u)s.next_opcode=1;
}
void radial_triple_rows(Access&m,PrimaryState&s){
  const auto first=s.first_accumulator,second=s.second_accumulator,row=s.jacobian;
  begin_scalar(m.word(row+0x6cu));const auto second_linear=simd(m.vector(second+0x10u));begin_scalar(m.word(row+0x7cu));
  const auto first_linear=simd(m.vector(first+0x10u)),first_axis=simd(m.vector(row+0x10u));
  const auto second_angular=simd(m.vector(second+0x20u));const auto difference=subtract(first_linear,second_linear);
  const auto first_product=multiply(simd(m.vector(first+0x20u)),first_axis);const auto linear_axis=simd(m.vector(row));
  const auto linear_product=multiply(difference,linear_axis);const auto second_axis=simd(m.vector(row+0x20u));
  const auto second_product=multiply(second_angular,second_axis);const auto first_projection=projection(add(add(first_product,second_product),linear_product));
  const auto other_second_linear=simd(m.vector(second+0x10u)),other_first_linear=simd(m.vector(first+0x10u));
  const auto other_first_axis=simd(m.vector(row+0x40u)),other_second_axis=simd(m.vector(row+0x50u));
  const auto other_difference=subtract(other_first_linear,other_second_linear);
  const auto other_first_product=multiply(simd(m.vector(first+0x20u)),other_first_axis);
  const auto other_second_product=multiply(simd(m.vector(second+0x20u)),other_second_axis);
  const auto other_linear_axis=simd(m.vector(row+0x30u));const auto other_sum=add(other_first_product,other_second_product);
  const auto other_linear_product=multiply(other_difference,other_linear_axis);const auto second_projection=projection(add(other_sum,other_linear_product));
  const auto third_first_axis=simd(m.vector(row+0x60u)),third_first_angular=simd(m.vector(first+0x20u));
  const auto third_second_angular=simd(m.vector(second+0x20u));const auto third_first_product=multiply(third_first_angular,third_first_axis);
  const auto third_second_axis=simd(m.vector(row+0x70u));const auto third_second_product=multiply(third_second_angular,third_second_axis);
  const auto third_projection=projection(add(third_first_product,third_second_product));
  scale_current(m.word(s.configuration+0xcu));begin_scalar(third_projection);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();multiply_by_effective_mass();
  const auto third_residual=take_scalar();discard_scalar();
  begin_scalar(m.word(row+0xcu));scale_current(m.word(s.configuration+0xcu));begin_scalar(first_projection);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();const auto first_residual=take_scalar();
  begin_scalar(m.word(row+0x3cu));scale_current(m.word(s.configuration+0xcu));begin_scalar(second_projection);scale_current(m.word(s.configuration+8u));
  const auto cap=m.word(s.schema+0xcu);subtract_projected_velocity();const auto second_residual=take_scalar();
  triple_coupled_pair(m,row,s.schema,first_residual,second_residual);begin_scalar(third_residual);
  // Squared norm remains extended; only the source's cap input is binary32.
  asm volatile("fld %%st(0)\n\tfmul %%st(1)\n\tfld %%st(2)\n\tfmul %%st(3)\n\tfaddp %%st,%%st(1)\n\tfld %%st(3)\n\tfmul %%st(4)\n\tfaddp %%st,%%st(1)":::"memory","st");
  begin_scalar(cap);scale_current(cap);std::uint16_t status;
  asm volatile("fld %%st(1)\n\tfcompp\n\tfnstsw %0":"=a"(status)::"memory","st");
  if(positive(status)){
    asm volatile("fsqrt":::"memory","st");const auto live_cap=m.word(s.schema+0xcu);
    asm volatile("fdivrs %0\n\tfld %%st(0)\n\tfmulp %%st,%%st(4)\n\tfld %%st(0)\n\tfmulp %%st,%%st(3)\n\tfxch %%st(1)\n\tfmul %%st(1)\n\tfxch %%st(1)\n\tfld %%st(0)"::"m"(live_cap):"memory","st");
    scale_current(m.word(row+0xcu));m.word(0x32748du,row+0xcu,take_scalar());
    asm volatile("fld %%st(0)":::"memory","st");scale_current(m.word(row+0x3cu));m.word(0x327495u,row+0x3cu,take_scalar());
    scale_current(m.word(row+0x7cu));m.word(0x32749bu,row+0x7cu,take_scalar());
  }else discard_scalar();
  scale_current(m.word(s.schema+0x14u));const auto first_mass=simd(m.vector(first+0x30u)),current_axis=simd(m.vector(row));
  const auto third_impulse=take_scalar();copy_first_pair_impulse();const auto first_impulse=broadcast(take_scalar());exchange_scalars();
  const auto first_weight=multiply(first_impulse,first_mass),second_weight=multiply(first_impulse,simd(m.vector(second+0x30u)));
  apply_full_weights(m,s,row,current_axis,first_weight,second_weight,s.result,{0x3274fcu,0x327506u,0x32751eu,0x327528u,0x327538u});
  const auto other_first_mass=simd(m.vector(first+0x30u));const auto other_impulse=broadcast(impulse_for_simd());const auto other_first_weight=multiply(other_impulse,other_first_mass);
  const auto other_second_mass=simd(m.vector(second+0x30u)),current_other_axis=simd(m.vector(row+0x30u));const auto other_second_weight=multiply(other_impulse,other_second_mass);
  apply_full_weights(m,s,row+0x30u,current_other_axis,other_first_weight,other_second_weight,s.result+4u,{0x327582u,0x32758cu,0x3275a8u,0x3275b2u,0x3275bcu});
  const auto third_first_mass=simd(m.vector(first+0x30u));begin_scalar(third_impulse);const auto third_scale=broadcast(third_impulse);
  const auto third_first_weight=multiply(third_scale,third_first_mass),third_second_weight=multiply(third_scale,simd(m.vector(second+0x30u)));
  const auto first_angular_delta=multiply(third_first_weight,simd(m.vector(row+0x60u)));
  const auto second_angular_delta=multiply(third_second_weight,simd(m.vector(row+0x70u)));
  m.vector(0x327604u,first+0x20u,words(add(simd(m.vector(first+0x20u)),first_angular_delta)));
  m.vector(0x32760eu,second+0x20u,words(add(simd(m.vector(second+0x20u)),second_angular_delta)));
  accumulate_result(m,s.result+8u,0x32761du);s.schema+=0x18u;s.jacobian+=0x80u;
  const auto next=m.byte(s.schema);s.result+=0xcu;if(next==1u)s.next_opcode=1;
}
void unilateral_pair(Access&m,PrimaryState&s,bool repeated){
  const auto first=s.first_accumulator,second=s.second_accumulator,row=s.jacobian;
  begin_scalar(m.word(row+0xcu));const auto second_linear=simd(m.vector(second+0x10u)),second_angular=simd(m.vector(second+0x20u));
  const auto first_linear=simd(m.vector(first+0x10u)),first_axis=simd(m.vector(row+0x10u)),linear_axis=simd(m.vector(row));
  const auto difference=subtract(first_linear,second_linear);const auto first_product=multiply(simd(m.vector(first+0x20u)),first_axis);
  const auto second_axis=simd(m.vector(row+0x20u));const auto second_product=multiply(second_angular,second_axis);
  const auto linear_product=multiply(difference,linear_axis);const auto first_projection=projection(add(add(first_product,second_product),linear_product));
  const auto other_second_linear=simd(m.vector(second+0x10u)),other_first_linear=simd(m.vector(first+0x10u));
  const auto other_first_axis=simd(m.vector(row+0x40u));__m128 other_second_axis,other_first_product,other_second_angular,other_difference;
  if(repeated){
    other_second_axis=simd(m.vector(row+0x50u));other_difference=subtract(other_first_linear,other_second_linear);
    other_first_product=multiply(simd(m.vector(first+0x20u)),other_first_axis);other_second_angular=simd(m.vector(second+0x20u));
  }else{
    other_second_axis=simd(m.vector(row+0x50u));other_difference=subtract(other_first_linear,other_second_linear);
    other_first_product=multiply(simd(m.vector(first+0x20u)),other_first_axis);other_second_angular=simd(m.vector(second+0x20u));
  }
  const auto other_second_product=multiply(other_second_angular,other_second_axis),other_linear_axis=simd(m.vector(row+0x30u));
  const auto other_linear_product=multiply(other_difference,other_linear_axis);const auto second_projection=projection(add(add(other_first_product,other_second_product),other_linear_product));
  scale_current(m.word(s.configuration+4u));begin_scalar(first_projection);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();const auto first_residual=take_scalar();
  begin_scalar(m.word(row+0x3cu));scale_current(m.word(s.configuration+4u));begin_scalar(second_projection);scale_current(m.word(s.configuration+8u));subtract_projected_velocity();
  // The second residual stays extended. Its coupled partner has the source
  // binary32 spill. Keep first-entry and repeated-row product order distinct.
  begin_scalar(m.word(row+0x2cu));scale_current(first_residual);copy_first_pair_impulse();scale_current(m.word(s.schema+4u));add_scalar_products();const auto first_impulse_bits=take_scalar();
  begin_scalar(m.word(row+0x5cu));
  if(repeated){multiply_by_effective_mass();begin_scalar(first_residual);scale_current(m.word(s.schema+4u));add_scalar_products();}
  else{begin_scalar(first_residual);scale_current(m.word(s.schema+4u));exchange_scalars();asm volatile("fmul %%st(2)":::"memory","st");add_scalar_products();}
  const auto second_impulse_bits=take_scalar();
  const auto first_stores=repeated?FullImpulseStores{0x326fc7u,0x326fd1u,0x326fe9u,0x326ffbu,0x32700bu}:FullImpulseStores{0x326c29u,0x326c34u,0x326c51u,0x326c5cu,0x326c76u};
  const auto second_stores=repeated?FullImpulseStores{0x327059u,0x327063u,0x32707fu,0x327089u,0x32708fu}:FullImpulseStores{0x326cbcu,0x326cc7u,0x326ce3u,0x326ceeu,0x326cf5u};
  const auto single_first_stores=repeated?FullImpulseStores{0x327222u,0x32722cu,0x327244u,0x32724eu,0x327253u}:FullImpulseStores{0x326d6du,0x326d7cu,0x326d99u,0x326da4u,0x326daau};
  const auto single_second_stores=repeated?FullImpulseStores{0x3272aau,0x3272b4u,0x3272d0u,0x3272dau,0x3272e0u}:FullImpulseStores{0x326dfbu,0x326e06u,0x326e22u,0x326e2du,0x326e34u};
  auto apply_single_first=[&]{
    const auto first_mass=simd(m.vector(first+0x30u));const auto impulse=broadcast(impulse_for_simd());
    const auto first_weight=multiply(impulse,first_mass),second_weight=multiply(impulse,simd(m.vector(second+0x30u)));
    apply_full_weights(m,s,row,linear_axis,first_weight,second_weight,s.result,single_first_stores);
  };
  auto apply_single_second=[&]{
    const auto first_mass=simd(m.vector(first+0x30u));const auto impulse=broadcast(impulse_for_simd());__m128 current_axis,first_weight,second_mass;
    if(repeated){first_weight=multiply(impulse,first_mass);second_mass=simd(m.vector(second+0x30u));current_axis=simd(m.vector(row+0x30u));}
    else{current_axis=simd(m.vector(row+0x30u));first_weight=multiply(impulse,first_mass);second_mass=simd(m.vector(second+0x30u));}
    const auto second_weight=multiply(impulse,second_mass);
    apply_full_weights(m,s,row+0x30u,current_axis,first_weight,second_weight,s.result+4u,single_second_stores);
  };
  auto isolated_first=[&]{begin_scalar(first_residual);scale_current(m.word(row+0x1cu));if(positive(compare_current(m.word(0x46a0e8u))))apply_single_first();else discard_scalar();};
  begin_scalar(first_impulse_bits);
  if(positive(compare_current(m.word(0x46a0e8u),true))){
    discard_scalar();begin_scalar(second_impulse_bits);
    if(positive(compare_current(m.word(0x46a0e8u),true))){
      const auto first_mass=simd(m.vector(first+0x30u));begin_scalar(first_impulse_bits);const auto impulse=broadcast(first_impulse_bits);
      const auto first_weight=multiply(impulse,first_mass),second_weight=multiply(impulse,simd(m.vector(second+0x30u)));
      apply_full_weights(m,s,row,linear_axis,first_weight,second_weight,s.result,first_stores);
      const auto other_first_mass=simd(m.vector(first+0x30u));begin_scalar(second_impulse_bits);const auto other_impulse=broadcast(second_impulse_bits);
      __m128 current_axis,other_first_weight,other_second_mass;
      if(repeated){other_first_weight=multiply(other_impulse,other_first_mass);other_second_mass=simd(m.vector(second+0x30u));current_axis=simd(m.vector(row+0x30u));}
      else{current_axis=simd(m.vector(row+0x30u));other_first_weight=multiply(other_impulse,other_first_mass);other_second_mass=simd(m.vector(second+0x30u));}
      const auto other_second_weight=multiply(other_impulse,other_second_mass);
      apply_full_weights(m,s,row+0x30u,current_axis,other_first_weight,other_second_weight,s.result+4u,second_stores);
    }else isolated_first();
  }else{
    scale_current(m.word(row+0x4cu));
    if(greater_or_unordered(compare_current(m.word(0x46a0e8u))))apply_single_second();
    else{discard_scalar();isolated_first();}
  }
}
void unilateral_pair_rows(Access&m,PrimaryState&s){
  unilateral_pair(m,s,false);const auto next=m.byte(s.schema+8u);s.schema+=8u;s.jacobian+=0x60u;s.result+=8u;
  if(next!=3u)return;
  unilateral_pair(m,s,true);const auto following=m.byte(s.schema+8u);s.schema+=8u;s.jacobian+=0x60u;s.result+=8u;
  if(following==8u)s.next_opcode=8;
}
#endif
}
void solve_primary_rows(Access&m,PrimaryState&s,unsigned type){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  switch(type){case 2:positive_full_rows(m,s);return;case 3:unilateral_pair_rows(m,s);return;case 4:clamped_angular_rows(m,s);return;case 5:clamped_full_rows(m,s);return;case 6:capped_full_rows(m,s);return;case 7:radial_pair_rows(m,s);return;case 8:radial_triple_rows(m,s);return;case 9:capped_angular_rows(m,s);return;case 10:angular_limit_rows(m,s);return;case 11:full_limit_rows(m,s);return;case 12:bilateral_angular_rows(m,s);return;case 13:bilateral_full_rows(m,s);return;default:throw std::runtime_error("Unknown native TU1.5 primary row family");}
#else
  (void)m;(void)s;(void)type;throw std::runtime_error("Exact native TU1.5 solver row backend unavailable");
#endif
}
} // namespace halo2::native_tu15_solver_detail
