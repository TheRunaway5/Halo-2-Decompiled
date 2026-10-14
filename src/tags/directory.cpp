#include "halo2/cache.hpp"
#include "directory.hpp"
#include <algorithm>

namespace halo2 {
namespace {
bool equal_name(std::string_view a, std::string_view b, TagNameMatch match) {
    if (match == TagNameMatch::exact) return a == b;
    if (a.size() != b.size()) return false;
    const auto lower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
    for (std::size_t i = 0; i < a.size(); ++i)
        if (lower(a[i]) != lower(b[i])) return false;
    return true;
}
}
bool TagQuery::matches(const Tag& tag) const {
    if ((handle && tag.handle() != *handle) ||
        (group && (tag.group.size() != 4 || tag.type() != *group)) ||
        (name && !equal_name(tag.name, *name, name_match)) ||
        (address && tag.address != *address) || (require_address && !tag.address) ||
        tag.size < minimum_size) return false;
    if (contains) {
        if (contains->address < tag.address ||
            contains->size > (std::uint64_t{1} << 32) - contains->address) return false;
        const auto offset = std::uint64_t(contains->address) - tag.address;
        if (offset > tag.size || contains->size > tag.size - offset) return false;
    }
    return true;
}
TagRange::Iterator::Iterator(std::span<const Tag> entries, TagQuery query, std::size_t index)
    : entries_(entries), query_(query), index_(index) { advance(); }
void TagRange::Iterator::advance() {
    while (index_ < entries_.size() && !query_.matches(entries_[index_])) ++index_;
}
TagRange::Iterator& TagRange::Iterator::operator++() { ++index_; advance(); return *this; }
const Tag* Cache::query_tag(TagQuery query) const {
    if (query.handle) {
        const auto* tag = find_tag(*query.handle);
        return tag && query.matches(*tag) ? tag : nullptr;
    }
    const auto selected = select_tags(query);
    const auto first = selected.begin();
    return first == selected.end() ? nullptr : &*first;
}
const Tag& Cache::tag_at_index(std::size_t index) const {
    if (index >= tags_.size()) throw FormatError("Tag table slot escapes cache directory");
    return tags_[index];
}
std::span<const std::byte> Cache::read_tag_bytes(std::uint32_t address, std::size_t size) const {
    return Bytes(cache_bytes()).slice(metadata_offset(address, size), size);
}
std::span<const std::byte> Cache::tag_payload(TagHandle handle) const {
    const auto* tag=find_tag(handle);
    if (!tag || !tag->address)
        throw FormatError("Resident source tag metadata is unavailable");
    if (tag->type()==tag_groups::scenario_structure_bsp || tag->type()==tag_groups::scenario_structure_lightmap)
        throw FormatError("Source BSP tag requires its own metadata mapping");
    auto extent=tag->size;
    if(!extent) {
        // Xbox caches deduplicate complete definitions: zero-sized tag
        // aliases retain the same address/group as their owned payload.
        // Resolve only that exact identity, never a neighbouring allocation.
        for(const auto& owner:tags_)if(owner.address==tag->address && owner.group==tag->group && owner.size) {
            if(extent && extent!=owner.size)throw FormatError("Ambiguous source tag alias extent");
            extent=owner.size;
        }
        if(!extent)throw FormatError("Source tag alias has no owned same-address payload");
    }
    return read_tag_bytes(tag->address,extent);
}
std::vector<TagGroupDefinition> read_tag_group_definitions(
    const Bytes& bytes, std::size_t index, std::int64_t header_magic) {
    if (bytes.u32(index + 28) != 0x74616773u)
        throw FormatError("Invalid cache tag-header signature");
    const auto count = bytes.u32(index + 4);
    const auto start = std::int64_t(bytes.u32(index)) - header_magic;
    if (!count || count > 32768 || start < 0)
        throw FormatError("Invalid cache tag-group directory");
    bytes.require(std::size_t(start), std::size_t(count) * 12);
    std::vector<TagGroupDefinition> groups;
    groups.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto at = std::size_t(start) + i * 12;
        TagGroupDefinition row{TagGroup(bytes.u32(at)), TagGroup(bytes.u32(at + 4)),
                               TagGroup(bytes.u32(at + 8))};
        if (row.group == tag_groups::none ||
            (!groups.empty() && row.group <= groups.back().group))
            throw FormatError("Cache tag groups must be unique and sorted");
        groups.push_back(row);
    }
    for (const auto& row : groups) {
        for (auto ancestor : {row.parent, row.grandparent}) {
            const auto found = std::lower_bound(groups.begin(), groups.end(), ancestor,
                [](const TagGroupDefinition& entry, TagGroup key) { return entry.group < key; });
            if (ancestor != tag_groups::none && (ancestor == row.group ||
                found == groups.end() || found->group != ancestor))
                throw FormatError("Cache tag-group ancestor is unavailable or self-referential");
        }
    }
    return groups;
}
void Cache::initialize_tag_directory(const Bytes& bytes, std::size_t index,
                                     std::int64_t header_magic) {
    tag_groups_ = read_tag_group_definitions(bytes, index, header_magic);
    scenario_tag_ = {bytes.u32(index + 12)};
    globals_tag_ = {bytes.u32(index + 16)};
}
void Cache::validate_tag_directory() const {
    for (std::size_t i = 0; i < tags_.size(); ++i) {
        const auto& tag = tags_[i];
        if (tag.handle().is_null()) continue;
        if (tag.handle().index() != i || i >= 32768 || !tag_group(tag.type()))
            throw FormatError("Cache tag identity disagrees with its slot or group table");
        if (std::uint64_t(tag.address) + tag.size > (std::uint64_t{1} << 32))
            throw FormatError("Cache tag payload wraps the source address space");
    }
    if (!scenario_tag_.is_null() && !find_tag(scenario_tag_, tag_groups::scenario))
        throw FormatError("Cache header references an invalid scenario tag");
    if (!globals_tag_.is_null() && !find_tag(globals_tag_, tag_groups::globals))
        throw FormatError("Cache header references an invalid globals tag");
}
const TagGroupDefinition* Cache::tag_group(TagGroup group) const {
    const auto found = std::lower_bound(tag_groups_.begin(), tag_groups_.end(), group,
        [](const TagGroupDefinition& row, TagGroup key) { return row.group < key; });
    return found != tag_groups_.end() && found->group == group ? &*found : nullptr;
}
const Tag* Cache::find_tag(TagHandle handle, std::optional<TagGroup> expected) const {
    // Retail cache_tag_instance_get sign-extends the low short and checks the
    // complete salted datum. A map can have sparse unused directory slots.
    const auto slot = handle.index();
    if (handle.is_null() || slot >= 32768 || slot >= tags_.size()) return nullptr;
    const auto& tag = tags_[slot];
    if (tag.datum != handle.value) return nullptr;
    if (expected) {
        const auto* definition = tag_group(tag.type());
        if (!definition || !definition->matches(*expected)) return nullptr;
    }
    return &tag;
}
TagHandle Cache::next_tag(TagIterator& iterator) const {
    // Retail 122C70 includes unloaded tags and retains current on exhaustion.
    while (iterator.next_index < tags_.size()) {
        const auto& tag = tags_[iterator.next_index++];
        if (tag.handle().is_null() || tag.group.empty() || tag.type() == tag_groups::none) continue;
        if (iterator.group && *iterator.group != tag_groups::none) {
            const auto* definition = tag_group(tag.type());
            if (!definition || !definition->matches(*iterator.group)) continue;
        }
        iterator.current = tag.handle();
        return iterator.current;
    }
    return {};
}
} // namespace halo2
