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
// layer 12 is the light response. Layer 11 is the per-light alpha pass 31150
// submits first when the light is shadowed, specular or flag 0x20: its
// implementation (30D00) writes the gel or shadow term into target alpha,
// which the DST_ALPHA light-response implementations then multiply by.
// shadow=false withholds feature bit 2 for a caller without the light's
// shadow buffer; both layers must be selected with the same value.
// Layer 10 (`shadow_generate`) selects the light's casters from the third
// command bank (4CE298), as 44360's layer table does. extra_features is OR'd
// into the 31150 mask: 0x48 after 494A0 for a cinematic per-object buffer.
ProjectedLightSelection native_projected_light_selection_tu15(PhysicsLiveWorldTu15&,
    std::uint32_t light,const NativeLightReceiver&,std::uint32_t layer=12,bool shadow=true,
    std::uint32_t extra_features=0);
}
