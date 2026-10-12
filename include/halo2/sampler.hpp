#pragma once
#include "halo2/cache.hpp"
#include <vulkan/vulkan.h>
#include <bit>
#include <cmath>

namespace halo2 {
inline VkSamplerAddressMode sampler_address(std::uint32_t mode) {
  switch(mode) {
  case 1:return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  case 2:return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  case 3:return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  case 4:return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  case 5:return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
  default:throw FormatError("unsupported retail texture address mode");
  }
}
struct NativeSampler {
  VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  VkSamplerCustomBorderColorCreateInfoEXT border{
    VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT};
  bool custom_border{};
};
// Create-info conversion is separated from device allocation for exact input
// tests. Filtering precision still belongs to the host GPU.
inline NativeSampler native_sampler(const ShaderTextureState &state,
                                    const VkPhysicalDeviceLimits &limits,
                                    bool anisotropy) {
  NativeSampler result;
  auto &s=result.info;
  auto filter=[](unsigned f) {
    if(f==1)return VK_FILTER_NEAREST;
    if(f==2 || f==3)return VK_FILTER_LINEAR;
    throw FormatError("unsupported retail texture filter");
  };
  s.minFilter=filter(state.min_filter);s.magFilter=filter(state.mag_filter);
  if(state.mip_filter>2)throw FormatError("unsupported retail mip filter");
  s.mipmapMode=state.mip_filter==2?VK_SAMPLER_MIPMAP_MODE_LINEAR:VK_SAMPLER_MIPMAP_MODE_NEAREST;
  s.addressModeU=sampler_address(state.address_u);
  s.addressModeV=sampler_address(state.address_v);
  s.addressModeW=sampler_address(state.address_w);
  s.mipLodBias=std::bit_cast<float>(state.lod_bias);
  if(!std::isfinite(s.mipLodBias) || std::abs(s.mipLodBias)>limits.maxSamplerLodBias)
    throw FormatError("retail mip bias exceeds native sampler limits");
  s.anisotropyEnable=state.min_filter==3 || state.mag_filter==3;
  s.maxAnisotropy=float(std::max(1u,state.anisotropy));
  if(s.anisotropyEnable && (!anisotropy || s.maxAnisotropy>limits.maxSamplerAnisotropy))
    throw FormatError("retail anisotropy exceeds native sampler limits");
  s.minLod=float(state.max_mip);
  // A 0.25 interval preserves minification when mip filtering is disabled.
  // maxLod=0 would force MAG even when the original specifies a distinct MIN.
  s.maxLod=state.mip_filter==0?s.minLod+0.25f:VK_LOD_CLAMP_NONE;
  s.borderColor=state.border_color==0?VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
      state.border_color==0xff000000?VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
      state.border_color==0xffffffff?VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
      VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
  result.custom_border=s.borderColor==VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
  const auto color=state.border_color;
  result.border.customBorderColor.float32[0]=float((color>>16)&255)/255;
  result.border.customBorderColor.float32[1]=float((color>>8)&255)/255;
  result.border.customBorderColor.float32[2]=float(color&255)/255;
  result.border.customBorderColor.float32[3]=float(color>>24)/255;
  result.border.format=VK_FORMAT_R8G8B8A8_UNORM;
  return result;
}
} // namespace halo2
