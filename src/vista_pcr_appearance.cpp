#include "halo2/vista_pcr_appearance.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
constexpr VistaPcrRgb white{0x3f800000,0x3f800000,0x3f800000};
constexpr VistaPcrRgb pink{0x3f800000,0x3ed1eb85,0x3f333333};
std::uint32_t word(std::span<const std::byte> b,unsigned o,unsigned n=4) {
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[o+i]))<<(i*8);return v;
}
VistaPcrRgb palette(std::int8_t index,const VistaPcrAppearanceAssets& a) {
  if(!a.globals_present || index<0 || std::size_t(index)>=a.appearance_palette.size())return white;
  return a.appearance_palette[std::size_t(index)];
}
VistaPcrRgb team_color(std::uint16_t index,const VistaPcrAppearanceAssets& a) {
  const auto signed_index=std::bit_cast<std::int16_t>(index);
  if(signed_index<0 || std::size_t(signed_index)>=a.team_palette.size())return pink;
  return a.team_palette[std::size_t(signed_index)];
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class FpScope {
  std::fenv_t saved;unsigned mxcsr;
 public:
  explicit FpScope(const VistaLoadingFloatingPoint& fp):mxcsr(_mm_getcsr()) {
    if((fp.x87_control_word&0x3f)!=0x3f || (fp.x87_control_word&0x300)==0x100 ||
       (fp.mxcsr&0x1f80)!=0x1f80 || (fp.mxcsr&0xffff0000))
      throw FormatError("Unsupported Vista PCR appearance floating point context");
    if(std::fegetenv(&saved))throw FormatError("Vista PCR appearance floating point environment unavailable");
    _mm_setcsr(fp.mxcsr);asm volatile("fldcw %0"::"m"(fp.x87_control_word):"memory");
  }
  ~FpScope(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
};
float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float sub(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float div(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float integer(std::int32_t value){return _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),value));}
VistaPcrRgb adjusted(VistaPcrRgb rgb) {
  const float r=std::bit_cast<float>(rgb[0]),g=std::bit_cast<float>(rgb[1]),b=std::bit_cast<float>(rgb[2]);
  const auto gbmax=g>b?g:b;const auto maximum=r>gbmax?r:(g>b?g:b);
  const auto gbmin=g>b?b:g;const auto minimum=r>gbmin?(g>b?b:g):r;
  const auto difference=sub(maximum,minimum);const auto saturation=maximum==0.f?0.f:div(difference,maximum);
  float hue{};
  if(saturation!=0.f) {
    if(r==maximum)hue=div(sub(g,b),difference);
    else if(g==maximum)hue=add(div(sub(b,r),difference),2.f);
    else hue=add(div(sub(r,g),difference),4.f);
    hue=mul(hue,std::bit_cast<float>(0x3e2aaaabu));if(0.f>hue)hue=add(hue,1.f);
  }
  const auto s=std::bit_cast<float>(0x3e555555u),v=std::bit_cast<float>(0x3f4aaaabu);
  const auto h=mul(hue,6.f);auto sector=_mm_cvttss_si32(_mm_set_ss(h));
  if(integer(sector)>h)sector=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(sector)-1);
  const auto f=sub(h,integer(sector));const auto sf=mul(s,f);
  const auto q=mul(sub(1.f,sf),v);const auto p=mul(sub(1.f,s),v);
  const auto t=mul(sub(1.f,mul(sub(1.f,f),s)),v);
  std::array<float,3> result;
  switch(sector){case 0:result={v,t,p};break;case 1:result={q,v,p};break;case 2:result={p,v,t};break;
    case 3:result={p,q,v};break;case 4:result={t,p,v};break;default:result={v,p,q};break;}
  return std::bit_cast<VistaPcrRgb>(result);
}
#endif
} // namespace
std::array<VistaPcrRgb,4> vista_pcr_appearance_colors(std::array<std::byte,16> appearance,
    std::uint16_t team,std::uint8_t no_team,const VistaPcrAppearanceAssets& a) {
  std::array<VistaPcrRgb,4> result;
  for(unsigned i=0;i<4;++i)result[i]=palette(std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(appearance[i])),a);
  if(no_team)result[0]=result[1]=white;
  else if(team!=0xffff){result[0]=team_color(team,a);result[1]=palette(std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(appearance[0])),a);}
  return result;
}
VistaPcrRgb vista_pcr_profile_name_color(VistaPcrRgb rgb,const VistaLoadingFloatingPoint& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FpScope scope(fp);return adjusted(rgb);
#else
  (void)rgb;(void)fp;throw FormatError("Vista PCR appearance SSE backend unavailable");
#endif
}
VistaPcrAppearanceView vista_pcr_player_presentation(const VistaPcrResults& bank,unsigned row,
    const VistaPcrAppearanceAssets& a,VistaPcrAppearanceServices& service,const VistaLoadingFloatingPoint& fp) {
  if(row>=16 || row>=bank.player_count)throw FormatError("Vista PCR appearance row outside retained display bank");
  VistaPcrAppearanceView out;const auto& b=bank.players[row];std::array<std::byte,8> identity;
  std::copy_n(b.begin()+0x100,8,identity.begin());const auto query=service.identity_query(identity);
  if(query){const auto index=service.primary_player_query(identity);if(index!=0xffffffffu)(void)service.player_query(index);}
  const auto rank=std::bit_cast<std::int32_t>(word(b,0x78));
  if(a.rank_owner_present)for(unsigned i=0;i<a.rank_ranges.size();++i)
    if(rank>=a.rank_ranges[i].first && rank<=a.rank_ranges[i].last){out.rank_range=std::uint16_t(i);break;}
  out.descriptor_flags=0x87|(out.rank_range==0xffff?0u:0x40u)|(query?0u:0x200u);
  std::copy_n(b.begin()+0xd0,16,out.appearance.begin());out.team=std::uint16_t(word(b,0x72,2));out.no_team=std::to_integer<std::uint8_t>(b[0x74]);
  for(unsigned i=0;i<32;++i){const auto c=word(b,i*2,2);if(!c)break;out.name+=char16_t(c);}
  out.primary_color=vista_pcr_appearance_colors(out.appearance,out.team,out.no_team,a)[0];
  out.name_color=query?vista_pcr_profile_name_color(out.primary_color,fp):VistaPcrRgb{0x3e99999a,0x3e99999a,0x3e99999a};
  out.primary_visible=true;out.secondary_visible=false;out.rank_visible=out.rank_range!=0xffff;
  out.group_visible[0]=1;
  out.emblem_visible=out.rank_range!=0xffff || !(a.emblem_definition_flags&2);
  out.primary_frame=a.previous_primary_frame;out.rank_frame=a.previous_rank_frame;
  if(a.primary_bitmap_present && std::bit_cast<std::int32_t>(a.primary_page_count)>0)out.primary_frame=0;
  if(out.rank_visible && a.rank_bitmap_present && std::int32_t(out.rank_range)<std::bit_cast<std::int32_t>(a.rank_page_count))out.rank_frame=out.rank_range;
  return out;
}
} // namespace halo2
