#pragma once
#include "halo2/native_tu15_vehicle_flip.hpp"
#include "halo2/native_tu15_object_activation_residuals.hpp"
namespace halo2 {
struct NativeTu15VehicleFlipBindingActivity {
    NativeTu15VehicleFlipStats flip;
    NativeTu15ObjectActivationBindingActivity activation;
};
// Native F1DD0 -> native BC200/B9800 with explicit activation DATA/services.
// No F2091 transport packet or guest calling convention is synthesized here.
bool native_tu15_flip_vehicle_with_activation(SourcePhysicsContactMemory&,std::uint32_t object,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    NativeTu15ObjectActivationScratch,const NativeTu15ObjectActivationResidualCall&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15VehicleFlipBindingActivity* = nullptr);
} // namespace halo2
