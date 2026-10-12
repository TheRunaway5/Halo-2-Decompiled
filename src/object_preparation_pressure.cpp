#include "halo2/object_preparation_pressure.hpp"
#include <algorithm>
#include <bit>
#include <initializer_list>
#include <utility>
#include <limits>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 { namespace {
constexpr auto none=0xffffffffu;
std::uint32_t add(std::uint32_t p,std::uint32_t n){if(n>none-p)throw std::out_of_range("object preparation address overflow");return p+n;}
struct Memory {
 const SourceObjectPreparationPressureServices &v;ObjectConstructionFloatingPointContext &fp;const ObjectConstructionObserver &observer;
 std::uint32_t read(std::uint32_t a,unsigned n=4)const{if(!v.read)throw std::runtime_error("object preparation read provider unavailable");auto b=v.read(a,n);if(b.size()!=n)throw std::out_of_range("object preparation read extent");std::uint32_t x=0;for(unsigned i=0;i<n;++i)x|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return x;}
 void write(std::uint32_t pc,std::uint32_t a,std::uint32_t x,unsigned n=4)const{if(!v.memory)throw std::runtime_error("object preparation write provider unavailable");auto b=v.memory(a,n);if(b.size()!=n)throw std::out_of_range("object preparation write extent");std::uint32_t before=0;for(unsigned i=0;i<n;++i){before|=std::to_integer<std::uint32_t>(b[i])<<(8*i);b[i]=std::byte(x>>(8*i));}if(observer)observer({pc,a,n,before,x&(n==4?none:((1u<<(8*n))-1))});}
 std::uint32_t call(std::uint32_t pc,std::uint32_t entry,
     std::initializer_list<std::pair<std::uint32_t,std::uint32_t>>args={},
     bool ecx_input=false,std::uint32_t ecx=0,bool al_input=false,std::uint8_t al=0)const {
  if(!v.invoke)throw std::runtime_error("object pressure lower owner unavailable");
  SourceObjectPressureCall c{};c.instruction=pc;c.entry=entry;c.ecx_input=ecx_input;c.ecx=ecx;c.al_input=al_input;c.al=al;
  for(auto [value,mask]:args){if(c.count==c.arguments.size())throw std::logic_error("object pressure argument extent");c.arguments[c.count]=value;c.argument_masks[c.count++]=mask;}return v.invoke(c,fp);
 }
};
#if defined(__i386__) || defined(__x86_64__)
struct Sse {
 ObjectConstructionFloatingPointContext &fp;unsigned saved;
 explicit Sse(ObjectConstructionFloatingPointContext &f):fp(f),saved(_mm_getcsr()) {
  if((fp.mxcsr&0x1f80)!=0x1f80||(fp.mxcsr&0xffff0000))throw std::runtime_error("source preparation unmasked SSE exception delivery unavailable");
  _mm_setcsr(fp.mxcsr);
 }
 ~Sse(){fp.mxcsr=_mm_getcsr();_mm_setcsr(saved);}
};
std::uint32_t scalar(ObjectConstructionFloatingPointContext &fp,std::uint32_t a,std::uint32_t b,bool sum){Sse scope(fp);std::uint32_t out;
 if(sum)asm volatile("movd %1,%%xmm1\n\tmovd %2,%%xmm0\n\taddss %%xmm0,%%xmm1\n\tmovd %%xmm1,%0":"=r"(out):"r"(a),"r"(b):"xmm0","xmm1");
 else asm volatile("movd %1,%%xmm1\n\tmovd %2,%%xmm0\n\tmulss %%xmm0,%%xmm1\n\tmovd %%xmm1,%0":"=r"(out):"r"(a),"r"(b):"xmm0","xmm1");
 return out;}
std::uint32_t convert(ObjectConstructionFloatingPointContext &fp,std::uint32_t word){Sse scope(fp);std::uint32_t out;auto value=std::bit_cast<std::int32_t>(word);asm volatile("cvtsi2ss %1,%%xmm0\n\tmovd %%xmm0,%0":"=r"(out):"r"(value):"xmm0");return out;}
bool greater(ObjectConstructionFloatingPointContext &fp,std::uint32_t a,std::uint32_t b){Sse scope(fp);unsigned char out;asm volatile("movd %1,%%xmm1\n\tmovd %2,%%xmm0\n\t.byte 0x0f,0x2f,0xc8\n\tseta %0":"=qm"(out):"r"(a),"r"(b):"xmm0","xmm1","cc");return out;}
#else
std::uint32_t scalar(ObjectConstructionFloatingPointContext&,std::uint32_t,std::uint32_t,bool){throw std::runtime_error("source preparation SSE backend unavailable");}
std::uint32_t convert(ObjectConstructionFloatingPointContext&,std::uint32_t){throw std::runtime_error("source preparation SSE backend unavailable");}
bool greater(ObjectConstructionFloatingPointContext&,std::uint32_t,std::uint32_t){throw std::runtime_error("source preparation SSE backend unavailable");}
#endif
}
void prepare_source_biped_request(const SourceObjectPreparationState &s,std::uint32_t q,
    const SourceObjectPreparationPressureServices &v,ObjectConstructionFloatingPointContext &fp,
    const ObjectConstructionObserver &observer){Memory m{v,fp,observer};auto definition=m.read(q);auto p=m.read(add(m.read(s.tag_table_slot),(definition&65535)*16+8));
 if(!(m.read(add(p,0x264),1)&1))return;
 if((m.read(add(p,0x264))>>4)&1)return;
 auto scale=m.read(add(p,0x270));constexpr std::array<std::uint32_t,3>pcs{0xddb06,0xddb19,0xddb2c};
 for(unsigned i=0;i<3;++i){auto product=scalar(fp,m.read(add(q,0x34+i*4)),scale,false);auto value=scalar(fp,product,m.read(add(q,0x1c+i*4)),true);m.write(pcs[i],add(q,0x1c+i*4),value);}}
std::uint32_t source_object_pressure_statistics(std::uint32_t pool,
    const SourceObjectPressureOperands &c,const SourceObjectPreparationPressureServices &v,
    ObjectConstructionFloatingPointContext &fp){Memory m{v,fp,{}};std::uint32_t sum=0;auto p=add(pool,0xf0384);
 for(unsigned i=0;i<16;++i){auto left=m.read(p),right=m.read(add(p,0x44));if(p<0xf02dc)throw std::out_of_range("source statistic address underflow");auto scale=m.read(p-0xf02dc);sum+=(left-right)*scale;p-=4;}
 return scalar(fp,convert(fp,sum),c.statistic_usage_scale_bits,false);}
std::uint32_t source_object_heap_pressure(const SourceHeapPressureState &s,
    const SourceObjectPressureOperands &c,const SourceObjectPreparationPressureServices &v,
    ObjectConstructionFloatingPointContext &fp){Memory m{v,fp,{}};auto second=m.read(s.second_overflow_slot);auto a=second&&std::bit_cast<std::int32_t>(m.read(add(second,0x20)))>0?3u:0u;
 auto first=m.read(s.first_overflow_slot);auto b=first&&std::bit_cast<std::int32_t>(m.read(add(first,0x20)))>0?3u:0u;auto pool=m.read(s.main_pool_slot);std::uint32_t level=0;
 if(pool){auto usage=scalar(fp,convert(fp,m.read(add(pool,0xf0340))),c.main_usage_scale_bits,false);level=3;
  if(greater(fp,c.top_limit_bits,usage)){usage=source_object_pressure_statistics(pool,c,v,fp);
   if(greater(fp,c.high_limit_bits,usage)){level=2;if(greater(fp,c.medium_limit_bits,usage)){level=1;if(greater(fp,c.low_limit_bits,usage))level=0;}}}}
 return std::max(std::max(level,a),b);}
void collect_source_object_memory(const SourceObjectPressureState &s,
    const SourceObjectPreparationPressureServices &v,ObjectConstructionFloatingPointContext &fp,
    const ObjectConstructionObserver &observer){Memory m{v,fp,observer};bool had=m.read(s.first_overflow_slot)!=0;m.call(0x14851c,0x1c6100);m.write(0x148521,s.collecting_byte,1,1);m.call(0x148528,0x1c5ff0);m.write(0x14852f,s.collecting_byte,0,1);if(had&&!m.read(s.first_overflow_slot))m.call(0x148542,0x1482f0);}
void make_source_object_memory_available(const SourceObjectPressureState &s,
    const SourceObjectPreparationPressureServices &v,ObjectConstructionFloatingPointContext &fp,
    const ObjectConstructionObserver &observer){Memory m{v,fp,observer};if(m.read(s.busy_byte,1))return;
 auto pressure=m.call(0x148374,0x148740);auto last=m.read(s.last_collection_tick);m.write(0x148381,s.busy_byte,1,1);
 for(unsigned outer=0;outer<3;++outer){if(std::bit_cast<std::int32_t>(pressure)<2)break;std::uint32_t total=0;
  if(std::bit_cast<std::int32_t>(pressure)>=3){m.call(0x1483a8,0x1c7ec0,{{1,none}});last=m.read(s.last_collection_tick);}
  pressure=m.call(0x1483b3,0x148740);
  for(unsigned inner=0;inner<3;++inner){if(std::bit_cast<std::int32_t>(pressure)<2)break;unsigned attempt=0;
   for(;;){bool recent=false;if(last!=none)recent=std::bit_cast<std::int32_t>(m.read(add(m.read(s.clock_slot),8))-last)<10;
    auto result=m.call(0x148418,0x1c7970,{{inner>1?1u:0u,255},{recent?1u:0u,255},{none,none}},true,outer,true,inner==2?1:0);
    if(!(result&255)){if(outer==0&&!recent){m.call(0x1484b3,0x148510);pressure=m.call(0x1484b8,0x148740);last=m.read(add(m.read(s.clock_slot),8));m.write(0x1484c7,s.last_collection_tick,last);}else last=m.read(s.last_collection_tick);break;}
    pressure=m.call(0x148425,0x148740);if(std::bit_cast<std::int32_t>(pressure)<2){last=m.read(s.last_collection_tick);break;}
    ++attempt;++total;
    // Source's signed remainder-normalization is zero exactly at multiples8.
    if((total&7)==0){bool had=m.read(s.first_overflow_slot)!=0;m.call(0x148458,0x1c6100);m.write(0x14845d,s.collecting_byte,1,1);m.call(0x148464,0x1c5ff0);m.write(0x14846b,s.collecting_byte,0,1);if(had&&!m.read(s.first_overflow_slot))m.call(0x14847d,0x1482f0);pressure=m.call(0x148482,0x148740);last=m.read(add(m.read(s.clock_slot),8));m.write(0x148492,s.last_collection_tick,last);}
    else last=m.read(s.last_collection_tick);
    if(attempt>50)break;
   }
  }
 }
 m.write(0x1484fc,s.busy_byte,0,1);
}
} // namespace halo2
