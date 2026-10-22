#include "halo2/vista_widget_text_measure.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif
namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a){if(!f)throw FormatError("Missing consumed Vista text measurement service");return f(std::forward<A>(a)...);}
struct Owner {
 const VistaTextMeasureServices& s;std::uint32_t id,args;std::span<std::byte> bank;
 Owner(const VistaTextMeasureServices& services,std::uint32_t identity,std::span<std::byte> bytes,std::uint32_t arguments,std::size_t count):s(services),id(identity),args(arguments),bank(bytes){if(!id||std::uint64_t(id)+bytes.size()>0x100000000ull||!args||std::uint64_t(args)+count*4>0x100000000ull)throw FormatError("Invalid Vista text measurement bank");}
 std::uint32_t read(std::uint32_t at,std::size_t n=4)const{if(at>=id&&std::uint64_t(at)+n<=std::uint64_t(id)+bank.size()){std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(bank[at-id+i])<<(i*8);return v;}return call(s.read,at,n);}
 void write(std::uint32_t at,std::uint32_t v,std::size_t n=4)const{if(at>=id&&std::uint64_t(at)+n<=std::uint64_t(id)+bank.size()){for(std::size_t i=0;i<n;++i)bank[at-id+i]=std::byte(v>>(i*8));}else call(s.write,at,v,n);}
 std::uint32_t get(std::uint32_t off,std::size_t n=4)const{return read(id+off,n);}void put(std::uint32_t off,std::uint32_t v,std::size_t n=4)const{write(id+off,v,n);}
 std::uint32_t arg(unsigned i)const{return read(args+i*4);}std::int32_t word(std::uint32_t at)const{return std::bit_cast<std::int16_t>(std::uint16_t(read(at,2)));}
 std::uint32_t length(std::uint32_t at)const{const auto first=at;for(;;){const auto c=read(at,2);at+=2;if(!c)return std::bit_cast<std::uint32_t>(std::bit_cast<std::int32_t>(at-first-2)>>1);}}
};
#if defined(__i386__) || defined(__x86_64__)
struct Fp {
 std::uint32_t sse;std::uint16_t cw;
 explicit Fp(const VistaTextMeasureServices& s):sse(_mm_getcsr()){asm volatile("fnstcw %0":"=m"(cw));if((s.mxcsr&0x1f80)!=0x1f80||(s.mxcsr&0xffff0000u)||(s.x87_control&0x3f)!=0x3f||(s.x87_control&0x300)==0x100)throw FormatError("Unsupported Vista text measurement FP context");_mm_setcsr(s.mxcsr);asm volatile("fldcw %0"::"m"(s.x87_control));}
 ~Fp(){asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(sse);}
 static __m128 load(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
 static std::uint32_t bits(__m128 v){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
 static std::uint32_t integer(std::int32_t v){return bits(_mm_cvtsi32_ss(_mm_setzero_ps(),v));}
 static std::uint32_t trunc(std::uint32_t v){return std::bit_cast<std::uint32_t>(_mm_cvttss_si32(load(v)));}
 static std::uint32_t sub(std::uint32_t a,std::uint32_t b){return bits(_mm_sub_ss(load(a),load(b)));}
 static std::uint32_t add(std::uint32_t a,std::uint32_t b){return bits(_mm_add_ss(load(a),load(b)));}
 static std::uint32_t mul(std::uint32_t a,std::uint32_t b){return bits(_mm_mul_ss(load(a),load(b)));}
 static std::uint32_t div(std::uint32_t a,std::uint32_t b){return bits(_mm_div_ss(load(a),load(b)));}
 static std::uint32_t store(std::uint32_t v){std::uint32_t r;asm volatile("flds %1;fstps %0":"=m"(r):"m"(v):"st");return r;}
 static bool greater(std::uint32_t a,std::uint32_t b){const auto x=load(a),y=load(b);unsigned char r;asm volatile("comiss %2,%1;seta %0":"=qm"(r):"x"(x),"x"(y):"cc");return r!=0;}
 static bool equal(std::uint32_t a,std::uint32_t b){const auto x=load(a),y=load(b);unsigned char z,p;asm volatile("ucomiss %3,%2;sete %0;setp %1":"=qm"(z),"=qm"(p):"x"(x),"x"(y):"cc");return z&&!p;}
};
#else
struct Fp{explicit Fp(const VistaTextMeasureServices&){throw FormatError("Missing x86 Vista text measurement provider");}static std::uint32_t integer(std::int32_t){throw FormatError("Missing source SSE");}static std::uint32_t trunc(std::uint32_t){throw FormatError("Missing source SSE");}static std::uint32_t sub(std::uint32_t,std::uint32_t){throw FormatError("Missing source SSE");}static std::uint32_t add(std::uint32_t,std::uint32_t){throw FormatError("Missing source SSE");}static std::uint32_t mul(std::uint32_t,std::uint32_t){throw FormatError("Missing source SSE");}static std::uint32_t div(std::uint32_t,std::uint32_t){throw FormatError("Missing source SSE");}static std::uint32_t store(std::uint32_t){throw FormatError("Missing source x87");}static bool greater(std::uint32_t,std::uint32_t){throw FormatError("Missing source SSE");}static bool equal(std::uint32_t,std::uint32_t){throw FormatError("Missing source SSE");}};
#endif
} //namespace
void vista_text_style_measure(std::uint32_t style,std::uint32_t arguments,
 std::uint32_t id,std::span<std::byte,0x28> scratch,const VistaTextMeasureServices& s){
 const Owner o{s,id,scratch,arguments,6};const Fp fp{s};const auto flag=o.arg(5)&255;const auto rect=o.arg(2),bounds=o.arg(0),result=o.arg(4);o.put(0x14,rect);
 // EBX retains the first rectangle across the first text getter; later calls
 // consume the retained local pointer again before invoking their text getter.
 auto measure=[&](std::uint32_t prefix,std::uint32_t output,bool initial,std::uint32_t text_offset=0){
  const auto z=Fp::store(o.arg(1));const auto f=o.read(style+0x16,2),b=o.read(style+0x1c);const auto scale=Fp::store(o.read(style+0x20));const auto a=o.read(style+0x18),font=o.read(style+4),live_rect=initial?rect:o.get(0x14);const auto text=call(s.text,style)+text_offset;
  call(s.core,std::array<std::uint32_t,12>{text,font,a,b,f,scale,bounds,z,live_rect,prefix,result,output});
 };
 if(!flag){measure(o.arg(3),id+0x20,true);return;}
 const auto first=call(s.text,style);const auto length=o.length(first);measure(length,id+0x20,true);
 o.put(0x10,0);const auto third=call(s.text,style);if(o.length(third))o.put(0x10,Fp::integer(o.word(id+0x26)));
 const auto offset=std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(o.arg(3))))*2;
 const auto fourth=call(s.text,style)+offset;const auto suffix=o.length(fourth);measure(suffix,id+0x18,false,offset);
 const auto left=o.word(id+0x1a),right=o.word(id+0x1e);const auto width=Fp::integer(right-left);o.put(0x18,width);if(Fp::greater(0,width))o.put(0x18,0);
 measure(o.arg(3),id+0x20,false);
 const auto last_right=o.word(result+6),last_left=o.word(result+2);const auto right_bits=o.get(0x10);const auto extent=Fp::integer(last_right-last_left);
 if(!Fp::equal(right_bits,o.read(0x79de6c))){auto x=Fp::sub(right_bits,o.get(0x18));x=Fp::sub(x,extent);const auto left_word=Fp::trunc(x);o.write(result+2,left_word,2);x=Fp::integer(std::bit_cast<std::int16_t>(std::uint16_t(left_word)));x=Fp::add(x,extent);o.write(result+6,Fp::trunc(x),2);}
}
void vista_text_core_measure(std::uint32_t arguments,std::uint32_t id,
 std::span<std::byte,0x83c> scratch,const VistaTextMeasureServices& s){
 const Owner o{s,id,scratch,arguments,12};const Fp fp{s};const auto input=o.arg(0),output=o.arg(11),layout_output=o.arg(10),viewport=o.arg(8),bounds=o.arg(6);
 if(input&&o.read(input,2))call(s.copy_wide,id+0x3c,input,0x400u);else o.put(0x3c,0,2);
 const auto prefix=std::bit_cast<std::int16_t>(std::uint16_t(o.arg(9)));o.write(id+0x3c+std::uint32_t(std::int32_t(prefix))*2,0,2);o.write(0xd77374,0);
 const auto rewrite=call(s.rewrite,id+0x3c,0x400u,0u);o.put(0x28,rewrite,1);if(rewrite)call(s.rewrite,id+0x3c,0x400u,o.get(0x28));
 const auto font=o.arg(1);call(s.font_definition,font);o.write(0x8cdfb8,font);
 const auto flags=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(o.arg(4)))));const auto b=o.arg(3),a=o.arg(2);o.write(0x8cdfc0,a);o.write(0x8cdfc4,b);o.write(0x8cdfbc,flags);o.write(0x8ce016,0,1);
 const auto x=o.word(bounds+2),y=o.word(bounds),right=o.word(bounds+6),bottom=o.word(bounds+4);
 o.put(0x1c,Fp::integer(x));o.put(0x20,Fp::integer(y));o.put(0x10,Fp::integer(right));const auto z=o.arg(7);o.put(0x24,z);o.put(0x14,Fp::integer(bottom));o.put(0x18,z);
 call(s.project_point,id+0x1c,viewport);call(s.project_point,id+0x10,viewport);
 const auto transformed_right=o.get(0x10);const auto top_word=Fp::trunc(o.get(0x20));const auto transformed_left=o.get(0x1c);const auto right_word=Fp::trunc(transformed_right);o.put(0x2c,top_word,2);
 const auto raw_right=o.word(bounds+6);o.put(0x32,right_word,2);const auto raw_width=raw_right-o.word(bounds+2);const auto left_word=Fp::trunc(transformed_left);auto scalar=Fp::sub(transformed_right,transformed_left);scalar=Fp::mul(scalar,o.arg(5));o.put(0x2e,left_word,2);const auto bottom_word=Fp::trunc(o.get(0x14));scalar=Fp::div(scalar,Fp::integer(raw_width));o.put(0x30,bottom_word,2);
 call(s.layout,std::array<std::uint32_t,5>{id+0x2c,id+0x3c,id+0x34,layout_output,scalar});
 const auto new_left=o.get(0x2e,2),new_top=o.get(0x2c,2),new_right=o.get(0x3a,2);o.write(output+2,new_left,2);const auto new_bottom=o.get(0x38,2);o.write(output+4,new_bottom,2);o.write(output,new_top,2);o.write(output+6,new_right,2);
}
} //namespace halo2
