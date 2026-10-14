#include "halo2/tags/system.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <limits>

namespace halo2 {
namespace {
void require_range(std::uint32_t address, std::size_t size) {
    if (size > (std::uint64_t{1} << 32) - address)
        throw FormatError("Tag read wraps the source address space");
}
NativeTagAddress add(NativeTagAddress address,std::size_t offset) {
    if(offset>std::numeric_limits<std::uint64_t>::max()-address.value)
        throw FormatError("Native tag address wraps");
    return NativeTagAddress{address.value+offset};
}
}

// File pointer translation is confined to this adapter. Engine views retain
// actual 64-bit addresses of immutable cache backing; no host pointer is
// serialized or supplied to recovered Xbox code.
struct TagMemory {
    explicit TagMemory(std::shared_ptr<const Cache> owner) : cache(std::move(owner)) {}
    std::shared_ptr<const Cache> cache;
    std::vector<std::byte> resource(std::uint32_t pointer, std::uint32_t size) const {
        return cache->raw_data(pointer, size);
    }
    std::optional<std::size_t> bsp;
    std::uint32_t bsp_address{}, bsp_size{}, bsp_file{};
    TagHandle bsp_tag, lightmap_tag;
    std::uint32_t bsp_root{}, lightmap_root{};

    std::span<const std::byte> read_source(std::uint32_t address, std::size_t size) const {
        require_range(address, size);
        if (!address || address == 0xffffffffu)
            throw FormatError("Tag read has a null source pointer");
        if (bsp && address >= bsp_address && std::uint64_t(address) - bsp_address <= bsp_size &&
            size <= bsp_size - (std::uint64_t(address) - bsp_address)) {
            return Bytes(cache->cache_bytes()).slice(std::size_t(bsp_file) + address - bsp_address, size);
        }
        if (!cache->pc_geometry()) {
            // Main metadata is an allocation, not a set of independent tag
            // payloads. Retail forwarding pointers can cross tag extents.
            const Bytes bytes(cache->cache_bytes());
            const auto start = std::uint64_t(bytes.u32(16)) + bytes.u32(20);
            const auto extent = bytes.u32(24);
            const auto offset = cache->metadata_offset(address,size);
            if (std::uint64_t(offset) < start ||
                std::uint64_t(offset) - start > extent || size > extent - (std::uint64_t(offset) - start))
                throw FormatError("Tag pointer escapes resident cache metadata");
        }
        return cache->read_tag_bytes(address, size);
    }
    NativeTagAddress native_address(std::uint32_t source,std::size_t size) const {
        static_assert(sizeof(std::uintptr_t)==8,"Native tag owners require a 64-bit engine");
        return NativeTagAddress{reinterpret_cast<std::uintptr_t>(read_source(source,size).data())};
    }
    std::uint32_t source_address(NativeTagAddress address,std::size_t size) const {
        (void)add(address,size);
        const auto bytes=cache->cache_bytes();
        const auto base=std::uint64_t(reinterpret_cast<std::uintptr_t>(bytes.data()));
        if(address.value<base || address.value-base>bytes.size() || size>bytes.size()-(address.value-base))
            throw FormatError("Native tag address escapes its cache owner");
        const auto offset=std::size_t(address.value-base);
        std::int64_t source;
        if(bsp && offset>=bsp_file && offset-bsp_file<=bsp_size && size<=bsp_size-(offset-bsp_file))
            source=std::int64_t(bsp_address)+(offset-bsp_file);
        else source=std::int64_t(offset)+cache->metadata_magic_;
        if(source<=0 || source>0xffffffffll)
            throw FormatError("Native tag address has no serialized source mapping");
        const auto mapped=read_source(std::uint32_t(source),size);
        if(reinterpret_cast<std::uintptr_t>(mapped.data())!=address.value)
            throw FormatError("Native tag source mapping differs from its retained owner");
        return std::uint32_t(source);
    }
    std::span<const std::byte> read(NativeTagAddress address,std::size_t size) const {
        return read_source(source_address(address,size),size);
    }
    std::optional<Tag> resident(TagHandle handle, std::optional<TagGroup> expected) const {
        const auto* original = cache->find_tag(handle, expected);
        if (!original) return {};
        Tag tag = *original;
        if (!cache->pc_geometry() && (tag.group == "sbsp" || tag.group == "ltmp")) {
            if (!bsp) return {};
            if (handle == bsp_tag) tag.address = bsp_root;
            else if (handle == lightmap_tag) tag.address = lightmap_root;
            else return {};
            // Retail rows leave these sizes zero. The containing BSP
            // allocation bounds pointer access, including shared child banks.
            tag.size = std::uint32_t(std::uint64_t(bsp_address) + bsp_size - tag.address);
        }
        if (!tag.address) return {};
        read_source(tag.address, tag.size);
        return tag;
    }
    void select_bsp(std::size_t index) {
        if (index >= cache->bsps_.size()) throw FormatError("Tag BSP selection escapes scenario");
        if (cache->pc_geometry()) { bsp = index; return; } // Converted PC metadata is already resident.
        const auto& mapping = cache->bsps_[index];
        const auto bytes = Bytes(cache->cache_bytes()).slice(mapping.file_offset, mapping.size);
        const Bytes header(bytes);
        if (mapping.size < 16 || header.u32(0) != mapping.size || header.u32(12) != 0x73627370u)
            throw FormatError("Invalid tag BSP resource header");
        const auto* scenario = cache->find_tag(cache->scenario_tag_, tag_groups::scenario);
        if (!scenario) throw FormatError("Tag BSP has no scenario owner");
        const Bytes scenario_header(read_source(scenario->address, scenario->size));
        if (index >= scenario_header.u32(0x210)) throw FormatError("Tag BSP mapping count differs");
        const auto mapping_address = std::uint64_t(scenario_header.u32(0x214)) + index * 0x44;
        if (mapping_address > 0xffffffffu) throw FormatError("Tag BSP mapping address wraps");
        const Bytes row(read_source(std::uint32_t(mapping_address), 0x44));
        const TagHandle lightmap{row.u32(0x1c)};
        if (row.u32(0) != mapping.file_offset || row.u32(4) != mapping.size ||
            row.u32(8) != mapping.memory_address || row.u32(0x14) != mapping.datum ||
            !cache->find_tag({mapping.datum}, tag_groups::scenario_structure_bsp) ||
            (!lightmap.is_null() && !cache->find_tag(lightmap, tag_groups::scenario_structure_lightmap)))
            throw FormatError("Tag BSP resource disagrees with scenario identities");
        const auto root_valid = [&](std::uint32_t root) {
            return root >= std::uint64_t(mapping.memory_address) + 16 &&
                   root < std::uint64_t(mapping.memory_address) + mapping.size;
        };
        if (std::uint64_t(mapping.memory_address) + mapping.size > (std::uint64_t{1} << 32) ||
            !root_valid(header.u32(4)) || (!lightmap.is_null() && !root_valid(header.u32(8))))
            throw FormatError("Tag BSP roots escape their resource allocation");
        // Publication occurs only in the new immutable adapter; the caller
        // swaps it in after every check succeeds.
        bsp = index; bsp_address = mapping.memory_address; bsp_size = mapping.size;
        bsp_file = mapping.file_offset; bsp_tag = {mapping.datum}; lightmap_tag = lightmap;
        bsp_root = header.u32(4); lightmap_root = header.u32(8);
    }
};

TagSystem::TagSystem(std::shared_ptr<const Cache> cache) {
    if (!cache) throw FormatError("Tag system requires an owned cache");
    memory_ = std::make_shared<TagMemory>(std::move(cache));
}
const TagMemory& TagSystem::memory() const {
    if (!memory_) throw FormatError("Tag system has no loaded cache");
    return *memory_;
}
std::span<const TagGroupDefinition> TagSystem::groups() const { return memory().cache->tag_groups(); }
TagHandle TagSystem::scenario() const { return memory().cache->scenario_tag(); }
TagHandle TagSystem::globals() const { return memory().cache->globals_tag(); }
std::optional<TagView> TagSystem::try_get(TagHandle handle, std::optional<TagGroup> expected) const {
    const auto tag = memory().resident(handle, expected);
    if (!tag) return {};
    return TagView(memory_, *tag, memory().native_address(tag->address,tag->size));
}
TagView TagSystem::get(TagHandle handle, std::optional<TagGroup> expected) const {
    auto result = try_get(handle, expected);
    if (!result) throw FormatError("Tag handle, group or resident payload is unavailable");
    return *result;
}
TagHandle TagSystem::find(TagGroup group, std::string_view path) const {
    // Vista tag_loaded searches the exact group and compares case-insensitively.
    // Inheritance is deliberately confined to get/list, as in the source.
    for (const auto& tag : memory().cache->select_tags({.group=group, .name=path,
            .name_match=TagNameMatch::ignore_ascii_case}))
        if (!tag.handle().is_null()) return tag.handle();
    return {};
}
std::vector<TagHandle> TagSystem::list(std::optional<TagGroup> group) const {
    TagIterator iterator{0, {}, group};
    std::vector<TagHandle> result;
    for (auto tag = memory().cache->next_tag(iterator); !tag.is_null(); tag = memory().cache->next_tag(iterator))
        result.push_back(tag);
    return result;
}
std::optional<TagView> TagSystem::resolve(TagReference reference) const {
    if (reference.handle.is_null()) return {};
    return get(reference.handle, reference.group);
}
void TagSystem::load_bsp(std::size_t index) {
    auto next = std::make_shared<TagMemory>(memory().cache);
    next->select_bsp(index);
    memory_ = std::move(next);
}
void TagSystem::unload_bsp() { memory_ = std::make_shared<TagMemory>(memory().cache); }
std::optional<std::size_t> TagSystem::selected_bsp() const { return memory().bsp; }
std::span<const std::byte> TagSystem::read(NativeTagAddress address, std::size_t size) const {
    return memory().read(address, size);
}
NativeTagAddress TagSystem::native_address(std::uint32_t address,std::size_t size) const {
    return memory().native_address(address,size);
}
std::uint32_t TagSystem::source_address(NativeTagAddress address,std::size_t size) const {
    return memory().source_address(address,size);
}
std::span<const std::byte> TagSystem::read_source(std::uint32_t address,std::size_t size) const {
    return memory().read_source(address,size);
}
std::vector<std::byte> TagSystem::resource(std::uint32_t pointer, std::uint32_t size) const {
    return memory().resource(pointer, size);
}
std::string TagSystem::string_id(std::uint32_t id) const { return memory().cache->string_id(id); }
std::span<const std::byte> TagStructView::bytes() const { return memory_->read(address_, size_); }
std::uint32_t TagStructView::source_address() const { return memory_->source_address(address_,size_); }
std::span<const std::byte> TagStructView::read(std::size_t offset, std::size_t size) const {
    if (offset > size_ || size > size_ - offset)
        throw FormatError("Tag field escapes its definition");
    return memory_->read(add(address_,offset), size);
}
TagStructView TagStructView::structure(std::size_t offset, std::size_t size) const {
    read(offset, size);
    return {memory_, add(address_,offset), size};
}
std::uint16_t TagStructView::u16(std::size_t offset) const { return Bytes(read(offset,2)).u16(0); }
std::uint32_t TagStructView::u32(std::size_t offset) const { return Bytes(read(offset,4)).u32(0); }
float TagStructView::f32(std::size_t offset) const { return Bytes(read(offset,4)).f32(0); }
TagReference TagStructView::reference(std::size_t offset) const {
    const Bytes fields(read(offset, 8));
    return {TagGroup(fields.u32(0)), TagHandle{fields.u32(4)}};
}
TagBlockView TagStructView::block(std::size_t offset, std::size_t stride) const {
    const auto descriptor = read_tag_block_descriptor(Bytes(read(offset,8)),0);
    const auto extent = descriptor.extent(stride);
    TagBlockView result;
    result.memory_ = memory_;
    result.count_ = std::size_t(descriptor.count); result.stride_ = stride;
    if (descriptor.count) result.address_=memory_->native_address(descriptor.address,extent);
    return result;
}
TagStructView TagBlockView::element(std::size_t index) const {
    if (index >= count_) throw FormatError("Tag block element is outside its count");
    if(stride_ && index>std::numeric_limits<std::size_t>::max()/stride_)
        throw FormatError("Native tag block element offset wraps");
    const auto address = add(address_,index*stride_);
    memory_->read(address, stride_);
    return {memory_, address, stride_};
}
std::span<const std::byte> TagStructView::data(std::size_t offset) const {
    const Bytes descriptor(read(offset, 8));
    const auto size = std::bit_cast<std::int32_t>(descriptor.u32(0));
    if (size < 0) throw FormatError("Tag data has a negative size");
    if (!size) return {};
    return memory_->read(memory_->native_address(descriptor.u32(4),std::size_t(size)),std::size_t(size));
}
} // namespace halo2
