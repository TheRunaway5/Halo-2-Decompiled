#pragma once
#include "halo2/tags/directory.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

namespace halo2 {
class Cache;
struct TagMemory;
class TagStructView;

// Immutable, owning views keep their cache alive. Selecting another BSP or
// closing a TagSystem does not invalidate a view already handed to a caller.
class TagBlockView {
public:
    std::size_t size() const { return count_; }
    TagStructView element(std::size_t index) const;
private:
    friend class TagStructView;
    std::shared_ptr<const TagMemory> memory_;
    std::uint32_t address_{};
    std::size_t count_{}, stride_{};
};
// The same field interface works for a tag root, a block element, and an
// embedded structure, so nested tag blocks need no caller pointer arithmetic.
class TagStructView {
public:
    std::uint32_t source_address() const { return address_; }
    std::span<const std::byte> bytes() const;
    std::span<const std::byte> read(std::size_t offset, std::size_t size) const;
    TagStructView structure(std::size_t offset, std::size_t size) const;
    std::uint16_t u16(std::size_t offset) const;
    std::uint32_t u32(std::size_t offset) const;
    float f32(std::size_t offset) const;
    TagBlockView block(std::size_t descriptor_offset, std::size_t element_size) const;
    std::span<const std::byte> data(std::size_t descriptor_offset) const;
    TagReference reference(std::size_t descriptor_offset) const;
protected:
    TagStructView(std::shared_ptr<const TagMemory> memory, std::uint32_t address, std::size_t size)
        : memory_(std::move(memory)), address_(address), size_(size) {}
private:
    friend class TagBlockView;
    std::shared_ptr<const TagMemory> memory_;
    std::uint32_t address_;
    std::size_t size_;
};
class TagView : public TagStructView {
public:
    const Tag& info() const { return tag_; }
private:
    friend class TagSystem;
    TagView(std::shared_ptr<const TagMemory> memory, Tag tag)
        : TagStructView(std::move(memory), tag.address, tag.size), tag_(std::move(tag)) {}
    Tag tag_;
};

// Engine-facing tag module. Cache performs file-format conversion; this
// module owns tag queries, residency, safe binary access and reference checks.
// Mutation (load/unload/close) is confined to the owning engine thread.
class TagSystem {
public:
    explicit TagSystem(std::shared_ptr<const Cache>);
    bool loaded() const { return bool(memory_); }
    void close() { memory_.reset(); }
    std::span<const TagGroupDefinition> groups() const;
    TagHandle scenario() const;
    TagHandle globals() const;
    std::optional<TagView> try_get(TagHandle, std::optional<TagGroup> expected = {}) const;
    TagView get(TagHandle, std::optional<TagGroup> expected = {}) const;
    TagHandle find(TagGroup, std::string_view path) const;
    std::vector<TagHandle> list(std::optional<TagGroup> group = {}) const;
    std::optional<TagView> resolve(TagReference) const;
    // Loading validates the complete resource and publishes both sbsp and
    // ltmp together. Failure preserves the previously selected BSP.
    void load_bsp(std::size_t index);
    void unload_bsp();
    std::optional<std::size_t> selected_bsp() const;
    std::span<const std::byte> read(std::uint32_t source_address, std::size_t size) const;
    std::vector<std::byte> resource(std::uint32_t encoded_pointer, std::uint32_t size) const;
    std::string string_id(std::uint32_t id) const;
private:
    const TagMemory& memory() const;
    std::shared_ptr<const TagMemory> memory_;
};
} // namespace halo2
