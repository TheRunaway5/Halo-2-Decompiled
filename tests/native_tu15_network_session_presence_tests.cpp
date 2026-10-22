#include "halo2/source_network_session_startup_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_static_registration_tu15.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
using Memory = halo2::SourceRuntimeMemory;
using Access = Memory::Access;
using Mount = void (*)(Memory&);
constexpr std::uint32_t flag = 0x4dc3e8u;
constexpr const char* owner = "network session cold initialized flag";
constexpr auto presence = halo2::mount_native_tu15_network_session_presence_storage;

void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

const Memory::CheckpointRegion& presence_region(const Memory::Checkpoint& saved) {
    const auto found = std::ranges::find(saved.regions, flag,
                                        &Memory::CheckpointRegion::address);
    check(found != saved.regions.end(), "Presence region is absent");
    return *found;
}

void check_presence(const Memory::Checkpoint& saved, std::byte value) {
    const auto& region = presence_region(saved);
    check(region.access == Access::ReadWrite && region.owner == owner &&
              region.page_bytes == 0 && region.committed.empty() &&
              region.bytes.size() == 1 && region.bytes[0] == value,
          "Presence region lost its exact byte, extent or ownership metadata");
}

void check_roundtrip(const Memory& memory, halo2::NativeRuntimeAddress native,
                     std::byte value) {
    check(memory.native_address(flag, 1) == native &&
              memory.source_address(native, 1) == flag,
          "Presence source token and native address no longer roundtrip");
    check(memory.view(native, 1).data() == memory.view(flag, 1).data(),
          "Presence source and native views refer to different backing");
    std::array<std::byte, 1> source_byte{}, native_byte{};
    memory.read(flag, source_byte);
    memory.read(native, native_byte);
    check(source_byte[0] == value && native_byte == source_byte,
          "Presence source and native reads differ");
}

void cold_and_live_reuse() {
    Memory memory;
    memory.mount(flag - 1, std::array{std::byte{0x5a}}, Access::ReadWrite,
                 "left adjacent control");
    memory.mount(flag + 1, std::array{std::byte{0xa5}}, Access::ReadOnly,
                 "right adjacent control");
    const auto original = memory.checkpoint();
    presence(memory);
    const auto cold = memory.checkpoint();
    check(cold.version == original.version &&
              cold.regions.size() == original.regions.size() + 1,
          "Cold presence mount added unexpected owners or changed the version");
    for (const auto& region : original.regions)
        check(std::ranges::find(cold.regions, region) != cold.regions.end(),
              "Cold presence mount changed an adjacent owner");
    check_presence(cold, std::byte{0});
    const auto native = memory.native_address(flag, 1);
    check(native.value > std::numeric_limits<std::uint32_t>::max(),
          "Presence backing did not exercise a native address above 4 GiB");
    for (const auto value : {std::byte{0}, std::byte{1}, std::byte{0xa7}}) {
        // Exercise both address adapters while testing storage reuse only.
        if (value == std::byte{1}) memory.write(flag, std::array{value});
        if (value == std::byte{0xa7}) memory.write(native, std::array{value});
        const auto before = memory.checkpoint();
        presence(memory);
        check(memory.checkpoint() == before,
              "Presence reuse changed a live byte or another bank owner");
        check_roundtrip(memory, native, value);
    }
}

template<class Setup>
void rejects_owner(const char* message, Setup setup, Mount mount = presence,
                   bool accessible = true) {
    Memory memory;
    memory.mount(0x60001000u, std::array{std::byte{0x71}, std::byte{0x82}},
                 Access::ReadWrite, "unrelated rejection control");
    setup(memory);
    const auto before = memory.checkpoint();
    const auto sentinel = memory.native_address(0x60001000u, 2);
    const auto native = accessible ? memory.native_address(flag, 1)
                                   : halo2::NativeRuntimeAddress{};
    bool rejected = false;
    try { mount(memory); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, message);
    check(memory.checkpoint() == before,
          "Rejected ownership changed complete bank bytes or metadata");
    check(memory.native_address(0x60001000u, 2) == sentinel,
          "Rejected ownership replaced unrelated backing");
    if (accessible)
        check(memory.native_address(flag, 1) == native &&
                  memory.source_address(native, 1) == flag,
              "Rejected ownership replaced the conflicting backing");
}

void conflicting_owners() {
    rejects_owner("Foreign owner was accepted", [](auto& memory) {
        memory.mount(flag, std::array{std::byte{1}}, Access::ReadWrite,
                     "foreign presence owner");
    });
    rejects_owner("Wider owner at the exact address was accepted", [](auto& memory) {
        memory.mount(flag, std::array{std::byte{1}, std::byte{0x37}},
                     Access::ReadWrite, owner);
    });
    rejects_owner("Interior address of a covering owner was accepted", [](auto& memory) {
        memory.mount(flag - 1, std::array{std::byte{0x37}, std::byte{0xa7}},
                     Access::ReadWrite, owner);
    });
    rejects_owner("Read-only presence owner was accepted", [](auto& memory) {
        memory.mount(flag, std::array{std::byte{1}}, Access::ReadOnly, owner);
    });
    rejects_owner("Committed paged metadata was accepted", [](auto& memory) {
        memory.reserve(flag, 1, 1, owner);
        memory.commit(flag, std::array{std::byte{1}});
    });
    rejects_owner("Uncommitted reservation was accepted", [](auto& memory) {
        memory.reserve(flag, 1, 1, owner);
    }, presence, false);
}

void write_guard_denial() {
    Memory memory;
    presence(memory);
    memory.write(flag, std::array{std::byte{1}});
    memory.mount(0x60001000u, std::array{std::byte{0x63}}, Access::ReadOnly,
                 "write-guard rejection control");
    const auto before = memory.checkpoint();
    const auto native = memory.native_address(flag, 1);
    unsigned guard_calls{}, writes{}, topology_changes{};
    struct ProtectionLifetime {
        const Memory& memory;
        ~ProtectionLifetime() {
            halo2::source_runtime_unregister_write_guard(memory, this);
            halo2::source_runtime_unregister_write_observer(memory, this);
            halo2::source_runtime_unregister_topology_observer(memory, this);
        }
    } protection{memory};
    halo2::source_runtime_register_write_guard(memory, &protection,
        [&](auto address, auto bytes) {
            check(address == flag && bytes == 1, "Unexpected presence write extent");
            ++guard_calls;
            throw std::out_of_range("presence write denied");
        });
    halo2::source_runtime_register_write_observer(memory, &protection,
        [&](auto, auto) { ++writes; });
    halo2::source_runtime_register_topology_observer(memory, &protection,
        [&](auto, auto) { ++topology_changes; });
    check_roundtrip(memory, native, std::byte{1});
    check(guard_calls == 0, "Read-only inspection invoked the write guard");
    bool denied = false;
    try { presence(memory); }
    catch (const std::out_of_range& error) {
        denied = std::string(error.what()) == "presence write denied";
    }
    check(denied && guard_calls == 1, "Write denial was swallowed, converted or retried");
    check(memory.checkpoint() == before && writes == 0 && topology_changes == 0,
          "Write denial changed the bank or emitted a mutation notification");
    check_roundtrip(memory, native, std::byte{1});
}

void compatible_mount_order(Mount first, Mount second) {
    Memory memory;
    first(memory);
    check_presence(memory.checkpoint(), std::byte{0});
    const auto native = memory.native_address(flag, 1);
    memory.write(flag, std::array{std::byte{1}});
    const auto before = memory.checkpoint();
    const auto expected_flag = presence_region(before);
    second(memory);
    const auto combined = memory.checkpoint();
    check(combined.version == before.version && presence_region(combined) == expected_flag,
          "Second storage provider changed the canonical live flag metadata or byte");
    for (const auto& region : before.regions)
        check(std::ranges::find(combined.regions, region) != combined.regions.end(),
              "Second storage provider changed a preexisting owner");
    check_roundtrip(memory, native, std::byte{1});
    presence(memory);
    check(memory.checkpoint() == combined,
          "Direct shared-helper reuse changed the combined storage bank");
    check_roundtrip(memory, native, std::byte{1});
}

void storage_provider_integration() {
    constexpr auto network = halo2::mount_native_tu15_network_session_data;
    constexpr auto registration = halo2::mount_native_tu15_static_registration_data;
    compatible_mount_order(network, registration);
    compatible_mount_order(registration, network);
    const auto foreign = [](auto& memory) {
        memory.mount(flag, std::array{std::byte{1}}, Access::ReadWrite,
                     "foreign integration owner");
    };
    rejects_owner("Network storage mounted before rejecting a foreign flag", foreign, network);
    rejects_owner("Static registration mounted before rejecting a foreign flag", foreign, registration);
}
}

int main() try {
    cold_and_live_reuse();
    conflicting_owners();
    write_guard_denial();
    storage_provider_integration();
    std::cout << "Network session presence storage tests passed\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
