#pragma once
#include "halo2/tags/types.hpp"
#include <iterator>

namespace halo2 {
enum class TagNameMatch { exact, ignore_ascii_case };
struct TagAddressRange { std::uint32_t address; std::size_t size; };

// Exact directory filters for decoders. Inherited group lookup remains the
// separate get/find_tag operation; selecting a group never adds its children.
struct TagQuery {
    std::optional<TagHandle> handle{};
    std::optional<TagGroup> group{};
    std::optional<std::string_view> name{};
    std::optional<std::uint32_t> address{};
    std::optional<TagAddressRange> contains{};
    bool require_address = false;
    std::size_t minimum_size = 0;
    TagNameMatch name_match = TagNameMatch::exact;
    bool matches(const Tag&) const;
};

// Non-owning, allocation-free selection in original directory order. The
// cache and any name supplied to its query must outlive iteration.
class TagRange {
public:
    class Iterator {
    public:
        using value_type = Tag;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
        using reference = const Tag&;
        using pointer = const Tag*;
        Iterator() = default;
        reference operator*() const { return entries_[index_]; }
        pointer operator->() const { return &**this; }
        Iterator& operator++();
        Iterator operator++(int) { auto copy = *this; ++*this; return copy; }
        bool operator==(const Iterator& other) const {
            return entries_.data() == other.entries_.data() && index_ == other.index_;
        }
    private:
        friend class TagRange;
        Iterator(std::span<const Tag>, TagQuery, std::size_t);
        void advance();
        std::span<const Tag> entries_;
        TagQuery query_;
        std::size_t index_{};
    };
    TagRange(std::span<const Tag> entries, TagQuery query) : entries_(entries), query_(query) {}
    Iterator begin() const { return {entries_, query_, 0}; }
    Iterator end() const { return {entries_, query_, entries_.size()}; }
private:
    std::span<const Tag> entries_;
    TagQuery query_;
};
} // namespace halo2
