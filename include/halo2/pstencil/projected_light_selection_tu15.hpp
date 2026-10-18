#pragma once
#include "halo2/pstencil/native_light_receiver_tu15.hpp"
#include "halo2/source_opaque_sort.hpp"

namespace halo2::pstencil {
struct ProjectedLightSelection {
    std::uint32_t features{};
    std::vector<SourceOpaqueSortRecord> records;
};
// Whole3BAA0 after the actual31150 feature inputs. Literal44540 caller:
//473690=12,473694=1,473698=4CE280 ->3BC20 ->3BAA0(bank,1,12,output).
// Selection/vertex class and sorting remain original; this submits no GPU work.
ProjectedLightSelection native_projected_light_selection_tu15(PhysicsLiveWorldTu15&,
    std::uint32_t light,const NativeLightReceiver&);
}
