#pragma once
#include <compare>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace halo2 {
// Cache identities remain 32 bits on every host. A datum includes its salt;
// the low 16 bits alone are not a tag handle.
struct TagHandle {
    std::uint32_t value = 0xffffffffu;
    constexpr bool is_null() const { return value == 0xffffffffu; }
    constexpr std::uint16_t index() const { return std::uint16_t(value); }
    auto operator<=>(const TagHandle&) const = default;
};
struct TagGroup {
    std::uint32_t value = 0xffffffffu;
    constexpr TagGroup() = default;
    constexpr explicit TagGroup(std::uint32_t code) : value(code) {}
    template<std::size_t N> constexpr explicit TagGroup(const char (&code)[N]) {
        static_assert(N == 5, "Tag groups contain exactly four characters");
        value = (std::uint32_t(std::uint8_t(code[0])) << 24) |
                (std::uint32_t(std::uint8_t(code[1])) << 16) |
                (std::uint32_t(std::uint8_t(code[2])) << 8) |
                 std::uint32_t(std::uint8_t(code[3]));
    }
    static TagGroup from_string(std::string_view);
    std::string string() const;
    auto operator<=>(const TagGroup&) const = default;
};
namespace tag_groups {
#define HALO2_TAG_GROUP(name, code) inline constexpr TagGroup name{code};
#include "halo2/tags/groups.def"
#undef HALO2_TAG_GROUP
inline constexpr TagGroup none{};
}
struct TagGroupName { TagGroup group; std::string_view name; };
std::span<const TagGroupName> tag_group_catalog();
std::string_view tag_group_name(TagGroup);
std::string_view tag_name_strip_path(std::string_view);

// Literal 12-byte cache hierarchy. Both ancestors are authored by the cache.
struct TagGroupDefinition {
    TagGroup group, parent, grandparent;
    bool matches(TagGroup expected) const {
        return group == expected || parent == expected || grandparent == expected;
    }
};
struct TagReference { TagGroup group; TagHandle handle; };
struct TagBlock {
    std::int32_t count{};
    std::uint32_t address{};
    std::size_t extent(std::size_t stride, std::size_t maximum_count = 0x7fffffff) const;
};
struct TagData { std::int32_t size{}; std::uint32_t address{}; };
struct StringId {
    std::uint32_t value{};
    constexpr std::uint32_t index() const { return value & 0x00ffffffu; }
    constexpr std::uint8_t length() const { return std::uint8_t(value >> 24); }
};
static_assert(sizeof(TagHandle) == 4 && sizeof(TagGroup) == 4);
static_assert(sizeof(TagGroupDefinition) == 12 && sizeof(TagReference) == 8);
static_assert(sizeof(TagBlock) == 8 && sizeof(TagData) == 8);

// Serialized cache directory row. Its address is a file-format/source token,
// never an engine pointer. Engine-facing TagView::Definition uses a distinct
// NativeTagAddress. Strings are owned so entries survive buffer growth.
struct Tag {
    std::string group, name;
    std::uint32_t datum{}, address{}, size{};
    TagHandle handle() const { return {datum}; }
    TagGroup type() const { return TagGroup::from_string(group); }
};
struct TagIterator {
    std::uint32_t next_index{};
    TagHandle current;
    std::optional<TagGroup> group;
};
} // namespace halo2
