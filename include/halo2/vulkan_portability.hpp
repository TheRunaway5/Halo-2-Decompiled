#pragma once
#include <vulkan/vulkan.h>
#include <algorithm>
#include <cstring>
#include <span>
#include <vector>

namespace halo2 {
inline bool vulkan_extension_available(std::span<const VkExtensionProperties> available,
                                        const char *name) {
    return std::any_of(available.begin(), available.end(), [name](const auto &extension) {
        return std::strcmp(extension.extensionName, name) == 0;
    });
}
inline void append_vulkan_extension(std::vector<const char *> &extensions, const char *name) {
    if(std::none_of(extensions.begin(), extensions.end(), [name](const char *extension) {
        return std::strcmp(extension, name) == 0;
    })) extensions.push_back(name);
}
inline VkInstanceCreateFlags enable_vulkan_portability_instance(
    std::span<const VkExtensionProperties> available, std::vector<const char *> &extensions) {
    if(!vulkan_extension_available(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) return 0;
    append_vulkan_extension(extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    return VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
}
inline void enable_vulkan_portability_device(std::span<const VkExtensionProperties> available,
                                             std::vector<const char *> &extensions) {
    // The extension name is usable without enabling the provisional feature structs.
    constexpr auto subset = "VK_KHR_portability_subset";
    if(vulkan_extension_available(available, subset)) append_vulkan_extension(extensions, subset);
}
}
