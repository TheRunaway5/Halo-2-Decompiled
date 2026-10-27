#pragma once
#include "halo2/cache.hpp"
#include "halo2/object_sky_lighting.hpp"
#include <map>

namespace halo2::path_tracing {
// The retail object-lighting availability bit does not control authored sky
// radiance. Keep a separate bank for ray queries, resident only while enabled.
class SkyLighting {
public:
    void set_active(bool active) {
        active_=active;
        if(!active_) {skies_.clear();owner_=nullptr;}
    }
    const ObjectSkyLightingDefinition* select(const Cache& cache,int ordinal) {
        if(!active_ || ordinal<0)return nullptr;
        if(owner_!=&cache) {skies_.clear();owner_=&cache;}
        const auto index=static_cast<std::size_t>(ordinal);
        auto found=skies_.find(index);
        if(found==skies_.end())
            found=skies_.emplace(index,cache.load_object_sky_lighting(index)).first;
        return found->second?&*found->second:nullptr;
    }
    std::size_t cached_skies()const {return skies_.size();}
private:
    bool active_{};
    const Cache* owner_{};
    std::map<std::size_t,std::optional<ObjectSkyLightingDefinition>> skies_;
};
}
