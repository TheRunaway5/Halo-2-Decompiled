#include "halo2/native_tu15_vehicle_flip_binding.hpp"
namespace halo2 {
bool native_tu15_flip_vehicle_with_activation(SourcePhysicsContactMemory&memory,std::uint32_t object,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15ObjectActivationScratch scratch,
 const NativeTu15ObjectActivationResidualCall&residual,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15VehicleFlipBindingActivity*activity){
    NativeTu15VehicleFlipServices services;
    services.get_velocities=[&](auto datum,auto&){return native_tu15_object_velocities(memory,datum);};
    services.set_velocities=[&](auto datum,const auto&linear,const auto&angular,auto&state){
        native_tu15_set_object_velocities_with_residuals(memory,datum,&linear,&angular,state,depth,scratch,
            residual,observer,activity?&activity->activation:nullptr);
    };
    return native_tu15_flip_vehicle(memory,object,fp,depth,services,observer,activity?&activity->flip:nullptr);
}
} // namespace halo2
