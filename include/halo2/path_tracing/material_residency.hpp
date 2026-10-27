#pragma once
#include "halo2/path_tracing/scene.hpp"
namespace halo2::path_tracing {
// Activate material images referenced by the complete ray scene, including
// offscreen static geometry, before update_geometry. Slot identities remain
// the resident source bank's indices. Unreferenced UI/material images are not
// decoded or fingerprinted. Later dynamic owners can activate existing slots.
void refresh_materials_for_geometry(Scene &,std::span<const Material>,std::span<const Vertex>);
}
