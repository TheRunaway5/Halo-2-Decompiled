#include "halo2/vista_widget_glyph_layout.hpp"
#include "halo2/cache.hpp"
#include <bit>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
using U=std::uint32_t;
template<class F> const F& required(const F& provider) {
  if(!provider)throw FormatError("Missing genuine Vista glyph layout provider");
  return provider;
}
std::int32_t si(U x) {return std::bit_cast<std::int32_t>(x);}
std::int32_t sw(U x) {return std::bit_cast<std::int16_t>(std::uint16_t(x));}
#if defined(__i386__) || defined(__x86_64__)
__m128 fv(U x) {return _mm_castsi128_ps(_mm_cvtsi32_si128(si(x)));}
U fb(__m128 x) {return U(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
U fi(std::int32_t x) {return fb(_mm_cvtsi32_ss(_mm_setzero_ps(),x));}
U add(U a,U b) {return fb(_mm_add_ss(fv(a),fv(b)));}
U sub(U a,U b) {return fb(_mm_sub_ss(fv(a),fv(b)));}
U mul(U a,U b) {return fb(_mm_mul_ss(fv(a),fv(b)));}
U trunc(U a) {return U(_mm_cvtt_ss2si(fv(a)));}
bool above(U a,U b) {
  unsigned char result;
  const auto av=fv(a),bv=fv(b);
  // COMISS sets CF/ZF/PF on unordered operands. SETA exactly preserves JA/JBE.
  asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(av),"x"(bv):"cc");
  return result!=0;
}
U x87_copy(U x) {
  U result;
  asm volatile("flds %1; fstps %0":"=m"(result):"m"(x):"st");
  return result;
}
#else
U fi(std::int32_t) {throw FormatError("Missing exact Vista x86 glyph arithmetic provider");}
U add(U,U) {throw FormatError("Missing exact Vista x86 glyph arithmetic provider");}
U sub(U,U) {throw FormatError("Missing exact Vista x86 glyph arithmetic provider");}
U mul(U,U) {throw FormatError("Missing exact Vista x86 glyph arithmetic provider");}
U trunc(U) {throw FormatError("Missing exact Vista x86 glyph arithmetic provider");}
bool above(U,U) {throw FormatError("Missing exact Vista x86 glyph arithmetic provider");}
U x87_copy(U) {throw FormatError("Missing exact Vista x86 glyph arithmetic provider");}
#endif
struct Memory {
  const VistaGlyphLayoutServices& s;
  U r(U a,std::size_t n=4)const{return required(s.read)(a,n);}
  void w(U a,U x,std::size_t n=4)const{required(s.write)(a,n==1?x&255:n==2?x&65535:x,n);}
  std::int32_t word(U a)const{return sw(r(a,2));}
  template<class Range> U args_range(U sp,const Range& values)const {
    U at=sp-U(values.size()*4);
    for(U v:values) {w(at,v);at+=4;}
    return sp-U(values.size()*4)-4;
  }
  U args(U sp,std::initializer_list<U> values)const{return args_range(sp,values);}
};
} //namespace
std::uint8_t vista_glyph_character_classify(U character) {
  return std::uint8_t(character+0xffff1eceu<=9u);
}
U vista_glyph_kerning(U d,U prev,U ch,const VistaGlyphLayoutServices& s) {
  const Memory m{s};
  if(!d||!prev||!ch||prev>255||ch>255)return 0;
  if(!(m.r(d+0x1a4+(prev>>5)*4)&(U(1)<<(prev&31))))return 0;
  U i=0,at=d+0x24;
  do {
    if(m.r(at,1)>=prev)break;
    ++i;at+=4;
    if(si(i)>=si(m.r(d+0x20)))return 0;
  }while(true);
  if(m.r(d+i*4+0x24,1)!=prev)return 0;
  at=d+i*4+0x24;
  while(m.r(at+1,1)<ch) {
    ++i;at+=4;
    if(si(i)>=si(m.r(d+0x20)))return 0;
    if(m.r(at,1)!=prev)return 0;
  }
  if(m.r(d+i*4+0x25,1)!=ch)return 0;
  return m.r(d+i*4+0x26,2);
}
void vista_glyph_collect_bounds(U e,const VistaGlyphLayoutServices& s) {
  const Memory m{s};
  const U definition=required(s.font_definition)(m.r(e+8));
  const U scalar=m.r(e+0x30),width=m.r(e+0x28),height=m.r(e+0x2c);
  const U x=m.r(e+0x18),one=m.r(0x79be18);
  const auto left=m.word(0x8cdfaa);
  U right=mul(width,scalar),bottom=mul(height,scalar);
  const U y=m.r(e+0x1c);
  right=add(right,x);bottom=add(bottom,y);
  right=add(right,one);bottom=add(bottom,one);
  if(above(fi(left),x))m.w(0x8cdfaa,trunc(x),2);
  if(above(fi(m.word(0x8cdfa8)),y))m.w(0x8cdfa8,trunc(y),2);
  if(above(right,fi(m.word(0x8cdfae))))m.w(0x8cdfae,trunc(right),2);
  if(above(bottom,fi(m.word(0x8cdfac))))m.w(0x8cdfac,trunc(bottom),2);
  if(definition) {
    m.w(0x8cdfb0,m.r(definition+4,2),2);
    m.w(0x8cdfb2,m.r(definition+6,2),2);
  }
}
VistaGlyphLineWidth vista_glyph_line_width(U state,U e,const VistaGlyphLayoutServices& s) {
  const Memory m{s};const U f=e-0x30;
  U x0=0,x1=0,x2=0,x3=0,x4=0,x5=0,ep=0,ea=0;
  bool bl=false;
  m.w(f+0x14,0);
  x0=m.r(e+0xc);x0=mul(x0,m.r(0x7bbeb8));
  m.w(f+0x1c,0);m.w(state+0xe,0xffff,2);m.w(f+0x18,0);
  m.w(f+0x13,0,1);m.w(f+0x24,x0);
  while(true) {
    bl=false;
  next:
    required(s.iterator_next)(state);
    if(m.r(state+0x20)!=4) {m.w(f+0x13,1,1);goto finish_or_next;}
    {
      const U ch=m.r(state+0x14),font=m.r(state);
      const U glyph=required(s.glyph)(font,ch);
      if(!glyph)goto finish_or_next;
      const U current=m.r(state+0x14),previous=m.r(state+0x18),definition=m.r(state+4);
      const U kern=vista_glyph_kerning(definition,previous,current,s);
      const auto advance=m.word(glyph);
      x1=m.r(e+0xc);
      const auto offset=m.word(glyph+8);
      const bool changed=m.r(state+0x1c,1)!=U(bl);
      x0=fi(advance);x2=fi(sw(kern));x3=fi(offset);
      x0=mul(x0,x1);x2=mul(x2,x1);x3=mul(x3,x1);x4=x0;
      if(changed) {
        x1=m.r(f+0x14);ea=ep;m.w(f+0x1c,ea);m.w(f+0x20,x1);
      } else ea=m.r(f+0x1c);
      U at=m.r(e+4);x1=m.r(at);
      at=m.r(e+8);x5=m.r(f+0x14);
      x1=add(x1,x4);x4=m.r(at+4);
      x1=add(x1,x3);x1=add(x1,x2);x1=add(x1,x5);
      if(!above(x4,x1)&&m.r(f+0x18)!=0&&m.r(e+0x10)!=3) {
        if(m.r(0x8ce016,1)!=0) {
          ep=U(m.word(state+0xe));x5=m.r(f+0x28);m.w(state+0x20,0);
          m.w(f+0x28,x5);return {m.r(f+0x28),ep};
        }
        if(m.r(0x8cdfbc,1)&1) {
          if(si(ea)>0) {x5=m.r(f+0x20);ep=ea;m.w(f+0x14,x5);bl=true;}
          m.w(f+0x13,1,1);
          if(bl) {m.w(f+0x28,x5);return {m.r(f+0x28),ep};}
          goto finish_or_next;
        }
        goto classified;
      }
      if(m.r(0x8ce016,1)!=0&&m.r(state+0xe,2)==0xffff) {
        x4=sub(x4,m.r(f+0x24));
        if(above(x1,x4)) {
          const U cursor=m.r(state+0xc,2);
          x0=add(x0,x3);x0=add(x0,x2);x0=add(x0,x5);
          m.w(state+0xe,cursor-1,2);m.w(f+0x28,x0);goto classified;
        }
      }
      m.w(f+0x18,m.r(f+0x18)+1);
      x0=add(x0,x3);x0=add(x0,x2);x0=add(x0,x5);m.w(f+0x14,x0);
    }
  classified:
    if(vista_glyph_character_classify(m.r(state+0x14))) {
      do {
        required(s.iterator_next)(state);
        if(vista_glyph_character_classify(m.r(state+0x14)))goto finish_or_next;
      }while(m.r(state+0x20)!=0);
      m.w(f+0x13,1,1);
    }
  finish_or_next:
    {
      const bool done=m.r(f+0x13,1)!=0;
      ep=U(m.word(state+0xc));
      if(!done)goto next;
      x5=m.r(f+0x14);m.w(f+0x28,x5);return {m.r(f+0x28),ep};
    }
  }
}
void vista_glyph_draw(U clip,std::uint16_t initial,U e,const VistaGlyphLayoutServices& s) {
  const Memory m{s};const U f=e-0x84;
  U x0=m.r(0x7bbec4),x1=m.r(0x7bbec0),x2=0,x3=x1,x4=x0,x5=x0;
  const U point=m.r(e+8);
  m.w(f+0x24,x4);m.w(f+0x2c,x5);m.w(f+0x20,x3);m.w(f+0x28,x1);
  if(clip) {
    x2=m.r(clip);if(above(x2,x0)){x4=x2;m.w(f+0x24,x4);}
    x2=m.r(clip+4);if(above(x1,x2)){x3=x2;m.w(f+0x20,x3);}
    x2=m.r(clip+8);if(above(x2,x0)){x5=x2;m.w(f+0x2c,x5);}
    x0=m.r(clip+0xc);if(above(x1,x0)){x1=x0;m.w(f+0x28,x1);}
    if(!above(x3,x4)||!above(x1,x5))return;
  }
  const U word_c4=m.r(0x8cdfc4,2),word_c0=m.r(0x8cdfc0,2),font=m.r(0x8cdfb8);
  const U text=m.r(e+0x14),state=f+0x40;
  if(!required(s.iterator_birth)(state,0x8cdfc8,{text,font,word_c0,word_c4,0x8cdfd8,0x8cdfdc}))return;
  const auto stop_initial=sw(m.r(e+0x18,2));
  m.w(state+0xc,initial,2);
  if(sw(initial)>=stop_initial)return;
  U glyph=0,ea=0;
  while(true) {
    required(s.iterator_next)(state);
    ea=m.r(state+0x14);
    if(!ea)return;
    if(m.r(state+0x20)!=4||(si(ea)>=0&&si(ea)<=0x1f))goto advance_loop;
    {
      const U glyph_font=m.r(state);
      glyph=required(s.glyph)(glyph_font,ea);
      if(!glyph)goto advance_loop;
      const auto width=m.word(glyph+4);
      x0=0;
      const auto height=m.word(glyph+6);
      const U ch=m.r(state+0x14);
      m.w(f+0x34,x0);m.w(f+0x30,x0);
      x0=fi(width);
      const U previous=m.r(state+0x18);
      m.w(f+0x10,x0);x0=fi(height);
      const U definition=m.r(state+4);
      m.w(f+0x14,x0);
      const U kern=vista_glyph_kerning(definition,previous,ch,s);
      const auto offset_x=m.word(glyph+8);
      x3=m.r(e+0x1c);x0=m.r(point);x4=m.r(f+0x20);
      const auto offset_y=m.word(glyph+0xa);
      x2=mul(fi(offset_x),x3);x0=add(x0,x2);m.w(f+0x18,x2);
      x1=mul(fi(sw(kern)),x3);x0=add(x0,x1);m.w(f+0x1c,x1);
      x1=m.r(point+4);x2=mul(fi(offset_y),x3);x1=sub(x1,x2);
      x2=m.r(f+0x10);x5=add(mul(x2,x3),x0);
      m.w(f+0x3c,x0);m.w(f+0x38,x1);
      if(above(x5,x4)){x2=sub(x4,x0);m.w(f+0x10,x2);}
      x5=m.r(f+0x24);
      if(above(x5,x0)) {
        x4=sub(x5,x0);x0=sub(x2,x4);x2=x0;
        m.w(f+0x34,x4);m.w(f+0x3c,x5);m.w(f+0x10,x2);
      }
      x0=m.r(f+0x14);x4=mul(x0,x3);x3=m.r(f+0x28);x4=add(x4,x1);
      if(above(x4,x3)){x0=sub(x3,x1);m.w(f+0x14,x0);}
      x4=m.r(f+0x2c);
      if(above(x4,x1)) {
        x3=sub(x4,x1);x0=sub(x0,x3);
        m.w(f+0x30,x3);m.w(f+0x38,x4);m.w(f+0x14,x0);
      }
      ea=m.r(state+0x28);
      if(ea!=7&&ea!=6) {
        if(vista_glyph_character_classify(m.r(state+0x18))) {
          const U previous_cursor=U(m.word(state+0xc))-1;
          m.w(state+0x28,5);m.w(state+0x34,previous_cursor);
          goto state_five;
        }
        ea=m.r(state+0x28);x2=m.r(f+0x10);x0=m.r(f+0x14);
      }
      if(ea==5) {
      state_five:
        if(vista_glyph_character_classify(m.r(state+0x14))) {
          x0=m.r(state+0x30);x0=sub(x0,m.r(state+0x2c));
          const U cursor=m.r(state+0x34,2);
          x0=mul(x0,m.r(0x79de3c));x0=add(x0,m.r(point));
          m.w(state+0x28,6);m.w(state+0xc,cursor,2);m.w(point,x0);
          goto advance_loop;
        }
        ea=m.r(state+0x28);
        if(ea==5) {
          x0=m.r(f+0x10);x0=mul(x0,m.r(e+0x1c));
          x0=add(x0,m.r(state+0x2c));x0=add(x0,m.r(f+0x18));x0=add(x0,m.r(f+0x1c));
          m.w(state+0x2c,x0);goto advance_loop;
        }
        x2=m.r(f+0x10);x0=m.r(f+0x14);
      }
      if(ea==6) {
        if(vista_glyph_character_classify(m.r(state+0x14))) {
          x0=m.r(state+0x38);m.w(point,x0);x0=0;
          m.w(state+0x28,7);m.w(state+0x2c,x0);goto advance_loop;
        }
        ea=m.r(state+0x28);x2=m.r(f+0x10);x0=m.r(f+0x14);
      }
      if(ea==7)m.w(state+0x28,8);
      x1=0;
      if(above(x2,x1)&&above(x0,x1)) {
        const U ch_draw=m.r(state+0x14);
        U color=m.r(e+0xc);
        if(required(s.color_classify)(ch_draw))color|=0xffffff;
        const U global_f0=m.r(0x8cdff0);
        const U scalar=x87_copy(m.r(e+0x1c));
        const U global_ec=m.r(0x8cdfec),secondary=m.r(e+0x10);
        m.w(f-0xc,scalar);
        const U draw_font=m.r(state);
        const U draw_h=x87_copy(m.r(f+0x14));m.w(f-0x10,draw_h);
        const U draw_w=x87_copy(m.r(f+0x10));m.w(f-0x14,draw_w);
        const U draw_oy=x87_copy(m.r(f+0x30));m.w(f-0x18,draw_oy);
        const U draw_ox=x87_copy(m.r(f+0x34));m.w(f-0x1c,draw_ox);
        const U draw_y=x87_copy(m.r(f+0x38));m.w(f-0x20,draw_y);
        const U draw_x=x87_copy(m.r(f+0x3c));m.w(f-0x24,draw_x);
        const std::array<U,14> a={state,draw_font,ch_draw,color,secondary,
          draw_x,draw_y,draw_ox,draw_oy,draw_w,draw_h,scalar,global_ec,global_f0};
        // The original callback pointer is reloaded after argument publication.
        const U target=m.r(e+4);
        if(target==0x498726) {
          const U child=m.args_range(f,a);
          vista_glyph_collect_bounds(child,s);
        } else required(s.raster)(target,a);
      }
      if(!vista_glyph_character_classify(m.r(state+0x14))) {
        x0=fi(m.word(glyph));x0=mul(x0,m.r(e+0x1c));x0=add(x0,m.r(point));
        x0=add(x0,m.r(f+0x18));x0=add(x0,m.r(f+0x1c));m.w(point,x0);
      } else {
        const auto advance=m.word(glyph);x0=m.r(e+0x1c);x1=fi(advance);
        x1=mul(x1,x0);x1=add(x1,m.r(point));x1=add(x1,m.r(f+0x18));x1=add(x1,m.r(f+0x1c));
        m.w(state+0x38,x1);x1=mul(fi(m.word(glyph+4)),x0);m.w(state+0x30,x1);
      }
    }
  advance_loop:
    {
      const auto stop=sw(m.r(e+0x18,2));
      if(m.word(state+0xc)>=stop)return;
    }
  }
}
void vista_glyph_layout(U e,const VistaGlyphLayoutServices& s) {
  const Memory m{s};const U f=e-0xbc,state=f+0x6c;
  const U callback=m.r(e+4);U out=m.r(e+0xc),rect=m.r(e+8);
  const auto left_initial=m.word(rect+2),top_initial=m.word(rect);
  U x0=fi(left_initial),x1=0,x2=0,x3=0;
  const U c4=m.r(0x8cdfc4,2);m.w(f+0x14,x0);x0=fi(top_initial);
  const U c0=m.r(0x8cdfc0,2),font=m.r(0x8cdfb8);
  m.w(f+0x4c,callback);const U text=m.r(e+0x1c);
  U row=0,bx=0;
  m.w(f+0x50,text);m.w(f+0x54,rect);m.w(f+0x58,out);
  m.w(f+0x1c,0);m.w(f+0x38,0);m.w(f+0x10,0);m.w(f+0x20,0);m.w(f+0x18,x0);
  if(!required(s.iterator_birth)(state,0x8cdfc8,{text,font,c0,c4,0x8cdfd8,0x8cdfdc}))goto finish;
  bx=m.r(state+0xc,2);
  while(true) {
    const auto left=m.word(rect+2),right=m.word(rect+6),align=m.word(state+0x12);
    m.w(f+0x3c,U(sw(bx)));
    const auto top=m.word(rect),bottom=m.word(rect+4);
    x1=fi(left);x0=fi(right);const U count=m.r(0x8cdff4,2);
    m.w(f+0x28,x0);x2=fi(top);x0=fi(bottom);m.w(f+0x2c,x2);m.w(f+0x30,x0);
    if(sw(count)>0) {
      const U column=m.r(f+0x1c);
      if(std::uint16_t(column))x0=fi(m.word(0x8cdff4+U(sw(column))*2));
      else {
        const auto later=m.word(0x8ce01a);
        const auto indent=std::uint16_t(row)?later:m.word(0x8ce018);
        x0=add(fi(indent),x1);
      }
      x3=m.r(e+0x18);m.w(f+0x24,x0);
      if(sw(column)<sw(count)) {x1=fi(m.word(0x8cdff6+U(sw(column))*2));x1=mul(x1,x3);m.w(f+0x28,x1);}
    } else {
      x3=m.r(e+0x18);
      x0=fi(m.word(std::uint16_t(row)?0x8ce01a:0x8ce018));
      x0=mul(x0,x3);x0=add(x0,x1);m.w(f+0x24,x0);
    }
    U definition=m.r(state+4);
    const U scalar=x87_copy(m.r(e+0x18));
    const auto definition_x=m.word(definition+0xa),a=m.word(definition+4),b=m.word(definition+6);
    x1=fi(definition_x);
    U d=U(m.word(definition+8));d+=U(m.word(e+0x14));
    const U line=U(m.word(f+0x10));
    U y=U(b)+d+U(a);y*=line+U(sw(row));y+=U(a);
    x1=add(mul(x1,x3),x0);x0=add(mul(fi(si(y)),x3),x2);
    m.w(f+0x14,x1);m.w(f+0x18,x0);
    const U width_entry=m.args(f,{f+0x14,f+0x24,scalar,U(align)});
    const auto width=vista_glyph_line_width(state,width_entry,s);
    definition=m.r(state+4);m.w(f+0x40,width.width_bits);bx=width.cursor;m.w(f+0x44,bx);
    if(align==1||align==3) {
      x1=m.r(f+0x24);
      const auto inset=m.word(definition+0xa);
      x0=m.r(f+0x28);x0=sub(x0,x1);x0=add(x0,x1);x0=sub(x0,m.r(f+0x40));
      x1=fi(inset);x0=sub(x0,x1);
      if(align==1||above(m.r(f+0x14),x0))m.w(f+0x14,x0);
    } else if(align==2) {
      x1=m.r(f+0x24);x0=m.r(f+0x28);x0=sub(x0,x1);x0=sub(x0,m.r(f+0x40));
      x0=mul(x0,m.r(0x79de3c));x0=add(x0,x1);m.w(f+0x14,x0);
    }
    x0=m.r(f+0x30);
    if(above(x0,m.r(f+0x18))) {
      const U clip=m.r(e+0x10);
      if(clip) {
        const auto c_left=m.word(clip+2),c_right=m.word(clip+6);
        x0=fi(c_left);const auto c_top=m.word(clip);m.w(f+0x5c,x0);
        x0=fi(c_right);const auto c_bottom=m.word(clip+4);m.w(f+0x60,x0);
        x0=fi(c_top);m.w(f+0x64,x0);x0=fi(c_bottom);m.w(f+0x68,x0);m.w(f+0x34,f+0x5c);
      }else m.w(f+0x34,0);
      const U cutoff=m.r(state+0xe,2);
      if(cutoff!=0xffff) {
        const U source=m.r(f+0x50);U index=U(sw(cutoff));U bytes=index*4;
        required(s.copy)(0x8ce020,source,bytes);
        U extra=0,utf16=f+0xb0;m.w(f+0x48,0);
        required(s.ellipsis)(utf16,f-4);
        if(m.r(utf16,2)!=0) {
          U destination=bytes+0x8ce020;
          do {
            if(extra>=4)break;
            const U converted=required(s.convert_one)(utf16,f+0x48);m.w(destination,converted);
            utf16+=2;++extra;destination+=4;
          }while(m.r(utf16,2)!=0);
        }
        const U terminator=required(s.convert_one)(0x79ce64,f+0x48);
        const U scalar_draw=x87_copy(m.r(e+0x18));
        const U cutoff_dword=m.r(state+0xe);
        const U stop=extra+cutoff_dword+1;
        const U primary=m.r(state+0x3c);
        index+=extra;m.w(0x8ce020+index*4,terminator);
        const U secondary=m.r(state+0x40),target=m.r(f+0x4c);
        const U start=m.r(f+0x3c);
        const U draw_entry=m.args(f,{target,f+0x14,primary,secondary,0x8ce020,stop,scalar_draw});
        const U draw_clip=m.r(f+0x34);
        vista_glyph_draw(draw_clip,std::uint16_t(start),draw_entry,s);
        rect=m.r(f+0x54);bx=m.r(f+0x44);row=m.r(f+0x38);
      } else {
        const U scalar_draw=x87_copy(m.r(e+0x18));
        const U secondary=m.r(state+0x40),primary=m.r(state+0x3c),draw_text=m.r(f+0x50);
        const U target=m.r(f+0x4c),draw_clip=m.r(f+0x34),start=m.r(f+0x3c);
        const U draw_entry=m.args(f,{target,f+0x14,primary,secondary,draw_text,bx,scalar_draw});
        vista_glyph_draw(draw_clip,std::uint16_t(start),draw_entry,s);
      }
    }
    const U kind=m.r(state+0x20);m.w(state+0xc,bx,2);
    if(kind==0) {out=m.r(f+0x58);break;}
    if(kind==1) {
      const U max=m.r(f+0x20);row=row+max+1;
      m.w(f+0x1c,0);m.w(f+0x10,0);m.w(f+0x38,row);m.w(f+0x20,0);
    } else if(kind==2) {
      const auto column=m.word(f+0x1c),count_now=m.word(0x8cdff4);
      if(column<count_now) {m.w(f+0x1c,m.r(f+0x1c)+1);m.w(f+0x10,0);}
    } else if(kind==3)m.w(f+0x10,0);
    else if(kind==4) {
      const U line=m.r(f+0x10)+1;
      const auto max=m.word(f+0x20);m.w(f+0x10,line);
      if(sw(line)>max)m.w(f+0x20,std::uint16_t(line));
    }
  }
finish:
  if(out) {
    const U x=trunc(m.r(f+0x14)),y=trunc(m.r(f+0x18));m.w(out,x,2);m.w(out+2,y,2);
  }
}
void vista_glyph_measure_dwords(U e,const VistaGlyphLayoutServices& s) {
  const Memory m{s};const U f=e-8;
  const U definition=required(s.font_definition)(m.r(0x8cdfb8));
  m.w(0x8cdfa8,0x7fff,2);m.w(0x8cdfaa,0x7fff,2);
  m.w(0x8cdfac,0x8000,2);m.w(0x8cdfae,0x8000,2);
  if(definition) {
    m.w(0x8cdfb0,m.r(definition+4,2),2);m.w(0x8cdfb2,m.r(definition+6,2),2);
  }
  const U text=m.r(e+8),scalar=x87_copy(m.r(e+0x14)),rect=m.r(e+4);
  const U child=m.args(f,{0x498726,rect,f,0,0,scalar,text});
  vista_glyph_layout(child,s);
  const U result=m.r(e+0x10),x=m.r(f);
  m.w(result+2,x,2);m.w(result+6,x+1,2);
  const U y=m.r(f+2);const U top=y-m.r(0x8cdfb0);
  m.w(result,top,2);const U bottom=m.r(0x8cdfb2,2)+y;m.w(result+4,bottom,2);
  const U first=m.r(0x8cdfa8),metrics=m.r(e+0xc);m.w(metrics,first);
  const U last=m.r(0x8cdfac);m.w(metrics+4,last);
}
void vista_glyph_measure(U e,const VistaGlyphLayoutServices& s) {
  const Memory m{s};const U converted=e-0x2000;
  required(s.convert)(m.r(e+8),converted,0x800);
  const U scalar=x87_copy(m.r(e+0x14));
  const U result=m.r(e+0x10);m.w(converted-4,scalar);
  const U metrics=m.r(e+0xc),rect=m.r(e+4);
  const U child=m.args(converted,{rect,converted,metrics,result,scalar});
  vista_glyph_measure_dwords(child,s);
}
} //namespace halo2
