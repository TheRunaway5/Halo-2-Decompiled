#include "halo2/vista_widget_header_footer.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... args) {
  if(!f)throw FormatError("Missing consumed Vista header/footer service");
  return f(std::forward<A>(args)...);
}
struct Owner {
  std::uint32_t source;
  std::span<std::byte> scratch;
  const VistaWidgetHeaderFooterServices& s;
  Owner(std::uint32_t id,std::span<std::byte> bytes,const VistaWidgetHeaderFooterServices& services)
      :source(id),scratch(bytes),s(services) {
    if(!id||std::uint64_t(id)+bytes.size()>0x100000000ull)
      throw FormatError("Invalid Vista header/footer scratch source extent");
  }
  std::uint32_t get(std::uint32_t at,std::size_t size=4) const {
    if(at>=source&&std::uint64_t(at)+size<=std::uint64_t(source)+scratch.size()) {
      std::uint32_t value{};for(std::size_t i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(scratch[at-source+i])<<(i*8);return value;
    }
    return call(s.read,at,size);
  }
  void put(std::uint32_t at,std::uint32_t value,std::size_t size=4) const {
    if(at>=source&&std::uint64_t(at)+size<=std::uint64_t(source)+scratch.size()) {
      for(std::size_t i=0;i<size;++i)scratch[at-source+i]=std::byte(value>>(i*8));
    }else call(s.write,at,value,size);
  }
  std::uint32_t local(std::uint32_t off)const{return source+off;}
  std::uint32_t lget(std::uint32_t off,std::size_t size=4)const{return get(local(off),size);}
  void lput(std::uint32_t off,std::uint32_t value,std::size_t size=4)const{put(local(off),value,size);}
  std::int32_t signed_word(std::uint32_t at)const{return std::bit_cast<std::int16_t>(std::uint16_t(get(at,2)));}
  std::uint32_t relocated(std::uint32_t value)const{return value==0xffffffffu?0u:get(0x882290)+value;}
  std::uint32_t lookup(VistaWidgetIdentity screen)const {
    const auto datum=call(s.definition,get(screen.value+0x70));
    return datum==0xffffffffu?0u:call(s.tag,datum);
  }
  std::uint32_t animation(std::uint32_t tag)const {
    if(!tag||std::bit_cast<std::int32_t>(get(tag+0x20))<=0)return 0;
    return get(relocated(get(tag+0x24))+2,2);
  }
  void bounds_copy(VistaWidgetIdentity widget,std::uint32_t at)const {
    put(widget.value+0x20,get(at));put(widget.value+0x24,get(at+4));
  }
  void color(std::uint32_t target,std::uint32_t source_at)const {
    for(std::uint32_t i=0;i<16;i+=4)put(target+i,get(source_at+i));
  }
  void zero(std::uint32_t at,std::uint32_t count)const {
    // Exact memset byte-store result; no callback supplies substitute bytes.
    for(std::uint32_t i=0;i<count;++i)put(at+i,0,1);
  }
  std::uint32_t invalid(std::uint32_t error)const {
    call(s.failure.store_errno,error);call(s.failure.invalid_parameter,std::array<std::uint32_t,5>{});return error;
  }
  std::uint32_t copy(std::uint32_t dest,std::uint32_t input,std::uint32_t capacity,std::uint32_t count=0xffffffffu)const {
    // Complete687AB5 branches reached by44BD27 and610B4E's bounded copy.
    if(!count&&!dest&&!capacity)return 0;
    if(!dest||!capacity)return invalid(0x16);
    if(!count){put(dest,0,2);return 0;}
    if(!input){put(dest,0,2);return invalid(0x16);}
    auto remaining=capacity;auto out=dest;auto in=input;
    if(count==0xffffffffu) {
      do {const auto value=get(in,2);put(out,value,2);out+=2;in+=2;if(!value)return 0;}while(--remaining);
      put(dest+capacity*2-2,0,2);return 0x50;
    }
    do {const auto value=get(in,2);put(out,value,2);out+=2;in+=2;if(!value)break;if(!--remaining)break;}while(--count);
    if(!count)put(out,0,2);
    if(remaining)return 0;
    put(dest,0,2);return invalid(0x22);
  }
  std::uint32_t segment(std::uint32_t text)const {
    auto current=text;
    const auto distance=[&]{return std::bit_cast<std::uint32_t>(std::bit_cast<std::int32_t>(current-text)>>1);};
    while(get(current,2)) {
      auto set=0x863954u;const auto first=get(set,2);
      if(first) {
        const auto value=get(current,2);auto item=first;
        do {if(item==value)return distance();set+=2;item=get(set,2);}while(item);
      }
      current+=2;
    }
    return distance();
  }
  std::uint32_t footer_index(std::uint32_t id)const {
    const auto offset=id-10;
    if(offset>0xe1)return 0xffffffffu;
    const auto target=get(0x60f310+get(0x60f320+offset,1)*4);
    if(target==0x60f2fb)return 3;
    if(target==0x60f301)return 0;
    if(target==0x60f304)return 2;
    if(target==0x60f309)return 0xffffffffu;
    throw FormatError("Unresolved original Vista footer-index target");
  }
};
#if defined(__i386__) || defined(__x86_64__)
struct Fp {
  unsigned saved;
  explicit Fp(std::uint32_t control):saved(_mm_getcsr()) {
    if((control&0x1f80)!=0x1f80||control&0xffff0000u)
      throw FormatError("Unmasked or invalid Vista footer SSE context");
    _mm_setcsr(control);
  }
  ~Fp(){_mm_setcsr(saved);}
  static __m128 bits(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
  static std::uint32_t cvt_sub(std::uint32_t integer,std::uint32_t sub) {
    const auto f=_mm_cvtsi32_ss(_mm_setzero_ps(),std::bit_cast<std::int32_t>(integer));
    return std::bit_cast<std::uint32_t>(_mm_cvttss_si32(_mm_sub_ss(f,bits(sub))));
  }
  static std::uint32_t width(std::uint32_t integer,std::uint32_t scale,std::uint32_t multiplier) {
    const auto product=_mm_mul_ss(bits(scale),bits(multiplier));
    const auto f=_mm_cvtsi32_ss(_mm_setzero_ps(),std::bit_cast<std::int32_t>(integer));
    return std::bit_cast<std::uint32_t>(_mm_cvttss_si32(_mm_add_ss(f,product)));
  }
};
#else
struct Fp {
  explicit Fp(std::uint32_t){throw FormatError("Vista footer arithmetic requires an exact x86 SSE provider");}
  static std::uint32_t cvt_sub(std::uint32_t,std::uint32_t){throw FormatError("Missing Vista SSE provider");}
  static std::uint32_t width(std::uint32_t,std::uint32_t,std::uint32_t){throw FormatError("Missing Vista SSE provider");}
};
#endif
} //namespace

void vista_widget_header_initialize(VistaWidgetIdentity screen,std::uint32_t scratch_id,
    std::span<std::byte,0x430> scratch,const VistaWidgetHeaderFooterServices& s) {
  const Owner o{scratch_id,scratch,s};const auto context=call(s.context);
  const auto first_id=o.get(screen.value+0x70);o.lput(0x10,0);
  const auto first_datum=call(s.definition,first_id);
  if(first_datum!=0xffffffffu)o.lput(0x10,call(s.tag,first_datum));
  const auto second_id=o.get(screen.value+0x70);o.lput(0x30,0,2);
  const auto second_datum=call(s.definition,second_id);
  const auto second=second_datum==0xffffffffu?0u:call(s.tag,second_datum);o.lput(0x14,0);
  if(second&&std::bit_cast<std::int32_t>(o.get(second+0x20))>0)
    o.lput(0x14,o.get(o.relocated(o.get(second+0x24))+2,2));
  const auto unit=o.get(0x79be18);const auto tag=o.lget(0x10);o.lput(0x18,unit);const auto gray=o.get(0x7ba848);
  o.lput(0x1c,gray);o.lput(0x20,gray);o.lput(0x24,gray);
  std::uint32_t font=1;
  if(tag&&(o.get(tag)&4))return;
  const VistaWidgetIdentity child{screen.value+0x80};
  o.lput(0x2a,0xff9c,2);o.lput(0x2e,0x64,2);o.lput(0x28,0x64,2);o.lput(0x2c,0x50,2);
  call(s.append,screen,child);
  if(context) {
    o.lput(0x18,o.get(context+0x168));o.lput(0x1c,o.get(context+0x16c));o.lput(0x20,o.get(context+0x170));
    const auto cached_tag=o.lget(0x10);o.lput(0x24,o.get(context+0x174));
    if(cached_tag) {
      const auto flags=o.get(cached_tag);std::uint32_t bounds,font_at;
      if(flags&1){bounds=0x1a8;font_at=0x166;}
      else if(flags&8){bounds=0x198;font_at=0x164;}
      else if(flags&0x10){bounds=0x188;font_at=0x162;}
      else {bounds=0x178;font_at=0x160;}
      const auto first=o.get(context+bounds);font=std::uint32_t(o.signed_word(context+font_at));o.lput(0x28,first);
      const auto last=o.get(context+bounds+4);o.lput(0x2c,last);
      call(s.header_locale,child,o.get(cached_tag+0x2c));
      const auto style=call(s.style,child);const auto text=call(s.text,style);o.copy(o.local(0x30),text,0x200);
    }
  }
  call(s.initialize_text,child,1u,o.lget(0x14),o.local(0x18),font,o.local(0x28));
  const auto style=call(s.style,child);o.put(style+0x14,o.get(style+0x14,2)|2,2);
  call(s.assign_text,child,o.local(0x30));o.put(screen.value+0xea,0x7fff,2);
}

void vista_widget_footer_initialize(VistaWidgetIdentity screen,std::uint32_t append,
    std::uint32_t scratch_id,std::span<std::byte,0x428> scratch,
    const VistaWidgetHeaderFooterServices& s) {
  const Owner o{scratch_id,scratch,s};const auto context=call(s.context);const auto tag=o.lookup(screen);
  const auto second=o.lookup(screen);o.lput(0x1c,0);
  if(second&&std::bit_cast<std::int32_t>(o.get(second+0x20))>0)
    o.lput(0x1c,o.get(o.relocated(o.get(second+0x24))+2,2));
  const auto unit=o.get(0x79be18);o.lput(0xc,unit);const auto gray=o.get(0x7ba848);
  o.lput(0x10,gray);o.lput(0x14,gray);o.lput(0x18,gray);o.lput(0x28,0,2);
  o.copy(o.local(0x28),0x79bd7c,0x200);
  o.lput(0x22,0xff9c,2);o.lput(0x26,0x64,2);o.lput(0x20,0xff9c,2);o.lput(0x24,0xff88,2);
  const VistaWidgetIdentity child{screen.value+0x53c};if(std::uint8_t(append))call(s.append,screen,child);
  if(context) {
    o.color(o.local(0xc),context+0x168);
    if(tag) {
      const auto color=o.get(tag+8);const auto font=std::uint32_t(o.signed_word(tag+6));o.lput(0xc,color);
      o.lput(0x10,o.get(tag+0xc));o.lput(0x14,o.get(tag+0x10));o.lput(0x18,o.get(tag+0x14));
      const auto flags=o.get(tag);const auto bounds=(flags&1)?0x1b0u:(flags&8)?0x1a0u:(flags&0x10)?0x190u:0x180u;
      o.lput(0x20,o.get(context+bounds));const auto last=o.get(context+bounds+4);o.lput(0x24,last);
      call(s.footer_locale,child,font);const auto style=call(s.style,child);const auto text=call(s.text,style);o.copy(o.local(0x28),text,0x200);
    }
  }
  call(s.initialize_text,child,2u,o.lget(0x1c),o.local(0xc),1u,o.local(0x20));
  call(s.assign_text,child,o.local(0x28));o.put(screen.value+0x5a6,0x7fff,2);
}

void vista_widget_footer_layout(VistaWidgetIdentity screen,std::uint32_t scratch_id,
    std::span<std::byte,0x4ac> scratch,const VistaWidgetHeaderFooterServices& s) {
  const Owner o{scratch_id,scratch,s};const Fp fp{s.mxcsr};
  const auto unit_first=o.get(0x79be18);const auto state=o.get(screen.value+0x42,1);o.lput(0x4c,unit_first);
  const auto gray=o.get(0x7ba848);o.lput(0x20,screen.value);o.lput(0x18,1);
  o.lput(0x50,gray);o.lput(0x54,gray);o.lput(0x58,gray);
  const auto shown=(state&2)?1u:(o.get(screen.value+0x42,1)&1);const auto first_id=o.get(screen.value+0x70);o.lput(0x17,shown?0u:1u,1);
  const auto first_datum=call(s.definition,first_id);const auto tag=first_datum==0xffffffffu?0u:call(s.tag,first_datum);
  if(!tag){o.put(screen.value+0x9fd,1,1);return;}
  VistaWidgetIdentity footer{screen.value+0x53c};
  if(o.get(tag,1)&4) {
    const auto index=o.footer_index(o.get(screen.value+0x70));
    if(index==0xffffffffu){o.put(screen.value+0x9fd,1,1);return;}
    footer=call(s.text_child,screen,index);
  }
  if(!footer){o.put(screen.value+0x9fd,1,1);return;}
  o.lput(0x34,0);o.lput(0x38,0);o.lput(0x3c,0);
  const auto index=std::uint16_t(o.animation(o.lookup(screen))-1u);
  if(index!=0xffff&&call(s.animation_descriptor,screen,std::uint32_t(index),4u,o.local(0x5c))) {
    const auto mode=o.lget(0x70);
    if(mode&&mode!=1){call(s.detach_kinds,screen,2u,5u);return;}
    const auto count=o.signed_word(o.local(0x64));
    if(count>0) {
      const auto frame=o.lget(0x60)+std::uint32_t(count)*20-12;
      const auto x=o.get(frame),y=o.get(frame+4),z=o.get(frame+8);
      o.lput(0x34,x);o.lput(0x38,y);o.lput(0x3c,z);
    }
  }
  const auto horizontal=o.get(0xe3e40c,2);const auto scale=o.get(0xe3e424);const auto vertical=o.get(0xe3e408,2);
  o.lput(0xa0,horizontal,2);o.lput(0x44,scale);const auto unit=o.get(0x79be18);
  o.lput(0x9c,0,2);o.lput(0x9e,0,2);o.lput(0xa2,vertical,2);o.put(0xe3e424,unit);
  call(s.detach_kinds,screen,2u,5u);
  const auto style=call(s.style,footer);const auto text=call(s.text,style);o.lput(0x1c,text);
  call(s.bounds,footer,o.local(0x90));
  o.lput(0x2c,std::uint32_t(o.signed_word(o.local(0x96))));
  o.lput(0x96,o.lget(0x96)*2,2);o.bounds_copy(footer,o.local(0x90));
  const auto top=std::uint32_t(o.signed_word(o.local(0x90))),bottom=std::uint32_t(o.signed_word(o.local(0x94)));
  o.lput(0x40,top);o.lput(0x48,bottom);o.lput(0xac,0,2);o.zero(o.local(0xae),0x3fe);o.lput(0x28,0);
  if(text)o.lput(0x24,screen.value+0xa08);
  auto cursor=text;
  while(cursor&&o.get(cursor,2)&&std::bit_cast<std::int32_t>(o.lget(0x18))<5) {
    const auto count=o.segment(cursor+2)+1;
    o.copy(o.local(0xac),cursor,0x200,count);
    if(!o.lget(0xac,2))break;
    const auto before_style=call(s.style,footer);o.put(before_style+0x1c,0);
    call(s.measure,footer,o.local(0x9c),o.lget(0x28),o.local(0x90),0u);
    const auto left=std::uint32_t(o.signed_word(o.local(0x92)));
    call(s.measure,footer,o.local(0x9c),count+o.lget(0x28),o.local(0x90),0u);
    const auto width=std::uint32_t(o.signed_word(o.local(0x96)))-left;
    const auto font_scale=o.get(0xe3e424);const auto multiplier=o.get(0x7cf210);const auto spacing=Fp::width(width,font_scale,multiplier);
    const auto right=o.lget(0x2c);o.lput(0x96,right,2);o.lput(0x92,right-spacing,2);
    const auto after_style=call(s.style,footer);o.put(after_style+0x1c,1);
    auto child=call(s.allocate,0x4bcu,0u);o.lput(0x30,child.value);
    if(child)child=call(s.construct_text,child,o.get(o.lget(0x20)+8,2));
    o.put(child.value+0x6c,1,1);call(s.append,VistaWidgetIdentity{o.lget(0x20)},child);
    const auto datum_screen=o.get(o.lget(0x20)+0x70);o.lput(0x30,0);
    const auto datum=call(s.definition,datum_screen);const auto current_tag=datum==0xffffffffu?o.lget(0x30):call(s.tag,datum);
    const auto animation=o.animation(current_tag);
    call(s.initialize_text,child,1u,animation,o.local(0x4c),1u,o.local(0x90));
    call(s.assign_text,child,o.local(0xac));
    const auto old_cursor=o.lget(0x1c);o.lput(0x28,o.lget(0x28)+count);
    o.put(child.value+0x6e,1,1);o.lput(0x1c,old_cursor+count*2);o.zero(o.local(0xac),0x400);
    const auto next=o.lget(0x24)+4;o.lput(0x18,o.lget(0x18)+1);o.lput(0x24,next);o.put(next,child.value);
    cursor=o.lget(0x1c);
  }
  const auto hidden=o.lget(0x17,1);auto count=o.lget(0x18);o.put(footer.value+0x6e,hidden?0u:1u,1);
  VistaWidgetIdentity previous{};
  if(std::bit_cast<std::int32_t>(count)>=2) {
    o.lput(0x1c,Fp::cvt_sub(o.lget(0x40),o.lget(0x38)),2);
    o.lput(0x18,screen.value+count*4+0xa04);o.lput(0x24,count-1);
    do {
      const VistaWidgetIdentity child{o.get(o.lget(0x18))};call(s.bounds,child,o.local(0x90));
      if(previous) {
        call(s.bounds,previous,o.local(0xa4));call(s.bounds,child,o.local(0x90));
        const auto left_current=std::uint32_t(o.signed_word(o.local(0x92)));const auto right_current=std::uint32_t(o.signed_word(o.local(0x96)));
        const auto width=right_current-left_current;
        const auto left=o.lget(0xa6);o.lput(0x96,left,2);o.lput(0x92,left-width,2);
      }else {
        const auto right=std::uint32_t(o.signed_word(o.local(0x96))),left=std::uint32_t(o.signed_word(o.local(0x92))),x=o.lget(0x34);
        const auto moved_right=Fp::cvt_sub(right,x),moved_left=Fp::cvt_sub(left,x);
        o.lput(0x96,moved_right,2);o.lput(0x92,moved_left,2);
      }
      o.lput(0x90,o.lget(0x1c,2),2);o.lput(0x94,o.lget(0x48,2),2);o.bounds_copy(child,o.local(0x90));
      const auto child_hidden=o.lget(0x17,1);
      o.lput(0x18,o.lget(0x18)-4);o.lput(0x24,o.lget(0x24)-1);o.put(child.value+0x6e,child_hidden,1);previous=child;
    }while(o.lget(0x24));
  }
  call(s.bounds,footer,o.local(0x90));o.lput(0x96,o.lget(0x2c,2),2);o.bounds_copy(footer,o.local(0x90));
  o.put(0xe3e424,o.lget(0x44));o.put(screen.value+0x9fd,o.lget(0x17,1),1);
}
} //namespace halo2
