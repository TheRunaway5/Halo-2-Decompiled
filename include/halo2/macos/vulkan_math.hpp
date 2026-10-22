#pragma once
#include "halo2/vulkan_portability.hpp"
#include <cstdint>
#include <stdexcept>

namespace halo2::macos {
// Owns the setting and its pNext storage through vkCreateInstance. Pin Metal
// compiler arithmetic per instance; environment settings must not re-enable
// context-dependent contraction/reassociation in the source vertex programs.
class SourceShaderMath {
public:
    SourceShaderMath()=default;
    SourceShaderMath(const SourceShaderMath&)=delete;
    SourceShaderMath &operator=(const SourceShaderMath&)=delete;
    bool configure(VkInstanceCreateInfo &info,std::span<const VkExtensionProperties> available,
                   std::vector<const char*> &extensions) {
#ifdef __APPLE__
        if(!vulkan_extension_available(available,VK_EXT_LAYER_SETTINGS_EXTENSION_NAME))
            throw std::runtime_error("MoltenVK source arithmetic requires VK_EXT_layer_settings");
        append_vulkan_extension(extensions,VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
        settings_.pNext=info.pNext;
        info.pNext=&settings_;
        info.enabledExtensionCount=static_cast<std::uint32_t>(extensions.size());
        info.ppEnabledExtensionNames=extensions.data();
        return true;
#else
        (void)info;(void)available;(void)extensions;
        return false;
#endif
    }
private:
#ifdef __APPLE__
    const std::int32_t fast_math_=0;
    const VkLayerSettingEXT setting_{"MoltenVK","MVK_CONFIG_FAST_MATH_ENABLED",VK_LAYER_SETTING_TYPE_INT32_EXT,1,&fast_math_};
    VkLayerSettingsCreateInfoEXT settings_{VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,nullptr,1,&setting_};
#endif
};
}
