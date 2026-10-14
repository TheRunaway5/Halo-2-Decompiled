#include "halo2/source_decal_draw.hpp"
#include "halo2/source_decal_pass.hpp"

namespace halo2 {
SourceDecalLayerState source_decal_layer_state_tu15(unsigned layer) {
    SourceDecalLayerState out;
    auto &p=out.pixel.words;
    // Complete literal program writes in 43D13/43DA9. Stage0 is PROJECT2D;
    // source immediate v10 is RGBA and v1.z remains baked brightness.
    p[0]=0x18140000u;p[26]=0xc0u;p[54]=1;
    if(layer<=1) {
        p[34]=0x08040000u;p[45]=0xc0u;p[8]=0x0f150000u;
        p[9]=0x0c1c1c00u;p[53]=0x11001u;
        out.source_blend=1;out.destination_blend=0x303;
    } else if(layer==2 || layer==3) {
        p[35]=layer==2?0x081ca03cu:0x081c203cu;
        p[46]=0xc00u;p[8]=0xcu;p[53]=0x11002u;
        out.source_blend=0x306;out.destination_blend=layer==2?0x300:0;
    } else if(layer==4 || layer==5) {
        const auto pass=source_decal_pass(layer);
        out.pixel=pass.program;out.source_blend=pass.source;
        out.destination_blend=pass.destination;out.blend_equation=pass.operation;
    } else throw FormatError("Original cached decal layer has no native draw state");
    return out;
}
}
