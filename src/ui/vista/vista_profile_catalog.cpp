#include "halo2/vista_profile_catalog.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A... a) {
  if(!f)throw FormatError("Missing selected-profile catalog service");
  return f(a...);
}
}
void vista_refresh_selected_profiles(std::span<std::byte> controllers,
    std::uint8_t& refresh_pending,const VistaProfileRefreshServices& s) {
  if(controllers.size()<4*0x127c)
    throw FormatError("Vista profile refresh requires all four actual controller records");
  const auto read=[&](std::size_t at) {
    std::uint32_t word{};for(unsigned i=0;i<4;++i)
      word|=std::to_integer<std::uint32_t>(controllers[at+i])<<(i*8);
    return word;
  };
  call(s.refresh_catalog);
  for(unsigned i=0;i<4;++i) {
    const auto at=i*0x127c;
    if(!(read(at)&8u) || read(at+0x121c)==0xffffffffu)continue;
    const auto index=call(s.remap_index,read(at+0x121c));
    if(index!=0xffffffffu) {
      for(unsigned j=0;j<4;++j)controllers[at+0x121c+j]=std::byte(index>>(j*8));
    } else {
      const std::array<std::uint32_t,6> descriptor{1,0xdd,4,call(s.controller_mask),0,0};
      call(s.missing_profile_dialog,descriptor);
    }
  }
  refresh_pending=0; //43E411.
}
} // namespace halo2
