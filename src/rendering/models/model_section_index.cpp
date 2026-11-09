#include "halo2/model_section_index.hpp"
#include <algorithm>
#include <cstdlib>
namespace halo2 {
std::span<const std::uint32_t> ModelSectionIndex::section(std::uint32_t id)const noexcept {
    const auto at=std::lower_bound(ranges.begin(),ranges.end(),id,
        [](const Range &range,std::uint32_t value){return range.section<value;});
    if(at==ranges.end() || at->section!=id)return {};
    return std::span<const std::uint32_t>(ordinals).subspan(at->first,at->count);
}
std::shared_ptr<const ModelSectionIndex> model_section_index(const RenderModel &model) {
    const auto &bank=model.mesh.vertices;
    if(auto retained=model.section_index.load();retained && retained->vertices==bank.data() &&
       retained->count==bank.size() && retained->checksum==model.checksum)return retained;
    auto index=std::make_shared<ModelSectionIndex>();
    index->vertices=bank.data();index->count=bank.size();index->checksum=model.checksum;
    std::vector<std::uint32_t> order(bank.size());
    for(std::uint32_t n=0;n<order.size();++n)order[n]=n;
    // Stable by section keeps every section's ordinals ascending.
    std::stable_sort(order.begin(),order.end(),[&](std::uint32_t a,std::uint32_t b){
        return bank[a].source_section<bank[b].source_section;});
    index->ordinals=std::move(order);
    for(std::uint32_t at=0;at<index->ordinals.size();) {
        const auto section=bank[index->ordinals[at]].source_section;
        auto end=at;while(end<index->ordinals.size() && bank[index->ordinals[end]].source_section==section)++end;
        index->ranges.push_back({section,at,end-at});at=end;
    }
    // Concurrent cold readers may build equal indices; the last publish wins.
    model.section_index.publish(index);
    return index;
}
std::size_t model_section_select(const RenderModel &model,std::span<const Vertex> posed,std::uint32_t section,
                                 std::vector<Vertex> &vertices,std::vector<std::uint32_t> *ordinals) {
    if(posed.size()!=model.mesh.vertices.size())
        throw FormatError("section selection requires a complete pose of its model bank");
    static const bool verify=[]{const auto *p=std::getenv("HALO2_VERIFY_SECTION_INDEX");return p && *p;}();
    const auto index=model_section_index(model);
    const auto selected=index->section(section);
    if(verify) {
        std::vector<std::uint32_t> scan;
        for(std::uint32_t n=0;n<posed.size();++n)if(posed[n].source_section==section)scan.push_back(n);
        if(scan.size()!=selected.size() || !std::equal(scan.begin(),scan.end(),selected.begin()))
            throw FormatError("Model section index differs from the linear section scan");
    }
    vertices.reserve(vertices.size()+selected.size());
    if(ordinals)ordinals->reserve(ordinals->size()+selected.size());
    for(const auto n:selected) {
        vertices.push_back(posed[n]);
        if(ordinals)ordinals->push_back(n);
    }
    return selected.size();
}
}
