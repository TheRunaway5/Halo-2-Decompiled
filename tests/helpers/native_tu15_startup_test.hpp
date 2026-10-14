#pragma once
#include "halo2/renderer_device_lease.hpp"
#include "halo2/renderer_palette.hpp"
#include "halo2/renderer_constant_mode.hpp"
#include "halo2/renderer_depth_mode.hpp"
#include "halo2/renderer_output_state.hpp"
#include "halo2/renderer_texture_coordinates.hpp"
namespace halo2 {int run_native_tu15_startup_test(int,char**,RendererDeviceLease = {},RendererPaletteFactory = {},RendererConstantMode = {},RendererDepthMode = {},RendererOutputState = {},RendererTextureCoordinates = {});}
