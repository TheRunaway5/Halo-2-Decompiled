#pragma once
#include "halo2/tags/directory.hpp"
#include <algorithm>
#include <vector>
namespace halo2::detail {
// Directory order and the full predicate decide results. Sorted interval
// metadata only eliminates entries that cannot possibly contain the query.
class TagOwnerIndex {
    struct Interval {std::uint32_t address;std::uint64_t end,maximum;std::size_t ordinal;};
    std::span<const Tag> tags_;
    std::vector<Interval> intervals_;
public:
    explicit TagOwnerIndex(std::span<const Tag> tags):tags_(tags) {
        intervals_.reserve(tags.size());
        for(std::size_t i=0;i<tags.size();++i)
            intervals_.push_back({tags[i].address,std::uint64_t(tags[i].address)+tags[i].size,0,i});
        std::sort(intervals_.begin(),intervals_.end(),[](const auto &a,const auto &b){return a.address<b.address;});
        std::uint64_t maximum{};
        for(auto &entry:intervals_)entry.maximum=maximum=std::max(maximum,entry.end);
    }
    std::vector<const Tag*> select(const TagQuery &query)const {
        std::vector<const Tag*> found;
        if(!query.contains) {
            for(const auto &tag:tags_)if(query.matches(tag))found.push_back(&tag);
            return found;
        }
        const auto [address,size]=*query.contains;
        if(size>(std::uint64_t{1}<<32)-address)return found;
        const auto limit=std::uint64_t(address)+size;
        auto at=std::upper_bound(intervals_.begin(),intervals_.end(),address,
            [](auto value,const auto &entry){return value<entry.address;});
        while(at!=intervals_.begin()) {
            --at;if(at->maximum<limit)break;
            if(at->end>=limit && query.matches(tags_[at->ordinal]))found.push_back(&tags_[at->ordinal]);
        }
        std::sort(found.begin(),found.end(),[&](const Tag *a,const Tag *b){return a-tags_.data()<b-tags_.data();});
        return found;
    }
};
}
namespace halo2 {
class Cache;
std::vector<const Tag*> indexed_tag_owners(const Cache&,TagQuery);
}
