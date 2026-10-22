#include "halo2/vista_widget_bitmap_update.hpp"
#include "halo2/cache.hpp"
#include <bit>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
struct Owner {
  const VistaWidgetBitmapUpdateServices& s;
  std::uint32_t r(std::uint32_t at,std::size_t n=4)const{return s.read(at,n);}
  void w(std::uint32_t at,std::uint32_t v,std::size_t n=4)const{s.write(at,v,n);}
  std::int32_t word(std::uint32_t at)const{return std::bit_cast<std::int16_t>(std::uint16_t(r(at,2)));}
};
void registry(const VistaWidgetBitmapUpdateServices&s) {
  if(!s.read||!s.write)throw FormatError("Vista bitmap update requires its resident source32 registry");
}
template<class F> const F& required(const F& f,const char*name) {
  if(!f)throw FormatError(name);
  return f;
}
#if defined(__i386__) || defined(__x86_64__)
__m128 value(std::uint32_t raw){return _mm_set_ss(std::bit_cast<float>(raw));}
std::uint32_t bits(__m128 x){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(x));}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1, %0":"+x"(a):"x"(b));return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1, %0":"+x"(a):"x"(b));return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1, %0":"+x"(a):"x"(b));return a;}
__m128 div(__m128 a,__m128 b){asm volatile("divss %1, %0":"+x"(a):"x"(b));return a;}
bool below(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2, %1; setb %0":"=qm"(out):"x"(a),"x"(b):"cc");return out!=0;}
bool above(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2, %1; seta %0":"=qm"(out):"x"(a),"x"(b):"cc");return out!=0;}
void fp_admit() {
  std::uint16_t cw{};asm volatile("fnstcw %0":"=m"(cw));
  if((_mm_getcsr()&0x1f80u)!=0x1f80u || (cw&0x3fu)!=0x3fu)
    throw FormatError("Unmasked Vista bitmap SSE/x87 exceptions are not admitted");
}
std::uint32_t period_integer(std::uint32_t raw) {
  std::uint16_t old{},truncate{};std::int64_t result{};
  asm volatile("flds %[raw]; fnstcw %[old]":[old]"=m"(old):[raw]"m"(raw):"st");
  truncate=std::uint16_t(old|0xc00u);
  asm volatile("fldcw %[truncate]; fistpq %[result]; fldcw %[old]"
    :[result]"=m"(result):[truncate]"m"(truncate),[old]"m"(old):"st");
  return std::uint32_t(std::bit_cast<std::uint64_t>(result));
}
std::uint32_t delta_seconds(std::uint32_t elapsed,const Owner&o) {
  const auto signed_elapsed=std::bit_cast<std::int32_t>(elapsed);
  std::uint32_t out{};
  asm volatile("fildl %0"::"m"(signed_elapsed):"st");
  if(signed_elapsed<0){const auto correction=o.r(0x79ca10u);asm volatile("fadds %0"::"m"(correction):"st");}
  const auto factor=o.r(0x79de48u);
  asm volatile("fmuls %1; fstps %0":"=m"(out):"m"(factor):"st");return out;
}
std::uint32_t fill_integer(__m128 fill,std::int32_t width) {
  auto product=_mm_cvtsi32_ss(_mm_setzero_ps(),width);product=mul(product,fill);
  const auto raw=bits(product);std::uint32_t out{};
  asm volatile("flds %1; fistpl %0":"=m"(out):"m"(raw):"st");return out;
}
void wrap(std::uint32_t at,__m128 one,bool positive,const Owner&o) {
  for(;;) {
    const auto old=o.r(at);const auto next=positive?sub(value(old),one):add(value(old),one);
    const bool again=positive?above(next,one):above(_mm_setzero_ps(),next);
    const auto raw=bits(next);o.w(at,raw);
    if(!again)return;
    if(raw==old)throw FormatError("Original Vista bitmap UV loop does not progress");
  }
}
#endif
}

std::uint32_t vista_widget_bitmap_frame(std::uint32_t widget,const VistaWidgetBitmapUpdateServices&s) {
  registry(s);const Owner o{s};const auto override=o.r(widget+0x94);if(override)return override;
  const auto definition=o.r(widget+0x70);const auto datum=o.r(definition+0x1c);
  const auto table=o.r(0x87cd50u);const auto index=o.word(widget+0x90);
  auto metadata=o.r(table+((datum&0xffffu)<<4)+8);metadata+=o.r(0x87cd54u);
  if(index>=0 && index<std::bit_cast<std::int32_t>(o.r(metadata+0x44))) {
    const auto offset=o.r(metadata+0x48);const auto base=offset==0xffffffffu?0u:o.r(0x882290u)+offset;
    return base+std::uint32_t(index)*0x74u;
  }
  const auto offset=o.r(metadata+0x48);return offset==0xffffffffu?0u:o.r(0x882290u)+offset;
}
std::uint32_t vista_widget_bitmap_resource(std::uint32_t frame,std::uint32_t flags,
    std::uint32_t out,const VistaWidgetBitmapUpdateServices&s) {
  registry(s);const Owner o{s};if(out)o.w(out,0,1);
  if(o.r(frame+0x50)==0) {
    if(o.r(frame+0x54)==0) {
      const auto old=o.r(frame+9,1);if((old&2u)&&!(old&4u)&&out)o.w(out,1,1);
      const auto descriptor=o.r(frame+0x34);
      const auto size=required(s.resource_size,"Vista bitmap requires actual4647DA")(descriptor);
      const auto allocation=required(s.allocate,"Vista bitmap requires actual687C86")(size);o.w(frame+0x54,allocation);
      const auto current=o.r(frame+9,1);
      if(!(current&2u)) {
        o.w(frame+9,current|2u,1);const auto index=o.r(0xe49248u);o.w(0xe3f608u+index*4u,frame);
        o.w(0xe49248u,o.r(0xe49248u)+1u);
        o.w(frame+0x64,0xffffffffu);o.w(frame+0x68,0xffffffffu);
        o.w(frame+0x5c,0xffffffffu);o.w(frame+0x60,0xffffffffu);
        const auto state=required(s.allocate,"Vista bitmap requires actual687C86")(8);o.w(frame+0x70,state);
      }
      const auto state=o.r(frame+0x70);o.w(state,0);o.w(state+4,0);
    }
    if((flags&2u)&&!(flags&1u)) {
      const auto state=o.r(frame+0x70);
      if(o.r(state+4)==0) {
        const auto data=o.r(frame+0x54);const auto descriptor=o.r(frame+0x34);
        const auto size=required(s.resource_size,"Vista bitmap requires actual4647DA")(descriptor);
        const auto offset=o.r(frame+0x1c);const auto file=o.r(frame+0x4c);
        const auto token=required(s.read_async,"Vista bitmap requires actual464E6F")({file,offset,size,data,state,3,0});
        const auto fresh=o.r(frame+0x70);o.w(fresh+4,token);
      }
    } else {
      const auto state=o.r(frame+0x70);const auto token=o.r(state+4);
      if(!token) {
        const auto data=o.r(frame+0x54);const auto descriptor=o.r(frame+0x34);
        const auto size=required(s.resource_size,"Vista bitmap requires actual4647DA")(descriptor);
        const auto offset=o.r(frame+0x1c);const auto file=o.r(frame+0x4c);
        required(s.read_sync,"Vista bitmap requires actual464D01")({file,offset,size,data});
        const auto fresh=o.r(frame+0x70);o.w(fresh,1,1);
      } else {
        required(s.reprioritize,"Vista bitmap requires actual432BE3")(token,7);
        const auto fresh=o.r(frame+0x70);required(s.wait,"Vista bitmap requires actual432CAE")(fresh,1,2);
      }
    }
    const auto state=o.r(frame+0x70);
    if(o.r(state,1)) {
      const auto result=required(s.prepare,"Vista bitmap requires actual503A2F")(frame,0);
      if(result){if(out)o.w(out,1,1);}
      else {
        required(s.finalize,"Vista bitmap requires actual675010")(frame);
        if(o.r(frame+0x17,1)!=3) {
          const auto data=o.r(frame+0x54);required(s.free,"Vista bitmap requires actual687C23")(data);o.w(frame+0x54,0);
        }
        const auto fresh=o.r(frame+0x70);o.w(fresh,0);o.w(fresh+4,0);
      }
    }
  }
  const auto handle=o.r(frame+0x50);if(!handle)return 0;
  const auto low=o.r(frame+0x64);const auto high=o.r(frame+0x68);
  bool stale=(low&high)==0xffffffffu;
  if(!stale) {
    const auto current_low=o.r(0xe3e430u);const auto difference=low-current_low;
    const auto current_high=o.r(0xe3e434u);const auto upper=high-current_high-std::uint32_t(low<current_low);
    // Source tests JS then JG directly from SBB, so signed overflow is live.
    // A mathematical signed-64 comparison would change its overflow corridor.
    const bool sign=(upper&0x80000000u)!=0;
    const bool overflow=((high^current_high)&(high^upper)&0x80000000u)!=0;
    stale=!sign && ((upper!=0&&!overflow)||difference>60u);
  }
  if(stale&&(flags&2u))required(s.touch,"Vista bitmap requires actual65EFC2")(frame);
  return handle;
}
std::uint32_t vista_widget_bitmap_resource_request(std::uint32_t frame,
    std::uint32_t z,const VistaWidgetBitmapUpdateServices&s) {
  registry(s);
#if defined(__i386__) || defined(__x86_64__)
  fp_admit();std::uint32_t passed{};asm volatile("flds %1; fstps %0":"=m"(passed):"m"(z):"st");(void)passed;
  return vista_widget_bitmap_resource(frame,3,0,s);
#else
  (void)frame;(void)z;throw FormatError("Original Vista bitmap x87 backend unavailable");
#endif
}
void vista_widget_bitmap_update(std::uint32_t widget,const VistaWidgetBitmapUpdateServices&s) {
  registry(s);
#if defined(__i386__) || defined(__x86_64__)
  fp_admit();const Owner o{s};const auto clock=o.r(0xd71900u);
  required(s.ambient_update,"Vista bitmap requires actual612CD8")(widget);
  const auto definition=o.r(widget+0x70);
  if(definition) {
    const auto fps=value(o.r(definition+0x24));const bool small=below(fps,value(o.r(0x79de48u)));
    const auto period=small?0u:bits(div(value(o.r(0x79de40u)),fps));
    const auto previous=o.r(widget+0x78);const auto elapsed=clock-o.r(widget+0x74);
    const auto interval=period_integer(period);const auto dt=previous?delta_seconds(clock-previous,o):0u;
    const auto flags=o.r(definition,1);auto one=value(o.r(0x79be18u));
    if(flags&4u) {
      auto fill=value(o.r(widget+0x8c));
      if(above(_mm_setzero_ps(),fill))fill=_mm_setzero_ps();else if(above(fill,one))fill=one;
      const auto bottom=o.word(definition+0x28);const auto top=o.word(definition+0xc);
      const auto amount=fill_integer(fill,bottom-top);const auto x=o.r(widget+0x22,2);o.w(widget+0x26,std::uint16_t(x+amount),2);
    } else {
      const auto datum=o.r(definition+0x1c);
      if(datum!=0xffffffffu) {
        if(interval) {
          const auto table=o.r(0x87cd50u);const auto metadata=o.r(table+((datum&0xffffu)<<4)+8);
          const auto quotient=elapsed/interval;const auto base=o.r(0x87cd54u);const auto count=o.r(metadata+base+0x44);
          if(!count)throw FormatError("Original Vista bitmap frame DIV faults on zero count");
          o.w(widget+0x90,std::uint16_t(quotient%count),2);
        }
        const auto frame=vista_widget_bitmap_frame(widget,s);const auto width=o.r(frame+4,2);const auto x=o.r(widget+0x22,2);
        const auto y=o.r(widget+0x20,2);o.w(widget+0x26,std::uint16_t(width+x),2);
        const auto height=o.r(frame+6,2);o.w(widget+0x24,std::uint16_t(y-height),2);
        vista_widget_bitmap_resource_request(frame,0,s);one=value(o.r(0x79be18u));
      }
    }
    const auto fresh=o.r(widget+0x70);auto u=mul(value(o.r(fresh+0x10)),value(dt));u=add(u,value(o.r(widget+0x7c)));o.w(widget+0x7c,bits(u));
    auto v=mul(value(o.r(fresh+0x14)),value(dt));const auto stored_u=value(o.r(widget+0x7c));const bool u_above=above(stored_u,one);
    v=add(v,value(o.r(widget+0x80)));o.w(widget+0x80,bits(v));
    if(u_above)wrap(widget+0x7c,one,true,o);
    if(above(value(o.r(widget+0x80)),one))wrap(widget+0x80,one,true,o);
    if(above(_mm_setzero_ps(),value(o.r(widget+0x7c))))wrap(widget+0x7c,one,false,o);
    if(above(_mm_setzero_ps(),value(o.r(widget+0x80))))wrap(widget+0x80,one,false,o);
  }
  o.w(widget+0x78,clock);
#else
  (void)widget;throw FormatError("Original Vista bitmap SSE/x87 backend unavailable");
#endif
}
} // namespace halo2
