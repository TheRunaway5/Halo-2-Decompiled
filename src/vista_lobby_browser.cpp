#include "halo2/vista_lobby_browser.hpp"
#include "halo2/cache.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t at) {
  if(b.size()<0x14)throw FormatError("Vista lobby browser requires its actual retained query fields");
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(i*8);return v;
}
}
VistaLobbyBrowserDialog vista_lobby_browser_dialog(std::span<const std::byte> b) {
  const auto pending=get(b,8);
  if(pending==1 || pending==7)return VistaLobbyBrowserDialog::notification106;
  if(pending==2)return VistaLobbyBrowserDialog::question107;
  if(pending==3 || pending==4 || pending==5)return VistaLobbyBrowserDialog::notification108;
  return VistaLobbyBrowserDialog::none;
}
void vista_lobby_browser_acknowledge(std::span<std::byte> b) {
  if(get(b,4)==0)for(unsigned i=0;i<4;++i)b[8+i]=std::byte{};
}
std::uint32_t vista_lobby_browser_percent(std::span<const std::byte> b,std::uint32_t received) {
  const auto state=get(b,4);if(state==0)return 100;
  if(std::bit_cast<std::int32_t>(state)>6)return 95;
  const auto total=get(b,0x10);if(total==0)return 0;
  const auto extended=std::uint64_t(std::int64_t(std::bit_cast<std::int32_t>(received)));
  return std::uint32_t((extended*95u)/total);
}
void vista_lobby_browser_update(std::span<std::byte> b,std::uint32_t received,
    bool has_text,const VistaLobbyBrowserServices& services) {
  const auto pending=get(b,8);
  const auto dialog=vista_lobby_browser_dialog(b);
  if(dialog!=VistaLobbyBrowserDialog::none) {
    if(!services.dialog)throw FormatError("Vista lobby browser requires its consumed dialog service");
    services.dialog(dialog);
  }
  if(pending!=0xffffffffu && pending!=0)vista_lobby_browser_acknowledge(b);
  if(has_text) {
    const auto percent=vista_lobby_browser_percent(b,received);
    if(!services.progress_text)throw FormatError("Vista lobby browser requires its consumed progress widget service");
    services.progress_text(percent,(percent-1u)<=98u);
  }
}
} //namespace halo2
