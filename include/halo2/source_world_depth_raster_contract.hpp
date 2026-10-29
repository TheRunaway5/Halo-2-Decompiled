#pragma once
#include <cstdint>

namespace halo2 {
inline bool source_world_depth_cull_admitted(std::uint32_t cull) {
    // Original3FF4B0(0) disables face culling;900 retains ordinary winding.
    // Reverse winding and other states remain outside this bounded scope.
    return cull==0 || cull==0x900;
}

// Original1BBD0 resets cull900.16B70 restores the previous+11C list
// before188E0 applies the selected implementation+2 list. Front winding,
// polygon offsets and AA remain owned by the ordinary camera/reset scope.
class SourceWorldDepthCullState {
public:
    explicit SourceWorldDepthCullState(std::uint32_t initial=0x900):cull_(initial) {}
    bool apply(std::uint32_t state,std::uint32_t value) {
        if(state==147) {
            if(!source_world_depth_cull_admitted(value))return false;
            cull_=value;
        } else if(state==146 || state==149 || state==154 || state==155)return false;
        return true;
    }
    std::uint32_t cull()const {return cull_;}
private:
    std::uint32_t cull_;
};
}
