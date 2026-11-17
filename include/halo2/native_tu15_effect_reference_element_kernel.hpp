#pragma once
#include <cstdint>

namespace halo2::native_effect_reference_element_detail {
enum class Group { unknown, static_definition, effect, particle };
inline Group classify(std::uint32_t group){
    switch(group){
    case 0x62736474u:return Group::static_definition;
    case 0x65666665u:return Group::effect;
    case 0x5052544du:case 0x70727433u:return Group::particle;
    default:return Group::unknown;
    }
}
// Resolve an effect reference through its live tag definition. Particle
// references first resolve their parent, then use the authored tag accessor.
// Views acquire fields lazily, including fields read after a returned child.
template<class View>void resolve(View&view){
    const auto group=view.group();const auto kind=classify(group);
    view.classified(group);
    switch(kind){
    case Group::static_definition:view.static_element();break;
    case Group::effect:view.effect_element();break;
    case Group::particle:view.parent_element();break;
    case Group::unknown:view.missing();break;
    }
}
template<class View>void bind_definition(View&view){
    const auto handle=view.handle();const auto table=view.table();
    view.publish_handle(handle);
    const auto definition=view.definition(table,handle&65535u);
    view.publish_definition(definition);view.finish();
}
template<class View>void element(View&view){
    const auto index=view.index();const auto definition=view.definition();
    const auto offset=view.scale(index);const auto elements=view.elements(definition);
    view.finish(offset,elements);
}
} // namespace halo2::native_effect_reference_element_detail
