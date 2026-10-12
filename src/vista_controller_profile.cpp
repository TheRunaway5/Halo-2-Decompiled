#include "halo2/vista_controller_profile.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <limits>

namespace halo2 {
namespace {
constexpr std::uint32_t absent=0xffffffffu;
std::uint32_t get(std::span<const std::byte> b,std::size_t o) noexcept {
  std::uint32_t v{};
  for(unsigned i=0;i<4;++i)v|=std::uint32_t(std::to_integer<unsigned char>(b[o+i]))<<(i*8);
  return v;
}
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v) noexcept {
  for(unsigned i=0;i<4;++i)b[o+i]=std::byte(v>>(i*8));
}
bool overlap(std::span<const std::byte> a,std::span<const std::byte> b) noexcept {
  const auto x=reinterpret_cast<std::uintptr_t>(a.data());
  const auto y=reinterpret_cast<std::uintptr_t>(b.data());
  return x<=y ? y-x<a.size() : x-y<b.size();
}
std::span<std::byte> admit(VistaControllerProfileBanks b,std::uint32_t c) {
  if(c>=vista_local_user_count || b.controllers.size()<vista_controller_user_bank_size ||
      b.local_users.size()<vista_local_user_bank_size ||
      b.input_controls.size()<vista_input_control_bank_size)
    throw FormatError("Vista controller/profile owner bank is truncated or controller is out of range");
  if(overlap(b.controllers,b.local_users) || overlap(b.controllers,b.input_controls) ||
      overlap(b.local_users,b.input_controls))
    throw FormatError("Vista controller/profile owner banks overlap");
  auto r=b.controllers.subspan(c*vista_controller_user_stride,vista_controller_user_stride);
  const auto slot=get(r,4);
  if(slot!=absent && slot>=vista_local_user_count)
    throw FormatError("Vista controller/profile mapping is outside the bounded user bank");
  return r;
}
void capacity(std::span<const std::byte> users) {
  for(std::size_t i=0;i<vista_local_user_count;++i)
    if(users[i*vista_local_user_stride]==std::byte{})return;
  throw FormatError("Vista controller/profile assignment would underflow a full original user bank");
}
void wide_copy(std::span<std::byte> dest,std::span<const std::byte> source) noexcept {
  // 44BD27/687AB5 with size32,countFFFFFFFF. Copy through first UTF-16 NUL,
  // preserve its tail, or truncate an overlong input at code unit31.
  for(std::size_t i=0;i<32;++i) {
    dest[2*i]=source[2*i];dest[2*i+1]=source[2*i+1];
    if(source[2*i]==std::byte{} && source[2*i+1]==std::byte{})return;
  }
  dest[62]=dest[63]=std::byte{};
}
VistaLocalUserIdentity identity(std::span<const std::byte> r,std::size_t o) noexcept {
  VistaLocalUserIdentity out{};std::copy_n(r.begin()+o,out.size(),out.begin());return out;
}
VistaLocalUserPayload payload(VistaControllerProfileBanks b,std::span<const std::byte> r) {
  VistaLocalUserPayload p{};p[0x7e]=p[0x7f]=std::byte{0xff}; // 452CAC
  const auto slot=get(r,4);
  if(slot<4)vista_local_user_read_payload(b.local_users,slot,{nullptr,&p,nullptr,nullptr});
  wide_copy(std::span(p).first(0x40),r.subspan(0x123c,0x40));
  for(std::size_t i=0;i<4;++i)put(p,0x40+i*4,get(r,0x115c+i*4));
  p[0x7d]=r[0x1224];p[0x80]=r[0x122c];p[0x81]=r[0x1228];p[0x82]=r[0x1230];
  const auto team=std::bit_cast<std::int32_t>(get(r,0x1220));
  p[0x7c]=std::byte(team==-1 ? 255 : std::clamp(team,0,16));
  return p;
}
void publish(VistaControllerProfileBanks b,std::uint32_t c,std::span<std::byte> r,
    VistaControllerProfileServices& s) {
  const auto context=s.presentation_context();
  if(context.type_word==2) {
    // 5AC4C0..5AC4C3 is an unchanged zero-returning original stub.
    const auto formatting_context=s.presentation_context();
    s.format_network_display_name(r.subspan(0x123c,0x40),formatting_context.network_text);
  } else if(get(r,0)&8)wide_copy(r.subspan(0x123c,0x40),r.subspan(0x1c,0x40));
  else r[0x123c]=r[0x123d]=std::byte{};
  const auto slot=get(r,4);
  if(slot==absent)return;
  const auto p=payload(b,r);
  auto old_identity=identity(r,8);
  // 606A97 consumes this read but then only publishes the payload. Its result
  // and identity value do not replace the user identity or gate publication.
  vista_local_user_read_identity(b.local_users,slot,old_identity);
  vista_local_user_write_payload(b.local_users,slot,c,p,0,0);
}
void expand(std::span<std::byte> controls,std::span<const std::byte> profile) noexcept {
  std::fill_n(controls.begin()+0x14,0x1644,std::byte{0xff}); // 461AD0
  for(std::size_t i=0;i<0x39;++i)
    std::copy_n(profile.begin()+0xc+i*0x40,0x40,controls.begin()+0x14+i*0x64);
}
void contract(std::span<std::byte> profile,std::span<const std::byte> controls) noexcept {
  std::fill_n(profile.begin()+0xc,0xe40,std::byte{0xff}); // 461B62
  for(std::size_t i=0;i<0x39;++i)
    std::copy_n(controls.begin()+0x14+i*0x64,0x40,profile.begin()+0xc+i*0x40);
}
std::uint8_t input_publish(VistaControllerProfileBanks b,std::uint32_t c,
    std::span<std::byte> r,std::uint32_t argument,VistaControllerProfileServices& s) {
  std::array<std::byte,vista_input_control_stride> control{};
  const auto stored=b.input_controls.subspan(c*vista_input_control_stride,vista_input_control_stride);
  std::copy(stored.begin(),stored.end(),control.begin()); // 461BF4
  auto profile=r.subspan(0x300,0xe4c);
  expand(control,profile);contract(profile,control);
  const auto byte=[&](std::size_t o){return std::to_integer<unsigned>(profile[o]);};
  const VistaInputBindingArguments args{(argument&0xffff0000u)|byte(5),byte(4),byte(8),argument};
  const auto result=s.apply_input_bindings(control,args);
  if(byte(5)==4 || byte(4)==4 || byte(8)==4) {expand(control,profile);contract(profile,control);}
  const auto horizontal=std::clamp(byte(6),1u,10u)-1;
  const auto vertical=std::clamp(byte(7),1u,10u)-1;
  // Literal tables at7CB75C/7CB784 and subtraction constants7CB7B0/7CB7AC.
  // All table values and differences are exact binary32 integers; this form
  // retains their bits under every original MXCSR rounding mode.
  put(control,0,std::bit_cast<std::uint32_t>(float(80+20*vertical-30)));
  put(control,4,std::bit_cast<std::uint32_t>(float(40+10*vertical-15)));
  put(control,8,std::bit_cast<std::uint32_t>(float(80+20*horizontal)));
  put(control,0xc,std::bit_cast<std::uint32_t>(float(40+10*horizontal)));
  const auto flags=get(profile,0);
  control[0x10]=std::byte(flags&1);control[0x11]=std::byte((flags>>1)&1);
  control[0x12]=std::byte((flags>>3)&1);control[0x1670]=std::byte((flags>>6)&1);
  std::copy(control.begin(),control.end(),stored.begin()); // 461C17
  s.refresh_input_dependencies(control,c);
  return result;
}
} // namespace

std::uint8_t vista_controller_profile_assign(VistaControllerProfileBanks b,std::uint32_t c,
    VistaPcProfile profile,std::uint32_t index,std::uint32_t argument,
    VistaControllerProfileServices& s) {
  auto r=admit(b,c);
  const bool first=(get(r,0)&8)==0;
  if(first)capacity(b.local_users);
  const bool changed=first || get(r,0x121c)!=index;
  if(first) {
    std::fill_n(r.begin()+8,8,std::byte{});
    const auto xuid=s.xuser_get_xuid(c);
    if(xuid.hresult_word==0)std::copy(xuid.output.begin(),xuid.output.end(),r.begin()+8);
    put(r,4,vista_local_user_allocate(b.local_users,identity(r,8)));
    put(r,0x121c,absent);
  }
  if(changed && get(r,0x121c)!=absent)s.release_profile(get(r,0x121c));
  std::copy(profile.begin(),profile.end(),r.begin()+0x14);
  put(r,0,get(r,0)|8);put(r,0x121c,index);
  const auto result=input_publish(b,c,r,argument,s);
  publish(b,c,r,s);
  if(changed && get(r,0x121c)!=absent)s.acquire_profile(get(r,0x121c));
  return result;
}

void vista_controller_profile_publish(VistaControllerProfileBanks b,std::uint32_t c,
    VistaControllerProfileServices& s) {auto r=admit(b,c);publish(b,c,r,s);}

void vista_controller_profile_remove(VistaControllerProfileBanks b,std::uint32_t c,
    VistaControllerProfileServices& s) {
  auto r=admit(b,c);
  put(r,0,get(r,0)&~0x10u);put(r,0x1228,0);r[0x122c]=std::byte{};
  std::fill_n(r.begin()+0x1234,8,std::byte{});
  std::copy_n(r.begin()+0x1c,0x40,r.begin()+0x123c);
  if(get(r,4)!=absent) {vista_local_user_remove(b.local_users,get(r,4));put(r,4,absent);}
  if(get(r,0)&8) {
    if(get(r,0x121c)!=absent)s.release_profile(get(r,0x121c));
    std::fill_n(r.begin()+0x14,0x1208,std::byte{});
    put(r,0,get(r,0)&~8u);put(r,0x121c,absent);
  }
  put(r,0,0);put(r,4,absent);std::fill_n(r.begin()+8,8,std::byte{});
  put(r,0x10,absent);put(r,0x1220,0);put(r,0x1224,0);
  r[0x123c]=r[0x123d]=std::byte{};
}

void vista_controller_identity_update(VistaControllerProfileBanks b,std::uint32_t c,
    VistaLocalUserIdentity value,VistaControllerProfileServices& s) {
  auto r=admit(b,c);
  if(get(r,4)==absent)capacity(b.local_users);
  std::copy(value.begin(),value.end(),r.begin()+0x1234);
  publish(b,c,r,s);
  if(get(r,4)==absent)put(r,4,vista_local_user_allocate(b.local_users,identity(r,8)));
  std::copy(value.begin(),value.end(),r.begin()+8);
  vista_local_user_write_identity(b.local_users,get(r,4),value);
}
void vista_controller_team_write(std::span<std::byte> controllers,std::uint32_t c,
    std::uint32_t team) {
  if(c>=vista_local_user_count || controllers.size()<vista_controller_user_bank_size)
    throw FormatError("Vista controller team bank is truncated or controller is out of range");
  put(controllers,c*vista_controller_user_stride+0x1220,team);
}
} // namespace halo2
