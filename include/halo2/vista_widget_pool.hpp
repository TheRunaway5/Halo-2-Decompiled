#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
struct VistaWidgetPoolServices {
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  // Actual allocator virtual0/4, after the original live vtable reads. The
  // UI's original allocator is8637B4, virtual0=43EC8D, virtual4=43EC9F.
  // The genuine provider retains aligned backing at a caller-owned source32
  // identity. It must admit allocation failure; no default storage is created.
  std::function<VistaWidgetIdentity(VistaWidgetIdentity,std::uint32_t,
      std::uint32_t)> backing_allocate; // allocator, target, size
  std::function<void(VistaWidgetIdentity,std::uint32_t,
      VistaWidgetIdentity)> backing_release; // allocator, target, backing
  std::function<void(VistaWidgetIdentity)> dispose; // Actual60B9AC.
  std::function<std::uint32_t()> timestamp; // Actual437E51; debug+3F only.
};
// Complete60D196/60D21C UI heap birth/shutdown. Original globalD758E4 is
// explicit source32 storage, separate from the UI-manager bank. Birth reads
// the actual source allocator/vtable/name; it does not publish loaded-cache
// readiness or invent a registry. Backing remains retained until shutdown.
void vista_widget_pool_birth(std::uint32_t source_kind,
    const VistaWidgetPoolServices&);
void vista_widget_pool_shutdown(const VistaWidgetPoolServices&);
// Complete48C08E and its actual gap-search/insertion leaves. Output is a
// borrowed source32 DWORD slot, including its original backreference identity
// when heap+3C iszero. The caller owns its lifetime. All arithmetic wraps at
// 32bits. Failure leaves the output slot untouched; previous writes survive.
bool vista_widget_pool_allocate(VistaWidgetIdentity heap,std::uint32_t output,
    std::uint32_t size,std::uint32_t metadata1,std::uint32_t metadata2,
    const VistaWidgetPoolServices&);
// Complete48BDBF/48BD80. argument is the source32 address of the caller's
// pointer DWORD, not a host pointer. Unlink does not wipe or release backing,
// clear that DWORD, validate membership, or treatNULL as a successful free.
std::uint32_t vista_widget_pool_length(std::uint32_t argument,
    const VistaWidgetPoolServices&);
void vista_widget_pool_unlink(VistaWidgetIdentity heap,std::uint32_t argument,
    const VistaWidgetPoolServices&);
// Complete609616: exact manager retirement order, genuine60B9AC service.
void vista_widget_pool_retire(const VistaWidgetPoolServices&);
// Complete60D2D8→60D239 and60D2EA. scratch/argument are actual caller-owned
// source32 stack/registry DWORD slots. The native caller must retain them for
// the whole call (and any original backreference lifetime). ignored is really
// ignored by60D2D8. Failed allocation retires then rereads globalD758E4 and
// retries once. No implicit heap birth or successful allocation is inferred.
VistaWidgetIdentity vista_widget_ui_allocate(std::uint32_t size,
    std::uint32_t ignored,std::uint32_t scratch,const VistaWidgetPoolServices&);
VistaWidgetIdentity vista_widget_ui_allocate_raw(std::uint32_t size,
    std::uint32_t metadata1,std::uint32_t metadata2,std::uint32_t scratch,
    const VistaWidgetPoolServices&); // Complete60D239, before wrapper zeroes.
void vista_widget_ui_release(std::uint32_t argument,
    const VistaWidgetPoolServices&);
} //namespace halo2
