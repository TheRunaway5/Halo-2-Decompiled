#include "halo2/physics_contact_record.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read_word(const SourcePhysicsContactMemory &memory,
                        std::uint32_t address) {
    std::array<std::byte, 4> raw;
    memory.read(address, raw);
    std::uint32_t value = 0;
    for (unsigned n = 0; n < raw.size(); ++n)
        value |= std::uint32_t(std::to_integer<unsigned char>(raw[n])) << (8 * n);
    return value;
}
struct Property { bool found; std::uint32_t value; };
Property property(const SourcePhysicsContactMemory &memory,
                  std::uint32_t body, std::uint32_t key) {
    const auto count = read_word(memory, body + 0x34u);
    // Original signed JLE: no record access for zero/negative counts.
    if (count == 0 || (count & 0x80000000u)) return {false, 0};
    const auto pairs = read_word(memory, body + 0x30u);
    for (std::uint32_t n = 0; n < count; ++n) {
        const auto at = pairs + n * 8u;
        if (read_word(memory, at) == key) return {true, read_word(memory, at + 4u)};
    }
    return {false, 0}; // Original exhausted scan, not absent body backing.
}
void store(SourcePhysicsContactMemory &memory,
           const SourcePhysicsContactConstruction &input,
           SourcePhysicsContactImage image,
           const SourcePhysicsContactRecordObserver &observer,
           std::uint32_t site, unsigned offset, unsigned size, std::uint32_t value) {
    SourcePhysicsContactRecordStore event;
    event.instruction = site +
        (image == SourcePhysicsContactImage::TitleUpdate15 ? 0x48c0u : 0u);
    event.address = input.row_address + offset;
    event.before.resize(size); event.written.resize(size);
    memory.read(event.address, event.before);
    for (unsigned n = 0; n < size; ++n)
        event.written[n] = std::byte((value >> (8 * n)) & 0xffu);
    memory.write(event.address, event.written);
    if (observer) observer(event);
}
} // namespace
std::uint32_t source_construct_physics_contact_record(
    SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactConstruction &input,
    SourcePhysicsContactImage image,
    const SourcePhysicsContactRecordObserver &observer) {
    const auto &a = input.stack_arguments;
    auto write = [&](std::uint32_t instruction, unsigned offset, unsigned size,
                     std::uint32_t value) {
        store(memory, input, image, observer, instruction, offset, size, value);
    };
    write(0x1d2df6, 0x0c, 4, input.source_key);
    write(0x1d2e03, 0x00, 2, input.contact_index_word);
    write(0x1d2e0b, 0x12, 2, a[8]);
    write(0x1d2e0f, 0x04, 4, a[0]);
    write(0x1d2e12, 0x08, 4, 0xffffffffu);
    write(0x1d2e19, 0x10, 2, a[7]);
    write(0x1d2e45, 0x14, 4, property(memory, a[2], 0x2001u).value);
    auto component = a[3];
    if (a[0] != 0xffffffffu) {
        const auto kind = a[0] >> 29;
        if (kind == 3 || kind == 4)
            throw std::runtime_error("Original encoded contact-key lookup owner unavailable");
        component = 0xffffffffu;
    }
    write(0x1d2e80, 0x38, 4, 0);
    write(0x1d2e8b, 0x18, 4, component);
    write(0x1d2e92, 0x3c, 4, a[5]);
    write(0x1d2e9d, 0x44, 1, a[4]);
    write(0x1d2ea0, 0x40, 4, a[6]);
    write(0x1d2ec5, 0x45, 1, property(memory, a[1], 0x2002u).value);
    const auto other_property = property(memory, a[2], 0x2002u);
    // The successful last-property branch has a separate original store site.
    // Presence, rather than nonzero value, selects the original store site.
    write(other_property.found ? 0x1d2eff : 0x1d2eeb, 0x46, 1, other_property.value);
    return input.row_address;
}
} // namespace halo2
