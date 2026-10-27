#pragma once
#include "halo2/frontend.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/original_loading_wait.hpp"
namespace halo2 {
// Resident admitted MCC/H2EK font3 and the native source font-quad path.
// This adapter owns neither map lifetime/readiness nor an estimated progress.
class NativeSourceLoadingWait {
public:
    explicit NativeSourceLoadingWait(const GameplayFontSource&);
    const Mesh& resident_mesh()const{return mesh_;}
    std::span<const Material> materials()const{return mesh_.materials;}
    std::span<const Vertex> vertices()const{return vertices_;}
    const OriginalLoadingWaitText& state()const{return state_;}
    void update(std::uint32_t actual_progress_bits,unsigned language);
private:
    RetailFont font_;
    Mesh mesh_;
    std::vector<Vertex> vertices_;
    OriginalLoadingWaitText state_{};
};
}
