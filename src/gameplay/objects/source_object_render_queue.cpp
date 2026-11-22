#include "halo2/source_object_render_queue.hpp"
#include "halo2/detail/source_object_render_queue_promotion.hpp"
#include <algorithm>
#include <string>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
template<class T> const T &lookup(std::span<const T> bank,std::uint32_t identity,
    const char *kind) {
    const T *found=nullptr;
    for(const auto &value:bank)if(value.identity==identity) {
        if(found)throw FormatError(std::string("source render queue duplicate ")+kind);
        found=&value;
    }
    if(!found || identity==none)throw FormatError(std::string("source render queue missing ")+kind);
    return *found;
}
template<class T> T field(const std::optional<T> &value,const char *name) {
    if(!value)throw FormatError(std::string("source render queue unresolved ")+name);
    return *value;
}
}
void source_object_render_queue_order(std::span<SourceBspAdmissionRecord> queue,
    std::span<const SourceObjectRenderQueueOwner> owners,
    std::span<const SourceObjectRenderQueueDefinition> definitions,
    std::span<const SourceObjectRenderQueueModel> models,
    std::span<const SourceObjectRenderQueueGeometry> geometry) {
    source_object_render_queue_detail::promote(queue,[&](const auto& record) {
        bool selected=false;
        if(record.id!=none) {
            const auto &owner=lookup(owners,record.id,"owner");
            if(field(owner.body_present,"object body presence")) {
                const auto definition=field(owner.definition,"object definition");
                if(definition!=none) {
                    const auto &d=lookup(definitions,definition,"definition");
                    if(field(d.present,"definition pointer presence")) {
                        const auto model=field(d.model,"definition model");
                        if(model!=none) {
                            const auto &m=lookup(models,model,"model");
                            if(field(m.present,"hlmt pointer presence")) {
                                const auto render=field(m.render_model,"hlmt render model");
                                if(render!=none)selected=field(lookup(geometry,render,"render model").prt_info_count,"mode PRTInfo count")>0;
                            }
                        }
                    }
                }
            }
        }
        return selected;
    });
}
std::uint16_t source_object_render_queue_fade_flags(std::uint16_t flags,
    SourceModelFadeBytes fade) {
    return std::uint16_t(flags|(fade.first==0?1u:0u));
}
}
