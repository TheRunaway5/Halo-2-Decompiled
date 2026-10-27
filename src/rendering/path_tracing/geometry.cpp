#include "halo2/path_tracing/geometry.hpp"
#include "rendering/models/model_skin_cpu_materialization.hpp"
#include <algorithm>
#include <limits>

namespace halo2::path_tracing {
namespace {
struct Record {
    std::uint32_t destination{},palette{},palette_count{},post_identity_placement{};
    std::array<std::uint32_t,4> bones{};
    std::array<float,4> weights{};
};
}
std::vector<Vertex> posed_vertices(std::span<const Vertex> input,
    std::span<const ModelSkinSubmission> submissions) {
    if(input.size()>std::numeric_limits<std::uint32_t>::max())
        throw FormatError("Path Tracing geometry exceeds its vertex index domain");
    // Validate complete declared ranges before materializing any one owner.
    std::vector<std::pair<std::size_t,std::size_t>> ranges;
    ranges.reserve(submissions.size());
    std::size_t records_count{},palette_count{};
    for(const auto& submission:submissions) {
        if(!submission.geometry)throw FormatError("Path Tracing skin has no geometry owner");
        const auto& geometry=*submission.geometry;
        const auto count=geometry.vertices.size();
        if(submission.first>input.size() || count>input.size()-submission.first ||
           geometry.bindings.size()!=count || geometry.source_vertices.size()!=count)
            throw FormatError("Path Tracing skin lost its complete source range");
        if(geometry.palette.size()>std::numeric_limits<std::uint32_t>::max()-palette_count)
            throw FormatError("Path Tracing skin palette exceeds its index domain");
        if(count)ranges.emplace_back(submission.first,submission.first+count);
        records_count+=count;palette_count+=geometry.palette.size();
    }
    std::sort(ranges.begin(),ranges.end());
    for(std::size_t i=1;i<ranges.size();++i)if(ranges[i].first<ranges[i-1].second)
        throw FormatError("Path Tracing skin submissions overlap");
    std::vector<Vertex> output(input.begin(),input.end());
    std::vector<Record> records;records.reserve(records_count);
    std::vector<std::array<float,12>> palette;palette.reserve(palette_count);
    for(const auto& submission:submissions) {
        const auto& geometry=*submission.geometry;
        const auto first=std::uint32_t(palette.size());
        palette.insert(palette.end(),geometry.palette.begin(),geometry.palette.end());
        for(std::size_t i=0;i<geometry.vertices.size();++i) {
            const auto& binding=geometry.bindings[i];
            Record record;
            record.destination=std::uint32_t(submission.first+i);
            record.palette=first;record.palette_count=std::uint32_t(geometry.palette.size());
            record.post_identity_placement=geometry.post_identity_placement;
            std::copy(binding.bones.begin(),binding.bones.end(),record.bones.begin());
            record.weights=binding.weights;records.push_back(record);
        }
    }
    model_skin_cpu_materialize(std::span{output},std::span{records},
        std::span<const std::array<float,12>>{palette});
    return output;
}
}
