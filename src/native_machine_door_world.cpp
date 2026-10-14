#include "halo2/simulation.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/math.hpp"
#include "halo2/render_interpolation.hpp"
#include <algorithm>

namespace halo2 {
void Simulation::step_devices() {
    // The retained TU1.5 object/world phases own live device motion.
    if(source_physics_movement_)return;
    std::array<SourceMachineDoorUnit,16> nearby;
    SourceDeviceWorld::DoorWorld world;
    world.tick=std::uint32_t(ticks_);
    world.query=[&](std::uint32_t,Vec3 origin,float radius) {
        // Native bound adapter over the actual current unit/scene owners.
        // The original BB050 cluster/link chronology remains a separate
        // service; the retained door reducer consumes this explicit bank.
        std::vector<SourceMachineDoorUnit> candidates;
        const auto append=[&](const Unit &unit) {
            if(!unit.exists || unit.bsp_hidden || unit.source_datum==null_datum ||
               unit.source_parent!=null_datum)return;
            const auto offset=unit.definition.bounding_offset;
            const auto forward=direction(unit.yaw);
            const auto center=unit.body.position+forward*offset.x+
                Vec3{-forward.y,forward.x,0}*offset.y+Vec3{0,0,offset.z};
            candidates.push_back({unit.source_datum,unit.source_player,unit.definition.unit_flags,
                0,0,unit.source_unit_10a,center,unit.definition.bounding_radius});
        };
        append(player_);for(const auto &unit:actors_)append(unit);
        for(const auto &object:scene_.objects()) {
            if(object.handle==null_datum || object.bsp_hidden || object.attachment_parent!=null_datum)continue;
            const auto &placement=content_.objects.at(object.placement);
            if(placement.type!=1)continue;
            if(!content_.unit_interactions)throw FormatError("door vehicle query lacks authored unit metadata");
            const auto found=content_.unit_interactions->units.find(placement.definition);
            if(found==content_.unit_interactions->units.end())
                throw FormatError("door vehicle query lacks its authored definition");
            const auto &definition=found->second;
            const auto root=scene_object_transform(object);
            SourceMachineDoorUnit unit{object.handle,null_datum,definition.unit_flags,
                definition.vehicle_flags,1,0,scene_.center(object.handle),definition.bound_radius*root.scale};
            const auto d=unit.center-origin;const float reach=unit.radius+radius;
            if(!(reach*reach>=(d.z*d.z+d.y*d.y)+d.x*d.x))continue;
            if(definition.vehicle_flags&0x100000u) {
                // Eligible vehicles consume the real live +10A/+13C words.
                // A placed render model cannot supply these unit-body fields.
                if(!object_bodies_.contains(object.handle))
                    throw FormatError("automatic-door vehicle requires its live unit body");
                const Bytes body(object_bodies_.body(object_lifetimes_,object.handle));
                unit.flags_10a=std::to_integer<std::uint8_t>(body.slice(0x10a,1)[0]);
                unit.player=body.u32(0x13c);
            }
            candidates.push_back(unit);
        }
        std::sort(candidates.begin(),candidates.end(),[](const auto &a,const auto &b){
            return (a.handle&0xffffu)<(b.handle&0xffffu);
        });
        unsigned count=0;
        for(const auto &unit:candidates) {
            const auto d=unit.center-origin;const float reach=unit.radius+radius;
            // Preserve BB050's collidable-list squared-distance order.
            if(!(reach*reach>=(d.z*d.z+d.y*d.y)+d.x*d.x))continue;
            if(count==nearby.size())break;
            nearby[count++]=unit;
        }
        return std::span<const SourceMachineDoorUnit>(nearby.data(),count);
    };
    devices_.step(tick_seconds,world);
}
}
