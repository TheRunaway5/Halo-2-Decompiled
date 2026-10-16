#include "halo2/source_final_startup_storage_tu15.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
void cold(SourceRuntimeMemory& memory, std::uint32_t address, std::size_t bytes) {
    memory.mount(address, std::vector<std::byte>(bytes), SourceRuntimeMemory::Access::ReadWrite,
        "TU1.5 final startup loader-zeroed constructor destination");
}
template<std::size_t N> void literal(SourceRuntimeMemory& memory, std::uint32_t address,
    const std::array<std::uint8_t, N>& values) {
    const auto bytes = std::as_bytes(std::span(values));
    // Some immutable operands/vtables already belong to earlier startup
    // declarations. Check their exact bytes/access; mount only uncovered
    // portions, without replacing an existing region or joining its lifetime.
    auto accessible = [&](std::uint32_t at, std::size_t count) {
        try { (void)memory.view(at, count); return true; }
        catch (const std::exception&) { return false; }
    };
    for (std::size_t offset = 0; offset < N;) {
        const bool existing = accessible(address + offset, 1);
        std::size_t count = 1;
        while (offset + count < N && (existing ? accessible(address + offset, count + 1) :
            !accessible(address + offset + count, 1))) ++count;
        const auto part = bytes.subspan(offset, count);
        if (existing) {
            if (!memory.is_read_only(address + offset, count) ||
                !std::ranges::equal(memory.view(address + offset, count), part))
                throw std::invalid_argument("Final startup shared immutable metadata differs");
        } else memory.mount(address + offset, part, SourceRuntimeMemory::Access::ReadOnly,
            "TU1.5 final startup immutable constructor metadata");
        offset += count;
    }
}
std::uint32_t word(const SourceRuntimeMemory& memory, std::uint32_t address) {
    const auto bytes = memory.view(address, 4);
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) value |= std::to_integer<std::uint32_t>(bytes[i]) << (8 * i);
    return value;
}
struct Binding { std::uint32_t cell, bytes; };
constexpr std::array<Binding, 33> bindings{{
    {0x5020a8,0x374},{0x5020a4,0x38},{0x5020c8,0x8886c},{0x505e78,0x640},
    {0x56aeb0,0x203c},{0x510fa8,0x6d0},{0x510fa4,0x190},
    {0x50eef4,0xc46c},{0x50eef0,0xf0cc},{0x50eeec,0x726c},
    {0x52be7c,0xc760},{0x52be80,0x163c},{0x510fac,0x20},
    {0x50eef8,0x690},{0x50eefc,0xf00},{0x52d87c,0x440c},
    {0x50eee0,0x2174},{0x50eedc,0x5c8},{0x505e74,0xfb8},{0x505e70,0x53c},
    {0x510fb0,0x10},{0x52d88c,0x1e0},{0x5112d4,0x24},
    {0x52be44,4},{0x52be70,0x1f40},{0x52d68c,0x1450},{0x52d690,0x850},
    {0x52be5c,0x1409b},{0x48605c,0x105470},{0x486070,0x24},
    {0x505e7c,0x40},{0x4f3010,0x204},{0x52d688,4}}};
NativeTu15FinalStartupStorageViews::Region region(const SourceRuntimeMemory& memory, Binding binding) {
    const auto source = word(memory, binding.cell);
    if (!source) throw std::invalid_argument("Final startup source storage binding is absent");
    memory.validate_mutable_view(source, binding.bytes);
    const auto native = memory.native_address(source, binding.bytes);
    if (native.value <= 0xffffffffu)
        throw std::invalid_argument("Final startup storage lacks native64 backing");
    return {binding.cell, source, binding.bytes, native};
}
void validate(const SourceRuntimeMemory& memory, std::uint32_t entry) {
    unsigned first = 0, count = 0;
    switch (entry) {
    case 0x1cfa40: count = 23; break;
    case 0x1cb130: first = 23; count = 7; break;
    case 0x219080: first = 30; count = 1; break;
    case 0x137870: first = 31; count = 1; break;
    case 0x22c980: first = 32; count = 1; break;
    case 0x31d400: return;
    default: throw std::invalid_argument("Final startup entry is not an initializer");
    }
    for (unsigned i = first; i < first + count; ++i) (void)region(memory, bindings[i]);
    if (entry == 0x1cb130) {
        const auto aligned = word(memory, 0x48605c), heap = word(memory, 0x486070);
        const auto prior = word(memory, 0x51df44);
        if ((aligned & 0xfffu) || heap != aligned + 0x105410u ||
            word(memory, aligned - 8) != 0x105470u || word(memory, aligned - 4) != 0x1000u ||
            word(memory, heap) != 0x46393cu || word(memory, heap + 0x18) != prior ||
            word(memory, heap + 0x1c) != 0x452e8u)
            throw std::invalid_argument("Final startup aligned heap constructor differs");
        memory.validate_mutable_view(prior, 0x452e8);
    }
}
}
void mount_native_tu15_final_startup_storage_data(SourceRuntimeMemory& memory) {
#include "native_tu15_final_startup_storage_data.inc"
}
bool source_final_startup_storage_tu15_is_initializer(std::uint32_t entry) {
    switch (entry) {
    case 0x1cfa40: case 0x1cb130: case 0x31d400: case 0x219080: case 0x137870: case 0x22c980:
        return true;
    default: return false;
    }
}
void source_run_final_startup_storage_with_native_storage(SourceRuntimeMemory& memory, std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi& abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services, SourcePhysicsPoolHostStorage& raw,
    const SourcePhysicsContactRecordObserver& observer) {
    if (!raw.backs(memory) || !source_final_startup_storage_tu15_is_initializer(entry))
        throw std::invalid_argument("Final startup native storage owner or initializer differs");
    auto joined = services;
    joined.required_owner = [&](auto& bank, const auto& call, auto& state, const auto& observe) {
        if (raw.try_call(bank, call, state)) return;
        if (!services.required_owner) throw std::runtime_error("Final startup has an unowned dependency");
        services.required_owner(bank, call, state, observe);
    };
    source_run_final_startup_storage_tu15(memory, entry, abi, fp, joined, observer);
    validate(memory, entry);
}
NativeTu15FinalStartupStorageViews native_tu15_final_startup_storage_views(const SourceRuntimeMemory& memory) {
    for (const auto entry : {0x1cfa40u,0x1cb130u,0x219080u,0x137870u,0x22c980u}) validate(memory, entry);
    NativeTu15FinalStartupStorageViews result;
    for (const auto binding : bindings) result.regions.push_back(region(memory, binding));
    for (const auto [address, bytes] : std::array<std::pair<unsigned, unsigned>, 3>{{
        {0x4fd538,0x20fc},{0x4ff798,131*20*4},{0x510fb8,0x31c}}}) {
        const auto native = memory.native_address(address, bytes);
        if (native.value <= 0xffffffffu) throw std::invalid_argument("Final startup static metadata lacks native64 backing");
        result.regions.push_back({address,address,bytes,native});
    }
    return result;
}
}
