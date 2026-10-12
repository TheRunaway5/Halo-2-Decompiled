#include "halo2/vista_mainmenu_network.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
namespace {
struct ChildDefinition {
  std::size_t size;
  std::uint32_t vtable,flags,value_a,value_b;
};
// Actual 44A82E registration order. Constructor flags at +6 are retained;
// reset only changes +4/+5 and slot17's separate eight-byte counter owner.
constexpr std::array<ChildDefinition,41> definitions{{
  {16,0x7bcc68,0,0x69,1},{16,0x7bcc68,0,0x12d,1},
  {16,0x7bcc68,0,0x131,1},{16,0x7bcc68,0,0x191,1},
  {16,0x7bcc68,0,0x195,1},{16,0x7bcc68,0,0x1f5,1},
  {16,0x7bcc68,0,0x1f9,1},{16,0x7bcc68,0,0x259,1},
  {16,0x7bcc68,0,0x25d,1},{16,0x7bcc68,0,0x2bd,1},
  {16,0x7bcc68,0,0x2c1,1},{16,0x7bcc68,0,0x321,1},
  {16,0x7bcc68,0,0x325,1},{16,0x7bcbdc,0,1,0xffffffff},
  {16,0x7bcbdc,0,2,0xffffffff},{16,0x7bcbdc,0,3,1},
  {8,0x7a0e94,0,0,0},{24,0x7bcc58,0,7,50},
  {16,0x7bcc2c,0,0xffffffff,1},{8,0x7a0e74,0,0,0},
  {12,0x7bcc1c,0x10000,0x29,0},{16,0x7bcc0c,0x10000,5,0},
  {8,0x7a0e84,0x10000,0,0},{12,0x7bcbcc,0x10000,0xe,0},
  {12,0x7bcbcc,0x10000,0xf,0},{16,0x7bcc0c,0x10000,1,0},
  {16,0x7bcc0c,0x10000,1,1},{16,0x7bcc0c,0x10000,1,2},
  {16,0x7bcc0c,0x10000,1,3},{16,0x7bcc0c,0x10000,1,4},
  {12,0x7bcc1c,0x10000,0x2a,0},{12,0x7bcc1c,0x10000,0x23,0},
  {12,0x7bcc1c,0x10000,0x24,0},{12,0x7bcc1c,0x10000,0x2b,0},
  {16,0x7bcc0c,0x10000,5,1},{16,0x7bcc0c,0x10000,1,6},
  {16,0x7bcc0c,0x10000,5,2},{12,0x7bcbfc,0x10000,8,0},
  {16,0x7bcbec,0x10000,0,5},{12,0x7bcbbc,0x10000,0,0},
  {12,0x7bcbbc,0x10000,1,0}
}};
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
}

VistaMainmenuGameState::VistaMainmenuGameState() {
  for(std::size_t i=0;i<children_.size();++i) {
    const auto& d=definitions[i];auto& bytes=children_[i];
    put(bytes,0,d.vtable);put(bytes,4,d.flags);
    if(d.size>=12)put(bytes,8,d.value_a);
    if(d.size>=16)put(bytes,12,d.value_b);
  }
}
std::span<std::byte> VistaMainmenuGameState::child(std::size_t i) {
  if(i>=children_.size())throw FormatError("Vista game-state child index is out of range");
  return std::span(children_[i]).first(definitions[i].size);
}
std::span<const std::byte> VistaMainmenuGameState::child(std::size_t i) const {
  if(i>=children_.size())throw FormatError("Vista game-state child index is out of range");
  return std::span(children_[i]).first(definitions[i].size);
}
void VistaMainmenuGameState::reset() {
  std::array<VistaGameStateResetChild,41> children;
  for(std::size_t i=0;i<children.size();++i) {
    children[i]={child(i),[this,i](std::uint32_t value) {
      // 4C1827 is the only overridden +8 among these 41 actual vtables.
      // 4C11B4 stores the low argument byte for every category.
      if(i==17 && (value&0xff)==0)counter_.fill(std::byte{});
      children_[i][4]=std::byte(value);
    }};
  }
  vista_mainmenu_reset_game_state(root_,children);
}

void vista_mainmenu_reset_game_state(std::span<std::byte> root,
    std::span<const VistaGameStateResetChild> children) {
  if(root.size()<0xac || children.size()!=41)
    throw FormatError("Vista mainmenu reset requires its complete root and 41 child owners");
  for(const auto& child:children)
    if(child.storage.size()<6 || !child.reset)
      throw FormatError("Vista mainmenu reset requires each child's storage and virtual reset");
  for(const auto& child:children) {
    child.reset(0);
    child.storage[5]=std::byte{};
  }
  root[2]=root[3]=root[0]=root[1]=std::byte{};
}

VistaMainmenuNetworkResult vista_mainmenu_open_network(std::uint32_t controller,
    const VistaMainmenuNetworkServices& s) {
  if(!s.pbuffer_get_byte || !s.reset_game_state || !s.network_ready ||
      !s.special_ui || !s.controller_mask || !s.dialog ||
      !s.ui_signin_context || !s.create_screen || !s.request_profile_admission)
    throw FormatError("Vista Network requires actual PBuffer, game-state, readiness and admission owners");
  std::uint8_t byte{};
  s.pbuffer_get_byte(byte);
  if(byte)return VistaMainmenuNetworkResult::pbuffer_blocked;
  s.reset_game_state();
  const auto ready=s.network_ready();
  // The original queries a second time when the first result is zero. Both
  // results matter; the special-dialog branch wins over network readiness.
  const auto special=s.special_ui() || s.special_ui();
  if(special) {
    s.dialog({1,0x32,4,std::uint32_t{1}<<(controller&31),0,0});
    return VistaMainmenuNetworkResult::special_dialog;
  }
  if(!ready) {
    const auto mask=s.controller_mask();
    s.dialog({1,0x3b,4,mask,0,0});
    return VistaMainmenuNetworkResult::network_unavailable_dialog;
  }
  if(!s.ui_signin_context()) {
    // 40A463 resolves D712C8 a second time for the admission call's ECX.
    // The second resolution does not repeat the context test.
    (void)s.ui_signin_context();
    s.request_profile_admission(0,0x612e35);
    return VistaMainmenuNetworkResult::profile_admission_requested;
  }
  // 60B0BC clears +C; 60C226 writes both words and all remaining descriptor
  // fields. The source takes the low WORD of 1<<CL for the controller mask.
  const auto mask=(std::uint32_t{1}<<(controller&31))&0xffff;
  s.create_screen({mask<<16,5,4,0,0xffffffff,0xffffffff,0xffffffff,0x61a238});
  return VistaMainmenuNetworkResult::browser_created;
}
} // namespace halo2
