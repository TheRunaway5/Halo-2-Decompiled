#include "../src/tags/native_directory.hpp"
#include "halo2/cache.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
unsigned checks{};
void check(bool ok, const char* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
bool equal(const Tag& a, const Tag& b) {
    return a.group == b.group && a.name == b.name && a.datum == b.datum &&
        a.address == b.address && a.size == b.size;
}
void unchanged(const std::vector<Tag>& tags, const std::vector<Tag>& before,
               std::optional<std::size_t> changed = {}) {
    check(tags.size() == before.size(), "Existing directory length changed");
    for (std::size_t i = 0; i < tags.size(); ++i)
        if (i != changed) check(equal(tags[i], before[i]), "An existing directory owner changed");
}
std::vector<Tag> full_directory(std::size_t count = 32768) {
    std::vector<Tag> tags;
    tags.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
        tags.push_back({"bitm", "mods\\Owner\\" + std::to_string(i),
            0xa1000000u | std::uint32_t(i), 0x100000u + std::uint32_t(i) * 16, 16});
    return tags;
}
void rejects_unchanged(std::vector<Tag>& tags) {
    const auto before = tags;
    bool rejected = false;
    try { (void)detail::insert_native_tag(tags, "stem", "shaders\\new_owner"); }
    catch (const FormatError&) { rejected = true; }
    check(rejected, "An unreachable signed-index slot was published");
    unchanged(tags, before);
}
}
int main() try {
    static_assert(sizeof(TagHandle) == 4 && sizeof(TagBlock) == 8 && sizeof(TagData) == 8);
    {
        std::vector<Tag> tags;
        const auto first = detail::insert_native_tag(tags, "stem", "Shaders\\Mixed Case");
        check(first.value == 0xd0000000u && first.index() == 0, "Native import salt or first slot changed");
        check(tags[0].group == "stem" && tags[0].name == "Shaders\\Mixed Case" &&
            !tags[0].address && !tags[0].size, "New identity or unloaded payload changed");
    }
    {
        std::vector<Tag> tags{{"", "", 0xffffffffu, 0, 0},
            {"stem", "unloaded", 0xabcd0001u, 0, 0}};
        const auto before = tags;
        const auto added = detail::insert_native_tag(tags, "bitm", "new");
        check(added.value == 0xd0000002u && tags.size() == 3,
            "A lower hole changed the established append order");
        check(equal(tags[0], before[0]) && equal(tags[1], before[1]),
            "Append replaced an existing null or unloaded owner");
    }
    {
        auto tags = full_directory(32767);
        const auto before = tags;
        const auto last = detail::insert_native_tag(tags, "spas", "last");
        check(last.value == 0xd0007fffu && tags.size() == 32768,
            "Last positive signed-low16 slot was rejected or changed");
        for (std::size_t i = 0; i < before.size(); ++i)
            check(equal(tags[i], before[i]), "Boundary append changed an existing salted identity");
        rejects_unchanged(tags);
        std::cout << "native_directory_host_address="
                  << std::uint64_t(reinterpret_cast<std::uintptr_t>(tags.data())) << '\n';
    }
    {
        auto tags = full_directory();
        // Nonresident and empty live identities are occupied, regardless of
        // their payload. Only the complete NONE datum makes a reusable slot.
        tags[1].address = tags[1].size = 0;
        tags[2].size = 0;
        tags[3] = {"stale", "unused", 0xffffffffu, 0x76543210u, 123};
        tags[32767] = {"", "", 0xffffffffu, 0, 0};
        const auto before = tags;
        const auto reused = detail::insert_native_tag(tags, "vrtx", "Mods\\New Program");
        check(reused.value == 0xd0000003u && tags[3].name == "Mods\\New Program" &&
            tags[3].group == "vrtx" && !tags[3].address && !tags[3].size,
            "First null lower slot did not retain the new typed identity");
        unchanged(tags, before, 3);
        const auto last = detail::insert_native_tag(tags, "bitm", "last-hole");
        check(last.value == 0xd0007fffu, "Last lower null slot was lost");
        rejects_unchanged(tags);
    }
    {
        auto tags = full_directory();
        tags.resize(65536, Tag{"", "", 0xffffffffu, 0, 0});
        rejects_unchanged(tags); // Upper-half padding cannot supply an owner.
        tags[7] = {"", "", 0xffffffffu, 0, 0};
        const auto before = tags;
        check(detail::insert_native_tag(tags, "stem", "lower-hole").value == 0xd0000007u,
            "Padded directory ignored a reachable null slot");
        unchanged(tags, before, 7);
    }
    std::cout << checks << " native directory checks passed; serialized identities remain32-bit\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
