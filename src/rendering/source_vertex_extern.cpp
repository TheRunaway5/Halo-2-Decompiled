#include "halo2/source_vertex_extern.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
std::array<std::uint32_t,2> remap_source_vertex_externs(
    ShaderConstants &destination,std::span<const float> scalars,
    std::span<const ShaderVector> colors,std::span<const ShaderRemap> remaps) {
    std::array<std::uint32_t,2> dirty{};
    for(const auto &r:remaps) {
        const unsigned component=(r.destination>>5)&31u;
        const unsigned bank=(r.destination>>4)&1u,reg=r.destination&15u;
        auto &out=destination[bank][reg];
        if(component<4) {
            if(r.source>=scalars.size() || !std::isfinite(scalars[r.source]))
                throw FormatError("Vertex scalar extern has no finite supplied value");
            //19BE0 pushes literal scalar mode0 and the packed texture
            // selector separately. High destination bits do not invert this.
            out[component]=scalars[r.source];
        } else if(component==4) {
            if(r.source>=colors.size())throw FormatError("Vertex RGB extern has no supplied value");
            // Original1B3F9 copies these three words; texture selector and
            // byte+2 have no effect, and alpha remains in its authored bank.
            std::copy_n(colors[r.source].begin(),3,out.begin());
        } else {
            throw FormatError("Vertex dimension extern requires its separate source texture owner");
        }
        dirty[bank]|=std::uint32_t{1}<<reg;
    }
    return dirty;
}
}
