#pragma once
#include "halo2/source_runtime_memory.hpp"
#include <memory>

namespace halo2 {
class NativeVistaWidgetManagers;
class NativeVistaWidgetCache;
// Native storage glue only. Every source32 identity and initial byte is chosen
// by the caller. No construction, source-global publication, cache readiness,
// pool algorithm, source function dispatch or original heap address is inferred.
class NativeVistaWidgetMemory final : public SourcePhysicsContactMemory {
public:
  using Access=SourceRuntimeMemory::Access;
  NativeVistaWidgetMemory();
  ~NativeVistaWidgetMemory() override;
  NativeVistaWidgetMemory(const NativeVistaWidgetMemory&)=delete;
  NativeVistaWidgetMemory& operator=(const NativeVistaWidgetMemory&)=delete;
  NativeVistaWidgetMemory(NativeVistaWidgetMemory&&)=delete;
  NativeVistaWidgetMemory& operator=(NativeVistaWidgetMemory&&)=delete;

  // Normal owned banks reuse SourceRuntimeMemory unchanged. Aligned banks own
  // host-aligned storage and require an aligned source identity too. Initial
  // bytes are copied exactly; actual allocation failure propagates bad_alloc.
  void mount(std::uint32_t,std::span<const std::byte>,Access,std::string owner);
  void mount_aligned(std::uint32_t,std::span<const std::byte>,std::size_t alignment,
      Access,std::string owner);
  // Generic borrowed banks must retain their actual owner, not a dummy token.
  // The owner must not resize or relocate the supplied span while registered.
  void borrow(std::uint32_t,std::span<std::byte>,Access,
      std::shared_ptr<const void> lifetime,std::string owner);
  void borrow(std::uint32_t,std::span<const std::byte>,
      std::shared_ptr<const void> lifetime,std::string owner);
  // Explicit reference bindings borrow the actual nonmovable owner. Caller
  // must keep it alive until release and for all held views. In a session,
  // destroy this registry before its manager/cache members. Shared bindings
  // instead retain the actual owner until release. Neither binding constructs
  // the supplied owner or copies its mutable bytes.
  void bind_managers(NativeVistaWidgetManagers&);
  void bind_managers(std::shared_ptr<NativeVistaWidgetManagers>);
  void bind_cache(NativeVistaWidgetCache&);
  void bind_cache(std::shared_ptr<NativeVistaWidgetCache>);
  void release(std::uint32_t region_address);

  // Bulk accesses may span adjacent banks; each operation validates all bytes
  // before publishing and stages aliases. Earlier owner operations survive a
  // later failure. A view must fit one bank. Other mounts do not invalidate it;
  // release of its own region ends the registry's view-lifetime guarantee.
  void read(std::uint32_t,std::span<std::byte>) const override;
  void write(std::uint32_t,std::span<const std::byte>) override;
  std::uint32_t read(std::uint32_t,std::size_t scalar_bytes) const;
  void write(std::uint32_t,std::uint32_t,std::size_t scalar_bytes);
  std::span<const std::byte> view(std::uint32_t,std::size_t) const;
  std::span<std::byte> mutable_view(std::uint32_t,std::size_t);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
