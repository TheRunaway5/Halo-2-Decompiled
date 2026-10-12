#include "halo2/vista_offline_identity.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t a) {
  std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[a+i])<<(8*i);
  return v;
}
void put(std::span<std::byte> b,std::size_t a,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[a+i]=std::byte(v>>(8*i));
}
std::uint8_t rol(std::uint8_t v,unsigned n) {
  return std::uint8_t((unsigned(v)<<n)|(unsigned(v)>>(8-n)));
}
std::uint8_t ror(std::uint8_t v,unsigned n) {return rol(v,8-n);}
// Literal byte-register operations1059992C..10599A24. Keeping these separate
// avoids changing x86 byte wrap/alias semantics into32-bit arithmetic.
std::array<std::byte,4> scramble(std::uint32_t v) {
  auto cl=std::uint8_t(v),dl=std::uint8_t(v>>8);
  auto al=std::uint8_t(v>>16),ah=std::uint8_t(v>>24);
  dl^=0x45;ah^=0x42;al^=0x59;al+=ah;cl^=0x4b;cl+=dl;
  ah=ror(ah,3);ah^=al;dl=rol(dl,2);dl^=cl;cl=rol(cl,4);cl+=ah;
  ah=ror(ah,1);const auto first=cl;cl=dl;ah^=first;cl+=al;
  auto retained=std::uint8_t(first^0x59);
  ah^=0x31;dl=rol(dl,3);dl^=cl;cl=rol(cl,4);dl^=0x54;
  cl^=0x45;retained+=dl;cl+=ah;dl=rol(dl,2);dl^=retained;
  retained=rol(retained,4);al=dl;al+=cl;ah=ror(ah,3);ah^=cl;
  dl=rol(dl,3);retained+=ah;dl^=al;cl=retained;dl^=0x33;
  ah=ror(ah,1);ah^=cl;al=rol(al,4);cl^=0x32;ah^=0x35;
  cl+=dl;al^=0x34;al+=ah;dl=rol(dl,2);dl^=cl;ah=ror(ah,3);
  ah^=al;cl=rol(cl,4);cl+=ah;ah=ror(ah,1);retained=cl;
  cl=dl;ah^=retained;cl+=al;al=retained;ah^=0x39;dl=rol(dl,3);
  al^=0x36;dl^=cl;cl=rol(cl,4);dl^=0x37;cl^=0x38;
  return {std::byte(al),std::byte(dl),std::byte(cl),std::byte(ah)};
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789ABCDEF";
  std::string s; s.reserve(bytes.size()*2);
  for(auto b:bytes) {
    auto n=std::to_integer<unsigned>(b);s+=digits[n>>4];s+=digits[n&15];
  }
  return s;
}
void unhex(std::span<std::byte> out,std::string_view text) {
  const auto nibble=[](char c) {return unsigned(c<='9'?c-'0':c-'A'+10);};
  for(std::size_t i=0;i<out.size();++i)
    out[i]=std::byte((nibble(text[2*i])<<4)|nibble(text[2*i+1]));
}
void machine_configure(VistaOfflineIdentityState& state,std::uint32_t lan,
    std::uint16_t port,std::string_view enet,std::string_view online,
    VistaOfflineIdentityServices& services) {
  auto& m=state.machine;std::fill(m.begin(),m.end(),std::byte{});
  auto address=services.interface_ipv4();put(m,4,address);
  constexpr std::array<std::string_view,8> ip_strings={
    "192.168.0.0","255.255.0.0","10.0.0.0","255.0.0.0",
    "172.16.0.0","255.240.0.0","127.0.0.0","255.0.0.0"};
  std::array<std::uint32_t,8> values;
  for(unsigned i=0;i<8;++i)values[i]=services.inet_addr(ip_strings[i]);
  bool admitted=false;
  for(unsigned i=0;i<8;i+=2)
    if((address&values[i+1])==(values[i]&values[i+1])) {admitted=true;break;}
  if(!admitted)address=services.inet_addr("127.0.0.1");
  if(lan)address=lan;
  put(m,4,address);
  // The offline caller's external IPv4 is literal0, not its XUID.
  put(m,0x28,0);const auto transformed=scramble(0);
  std::copy(transformed.begin(),transformed.end(),m.begin()+8);
  m[0xc]=std::byte(port>>8);m[0xd]=std::byte(port);
  unhex(std::span(m).subspan(0xe,6),enet);
  unhex(std::span(m).subspan(0x14,20),online);
  services.initialize_machine_context(std::span(m).subspan(0x90));
  m[0x30]=std::byte{1};
}
} // namespace

bool vista_offline_identity_configure(VistaOfflineIdentityState& state,
    VistaOfflineIdentityScratch& scratch,std::uint32_t lan,std::uint16_t port,
    VistaOfflineIdentityServices& services) {
  services.random_bytes(scratch.enet);services.random_bytes(scratch.online);
  //10527CC0 encodes online first and enet second, before the actual CRT call.
  const auto online=hex(scratch.online),enet=hex(scratch.enet);
  const auto value=services.crt_rand();const auto low=std::uint32_t(value);
  if(!low)return false;
  put(state.user_info,0xc,1);put(state.user_info,0,low);
  put(state.user_info,4,value<0?0xffffffffu:0u);
  put(state.user_info,0x10,0);put(state.user_info,0x14,0);
  machine_configure(state,lan,port,enet,online,services);
  services.master_status_changed(1);
  state.login_token="12345678901234567890123456789012";
  return true;
}
std::uint32_t vista_offline_identity_xuid(std::span<const std::byte> b,
    std::array<std::byte,8>& output) {
  if(b.size()<0x28)throw std::invalid_argument("Truncated supplied-DLL user bank");
  output.fill(std::byte{});const auto state=word(b,0xc);
  if(state!=1 && state!=2)return 0x4dd;
  std::copy_n(b.begin(),8,output.begin());return 0;
}
std::uint32_t vista_offline_identity_machine(std::span<const std::byte> b,
    std::array<std::byte,36>& output) {
  if(b.size()<0x2a8)throw std::invalid_argument("Truncated supplied-DLL machine bank");
  output.fill(std::byte{});if(b[0x30]==std::byte{})return 0;
  std::copy_n(b.begin()+4,36,output.begin());return 2;
}
void vista_xnet_random(VistaXnetRandomState& state,std::span<std::byte> bytes,
    const std::function<std::uint64_t()>& counter) {
  if(!state.initialized) {
    if(!counter)throw std::invalid_argument("Actual performance-counter owner required");
    const auto seed=counter();state.i=0;state.j=0;
    for(unsigned i=0;i<256;++i)state.permutation[i]=std::uint8_t(i);
    std::uint8_t j=0;
    for(unsigned i=0;i<256;++i) {
      j+=state.permutation[i];j+=std::uint8_t(seed>>(8*(i&7)));
      std::swap(state.permutation[i],state.permutation[j]);
    }
    state.initialized=true;
  }
  for(auto& b:bytes) {
    ++state.i;state.j+=state.permutation[state.i];
    std::swap(state.permutation[state.i],state.permutation[state.j]);
    b^=std::byte(state.permutation[std::uint8_t(
        state.permutation[state.i]+state.permutation[state.j])]);
  }
}
std::int32_t vista_crt_rand(std::uint32_t& state) noexcept {
  state=state*0x343fdu+0x269ec3u;return std::int32_t((state>>16)&0x7fff);
}
} // namespace halo2
