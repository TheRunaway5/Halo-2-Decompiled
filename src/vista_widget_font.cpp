#include "halo2/vista_widget_font.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif
namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista font service");
  return f(std::forward<A>(a)...);
}
struct Owner {
  const VistaWidgetFontServices& s;std::uint32_t identity{};std::span<std::byte> bank;
  explicit Owner(const VistaWidgetFontServices& services):s(services){}
  Owner(const VistaWidgetFontServices& services,std::uint32_t id,std::span<std::byte> bytes)
      :s(services),identity(id),bank(bytes) {
    if(!id||std::uint64_t(id)+bytes.size()>0x100000000ull)throw FormatError("Invalid Vista font source bank");
  }
  std::uint32_t read(std::uint32_t at,std::size_t n=4)const {
    if(identity&&at>=identity&&std::uint64_t(at)+n<=std::uint64_t(identity)+bank.size()) {
      std::uint32_t value{};for(std::size_t i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(bank[at-identity+i])<<(8*i);return value;
    }
    return call(s.read,at,n);
  }
  void write(std::uint32_t at,std::uint32_t value,std::size_t n=4)const {
    if(identity&&at>=identity&&std::uint64_t(at)+n<=std::uint64_t(identity)+bank.size()) {
      for(std::size_t i=0;i<n;++i)bank[at-identity+i]=std::byte(value>>(8*i));
    }else call(s.write,at,value,n);
  }
  std::uint32_t get(std::uint32_t off,std::size_t n=4)const{return read(identity+off,n);}
  void put(std::uint32_t off,std::uint32_t value,std::size_t n=4)const{write(identity+off,value,n);}
  std::int32_t signed_word(std::uint32_t off)const{return std::bit_cast<std::int16_t>(std::uint16_t(get(off,2)));}
  std::pair<std::uint32_t,bool> escape(std::uint16_t c)const {
    if(read(0x812a44,1)) {
      constexpr std::array<std::pair<std::uint16_t,std::uint32_t>,6> table{{{0x7c,0x7c},{0x6c,0xe405},{0x72,0xe406},{0x63,0xe407},{0x6e,13},{0x74,9}}};
      for(const auto& [input,output]:table)if(input==c)return {output,true};
      if(c) {const auto now=call(s.clock);if(now>read(0x882d68))write(0x882d68,now+0xea60u);}
    }
    return {0x7c,false};
  }
};
#if defined(__i386__) || defined(__x86_64__)
struct Fp {
  unsigned saved_sse;std::uint16_t saved_x87;
  explicit Fp(const VistaWidgetFontServices& s):saved_sse(_mm_getcsr()) {
    asm volatile("fnstcw %0":"=m"(saved_x87));
    if((s.mxcsr&0x1f80)!=0x1f80||(s.mxcsr&0xffff0000u)||(s.x87_control&0x3f)!=0x3f||(s.x87_control&0x300)==0x100)
      throw FormatError("Unsupported Vista font FP context");
    _mm_setcsr(s.mxcsr);asm volatile("fldcw %0"::"m"(s.x87_control));
  }
  ~Fp(){asm volatile("fldcw %0"::"m"(saved_x87));_mm_setcsr(saved_sse);}
  static std::uint32_t store(std::uint32_t bits) {
    std::uint32_t output;asm volatile("flds %1; fstps %0":"=m"(output):"m"(bits):"st");return output;
  }
  static std::uint32_t scaled_integer(std::uint32_t value,std::uint32_t scale) {
    std::uint16_t original;asm volatile("fnstcw %0":"=m"(original));const auto truncate=std::uint16_t(original|0xc00);
    std::uint64_t output;asm volatile("flds %1; fmuls %2; fldcw %3; fistpq %0; fldcw %4":"=m"(output):"m"(value),"m"(scale),"m"(truncate),"m"(original):"st");return std::uint32_t(output);
  }
  static bool greater(std::uint32_t a,std::uint32_t b) {
    const auto x=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(a)));
    const auto y=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(b)));
    unsigned char result;asm volatile("comiss %2, %1; seta %0":"=qm"(result):"x"(x),"x"(y):"cc");return result!=0;
  }
};
#else
struct Fp {
  explicit Fp(const VistaWidgetFontServices&){throw FormatError("Missing exact x86 Vista font FP provider");}
  static std::uint32_t store(std::uint32_t){throw FormatError("Missing Vista x87 provider");}
  static std::uint32_t scaled_integer(std::uint32_t,std::uint32_t){throw FormatError("Missing Vista x87 provider");}
  static bool greater(std::uint32_t,std::uint32_t){throw FormatError("Missing Vista SSE provider");}
};
#endif
} //namespace
std::uint32_t vista_font_convert(std::uint32_t input,std::uint32_t output,
    std::uint32_t capacity,const VistaWidgetFontServices& s) {
  const Owner o{s};auto result=input;
  if(std::bit_cast<std::int32_t>(capacity)<=0)return result;
  do {
    if(capacity==1){o.write(output,0);return result;}
    auto next=input+2;result=o.read(input,2);
    if(result==0x7c) {const auto [value,consumed]=o.escape(std::uint16_t(o.read(next,2)));result=value;if(consumed)next+=2;}
    input=next;o.write(output,result);if(!result)return result;
    --capacity;output+=4;
  }while(std::bit_cast<std::int32_t>(capacity)>0);
  return result;
}
std::uint8_t vista_font_iterator_birth(std::uint32_t id,std::span<std::byte,0x44> bank,
    std::uint32_t vector,std::array<std::uint32_t,6> args,const VistaWidgetFontServices& s) {
  const Owner o{s,id,bank};const Fp fp{s};
  const auto width=o.read(0xe3da00);bool small=false;
  if(std::bit_cast<std::int32_t>(width)<=800)small=std::bit_cast<std::int32_t>(o.read(0xe3da04))<=600;
  for(std::uint32_t i=0;i<0x44;++i)o.put(i,0,1);
  const auto font=small?call(s.fallback_font,args[1]):args[1];
  o.put(8,args[0]);o.put(0x10,args[2],2);o.put(0x12,args[3],2);o.put(0,font);o.put(0xc,0,2);o.put(0x1c,0,1);
  o.put(0x1d,o.read(0x8cdfbc,1)&1,1);
  const auto unit=o.read(0x79c160);auto packed=Fp::scaled_integer(o.read(vector),unit);
  for(std::uint32_t i=4;i<16;i+=4)packed=(packed<<8)|Fp::scaled_integer(o.read(vector+i),unit);
  o.put(0x3c,packed);
  if(o.read(args[4],1)) {
    const auto secondary=o.read(args[5]);const auto primary=o.read(vector);
    auto other=Fp::scaled_integer(Fp::greater(secondary,primary)?primary:secondary,unit);
    for(std::uint32_t i=4;i<16;i+=4)other=(other<<8)|Fp::scaled_integer(o.read(args[5]+i),unit);
    o.put(0x40,other);
  }else o.put(0x40,0);
  o.put(0x28,8);o.put(0x2c,0);o.put(0x30,0);o.put(0x34,0);o.put(0x38,0);o.put(0xe,0,2);
  const auto definition=call(s.font_definition,font);o.put(4,definition);return definition?1u:0u;
}
std::uint32_t vista_font_iterator_next(std::uint32_t id,std::span<std::byte,0x44> bank,
    const VistaWidgetFontServices& s) {
  const Owner o{s,id,bank};const auto text=o.get(8);std::uint32_t character{},kind{};
  do {
    const auto cursor=o.get(0xc,2);const auto signed_cursor=std::bit_cast<std::int16_t>(std::uint16_t(cursor));
    character=o.read(text+std::uint32_t(std::int32_t(signed_cursor))*4);o.put(0xc,cursor+1,2);
  }while(character==10);
  if(character==0)kind=0;
  else if(character==9)kind=2;
  else if(character==13)kind=1;
  else if(character>=0xe405&&character<=0xe407){o.put(0x12,character-0xe405,2);kind=3;}
  else {
    if(o.get(0x1d,1))o.put(0x1c,call(s.pair_classify,o.get(0x14),character),1);
    else o.put(0x1c,0,1);
    kind=4;
  }
  if(o.get(0x24)==4)o.put(0x18,o.get(0x14));else o.put(0x18,0);
  const auto previous=o.signed_word(0xc)==1?kind:o.get(0x20);
  o.put(0x14,character);o.put(0x24,previous);o.put(0x20,kind);return kind;
}
std::uint8_t vista_font_prepare_dwords(std::uint32_t input,std::uint32_t font,
    std::uint32_t id,std::span<std::byte,0x44> bank,const VistaWidgetFontServices& s) {
  const Owner o{s,id,bank};const auto second=o.read(0x8cdfc4,2);const auto first=o.read(0x8cdfc0,2);
  const auto born=vista_font_iterator_birth(id,bank,0x8cdfc8,{input,font,first,second,0x8cdfd8,0x8cdfdc},s);
  bool ready=true;
  if(born)for(;;) {
    vista_font_iterator_next(id,bank,s);const auto kind=o.get(0x20);
    if(kind==4) {const auto character=o.get(0x14);if(!call(s.glyph_request,font,character))ready=false;}
    else if(!kind)break;
  }
  return ready?1u:0u;
}
std::uint8_t vista_font_prepare(std::uint32_t input,std::uint32_t font,
    std::uint32_t converted_id,std::span<std::byte,0x2000> converted,
    std::uint32_t state_id,std::span<std::byte,0x44> state,const VistaWidgetFontServices& s) {
  const Owner admission{s,converted_id,converted};
  vista_font_convert(input,converted_id,0x800,s);
  return vista_font_prepare_dwords(converted_id,font,state_id,state,s);
}
void vista_widget_measure(VistaWidgetIdentity widget,std::uint32_t rect,
    std::uint32_t prefix,std::uint32_t result,std::uint32_t flag,
    std::uint32_t scratch_id,std::span<std::byte,0x20> scratch,
    const VistaWidgetFontServices& s) {
  const Owner o{s,scratch_id,scratch};const Fp fp{s};
  const auto y=o.read(widget.value+0x5c),x=o.read(widget.value+0x58),z=o.read(widget.value+0x60);
  o.put(0x10,y);o.put(0xc,x);o.put(0x14,z);call(s.bounds,widget,scratch_id+0x18);
  const auto scale=Fp::store(o.get(0x14));const auto style=call(s.style,widget);
  call(s.style_measure,style,std::array<std::uint32_t,6>{scratch_id+0x18,scale,rect,prefix,result,flag});
}
} //namespace halo2
