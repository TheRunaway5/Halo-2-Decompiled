#include "rendering/renderer/object_motion_blur_pass.hpp"
#include "source_weather_plate_gpu_fixture.hpp"

namespace {
using halo2::ObjectMotionBlurPass;
using halo2::ObjectMotionBlurVertex;
using halo2::Vec3;
using halo2_weather_plate_fixture::Buffer;
using halo2_weather_plate_fixture::Gpu;
using halo2_weather_plate_fixture::check;
using halo2_weather_plate_fixture::require;

constexpr VkExtent2D extent{96, 32};
constexpr VkRect2D first_view{{0, 0}, {48, extent.height}};
constexpr VkFormat depth_format = VK_FORMAT_D24_UNORM_S8_UINT;
constexpr VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;
using Pixel = std::array<std::uint8_t, 4>;
constexpr Pixel background{21, 88, 102, 31};
constexpr Pixel static_left{181, 109, 20, 173};
constexpr Pixel static_right{16, 170, 111, 174};
constexpr Pixel red_upper{232, 29, 12, 79};
constexpr Pixel blue_upper{11, 29, 231, 120};
constexpr Pixel red_lower{232, 73, 12, 91};
constexpr Pixel blue_lower{11, 73, 231, 135};
constexpr Pixel occluder{33, 204, 50, 203};
constexpr Pixel second_view{11, 251, 231, 220};

bool initialize(Gpu& gpu) {
    std::uint32_t layer_count{};
    check(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    std::vector<VkLayerProperties> layers(layer_count);
    check(vkEnumerateInstanceLayerProperties(&layer_count, layers.data()));
    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    const bool validate = std::ranges::any_of(layers, [&](const auto& layer) {
        return std::string_view(layer.layerName) == validation_layer;
    });
    const char* debug_extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback = halo2_weather_plate_fixture::diagnostic;
    VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance.pApplicationInfo = &application;
    if (validate) {
        instance.enabledLayerCount = instance.enabledExtensionCount = 1;
        instance.ppEnabledLayerNames = &validation_layer;
        instance.ppEnabledExtensionNames = &debug_extension;
        instance.pNext = &debug;
    }
    const auto result = vkCreateInstance(&instance, nullptr, &gpu.instance);
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER) return false;
    check(result);
    if (validate) {
        const auto create_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(gpu.instance, "vkCreateDebugUtilsMessengerEXT"));
        require(create_messenger != nullptr, "object motion blur requires the enabled Vulkan debug extension");
        check(create_messenger(gpu.instance, &debug, nullptr, &gpu.messenger));
    }

    std::uint32_t device_count{};
    check(vkEnumeratePhysicalDevices(gpu.instance, &device_count, nullptr));
    std::vector<VkPhysicalDevice> physical_devices(device_count);
    check(vkEnumeratePhysicalDevices(gpu.instance, &device_count, physical_devices.data()));
    for (const auto physical : physical_devices) {
        VkFormatProperties format{};
        vkGetPhysicalDeviceFormatProperties(physical, depth_format, &format);
        constexpr auto required = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        if ((format.optimalTilingFeatures & required) != required) continue;
        std::uint32_t family_count{};
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, nullptr);
        std::vector<VkQueueFamilyProperties> families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, families.data());
        const auto graphics = std::ranges::find_if(families, [](const auto& family) {
            return family.queueCount && (family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
        });
        if (graphics == families.end()) continue;
        gpu.physical = physical;
        gpu.family = std::uint32_t(graphics - families.begin());
        break;
    }
    if (!gpu.physical) return false;

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue.queueFamilyIndex = gpu.family;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;
    VkDeviceCreateInfo device{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device.queueCreateInfoCount = 1;
    device.pQueueCreateInfos = &queue;
    check(vkCreateDevice(gpu.physical, &device, nullptr, &gpu.device));
    vkGetDeviceQueue(gpu.device, gpu.family, 0, &gpu.queue);
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = gpu.family;
    check(vkCreateCommandPool(gpu.device, &pool, nullptr, &gpu.pool));
    VkCommandBufferAllocateInfo command{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command.commandPool = gpu.pool;
    command.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command.commandBufferCount = 1;
    check(vkAllocateCommandBuffers(gpu.device, &command, &gpu.command));
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(gpu.physical, &properties);
    std::cout << "Object motion blur GPU device " << properties.deviceName
        << (validate ? " with validation\n" : "\n");
    return true;
}

struct DepthView {
    VkDevice device;
    VkImageView view{};

    DepthView(VkDevice owner, VkImage depth) : device(owner) {
        VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        info.image = depth;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = depth_format;
        info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        check(vkCreateImageView(device, &info, nullptr, &view));
    }
    ~DepthView() { vkDestroyImageView(device, view, nullptr); }
};

VkFramebuffer color_frame(Gpu& gpu, VkImageView color, VkRenderPass& pass) {
    VkAttachmentDescription attachment{};
    attachment.format = color_format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    const VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    check(vkCreateRenderPass(gpu.device, &info, nullptr, &pass));
    gpu.passes.push_back(pass);

    VkFramebufferCreateInfo frame{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    frame.renderPass = pass;
    frame.attachmentCount = 1;
    frame.pAttachments = &color;
    frame.width = extent.width;
    frame.height = extent.height;
    frame.layers = 1;
    VkFramebuffer framebuffer{};
    check(vkCreateFramebuffer(gpu.device, &frame, nullptr, &framebuffer));
    gpu.frames.push_back(framebuffer);
    return framebuffer;
}

VkClearColorValue clear_color(const Pixel& pixel) {
    VkClearColorValue value{};
    for (unsigned component = 0; component < pixel.size(); ++component)
        value.float32[component] = float(pixel[component]) / 255.0f;
    return value;
}

void fill_region(VkCommandBuffer command, VkRect2D rect, const Pixel& color, float depth) {
    const VkClearRect clear_rect{rect, 0, 1};
    VkClearAttachment attachments[2]{};
    attachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    attachments[0].clearValue.color = clear_color(color);
    attachments[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    attachments[1].clearValue.depthStencil = {depth, 0};
    vkCmdClearAttachments(command, 2, attachments, 1, &clear_rect);
}

void transition_color(VkCommandBuffer command, VkImage image, VkImageLayout old_layout,
    VkImageLayout new_layout, VkAccessFlags before, VkAccessFlags after,
    VkPipelineStageFlags source, VkPipelineStageFlags destination) {
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = before;
    barrier.dstAccessMask = after;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(command, source, destination, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void copy_scene(VkCommandBuffer command, VkImage color, VkImage copy) {
    constexpr auto render_target = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    constexpr auto copy_source = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    constexpr auto copy_destination = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    constexpr auto color_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    constexpr auto transfer = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transition_color(command, color, render_target, copy_source,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, color_stage, transfer);
    transition_color(command, copy, VK_IMAGE_LAYOUT_UNDEFINED, copy_destination,
        0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, transfer);
    VkImageCopy region{};
    region.srcSubresource = region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.extent = {extent.width, extent.height, 1};
    vkCmdCopyImage(command, color, copy_source, copy, copy_destination, 1, &region);
    transition_color(command, copy, copy_destination, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, transfer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    transition_color(command, color, copy_source, render_target, VK_ACCESS_TRANSFER_READ_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, transfer, color_stage);
}

void append_quad(std::vector<ObjectMotionBlurVertex>& vertices, float left, float right, float motion) {
    const auto position = [](float x, float y) {
        return Vec3{2 * x / first_view.extent.width - 1, 2 * y / extent.height - 1, 0.5f};
    };
    const auto append = [&](float x, float y) {
        vertices.push_back({position(x, y), position(x - motion, y)});
    };
    append(left, 4);
    append(right, 4);
    append(left, 28);
    append(left, 28);
    append(right, 4);
    append(right, 28);
}

void verify_pixels(Buffer readback) {
    const auto* pixels = static_cast<const Pixel*>(readback.mapped);
    const auto at = [&](unsigned x, unsigned y) -> const Pixel& {
        return pixels[y * extent.width + x];
    };
    for (unsigned y = 0; y < extent.height; ++y) {
        for (unsigned x = 0; x < extent.width; ++x) {
            const auto& actual = at(x, y);
            if (x >= 48) {
                require(actual == second_view, "object blur changed pixels in another viewport");
            } else if (y < 4 || y >= 28 || (x >= 8 && x < 16)) {
                require(actual == background, "object blur changed uncovered background");
            } else if (x < 8) {
                require(actual == (x < 4 ? static_left : static_right), "stationary object blurred");
            } else if (x >= 36 && x < 40) {
                require(actual == occluder, "moving geometry blurred an opaque nearer occluder");
            } else {
                const auto& red = y < 16 ? red_upper : red_lower;
                const auto& blue = y < 16 ? blue_upper : blue_lower;
                const auto& authored = x < 28 ? red : blue;
                require(actual[1] == authored[1], "blur crossed a depth or viewport boundary, or drifted vertically");
                require(actual[3] == authored[3], "object blur changed the completed scene alpha");
                if (x == 16 || x >= 41)
                    require(actual == authored, "object blur sampled off the visible object edge");
            }
        }
    }
    for (unsigned y : {12u, 19u}) {
        const auto& left = at(27, y);
        const auto& right = at(28, y);
        require(left[0] > 126 && left[0] < 212 && left[2] > 32,
            "moving object edge did not mix toward blue along the displacement");
        require(right[0] > 31 && right[0] < 126 && right[2] < 211,
            "moving object edge did not mix toward red along the displacement");
    }
}

void verify_pass(Gpu& gpu, const std::filesystem::path& shaders, bool w_depth) {
    const auto color = gpu.image(extent, color_format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const auto copy = gpu.image(extent, color_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    const auto depth = gpu.image(extent, depth_format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    const DepthView depth_view(gpu.device, depth.handle);
    const auto scene_pass = gpu.scene_pass(true, depth_format);
    const auto scene_frame = gpu.framebuffer(scene_pass, extent, color, depth);
    VkRenderPass post_pass{};
    const auto post_frame = color_frame(gpu, color.view, post_pass);
    ObjectMotionBlurPass blur(gpu.physical, gpu.device, VK_NULL_HANDLE, post_pass, shaders,
        depth.handle, depth_view.view, copy.view);
    const auto output = gpu.buffer(extent.width * extent.height * sizeof(Pixel), VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    gpu.begin();
    VkClearValue clear[2]{};
    clear[0].color = clear_color(background);
    clear[1].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass = scene_pass;
    begin.framebuffer = scene_frame;
    begin.renderArea = {{0, 0}, extent};
    begin.clearValueCount = 2;
    begin.pClearValues = clear;
    vkCmdBeginRenderPass(gpu.command, &begin, VK_SUBPASS_CONTENTS_INLINE);
    // A constant clip W of 64 means source W depth is 64/1024, although device Z remains 0.5.
    const float completed_depth = w_depth ? 0.0625f : 0.5f;
    fill_region(gpu.command, {{0, 4}, {4, 24}}, static_left, completed_depth);
    fill_region(gpu.command, {{4, 4}, {4, 24}}, static_right, completed_depth);
    fill_region(gpu.command, {{16, 4}, {12, 12}}, red_upper, completed_depth);
    fill_region(gpu.command, {{28, 4}, {20, 12}}, blue_upper, completed_depth);
    fill_region(gpu.command, {{16, 16}, {12, 12}}, red_lower, completed_depth);
    fill_region(gpu.command, {{28, 16}, {20, 12}}, blue_lower, completed_depth);
    fill_region(gpu.command, {{36, 4}, {4, 24}}, occluder, completed_depth * 0.5f);
    fill_region(gpu.command, {{48, 0}, {48, extent.height}}, second_view, completed_depth);
    vkCmdEndRenderPass(gpu.command);

    std::vector<ObjectMotionBlurVertex> vertices;
    append_quad(vertices, 0, 8, 0);
    append_quad(vertices, 16, 48, 28);
    copy_scene(gpu.command, color.handle, copy.handle);
    blur.begin_frame();
    blur.record(gpu.command, post_frame, {0, 0, float(first_view.extent.width), float(extent.height), 0, 1},
        first_view, glm::mat4(w_depth ? 64.0f : 1.0f), {}, w_depth, 0.5f, vertices);
    gpu.readback(color.handle, extent, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, output);
    gpu.end();
    verify_pixels(output);
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "object motion blur GPU test requires the shader directory");
        {
            Gpu gpu;
            if (!initialize(gpu)) {
                std::cout << "Object motion blur GPU test requires a graphics device with sampled D24S8\n";
                return 77;
            }
            verify_pass(gpu, argv[1], false);
            verify_pass(gpu, argv[1], true);
        }
        require(halo2_weather_plate_fixture::validation_errors == 0, "object motion blur reported Vulkan validation errors");
        std::cout << "Object blur softened Z and W depth color edges while preserving alpha, occlusion and viewport boundaries\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
