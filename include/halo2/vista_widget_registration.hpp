#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
// Source32 identity in the caller's retained bank registry. Never a host pointer.
struct VistaWidgetIdentity {
  std::uint32_t value{};
  explicit operator bool()const noexcept{return value!=0;}
  friend bool operator==(VistaWidgetIdentity,VistaWidgetIdentity)=default;
};
struct VistaWidgetRecord {
  std::uint32_t source_identity{};
  std::span<const std::byte> bytes;
};
struct VistaWidgetPane {
  std::uint16_t animation{};
  std::int32_t button_count{};
  std::span<const VistaWidgetRecord> bitmaps,scenes,grids,texts,tables;
};
struct VistaWidgetScreen {
  std::uint32_t flags{};
  std::int32_t pane_count{};
  std::uint16_t shape{};
};
// Literal 610A44 descriptor; callers retain the words/tail and actual source32
// pointer-table identities. The ordinary {0,1,count,pointers,list,0} descriptor
// used by PC14 and PC206 is supported; its unused tail remains caller-owned.
struct VistaWidgetRegistration {
  std::array<std::uint32_t,6> words{};
  std::array<std::byte,0x50> tail{};
  std::span<const VistaWidgetIdentity> buttons;
};
struct VistaWidgetLifetime {
  VistaWidgetIdentity screen;
  // Host lifetime tracking, not an invented field in the original source bank.
  // 610227 rebuilds even a same-pane request; embedded identities remain valid.
  std::uint64_t pane_generation{};
};
enum class VistaWidgetCreateKind {bitmap,scene,grid,text,table};
struct VistaWidgetCreate {
  VistaWidgetCreateKind kind{};
  VistaWidgetRecord record;
  // Grid row index or authored text index. Other wrappers have no index arg.
  std::int32_t element{};
};
struct VistaWidgetRegistrationServices {
  // At least 0x70 bytes per widget, 0xA7 per registered list and 0xA44 per
  // screen. Identities/storage remain valid until the explicit free callback.
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<std::optional<VistaWidgetScreen>(std::uint32_t)> screen; //60C701/639623.
  std::function<std::optional<VistaWidgetPane>(VistaWidgetIdentity)> pane; //60F815.
  std::function<void(VistaWidgetIdentity)> header; //60FD41.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> footer; //60FF73(1).
  std::function<void(VistaWidgetIdentity)> button; //registered widget virtual54.
  std::function<void(VistaWidgetIdentity,std::uint8_t)> list; //614990(false).
  // 60F402's two literal source32 addresses: list+A8, screen+A44.
  std::function<void(std::uint32_t,std::uint32_t)> list_callback;
  // Genuine wrapper contract: construct, append to this screen, initialize,
  // then return the retained identity. NULL allocation skips that child. The
  // constructor owns cached UI clock reads, nested skin children and bytes.
  std::function<VistaWidgetIdentity(VistaWidgetIdentity,const VistaWidgetCreate&)> create;
  std::function<void(VistaWidgetIdentity,std::uint32_t)> animate; //612604.
  std::function<void(VistaWidgetIdentity)> base_layout; //virtual60=610B4E.
  std::function<void(VistaWidgetIdentity,std::uint16_t)> shape; //60F975(shape-1).
  std::function<void(VistaWidgetIdentity)> shutdown; //widget virtual8.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> destructor; //virtual0(0).
  std::function<void(VistaWidgetIdentity)> free; //60D2EA.
};
// Source 61208E/611891, including reparenting and clearing the detached index.
void vista_widget_append(VistaWidgetIdentity parent,VistaWidgetIdentity child,
    const VistaWidgetRegistrationServices&);
void vista_widget_detach(VistaWidgetIdentity parent,VistaWidgetIdentity child,
    const VistaWidgetRegistrationServices&);
// Ordinary 610A44/6107DF path, then 60FB1B's five ordered creation loops and
// external 610B4E/60F975 owners. Forced/multi-list descriptors are unsupported.
void vista_widget_register(VistaWidgetLifetime&,const VistaWidgetRegistration&,
    const VistaWidgetRegistrationServices&);
// 610227 +60FA8B +60FB1B, source 16-bit pane arithmetic and retained kinds.
void vista_widget_change_pane(VistaWidgetLifetime&,std::uint16_t request_word,
    const VistaWidgetRegistrationServices&);
} //namespace halo2
