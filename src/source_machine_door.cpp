#include "halo2/source_machine_door.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
SourceMachineDoorStep source_machine_door_step(const SourceMachineDoorDefinition &definition,
    SourceMachineDoorState &state,const SourceMachineDoorInput &input,float dt,
    const SourceMachineDoorQuery &query) {
    if(!std::isfinite(dt) || dt<=0 || !std::isfinite(input.position) ||
       !std::isfinite(definition.activation_radius) || !std::isfinite(definition.open_seconds) ||
       !std::isfinite(input.radius))throw FormatError("invalid source automatic-door tick");
    SourceMachineDoorStep result;
    if(!(state.flags&1) && !((input.tick+input.handle)&3) && !input.editor) {
        if(!query)throw FormatError("automatic door requires its actual unit query");
        result.queried=true;
        constexpr float epsilon=0.00009999999747378752f;
        result.query_radius=definition.activation_radius<epsilon?input.radius:definition.activation_radius;
        const auto units=query(result.query_radius);
        if(units.size()>16)throw FormatError("automatic-door query exceeds original capacity16");
        bool open=false;
        for(const auto &unit:units) {
            if(unit.type>1)throw FormatError("automatic-door query returned a non-unit");
            if((unit.type==1 && !(unit.vehicle_flags&0x100000u)) ||
               (unit.flags_10a&4) || (unit.unit_flags&0x4000u))continue;
            if(((state.flags&4) || ((state.flags&8) && unit.player!=~0u)) && input.position==0) {
                const auto marker=input.one_sided_marker.value_or(
                    SourceMachineDoorMarker{input.center,input.forward});
                const auto dx=unit.center.x-marker.position.x;
                const auto dy=unit.center.y-marker.position.y;
                const auto dz=unit.center.z-marker.position.z;
                const float side=(marker.forward.x*dx+marker.forward.z*dz)+marker.forward.y*dy;
                if(side>0)continue;
            }
            open=true;
        }
        if(open) {
            if(input.position_group_present)result.position_targets[result.target_count++]=1;
            state.open_ticks=-3;
        }
    }
    if(input.position==1 && !(state.flags&2) && !input.editor) {
        state.open_ticks=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state.open_ticks)+1u);
        //115960 converts the signed counter to float before the SSE multiply.
        const float elapsed=float(state.open_ticks)*dt;
        if(elapsed>definition.open_seconds && input.position_group_present)
            result.position_targets[result.target_count++]=0;
    } else state.open_ticks=0;
    return result;
}
}
