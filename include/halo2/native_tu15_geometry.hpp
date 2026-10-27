#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
namespace halo2 {
using NativeTu15GeometryVector=std::array<std::uint32_t,4>;
enum class NativeTu15GeometryPublication {
    composed_column,composed_translation,transposed_direction,transformed_position
};
// Lazy semantic views preserve genuine source aliases and access boundaries.
// No original register file or instruction dispatcher belongs to these kernels.
class NativeTu15GeometryStorage {
public:
    virtual ~NativeTu15GeometryStorage()=default;
    virtual NativeTu15GeometryVector basis_column(unsigned)const=0;
    virtual NativeTu15GeometryVector source_column(unsigned)const=0;
    virtual NativeTu15GeometryVector destination_column(unsigned)const=0;
    virtual void destination_column(unsigned,const NativeTu15GeometryVector&,
                                   NativeTu15GeometryPublication)=0;
};
// Actual2E44D0: transform all four right-hand columns, then add the left
// translation to the fourth result. Columns are reread at their source phases.
void native_tu15_compose_affine_geometry(NativeTu15GeometryStorage&,NativeTu15PhysicsFrameFp&);
// Actual2E47B0: the original transpose swizzle repeats result Z in lane W.
void native_tu15_transform_transposed_direction(NativeTu15GeometryStorage&,NativeTu15PhysicsFrameFp&);
// Actual2E4690: source association is X+Y+(Z+translation), including lane W.
void native_tu15_transform_geometry_position(NativeTu15GeometryStorage&,NativeTu15PhysicsFrameFp&);
// Retained-layout adapter only. Destination rejection precedes observer
// delivery; a completed-store observer rejection retains the genuine store.
class NativeTu15GeometryMemory final:public NativeTu15GeometryStorage {
    SourcePhysicsContactMemory&memory_;
    std::uint32_t destination_,basis_,source_;
    SourcePhysicsContactRecordObserver observer_;
    NativeTu15GeometryVector vector(std::uint32_t)const;
public:
    NativeTu15GeometryMemory(SourcePhysicsContactMemory&,std::uint32_t destination,
        std::uint32_t basis,std::uint32_t source,const SourcePhysicsContactRecordObserver& = {});
    NativeTu15GeometryVector basis_column(unsigned)const override;
    NativeTu15GeometryVector source_column(unsigned)const override;
    NativeTu15GeometryVector destination_column(unsigned)const override;
    void destination_column(unsigned,const NativeTu15GeometryVector&,NativeTu15GeometryPublication)override;
};
} // namespace halo2
