#include "native_directory.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <utility>

namespace halo2::detail {
TagHandle insert_native_tag(std::vector<Tag>& tags, std::string group, std::string name) {
    constexpr std::size_t capacity = 32768;
    auto slot = tags.size();
    if (slot >= capacity) {
        const auto end = tags.begin() + capacity;
        const auto free = std::find_if(tags.begin(), end,
            [](const Tag& tag) { return tag.handle().is_null(); });
        if (free == end) throw FormatError("Native shader tag directory capacity exhausted");
        slot = std::size_t(free - tags.begin());
    }
    const TagHandle handle{0xd0000000u | std::uint32_t(slot)};
    Tag tag{std::move(group), std::move(name), handle.value, 0, 0};
    if (slot == tags.size()) tags.push_back(std::move(tag));
    else tags[slot] = std::move(tag);
    return handle;
}
}
