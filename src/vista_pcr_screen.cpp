#include "halo2/vista_pcr_screen.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <utility>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) invoke(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista PCR widget/service owner");
  return f(std::forward<A>(a)...);
}
std::uint32_t word(std::span<const std::byte> b,unsigned at,unsigned n=4) {
  if(at>b.size() || n>b.size()-at)throw FormatError("Vista PCR row address outside display bank");
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[at+i]))<<(8*i);return v;
}
std::u16string string(std::span<const std::byte> b,unsigned at,unsigned capacity) {
  std::u16string out;for(unsigned i=0;i<capacity;++i) {
    const auto c=word(b,at+i*2,2);if(!c)break;out.push_back(char16_t(c));
  }return out;
}
void append(std::u16string& out,std::u16string_view value) {
  //44BCD5 walks the existing terminator, then reserves one of512 units.
  if(out.size()<511)out.append(value.substr(0,511-out.size()));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class FpScope {
  std::fenv_t saved;unsigned mxcsr;
public:
  explicit FpScope(const VistaLoadingFloatingPoint& fp):mxcsr(_mm_getcsr()) {
    if((fp.x87_control_word&0x3f)!=0x3f || (fp.x87_control_word&0x300)==0x100 ||
       (fp.mxcsr&0x1f80)!=0x1f80 || (fp.mxcsr&0xffff0000))
      throw FormatError("Unsupported Vista PCR floating point context");
    if(std::fegetenv(&saved))throw FormatError("Vista PCR floating point environment unavailable");
    _mm_setcsr(fp.mxcsr);asm volatile("fldcw %0"::"m"(fp.x87_control_word):"memory");
  }
  ~FpScope(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
};
std::int32_t accuracy(std::int32_t hits,std::int32_t shots,const VistaLoadingFloatingPoint& fp) {
  if(shots<=0)return 0;
  FpScope scope(fp);std::int32_t out;const float hundred=100.f;
  asm volatile("cvtsi2ssl %1,%%xmm0; mulss %3,%%xmm0; cvtsi2ssl %2,%%xmm1; divss %%xmm1,%%xmm0; cvttss2sil %%xmm0,%0"
      :"=r"(out):"rm"(hits),"rm"(shots),"m"(hundred):"xmm0","xmm1","memory");return out;
}
std::uint32_t fade(std::int32_t elapsed,const VistaLoadingFloatingPoint& fp) {
  FpScope scope(fp);std::int32_t out;float spill;
  const float one=1.f,scale=std::bit_cast<float>(0x3a03126fu),alpha=255.f;
  asm volatile("cvtsi2ssl %2,%%xmm0; mulss %4,%%xmm0; movss %3,%%xmm1; subss %%xmm0,%%xmm1; mulss %5,%%xmm1; movss %%xmm1,%1; flds %1; fistpl %0"
      :"=m"(out),"=m"(spill):"rm"(elapsed),"m"(one),"m"(scale),"m"(alpha):"xmm0","xmm1","st","memory");
  return std::bit_cast<std::uint32_t>(out)<<24;
}
#else
std::int32_t accuracy(std::int32_t,std::int32_t,const VistaLoadingFloatingPoint&) {
  throw FormatError("Vista PCR SSE backend unavailable");
}
std::uint32_t fade(std::int32_t,const VistaLoadingFloatingPoint&) {
  throw FormatError("Vista PCR x87/SSE backend unavailable");
}
#endif
} // namespace
void vista_pcr_bind_row(VistaPcrPane pane,unsigned row,const VistaPcrResults& bank,
    const VistaPcrRowServices& s,const VistaLoadingFloatingPoint& fp) {
  if(unsigned(pane)>unsigned(VistaPcrPane::weapons))throw FormatError("Invalid Vista PCR pane");
  const bool team=pane==VistaPcrPane::teams;
  if(row>=16 || row>=(team?bank.team_count:bank.player_count))throw FormatError("Invalid Vista PCR displayed row");
  std::array<bool,5> children{};bool bitmap{};
  if(team) {
    for(unsigned i=0;i<3;++i)children[i]=invoke(s.text_child,i);
    bitmap=invoke(s.bitmap_child,0u);
  } else {
    if(!invoke(s.grid_child,0u))throw FormatError("Missing Vista PCR player row grid");
    const unsigned count=pane==VistaPcrPane::opponents||pane==VistaPcrPane::medals?3:5;
    if(pane==VistaPcrPane::summary)for(auto i:{0u,1u,4u,2u,3u})children[i]=invoke(s.text_child,i);
    else for(unsigned i=0;i<count;++i)children[i]=invoke(s.text_child,i);
  }
  auto set=[&](unsigned i,std::u16string_view value){invoke(s.text,i,value);};
  auto fmt=[&](std::u16string_view format,std::initializer_list<std::int32_t> values) {
    return invoke(s.format,format,std::span<const std::int32_t>(values.begin(),values.size()));
  };
  if(team) {
    const auto& b=bank.teams[row];const auto color=invoke(s.team_color,std::uint16_t(word(b,0x62,2)));
    for(auto [i,at,cap]:{std::array<unsigned,3>{0,0,32},{1,0x64,16},{2,0x40,16}})if(children[i]) {
      set(i,string(b,at,cap));invoke(s.text_color,i,color.text);
    }
    if(bitmap){invoke(s.bitmap_color,0u,color.bitmap);invoke(s.visible,0u,true);}return;
  }
  unsigned selected{};
  if(pane==VistaPcrPane::opponents) {
    if(invoke(s.selected_row)!=0xffffffffu)selected=invoke(s.selected_row)&0xffff;
    if(selected>=16)throw FormatError("Vista PCR opponent selection outside retained bank");
  }
  invoke(s.appearance,row);const auto& b=bank.players[row];
  if(children[0])set(0,string(b,0,32));
  auto integer=[&](unsigned i,unsigned offset){if(children[i])set(i,fmt(u"%d",{std::bit_cast<std::int32_t>(word(b,offset))}));};
  switch(pane) {
  case VistaPcrPane::teams:break;
  case VistaPcrPane::summary:
    if(children[1])set(1,string(b,0xe0,16));
    if(children[4])set(4,string(b,0x40,16));
    if(children[2]&&children[3]) {
      std::int32_t first{},second{};std::u16string a;
      if(bank.variant==2) {
        first=std::bit_cast<std::int32_t>(word(b,0x108));second=std::bit_cast<std::int32_t>(word(b,0x10c));
        a=fmt(invoke(s.localized,0x0b000734u),{first/60,first%60});
      } else {
        if(bank.variant==1||bank.variant==3||bank.variant==4||bank.variant==7||bank.variant==8||bank.variant==9) {
          first=std::bit_cast<std::int32_t>(word(b,0x108));second=std::bit_cast<std::int32_t>(word(b,0x10c));
        }a=fmt(u"%d",{first});
      }
      const auto c=fmt(u"%d",{second});set(2,a);set(3,c);
    }break;
  case VistaPcrPane::numeric:
    integer(1,0x60);integer(2,0x68);integer(3,0x64);integer(4,0x6c);break;
  case VistaPcrPane::opponents:
    if(children[1])set(1,fmt(u"%d",{std::bit_cast<std::int32_t>(word(bank.players[selected],0x90+row*4))}));
    integer(2,0x90+selected*4);break;
  case VistaPcrPane::medals: {
    integer(1,0x7c);if(!children[2])break;
    static constexpr std::array<std::uint32_t,24> names={0xf00073f,0xf000740,0xf000741,0xf000742,0xf000743,0xf000744,0xb000745,0xe000746,0x9000747,0xc000748,0xe000749,0xf00074a,0xb00074b,0x1000074c,0x1100074d,0x1100074e,0x1100074f,0x11000750,0x9000751,0x11000752,0xb000753,0xa000754,0x11000755,0xd000756};
    std::u16string value;unsigned count{};const auto mask=word(b,0x80);
    for(int i=23;i>=0&&count<8;--i)if(mask&(1u<<i)) {
      if(count)append(value,u" ");
      append(value,invoke(s.localized,names[i]));++count;
    }
    if(count){set(2,value);invoke(s.visible,2u,true);}else invoke(s.visible,2u,false);
    break;
  }
  case VistaPcrPane::weapons:
    integer(1,0x88);integer(2,0x84);
    if(children[3]) {
      const auto value=accuracy(std::bit_cast<std::int32_t>(word(b,0x88)),std::bit_cast<std::int32_t>(word(b,0x84)),fp);
      set(3,fmt(invoke(s.localized,0x11000757u),{value}));
    }integer(4,0x8c);break;
  }
}
std::array<std::optional<std::uint32_t>,5> vista_pcr_headings(VistaPcrPane pane,std::uint32_t variant) {
  using Heading=std::array<std::optional<std::uint32_t>,5>;
  switch(pane) {
  case VistaPcrPane::teams:return Heading{0x04000279,0x05000731,0x05000733,{}, {}};
  case VistaPcrPane::summary:{
    Heading out{0x06000732,0x05000731,{}, {},0x05000733};
    std::optional<std::array<std::uint32_t,2>> columns;
    switch(variant){case 2:columns={{0x08000721,0x0a000722}};break;
      case 1:columns={{0x0a000723,0x0c000724}};break;case 3:columns={{0x0d000728,0x0f000729}};break;
      case 4:columns={{0x0d000726,0x0f000727}};break;case 7:columns={{0x0c00072b,0x0d00072c}};break;
      case 8:columns={{0x0b00072d,0x0a00072e}};break;case 9:columns={{0x0a00072f,0x0a000730}};break;default:break;}
    if(columns){out[2]=(*columns)[0];out[3]=(*columns)[1];}return out;
  }
  case VistaPcrPane::numeric:return Heading{0x06000732,0x05000737,0x07000735,0x06000736,0x0800073a};
  case VistaPcrPane::opponents:return Heading{0x06000732,0x0a000738,0x0d000739,{}, {}};
  case VistaPcrPane::medals:return Heading{0x06000732,0x0c00073d,0x0a00073e,{}, {}};
  case VistaPcrPane::weapons:return Heading{0x06000732,0x09000758,0x0b000759,0x0e00075a,0x0a00075b};
  }
  throw FormatError("Invalid Vista PCR pane");
}
std::u16string vista_pcr_footer(VistaPcrPane pane,std::uint8_t lobby,std::uint8_t host,
    std::uint8_t details,const std::function<std::u16string(std::uint32_t)>& localized) {
  if(unsigned(pane)>5)throw FormatError("Invalid Vista PCR pane");
  auto result=invoke(localized,host||lobby?0x06000763u:0x0d000764u);
  if(details && pane!=VistaPcrPane::teams) {
    auto prefix=invoke(localized,0x13000906u);append(prefix,result);result=std::move(prefix);
  }
  return result;
}
VistaPcrVoicePresentation vista_pcr_voice(const VistaPcrResults& bank,unsigned row,
    std::uint8_t lobby,const VistaPcrVoiceServices& services) {
  if(row>=bank.player_count || row>=16)throw FormatError("Invalid Vista PCR voice row");
  if(lobby)return {};
  std::array<std::byte,8> identity;std::copy_n(bank.players[row].begin()+0x100,8,identity.begin());
  const auto slot=invoke(services.identity_slot,identity);
  if(slot==0xffffffffu)return {};
  if(!invoke(services.enabled,slot))return {true,2};
  return {true,invoke(services.talking,slot)?0u:1u};
}
bool vista_pcr_input(std::uint8_t lobby,std::uint32_t code,std::uint32_t controller,const VistaPcrInputServices& s) {
  const auto host=invoke(s.host);
  if(code==3) {
    if(!invoke(s.details_available))return false;
    std::array<std::uint32_t,2> identity{};
    if(invoke(s.selected_identity,identity))invoke(s.player_details,identity);
    return true;
  }
  if(code!=1&&code!=13)return false;
  if(lobby){invoke(s.close,3u);return true;}
  if(host){if(invoke(s.host))invoke(s.continue_session);return true;}
  invoke(s.dialog,std::array<std::uint32_t,8>{3,0x98,4,1u<<(controller&31),0x40c00d,0,0,0});return true;
}
void vista_pcr_draw(std::uint8_t lobby,std::uint32_t creation,const VistaPcrDrawServices& s,const VistaLoadingFloatingPoint& fp) {
  if(!lobby) {
    const auto elapsed=std::bit_cast<std::int32_t>(invoke(s.elapsed,creation));
    const auto argb=elapsed<=2000?0xff000000u:fade(std::clamp(elapsed-2000,0,2000),fp);
    invoke(s.black,argb);
  }invoke(s.base_draw);
}
} // namespace halo2
