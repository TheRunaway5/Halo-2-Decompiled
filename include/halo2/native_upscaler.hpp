#pragma once
#include "halo2/upscaling_settings.hpp"
#include <vulkan/vulkan.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
namespace halo2 {
struct UpscalingImage { VkImage image{};VkImageView view{};VkFormat format{}; };
struct UpscalingDispatch {
    VkCommandBuffer command{};
    UpscalingImage color,depth,motion,reactive,output;
    float jitter_x{},jitter_y{},delta_seconds=1.f/60,near_plane=.05f,far_plane=10000.f,vertical_fov=1.2f;
    bool reset{};
};
// Owns only the vendor context. The renderer owns resources and synchronization.
class NativeUpscaler {
public:
    NativeUpscaler(VkInstance,VkPhysicalDevice,VkDevice,VkQueue,unsigned queue_family,
                   VkExtent2D input,VkExtent2D output,Upscaler,UpscalingQuality);
    ~NativeUpscaler();
    NativeUpscaler(const NativeUpscaler&)=delete;
    void dispatch(const UpscalingDispatch&);
    static std::string availability(VkPhysicalDevice,Upscaler);
    static std::vector<std::string> instance_extensions();
    static std::vector<std::string> device_extensions(VkInstance,VkPhysicalDevice);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
