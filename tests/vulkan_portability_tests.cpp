#include "halo2/vulkan_portability.hpp"
#include <array>
#include <cstdlib>
#include <iostream>

int main() {
    const auto require=[](bool condition) { if(!condition) std::exit(EXIT_FAILURE); };
    std::vector<const char *> names{"VK_KHR_surface"};
    require(halo2::enable_vulkan_portability_instance({}, names)==0 && names.size()==1);
    halo2::enable_vulkan_portability_device({}, names);
    require(names.size()==1);
    std::array<VkExtensionProperties, 2> available{};
    std::strcpy(available[0].extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    std::strcpy(available[1].extensionName, "VK_KHR_portability_subset");
    require(halo2::enable_vulkan_portability_instance(available,names)==VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR);
    require(halo2::enable_vulkan_portability_instance(available,names)==VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR);
    require(names.size()==2);
    halo2::enable_vulkan_portability_device(available,names);
    halo2::enable_vulkan_portability_device(available,names);
    require(names.size()==3);
    std::cout << "Vulkan portability extension negotiation passed\n";
}
