#pragma once
#include "halo2/model.hpp"
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
namespace halo2 {
// Per-section vertex ordinals of an immutable model bank: for each section id,
// the ascending ordinals n with mesh.vertices[n].source_section==id. Built once
// per bank and published on the model (copies start empty and rebuild).
struct ModelSectionIndex {
    const Vertex *vertices{};std::size_t count{};std::uint32_t checksum{};
    struct Range {std::uint32_t section,first,count;};
    std::vector<Range> ranges;           // ascending by section id
    std::vector<std::uint32_t> ordinals; // grouped by range, ascending inside
    std::span<const std::uint32_t> section(std::uint32_t id)const noexcept;
};
std::shared_ptr<const ModelSectionIndex> model_section_index(const RenderModel &model);
// Appends posed[n] (and n to ordinals, when given) for every vertex of
// `section`, ascending n: the same records in the same order as the linear
// scan `for n: if(posed[n].source_section==section)`. posed must be a complete
// pose of model (one record per model vertex, in model order). Returns the
// number appended. HALO2_VERIFY_SECTION_INDEX=1 also runs the scan and
// rejects any difference.
std::size_t model_section_select(const RenderModel &model,std::span<const Vertex> posed,std::uint32_t section,
                                 std::vector<Vertex> &vertices,std::vector<std::uint32_t> *ordinals=nullptr);
}
