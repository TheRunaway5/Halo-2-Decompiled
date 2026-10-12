#pragma once
#include "halo2/source_shadow.hpp"
#include <map>

namespace halo2 {
struct SourcePropInitialQueryState {
    ShadowCasterSphere sphere;
    Vec3 velocity;
    // Only D1850's runtime bit13 is represented. Other generic object flags
    // are owned by their actual producers, not this cached-bounds service.
    std::uint32_t query_runtime_flags{};
    bool frozen{}; // B7B40 object+C1bit0 from the literal cold factory request.
};
// Original cold B7B40 calls BD020/BDEF0 before B8890 activates physics. Its
// first cached sphere therefore uses the actual initial WORLD node0 and the
// authored bound. This service retains that constructor output only; a changed
// root/scale or an advanced physics state requires the original later producer.
class SourcePropInitialState {
public:
    const SourcePropInitialQueryState& capture(std::uint32_t full_handle,
        std::uint32_t definition,const ShadowCasterDefinition&,
        const Transform& initial_world_node0,float initial_object_scale);
    bool contains(std::uint32_t full_handle)const;
    const SourcePropInitialQueryState& retained(std::uint32_t full_handle,
        std::uint32_t definition,const Transform& current_world_node0,
        float current_object_scale,bool physics_advanced)const;
    void destroy(std::uint32_t full_handle);
private:
    struct Entry {
        std::uint32_t definition{};
        Transform initial_world_node0;
        float initial_object_scale{};
        SourcePropInitialQueryState state;
    };
    std::map<std::uint32_t,Entry> entries_;
};
}
