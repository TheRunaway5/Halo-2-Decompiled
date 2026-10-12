#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
struct VistaWidgetBackingServices {
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  // Genuine native-platform providers for the original PE IAT calls. Handles
  // and allocation identities are source32 registry values, never host casts.
  // HeapAlloc is called with flags0; original bytes must not be guessed zero.
  std::function<std::uint32_t()> process_heap; // ActualGetProcessHeap.
  std::function<VistaWidgetIdentity(std::uint32_t,std::uint32_t,
      std::uint32_t)> allocate; // ActualHeapAlloc(heap,flags,size).
  std::function<std::uint32_t(std::uint32_t,std::uint32_t,
      VistaWidgetIdentity)> release; // ActualHeapFree(heap,flags,original).
};
// Complete437E69→48F509/48F50F. Minimum exponent3; x86SHL masks the supplied
// exponent to5bits. DWORD size/address addition and original round-forward
// behavior are retained, even if the platform's pointer was already aligned.
// NonNULL allocation writes size at aligned-8, then source offset at aligned-4.
// Failure returnsNULL without writes. The real provider owns initial bytes,
// admission, handle, backing lifetime and failure; no implicit heap birth.
VistaWidgetIdentity vista_widget_aligned_backing_allocate(std::uint32_t size,
    std::uint32_t alignment_exponent,const VistaWidgetBackingServices&);
// Actual UI allocator virtual0=43EC8D:437E69(size,12). Its this identity is not
// read by the original body. The returned data is4KiB aligned when arithmetic
// and admitted platform backing permit the original writes to complete.
VistaWidgetIdentity vista_widget_backing_allocate(std::uint32_t size,
    const VistaWidgetBackingServices&);
// Complete43EC9F→437EC3→48F524. Reads data-4 BEFOREGetProcessHeap, subtracts
// that offset with DWORD wrap and callsHeapFree(fresh_handle,0,original).
// NoNULL guard/header clearing/validation or success conversion is invented.
// The exact originalHeapFree DWORD return is preserved for the provider caller.
std::uint32_t vista_widget_backing_release(VistaWidgetIdentity data,
    const VistaWidgetBackingServices&);
} //namespace halo2
