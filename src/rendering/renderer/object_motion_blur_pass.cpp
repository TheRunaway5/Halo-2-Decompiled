#include "object_motion_blur_pass.hpp"
#include "../effects/frame_upload_resources.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <vector>

namespace halo2 {
namespace {

constexpr auto shader_stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
constexpr auto depth_stages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
constexpr auto depth_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

struct BlurPush {
    glm::mat4 matrix;
    glm::ivec4 bounds;
    glm::vec2 extent;
    float shutter_fraction;
    std::uint32_t w_depth;
};
static_assert(offsetof(BlurPush, bounds) == 64 && offsetof(BlurPush, extent) == 80);
static_assert(offsetof(BlurPush, shutter_fraction) == 88 && offsetof(BlurPush, w_depth) == 92);
static_assert(sizeof(BlurPush) == 96);

struct ShaderModule {
    VkDevice device;
    VkShaderModule handle{};

    ShaderModule(VkDevice owner, const std::filesystem::path& filename) : device(owner) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        const auto size = file.tellg();
        if (size <= 0 || size % 4)
            throw FormatError("Invalid object motion blur shader: " + filename.string());
        std::vector<std::uint32_t> words(std::size_t(size) / sizeof(std::uint32_t));
        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(words.data()), size))
            throw FormatError("Truncated object motion blur shader: " + filename.string());
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = std::size_t(size);
        info.pCode = words.data();
        frame_upload::check(vkCreateShaderModule(device, &info, nullptr, &handle), "object motion blur shader");
    }

    ~ShaderModule() { vkDestroyShaderModule(device, handle, nullptr); }
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;
};

}

struct ObjectMotionBlurPass::Impl {
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkRenderPass render_pass{};
    VkImage depth{};
    VkSampler sampler{};
    VkDescriptorSetLayout set_layout{};
    VkDescriptorPool pool{};
    VkDescriptorSet set{};
    VkPipelineLayout layout{};
    VkPipeline pipeline{};
    std::vector<std::unique_ptr<frame_upload::Stream>> uploads;
    std::size_t next_upload{};

    ~Impl() {
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if (layout) vkDestroyPipelineLayout(device, layout, nullptr);
        if (pool) vkDestroyDescriptorPool(device, pool, nullptr);
        if (set_layout) vkDestroyDescriptorSetLayout(device, set_layout, nullptr);
        if (sampler) vkDestroySampler(device, sampler, nullptr);
    }

    void initialize(VkPipelineCache cache, const std::filesystem::path& shaders,
                    VkImageView depth_view, VkImageView copied_color) {
        using frame_upload::check;
        VkSamplerCreateInfo sampling{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampling.magFilter = sampling.minFilter = VK_FILTER_NEAREST;
        sampling.addressModeU = sampling.addressModeV = sampling.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        check(vkCreateSampler(device, &sampling, nullptr, &sampler), "object motion blur sampler");

        const VkDescriptorSetLayoutBinding bindings[]{
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        };
        VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        set_info.bindingCount = std::size(bindings);
        set_info.pBindings = bindings;
        check(vkCreateDescriptorSetLayout(device, &set_info, nullptr, &set_layout), "object motion blur descriptor layout");

        const VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2};
        VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &pool_size;
        check(vkCreateDescriptorPool(device, &pool_info, nullptr, &pool), "object motion blur descriptor pool");
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = pool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &set_layout;
        check(vkAllocateDescriptorSets(device, &allocation, &set), "object motion blur descriptor set");
        const VkDescriptorImageInfo images[]{
            {sampler, depth_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {sampler, copied_color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        };
        std::array<VkWriteDescriptorSet, 2> writes{};
        for (std::uint32_t binding = 0; binding < writes.size(); ++binding) {
            auto& write = writes[binding];
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = binding;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &images[binding];
        }
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);

        const VkPushConstantRange push{shader_stages, 0, sizeof(BlurPush)};
        VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &set_layout;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push;
        check(vkCreatePipelineLayout(device, &layout_info, nullptr, &layout), "object motion blur pipeline layout");

        const ShaderModule vertex_module(device, shaders / "object_motion_blur.vert.spv");
        const ShaderModule fragment_module(device, shaders / "object_motion_blur.frag.spv");
        const VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                VK_SHADER_STAGE_VERTEX_BIT, vertex_module.handle, "main", nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                VK_SHADER_STAGE_FRAGMENT_BIT, fragment_module.handle, "main", nullptr},
        };
        const VkVertexInputBindingDescription stream{0, sizeof(ObjectMotionBlurVertex), VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]{
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectMotionBlurVertex, current)},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjectMotionBlurVertex, previous)},
        };
        VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex.vertexBindingDescriptionCount = 1;
        vertex.pVertexBindingDescriptions = &stream;
        vertex.vertexAttributeDescriptionCount = std::size(attributes);
        vertex.pVertexAttributeDescriptions = attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.lineWidth = 1.0f;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState color{};
        color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &color;
        const VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = std::size(dynamic_states);
        dynamic.pDynamicStates = dynamic_states;

        VkGraphicsPipelineCreateInfo graphics{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        graphics.stageCount = std::size(stages);
        graphics.pStages = stages;
        graphics.pVertexInputState = &vertex;
        graphics.pInputAssemblyState = &assembly;
        graphics.pViewportState = &viewport;
        graphics.pRasterizationState = &raster;
        graphics.pMultisampleState = &samples;
        graphics.pColorBlendState = &blend;
        graphics.pDynamicState = &dynamic;
        graphics.layout = layout;
        graphics.renderPass = render_pass;
        check(vkCreateGraphicsPipelines(device, cache, 1, &graphics, nullptr, &pipeline), "object motion blur pipeline");
    }

    void depth_barrier(VkCommandBuffer command, bool to_shader) const {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image = depth;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1};
        barrier.oldLayout = to_shader ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = to_shader ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.srcAccessMask = to_shader ? depth_access : VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = to_shader ? VK_ACCESS_SHADER_READ_BIT : depth_access;
        const auto fragment = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        vkCmdPipelineBarrier(command, to_shader ? depth_stages : fragment,
            to_shader ? fragment : depth_stages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
};

ObjectMotionBlurPass::ObjectMotionBlurPass(VkPhysicalDevice physical, VkDevice device,
    VkPipelineCache cache, VkRenderPass render_pass, const std::filesystem::path& shaders,
    VkImage depth, VkImageView depth_view, VkImageView copied_color)
    : impl_(std::make_unique<Impl>()) {
    impl_->physical = physical;
    impl_->device = device;
    impl_->render_pass = render_pass;
    impl_->depth = depth;
    impl_->initialize(cache, shaders, depth_view, copied_color);
}

ObjectMotionBlurPass::~ObjectMotionBlurPass() = default;

void ObjectMotionBlurPass::begin_frame() { impl_->next_upload = 0; }

void ObjectMotionBlurPass::record(VkCommandBuffer command, VkFramebuffer framebuffer,
    VkViewport viewport, VkRect2D clip, const glm::mat4& matrix, glm::vec2 jitter,
    bool w_depth, float shutter_fraction, std::span<const ObjectMotionBlurVertex> vertices) {
    if (vertices.empty() || !clip.extent.width || !clip.extent.height ||
        !std::isfinite(shutter_fraction) || shutter_fraction <= 0.0f)
        return;
    if (vertices.size() % 3 || vertices.size() > std::numeric_limits<std::uint32_t>::max())
        throw FormatError("Object motion blur requires a complete triangle stream");

    auto& pass = *impl_;
    if (pass.next_upload == pass.uploads.size())
        pass.uploads.push_back(std::make_unique<frame_upload::Stream>(pass.physical, pass.device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
    auto& upload = *pass.uploads[pass.next_upload++];
    upload.write(std::as_bytes(vertices), false);

    const auto right = clip.offset.x + std::int32_t(clip.extent.width);
    const auto bottom = clip.offset.y + std::int32_t(clip.extent.height);
    const BlurPush push{matrix, {clip.offset.x, clip.offset.y, right, bottom},
        {viewport.width, viewport.height}, shutter_fraction, w_depth};
    viewport.x += jitter.x;
    viewport.y += jitter.y;

    pass.depth_barrier(command, true);
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass = pass.render_pass;
    begin.framebuffer = framebuffer;
    begin.renderArea = clip;
    vkCmdBeginRenderPass(command, &begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &clip);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pass.pipeline);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pass.layout, 0, 1, &pass.set, 0, nullptr);
    vkCmdPushConstants(command, pass.layout, shader_stages, 0, sizeof(push), &push);
    const auto buffer = upload.handle();
    constexpr VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command, 0, 1, &buffer, &offset);
    vkCmdDraw(command, std::uint32_t(vertices.size()), 1, 0, 0);
    vkCmdEndRenderPass(command);
    pass.depth_barrier(command, false);
}

}
