#pragma once
#include "halo2/scene.hpp"
#include <array>
#include <cstring>
#include <map>
#include <span>
#include <vector>

namespace halo2 {
// Frame-local aliases into an append-only CPU vertex bank. Sharing requires
// the complete raw stream to match, including authored stipple/part metadata.
// Draw order, descriptors, owners and view constants belong to the caller.
template<class Key> class FrameVertexReuse {
    struct Range {std::size_t first{},count{};};
    std::vector<Vertex> &vertices_;
    std::map<Key,std::vector<Range>> ranges_;
    bool enabled_;
public:
    FrameVertexReuse(std::vector<Vertex> &vertices,bool enabled):vertices_(vertices),enabled_(enabled) {}
    std::size_t append(const Key &key,std::span<const Vertex> vertices) {
        return append_parts(key,std::array{vertices});
    }
    template<class Parts> std::size_t append_parts(const Key &key,const Parts &parts) {
        std::size_t count{};for(const auto part:parts)count+=part.size();
        auto *ranges=enabled_?&ranges_[key]:nullptr;
        if(ranges)for(const auto &range:*ranges) {
            if(range.count!=count)continue;
            std::size_t offset{};bool equal=true;
            for(const auto part:parts) {
                if(!part.empty() && std::memcmp(vertices_.data()+range.first+offset,
                    part.data(),part.size_bytes())!=0) {equal=false;break;}
                offset+=part.size();
            }
            if(equal)return range.first;
        }
        const auto first=vertices_.size();
        for(const auto part:parts)vertices_.insert(vertices_.end(),part.begin(),part.end());
        if(ranges)ranges->push_back({first,count});
        return first;
    }
};
}
