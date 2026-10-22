#include "halo2/source_king_draw.hpp"
namespace halo2 {
std::array<Vertex,192> source_king_boundary_triangles(const SourceKingBoundary& boundary,
    std::uint32_t material) {
    std::array<Vertex,192> result;
    for(unsigned i=0;i<64;++i) {
        std::array<unsigned,3> triangle{i,i+1,i+2};
        if(i&1)std::swap(triangle[0],triangle[1]);
        for(unsigned corner=0;corner<3;++corner) {
            const auto& source=boundary.strip[triangle[corner]];auto& vertex=result[i*3+corner];
            vertex.position={source.position[0],source.position[1],source.position[2]};
            vertex.uv={source.uv[0],source.uv[1]};
            vertex.color={source.rgba[0],source.rgba[1],source.rgba[2]};
            vertex.source_color_alpha=source.rgba[3];vertex.material=material;
        }
    }
    return result;
}
}
