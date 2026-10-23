#pragma once
#include "halo2/model_skin_submission.hpp"
#include "rendering/models/model_skin_gpu.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <numeric>

namespace halo2 {
// Immediate-frame provenance for the bytes actually appended to debug_buffer.
// Registered banks must remain alive and unchanged through the final upload.
// Shared geometry/bindings/palettes must remain immutable; the plan retains
// their owner. Equal local bytes never merge poses.
// Register all banks before appending, disable byte dedup for contains(parts),
// and append every actual copied part at its final destination before validate.
class FrameModelSkinPlan {
    struct AddressRange {std::uintptr_t first{},end{};};
    struct Source {
        std::span<const Vertex> vertices;
        std::shared_ptr<const ModelSkinGeometry> geometry;
        std::uint32_t palette{};
        std::vector<std::size_t> mapped;
    };
    struct DestinationRange {std::uint64_t first{},end{};};
    struct PendingRun {
        std::size_t source{},vertex{},count{};
        DestinationRange destination;
    };
    std::vector<Source> sources_;
    std::map<const ModelSkinGeometry*,std::uint32_t> palettes_;
    std::vector<std::array<float,12>> palette_;
    std::vector<ModelSkinGpuRecord> records_;
    std::vector<DestinationRange> destinations_;
    std::size_t expected_{},mapped_{};
    bool appending_{};

    static AddressRange address(std::span<const Vertex> vertices) {
        if(vertices.empty())return {};
        const auto first=reinterpret_cast<std::uintptr_t>(vertices.data());
        if(!first || first%alignof(Vertex) ||
           vertices.size()>(std::numeric_limits<std::uintptr_t>::max()-first)/sizeof(Vertex))
            throw FormatError("frame model skin source has an invalid address range");
        return {first,first+vertices.size()*sizeof(Vertex)};
    }
    static bool overlaps(AddressRange a,AddressRange b) {
        return a.first<b.end && b.first<a.end;
    }
    static bool same_local(const Vertex &a,const Vertex &b) {
        return !std::memcmp(&a.position,&b.position,sizeof(Vec3)) &&
            !std::memcmp(&a.normal,&b.normal,sizeof(Vec3)) &&
            !std::memcmp(&a.tangent,&b.tangent,sizeof(Vec3)) &&
            !std::memcmp(&a.binormal,&b.binormal,sizeof(Vec3)) &&
            a.source_section==b.source_section && a.source_part==b.source_part;
    }
    static bool same_binding(const SkinBinding &a,const SkinBinding &b) {
        return a.bones==b.bones &&
            !std::memcmp(a.weights.data(),b.weights.data(),sizeof(a.weights));
    }
    static void geometry_valid(const ModelSkinGeometry &geometry) {
        const auto count=geometry.vertices.size();
        if(geometry.bindings.size()!=count || geometry.source_vertices.size()!=count)
            throw FormatError("frame model skin geometry lost its parallel bindings/source ordinals");
        if(geometry.palette.size()>std::numeric_limits<std::uint32_t>::max() ||
           !model_skin_gpu_arithmetic_admitted(geometry.vertices,geometry.bindings,geometry.palette))
            throw FormatError("frame model skin geometry leaves its admitted palette/local domain");
        // A section can appear repeatedly, with a different selected material.
        // Its original expanded ordinal must still name the same deformation.
        const auto compare=[&](std::size_t first,std::size_t next) {
            if(!same_local(geometry.vertices[first],geometry.vertices[next]) ||
               !same_binding(geometry.bindings[first],geometry.bindings[next]))
                throw FormatError("frame model skin repeated source ordinal changes its local vertex/binding");
        };
        if(!count)return;
        const auto largest=*std::max_element(geometry.source_vertices.begin(),geometry.source_vertices.end());
        // Expanded ordinals normally occupy a compact original vertex bank.
        // One dense index avoids a heap node for every vertex. Sparse/malformed
        // ordinal banks still receive the identical duplicate check without an
        // allocation controlled by an arbitrary uint32 ordinal.
        if(std::uint64_t(largest)+1<=std::uint64_t(count)*8) {
            constexpr auto absent=std::numeric_limits<std::size_t>::max();
            std::vector<std::size_t> ordinals(std::size_t(largest)+1,absent);
            for(std::size_t i=0;i<count;++i) {
                auto &first=ordinals[geometry.source_vertices[i]];
                if(first==absent)first=i;else compare(first,i);
            }
        } else {
            std::vector<std::size_t> order(count);std::iota(order.begin(),order.end(),0);
            std::sort(order.begin(),order.end(),[&](auto a,auto b) {
                const auto first=geometry.source_vertices[a],next=geometry.source_vertices[b];
                return first<next || (first==next && a<b);
            });
            for(std::size_t i=1;i<count;++i)
                if(geometry.source_vertices[order[i-1]]==geometry.source_vertices[order[i]])
                    compare(order[i-1],order[i]);
        }
    }
public:
    void add_bank(std::span<const Vertex> vertices,std::span<const ModelSkinSubmission> submissions) {
        if(appending_)throw FormatError("frame model skin bank registered after append began");
        address(vertices);
        if(submissions.empty())return;
        // Complete structural validation before adding any of this bank.
        std::vector<Source> pending;
        auto palettes=palettes_;
        std::vector<std::shared_ptr<const ModelSkinGeometry>> new_palettes;
        std::size_t palette_size=palette_.size(),expected=expected_;
        for(const auto &submission:submissions) {
            if(!submission.geometry)throw FormatError("frame model skin submission has no geometry owner");
            const auto &geometry=*submission.geometry;
            const auto count=geometry.vertices.size();
            if(submission.first>vertices.size() || count>vertices.size()-submission.first)
                throw FormatError("frame model skin submission escapes its source bank");
            // The shared geometry is immutable for this entire frame. A
            // palette identity already admitted in this plan need not repeat
            // its arithmetic and expanded-ordinal scan for another bank.
            if(!palettes.contains(&geometry))geometry_valid(geometry);
            const auto source=vertices.subspan(submission.first,count);
            for(std::size_t i=0;i<count;++i)if(!same_local(source[i],geometry.vertices[i]))
                throw FormatError("frame model skin source differs from its admitted local attributes");
            if(!count)continue;
            const auto range=address(source);
            for(const auto &prior:sources_)if(overlaps(range,address(prior.vertices)))
                throw FormatError("frame model skin source aliases another submission");
            for(const auto &prior:pending)if(overlaps(range,address(prior.vertices)))
                throw FormatError("frame model skin source aliases another submission");
            auto palette=palettes.find(&geometry);
            if(palette==palettes.end()) {
                if(geometry.palette.size()>std::numeric_limits<std::uint32_t>::max()-palette_size)
                    throw FormatError("frame model skin merged palette exceeds its index width");
                palette=palettes.emplace(&geometry,std::uint32_t(palette_size)).first;
                palette_size+=geometry.palette.size();new_palettes.push_back(submission.geometry);
            }
            if(count>std::numeric_limits<std::size_t>::max()-expected)
                throw FormatError("frame model skin source count overflows");
            expected+=count;
            pending.push_back({source,submission.geometry,palette->second,std::vector<std::size_t>(count)});
        }
        if(pending.size()>sources_.max_size()-sources_.size())
            throw FormatError("frame model skin source count exceeds its storage width");
        const auto source_required=sources_.size()+pending.size();
        if(source_required>sources_.capacity()) {
            const auto capacity=sources_.capacity();
            const auto grown=capacity<=sources_.max_size()/2?capacity*2:sources_.max_size();
            sources_.reserve(std::max(source_required,grown));
        }
        if(palette_size>palette_.capacity()) {
            const auto capacity=palette_.capacity();
            const auto grown=capacity<=palette_.max_size()/2?capacity*2:palette_.max_size();
            palette_.reserve(std::max(palette_size,grown));
        }
        for(const auto &geometry:new_palettes)
            palette_.insert(palette_.end(),geometry->palette.begin(),geometry->palette.end());
        for(auto &source:pending)sources_.push_back(std::move(source));
        palettes_=std::move(palettes);expected_=expected;
    }
    bool contains(std::span<const Vertex> vertices)const {
        const auto range=address(vertices);
        for(const auto &source:sources_)if(overlaps(range,address(source.vertices)))return true;
        return false;
    }
    template<class Parts> bool contains_parts(const Parts &parts)const {
        for(const auto part:parts)if(contains(part))return true;
        return false;
    }
    void append(std::span<const Vertex> vertices,std::size_t destination_first) {
        appending_=true;
        const auto part=address(vertices);
        if(vertices.size()>std::numeric_limits<std::size_t>::max()-destination_first)
            throw FormatError("frame model skin append destination overflows");
        std::vector<PendingRun> pending;
        std::size_t additional{};
        for(std::size_t s=0;s<sources_.size();++s) {
            const auto &source=sources_[s];const auto range=address(source.vertices);
            if(!overlaps(part,range))continue;
            const auto first=std::max(part.first,range.first),end=std::min(part.end,range.end);
            if((first-part.first)%sizeof(Vertex) || (first-range.first)%sizeof(Vertex))
                throw FormatError("frame model skin append cuts through a source vertex");
            const auto input=(first-range.first)/sizeof(Vertex),offset=(first-part.first)/sizeof(Vertex);
            const auto count=(end-first)/sizeof(Vertex),destination=destination_first+offset;
            if(count && (destination>std::numeric_limits<std::uint32_t>::max() ||
                count-1>std::numeric_limits<std::uint32_t>::max()-destination))
                throw FormatError("frame model skin destination exceeds its index width");
            for(std::size_t i=0;i<count;++i)
                if(!same_local(source.vertices[input+i],source.geometry->vertices[input+i]))
                    throw FormatError("frame model skin source changed after registration");
            if(count>records_.max_size()-additional || records_.size()>records_.max_size()-additional-count)
                throw FormatError("frame model skin record count exceeds its storage width");
            additional+=count;
            pending.push_back({s,input,count,{destination,std::uint64_t(destination)+count}});
        }
        // Source registration order need not match the renderer's grouping.
        std::sort(pending.begin(),pending.end(),[](const auto &a,const auto &b){return a.destination.first<b.destination.first;});
        for(std::size_t i=0;i<pending.size();++i) {
            const auto range=pending[i].destination;
            const auto next=std::lower_bound(destinations_.begin(),destinations_.end(),range.first,
                [](const auto &prior,auto first){return prior.first<first;});
            if((i && pending[i-1].destination.end>range.first) ||
               (next!=destinations_.end() && next->first<range.end) ||
               (next!=destinations_.begin() && (next-1)->end>range.first))
                throw FormatError("frame model skin append aliases an existing GPU destination");
        }
        if(pending.empty())return;
        const auto required=records_.size()+additional;
        if(required>records_.capacity()) {
            // Exact reserve(size+run) on every material append repeatedly
            // copied all previous records. Retain bounded geometric growth.
            const auto capacity=records_.capacity();
            const auto grown=capacity<=records_.max_size()/2?capacity*2:records_.max_size();
            records_.reserve(std::max(required,grown));
        }
        if(pending.size()>destinations_.max_size()-destinations_.size())
            throw FormatError("frame model skin destination range count exceeds its storage width");
        const auto range_required=destinations_.size()+pending.size();
        if(range_required>destinations_.capacity()) {
            const auto capacity=destinations_.capacity();
            const auto grown=capacity<=destinations_.max_size()/2?capacity*2:destinations_.max_size();
            destinations_.reserve(std::max(range_required,grown));
        }
        for(const auto &entry:pending) {
            const auto &source=sources_[entry.source];
            for(std::size_t i=0;i<entry.count;++i) {
                const auto &binding=source.geometry->bindings[entry.vertex+i];
                ModelSkinGpuRecord record;
                record.destination=std::uint32_t(entry.destination.first+i);record.palette=source.palette;
                for(unsigned j=0;j<4;++j)record.bones[j]=binding.bones[j];
                record.weights=binding.weights;record.palette_count=std::uint32_t(source.geometry->palette.size());
                record.post_identity_placement=source.geometry->post_identity_placement;
                records_.push_back(record);
                auto &count=sources_[entry.source].mapped[entry.vertex+i];if(!count)++mapped_;++count;
            }
            const auto range=entry.destination;
            auto next=std::lower_bound(destinations_.begin(),destinations_.end(),range.first,
                [](const auto &prior,auto first){return prior.first<first;});
            if(next!=destinations_.begin() && (next-1)->end==range.first) {
                auto prior=next-1;prior->end=range.end;
                if(next!=destinations_.end() && next->first==range.end) {
                    prior->end=next->end;destinations_.erase(next);
                }
            } else if(next!=destinations_.end() && next->first==range.end)next->first=range.first;
            else destinations_.insert(next,range);
        }
    }
    template<class Parts> void append_parts(const Parts &parts,std::size_t destination_first) {
        for(const auto part:parts) {
            append(part,destination_first);
            if(part.size()>std::numeric_limits<std::size_t>::max()-destination_first)
                throw FormatError("frame model skin append destination overflows");
            destination_first+=part.size();
        }
    }
    std::span<const ModelSkinGpuRecord> records()const{return records_;}
    std::span<const std::array<float,12>> palette()const{return palette_;}
    std::size_t expected_vertices()const{return expected_;}
    std::size_t mapped_vertices()const{return mapped_;}
    void validate_complete()const {
        if(mapped_!=expected_)throw FormatError("frame model skin grouping omitted a registered source vertex");
    }
};
}
