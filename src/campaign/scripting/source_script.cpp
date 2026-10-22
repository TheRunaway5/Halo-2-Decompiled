#include "halo2/source_script.hpp"
#include <stdexcept>

namespace halo2 {
std::uint16_t source_hs_random_range(std::uint32_t &state,
                                   std::int16_t lower,std::int16_t upper) {
    state=state*0x19660du+0x3c6ef35fu;
    const auto difference=std::uint32_t(std::int32_t(upper)-std::int32_t(lower));
    const auto product=difference*(state>>16);
    return std::uint16_t((product>>16)+std::uint16_t(lower));
}
std::optional<unsigned> source_hs_begin_random(std::uint32_t &state,
                                              unsigned count,std::uint32_t visited) {
    if(count>32)throw std::invalid_argument("source begin_random child count exceeds retained frame");
    state=state*0x19660du+0x3c6ef35fu;
    if(!count)return {};
    const auto start=((state>>16)*count)>>16;
    for(unsigned step=0;step<count;++step){
        const auto index=(start+step)%count;
        if(!(visited&(1u<<index)))return index;
    }
    return {};
}
std::uint32_t source_hs_object_list(std::uint32_t object,
                                  const SourceHsObjectListServices &services) {
    if(object==0xffffffffu)return object;
    if(!services.allocate || !services.append)
        throw std::invalid_argument("source object-list conversion requires owned list services");
    const auto list=services.allocate();
    services.append(list,object);
    return list;
}
std::uint32_t source_hs_named_object_list(std::int16_t name,
    std::span<const std::uint32_t> names,const SourceHsObjectListServices &services) {
    if(name<0 || name>=640)return 0xffffffffu;
    if(names.size()!=640)
        throw std::invalid_argument("source object-name bank must contain 640 handles");
    return source_hs_object_list(names[unsigned(name)],services);
}
}
