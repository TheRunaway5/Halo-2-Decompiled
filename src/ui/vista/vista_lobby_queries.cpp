#include "halo2/vista_lobby_queries.hpp"
#include "halo2/cache.hpp"
#include "halo2/vista_session_birth.hpp"
#include <bit>
#include <cfenv>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t at) {
  if(at>b.size() || b.size()-at<4)throw FormatError("Vista lobby query source field is truncated");
  std::uint32_t out{};
  for(unsigned i=0;i<4;++i)out|=std::uint32_t(std::to_integer<unsigned char>(b[at+i]))<<(8*i);
  return out;
}
std::int32_t integer(std::span<const std::byte> b,std::size_t at) {
  return std::bit_cast<std::int32_t>(word(b,at));
}
void complete(std::span<const std::byte> b) {
  if(b.size()<vista_session_source_size)
    throw FormatError("Vista lobby query requires the actual complete session bank");
}
VistaLobbyQuerySession current(const VistaLobbyQueryServices& s) {
  if(!s.current_session)throw FormatError("Vista lobby query has no actual current-session provider");
  auto b=s.current_session();if(b)complete(*b);return b;
}
void observed(const VistaLobbyQueryServices& s,VistaLobbyQueryCall call,std::uint32_t argument) {
  if(s.observed)s.observed(call,argument);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class FpScope {
  std::fenv_t saved_;unsigned mxcsr_;
 public:
  explicit FpScope(const VistaLoadingFloatingPoint& fp):mxcsr_(_mm_getcsr()) {
    if((fp.x87_control_word&~0xc00u)!=0x027fu || (fp.mxcsr&~0x6000u)!=0x1f80u)
      throw FormatError("Vista maximum initializer floating point context is unverified");
    if(std::fegetenv(&saved_))throw FormatError("Vista maximum initializer floating point environment unavailable");
    _mm_setcsr(fp.mxcsr);asm volatile("fldcw %0"::"m"(fp.x87_control_word):"memory");
  }
  ~FpScope(){std::fesetenv(&saved_);_mm_setcsr(mxcsr_);}
};
float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
std::int32_t fistp(float value) {
  std::int32_t out;asm volatile("flds %1; fistpl %0":"=m"(out):"m"(value):"st");return out;
}
#endif
}
std::uint32_t vista_lobby_status_string(std::int32_t readiness,const VistaLobbyStatusStringServices& s) {
  // Original616890 jump table, including repeated/default targets.
  constexpr std::array<std::uint32_t,27> values{
    0x0a0001e4,0x0b0001e5,0x0f000206,0x0f000207,0x0f000915,0x0a000209,0x1100020a,0,
    0,0,0x240001f0,0,0x0c0001f3,0x0e0001f4,0x210001f5,0x210001f5,0x230001f6,
    0x0f0001f8,0x0f0001f9,0x140001fa,0x190001fb,0x160001fc,0x0f0001fd,0x1c0001f7,
    0,0x0f000207,0x1a000208};
  const auto index=std::bit_cast<std::uint32_t>(readiness);
  if(index>=values.size())return 0;
  if(index!=7)return values[index];
  if(!s.selected_map_third_output)
    throw FormatError("Vista readiness7 status string requires its actual selected-map service");
  std::uint32_t third{};(void)s.selected_map_third_output(third);
  return third?0x190007b9u:0x190008c4u;
}
VistaLobbyQuerySession vista_lobby_query_current_session(std::span<const std::byte> b,std::uint8_t enabled) {
  if(!enabled)return {};
  complete(b);if(word(b,0x73a0)==0)return {};return b;
}
std::int32_t vista_lobby_player_count(const VistaLobbyQueryServices& s) {
  const auto b=current(s);if(!b)return 0;
  const auto state=word(*b,0x73a0);if(state<3 || state>8)return 0;
  observed(s,VistaLobbyQueryCall::membership_view,3);
  observed(s,VistaLobbyQueryCall::membership_generation,word(*b,0x70));
  return integer(*b,0x1254);
}
VistaLobbyMaximumParameters vista_lobby_maximum_parameters(std::span<const std::byte> b) {
  if(b.size()<0xa0)throw FormatError("Vista lobby maximum requires its actual retained global parameter bank");
  VistaLobbyMaximumParameters out;
  for(unsigned i=0;i<out.thresholds.size();++i)out.thresholds[i]=integer(b,i*4);
  out.minimum_capacity=integer(b,0x90);out.minimum_field_ac=integer(b,0x94);
  out.minimum_field_a8=integer(b,0x98);out.field_ac_margin=integer(b,0x9c);return out;
}
std::array<std::byte,0xa0> vista_lobby_maximum_parameters_initialize(const VistaLoadingFloatingPoint& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FpScope scope(fp);std::array<std::byte,0xa0> out{};
  const auto put=[&](unsigned at,std::uint32_t value) {
    for(unsigned j=0;j<4;++j)out[at+j]=std::byte(value>>(8*j));
  };
  // Literal7C35C8/7C60F8/7A03CC/7C60F4/7C60F0 float words.
  const auto value=[](std::uint32_t bits){return std::bit_cast<float>(bits);};
  for(std::uint32_t i=0;i<18;++i) {
    std::uint32_t first{},second{};
    if(i>1) {
      std::int32_t ebx=0x2000,ebp=0xa000;
      if(i>=4) {
        auto x=_mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),std::int32_t(i)-4));
        x=mul(x,value(0x3daaaaab));
        ebx=fistp(add(mul(x,value(0x46100000)),value(0x46000000)));
        x=add(mul(x,value(0x46f00000)),value(0x47200000));
        ebp=fistp(x);
      }
      first=(i-1)*std::bit_cast<std::uint32_t>(ebx)+0x3c00;
      second=(i-1)*std::bit_cast<std::uint32_t>(ebp)+0xf000;
    }
    put(i*4,first);put(0x48+i*4,second);
  }
  put(0x94,0x20000);put(0x98,0x20000);put(0x90,8);put(0x9c,0x2800);
  return out;
#else
  (void)fp;throw FormatError("Vista maximum initializer SSE/x87 backend unavailable");
#endif
}
std::int32_t vista_lobby_maximum(const VistaLobbyQueryServices& s,const VistaLobbyMaximumParameters& p) {
  const auto b=current(s);if(!b)return 0;
  observed(s,VistaLobbyQueryCall::membership_view,2);
  const auto initial=integer(*b,0x64);
  observed(s,VistaLobbyQueryCall::maximum_consumer,std::bit_cast<std::uint32_t>(initial));
  // Source5E275C accesses the initial peer before considering membership count.
  // Reject an unowned pointer/index rather than supply a fabricated capacity.
  if(initial< -1 || initial>16)throw FormatError("Vista maximum initial peer exceeds the actual membership owner");
  const auto count=integer(*b,0x84);
  if(count>17)throw FormatError("Vista maximum peer count exceeds the actual membership owner");
  std::int32_t field_ac=initial==-1?0:integer(*b,0x70+0xac+std::size_t(initial)*0x10c);
  const auto peer_mask=(std::uint32_t(1)<<(std::bit_cast<std::uint32_t>(count)&31u))-1u;
  for(std::int32_t i=0;i<count;++i) {
    const auto a=0x70+std::size_t(i)*0x10c;
    const auto candidate_a8=integer(*b,a+0xa8),candidate_ac=integer(*b,a+0xac);
    if(candidate_a8<p.minimum_field_a8 || candidate_ac<p.minimum_field_ac || word(*b,a+0xb4)!=peer_mask)continue;
    const auto threshold=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(field_ac)+
        std::bit_cast<std::uint32_t>(p.field_ac_margin));
    if(candidate_ac>=threshold)field_ac=candidate_ac;
  }
  std::int32_t capacity{};
  for(std::int32_t i=16;i>0;--i)if(field_ac>=p.thresholds[std::size_t(i)]){capacity=i;break;}
  return capacity<=p.minimum_capacity?p.minimum_capacity:capacity;
}
} // namespace halo2
