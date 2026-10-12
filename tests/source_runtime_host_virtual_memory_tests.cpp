#include "halo2/source_runtime_host_virtual_memory.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
template <class F> void fails(F &&f) {
  bool failed = false;
  try { f(); } catch (const std::exception &) { failed = true; }
  check(failed, "Unsupported resource or inaccessible backing must stop");
}
}
int main() {
  try {
    halo2::SourceRuntimeMemory memory;
    {
      halo2::SourceRuntimeHostVirtualMemory vm(memory);
      const auto reservation = vm.allocate({0, 0, 0x100000, 0x2000, 4});
      check(!reservation.status && reservation.bytes == 0x100000,
            "Actual host reservation");
      fails([&] { memory.view(reservation.base, 1); });
      const auto committed = vm.allocate({reservation.base, 0, 0x1000, 0x1000, 4});
      check(committed.base == reservation.base && committed.bytes == 0x1000,
            "Commit retains reservation identity");
      check(std::ranges::all_of(memory.view(committed.base, committed.bytes),
                [](auto b) { return b == std::byte{}; }),
            "Actual anonymous first-page initial bytes");
      memory.mutable_view(committed.base, 1)[0] = std::byte{0x73};
      vm.allocate({committed.base, 0, 0x2000, 0x1000, 4});
      check(memory.view(committed.base, 1)[0] == std::byte{0x73},
            "Mixed old/new commitment must retain source writes");
      fails([&] { memory.view(committed.base + 0x2000, 1); });
      fails([&] { vm.allocate({committed.base, 0, 0, 0x1000, 4}); });
      fails([&] { vm.allocate({committed.base, 0, 0x10, 0x1000, 4}); });
      fails([&] { vm.allocate({committed.base + 0xff000, 0, 0x2000, 0x1000, 4}); });
      fails([&] { vm.release(committed.base, 0, 0x8000); });
      fails([&] { vm.release(committed.base + 0x1000, 0xff000, 0x8000); });
      vm.release(reservation.base, reservation.bytes, 0x8000);
      fails([&] { memory.view(committed.base, 1); });
      fails([&] { vm.allocate({committed.base, 0, 0x1000, 0x1000, 4}); });
      const auto retained = vm.allocate({0, 0, 0x10000, 0x2000, 4});
      vm.allocate({retained.base, 0, 0x1000, 0x1000, 4});
      check(!memory.view(retained.base, 1).empty(), "Second real reservation");
    }
    fails([&] { memory.view(0x10100000, 1); });
    // A token collision must unwind the real reservation without replacing
    // an unrelated source owner or admitting a commit into that owner's bank.
    const std::array marker{std::byte{0x19}};
    memory.mount(0x20000000, marker, halo2::SourceRuntimeMemory::Access::ReadWrite,
                  "existing unrelated owner");
    halo2::SourceRuntimeHostVirtualMemory collision(memory, 0x20000000);
    fails([&] { collision.allocate({0, 0, 0x10000, 0x2000, 4}); });
    check(memory.view(0x20000000, 1)[0] == marker[0], "Collision preserves its owner");
    std::cout << "Real retained reservation/commit/recommit/release and source visibility pass\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
