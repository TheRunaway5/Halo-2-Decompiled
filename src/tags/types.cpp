#include "halo2/tags/types.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
TagGroup TagGroup::from_string(std::string_view code) {
    if (code.size() != 4) throw FormatError("Tag group must contain four characters");
    std::uint32_t value = 0;
    for (unsigned char ch : code) value = (value << 8) | ch;
    return TagGroup(value);
}
std::string TagGroup::string() const {
    std::string out(4, '\0');
    for (unsigned i = 0; i < 4; ++i) out[i] = char(value >> (24 - i * 8));
    return out;
}
std::span<const TagGroupName> tag_group_catalog() {
    static constexpr TagGroupName names[]{
#define HALO2_TAG_GROUP(name, code) {tag_groups::name, #name},
#include "halo2/tags/groups.def"
#undef HALO2_TAG_GROUP
    };
    return names;
}
std::string_view tag_group_name(TagGroup group) {
    for (const auto& entry : tag_group_catalog())
        if (entry.group == group) return entry.name;
    return {};
}
std::string_view tag_name_strip_path(std::string_view name) {
    const auto separator = name.rfind('\\');
    return separator == std::string_view::npos ? name : name.substr(separator + 1);
}
} // namespace halo2
