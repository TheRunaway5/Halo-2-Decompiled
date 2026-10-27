#pragma once
#include "halo2/native_tu15_geometry.hpp"
namespace halo2 {
// Actual2E4550: transpose the original three basis columns, transform all
// four source columns, then reread the basis/translation before correcting
// the final column. Source W-lane swizzles and each alias remain significant.
void native_tu15_compose_inverse_affine_geometry(NativeTu15GeometryStorage&,NativeTu15PhysicsFrameFp&);
class NativeTu15InverseGeometryMemory final:public NativeTu15GeometryStorage {
  SourcePhysicsContactMemory&memory_;std::uint32_t destination_,basis_,source_;
  SourcePhysicsContactRecordObserver observer_;
  NativeTu15GeometryVector vector(std::uint32_t)const;
public:
  NativeTu15InverseGeometryMemory(SourcePhysicsContactMemory&,std::uint32_t destination,
    std::uint32_t basis,std::uint32_t source,const SourcePhysicsContactRecordObserver& = {});
  NativeTu15GeometryVector basis_column(unsigned)const override;
  NativeTu15GeometryVector source_column(unsigned)const override;
  NativeTu15GeometryVector destination_column(unsigned)const override;
  void destination_column(unsigned,const NativeTu15GeometryVector&,NativeTu15GeometryPublication)override;
};
} // namespace halo2
