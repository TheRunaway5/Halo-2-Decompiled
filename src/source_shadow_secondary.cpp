#include "halo2/source_shadow_secondary.hpp"

namespace halo2 {
namespace {
template<class T> const T& lookup(std::span<const T> records,std::uint32_t identity){
    const T* result=nullptr;
    for(const auto& record:records)if(record.identity==identity){
        if(result)throw FormatError("duplicate source secondary lookup identity");
        result=&record;
    }
    if(!result)throw FormatError("unresolved source secondary lookup identity");
    return *result;
}
template<class T> T required(const std::optional<T>& value){
    if(!value)throw FormatError("unresolved source secondary lookup field");
    return *value;
}
std::span<SourceBspAdmissionRecord> active(SourceShadowSecondaryQueue& q){
    if(q.count<=0)return {};
    if(std::size_t(q.count)>q.storage.size())
        throw FormatError("source secondary queue count exceeds supplied storage");
    return q.storage.first(std::size_t(q.count));
}
}
void source_shadow_secondary_classify(SourceShadowSecondaryRows& rows,
    std::span<const SourceShadowSecondaryOwner> owners,
    std::span<const SourceShadowSecondaryDefinition> definitions,
    std::span<const SourceShadowSecondaryModel> models,
    const SourceShadowSecondaryClassification& input){
    constexpr std::uint32_t none=0xffffffffu;
    if(input.root==none)return;
    const auto definition=required(lookup(owners,input.root).definition);
    const auto model=required(lookup(definitions,definition).model);
    if(model==none){rows.clusters.count=0;return;}
    //The actual routine reads these counts only after the modelNONE branch.
    auto objects=active(rows.objects),clusters=active(rows.clusters),instances=active(rows.instances);
    for(auto& row:objects){
        if(row.id==input.root){
            auto flags=required(lookup(models,model).render_model)==none?std::uint16_t(0):row.flags;
            if(input.root_receives==0)flags&=0xefffu;
            row.flags=flags;
        }else{
            const auto& owner=lookup(owners,row.id);
            const auto type=required(owner.type);
            auto flags=std::uint16_t(row.flags&0xdfffu);
            //165B0C reads flags only when the explicit type mask misses.
            if(!(input.receiver_type_mask&(1u<<(type&31))) && !(required(owner.flags)&0x80000000u))
                flags&=0xefffu;
            row.flags=flags;
        }
    }
    for(auto& row:clusters)row.flags&=0xdfffu;
    for(auto& row:instances)row.flags&=0xdfffu;
}
}
