#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <map>
#include <ostream>
#include <span>
#include <vector>

namespace halo2 {
// Borrow immutable CPU streams through this immediate frame's upload. Every
// keyed span must remain alive and unchanged until copy_to/write_to returns.
// Draw/owner/view state stays with the caller; only identical raw storage is
// shared. Raw, unkeyed appends may be edited before upload (the UI tail).
template<class Key> class FrameVertexUploadPlan {
    struct Range {std::size_t first{},count{},chunk_first{},chunk_count{};};
    std::vector<std::span<const Vertex>> chunks_;
    std::map<Key,std::vector<Range>> ranges_;
    std::size_t size_{};
    bool share_;
    template<class Parts> bool equal(const Range &range,const Parts &parts) const {
        auto input=std::begin(parts);const auto end=std::end(parts);
        std::size_t input_offset{};
        for(const auto retained:std::span{chunks_}.subspan(range.chunk_first,range.chunk_count)) {
            std::size_t offset{};
            while(offset<retained.size()) {
                while(input!=end && input_offset==(*input).size()) {++input;input_offset=0;}
                if(input==end)return false;
                const auto incoming=*input;
                const auto count=std::min(retained.size()-offset,incoming.size()-input_offset);
                if(std::memcmp(retained.data()+offset,incoming.data()+input_offset,count*sizeof(Vertex)))return false;
                offset+=count;input_offset+=count;
            }
        }
        return true;
    }
public:
    explicit FrameVertexUploadPlan(bool share):share_(share) {}
    std::size_t size() const {return size_;}
    bool empty() const {return !size_;}
    std::size_t append(const Key &key,std::span<const Vertex> vertices) {
        return append_parts(key,std::array{vertices});
    }
    template<class Parts> std::size_t append_parts(const Key &key,const Parts &parts) {
        std::size_t count{};for(const auto part:parts)count+=part.size();
        auto *ranges=share_?&ranges_[key]:nullptr;
        if(ranges)for(const auto &range:*ranges)
            if(range.count==count && equal(range,parts))return range.first;
        const auto first=size_,chunk_first=chunks_.size();
        for(const auto part:parts)append_raw(part);
        if(ranges)ranges->push_back({first,count,chunk_first,chunks_.size()-chunk_first});
        return first;
    }
    std::size_t append_raw(std::span<const Vertex> vertices) {
        const auto first=size_;
        if(!vertices.empty())chunks_.push_back(vertices);
        size_+=vertices.size();return first;
    }
    void copy_to(void *destination) const {
        copy_to(destination,0,size_);
    }
    // The finalized prefix can be copied independently of the editable UI
    // tail. Both source storage and destination must outlive the copy.
    void copy_to(void *destination,std::size_t first,std::size_t count) const {
        if(first>size_ || count>size_-first)throw FormatError("frame vertex copy range escapes its plan");
        if(count && !destination)throw FormatError("frame vertex copy lacks its destination");
        auto *output=static_cast<std::byte *>(destination);
        for(const auto part:chunks_) {
            if(first>=part.size()){first-=part.size();continue;}
            const auto n=std::min(count,part.size()-first);
            if(n)std::memcpy(output,part.data()+first,n*sizeof(Vertex));
            output+=n*sizeof(Vertex);count-=n;first=0;
            if(!count)break;
        }
    }
    void write_to(std::ostream &output) const {
        for(const auto part:chunks_)output.write(reinterpret_cast<const char *>(part.data()),std::streamsize(part.size_bytes()));
    }
};
}
