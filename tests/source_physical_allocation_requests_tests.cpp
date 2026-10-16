#include "halo2/source_runtime_host_physical_memory.hpp"
#include <algorithm>
#include <array>
#include <iostream>
using namespace halo2;
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
int main() try {
  SourceRuntimeMemory bank;
  {
    SourceRuntimeHostPhysicalMemory owner(bank);
    const auto arena =
        owner.allocate({0x30e4000u, 0x61000u, 0x3144fffu, 0u, 4u});
    check(arena == 0x80061000u, "Original bounded arena allocation changed");
    const SourcePhysicalAllocationRequest original_request{
        0x60u, 0u, 0xffffffffu, 0x1000u, 4u};
    const auto first = owner.allocate(original_request);
    check(first == 0x80000000u,
          "Actual unrestricted source request did not use free native pages");
    const auto second = owner.allocate(original_request);
    check(second == 0x80001000u,
          "Repeated source request overlapped retained pages");
    const auto crossed =
        owner.allocate({0x60000u, 0u, 0xffffffffu, 0x1000u, 4u});
    check(crossed == 0x83145000u,
          "Unrestricted request overlapped original arena");
    const std::array value{std::byte{0x71}, std::byte{0x1e}, std::byte{0x81},
                           std::byte{0x29}};
    bank.write(first, value);
    check(std::ranges::equal(bank.view(first, 4), value),
          "Real page storage lost source bytes");
    const auto write_combined =
        owner.allocate({0x100000u, 0u, 0xffffffffu, 0x1000u, 0x404u});
    check(write_combined == 0x831a5000u,
          "Original SDK write-combined request lost its native page owner");
    bank.write(write_combined, value);
    check(std::ranges::equal(bank.view(write_combined, 4), value),
          "Write-combined native pages lost source bytes");
    const auto saved = owner.ownership_checkpoint();
    check(saved.allocations.size() == 5,
          "Shared physical ownership lost a reservation");
    for (auto q : std::array<SourcePhysicalAllocationRequest, 4>{
             {{0x60u, 0u, 0x80000000u, 0x1000u, 4u},
              {0x60u, 0x80000000u, 0xffffffffu, 0x1000u, 4u},
              {0x60u, 0u, 0xffffffffu, 3u, 4u},
              {0x60u, 0u, 0xffffffffu, 0x1000u, 0x204u}}}) {
      bool rejected = false;
      try {
        owner.allocate(q);
      } catch (const std::runtime_error &) {
        rejected = true;
      }
      check(rejected,
            "Unrestricted sentinel admitted an unsupported range or alignment");
    }
    check(owner.allocate({0x2000u, 0x7ffff000u, 0xffffffffu, 0x1000u, 4u}) ==
              0u,
          "Physical token overflow did not report real namespace exhaustion");
  }
  bool retired = false;
  try {
    bank.view(0x80000000u, 4);
  } catch (const std::out_of_range &) {
    retired = true;
  }
  check(retired, "Physical resource retirement lost ownership");
  std::cout << "PASS original bounded/unrestricted/repeated allocation, "
               "nonoverlap, real bytes, checkpoint ownership, rejection, "
               "exhaustion and retirement\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
