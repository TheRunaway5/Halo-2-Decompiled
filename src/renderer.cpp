#include <ranges>
#include "shader_constant_compare.hpp"
#include "frame_vertex_reuse.hpp"
#include "frame_vertex_upload_plan.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "halo2/renderer.hpp"
#include "halo2/debug_panel.hpp"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "halo2/atmosphere.hpp"
#include "halo2/menu_fog_state.hpp"
#include "halo2/bloom.hpp"
#include "halo2/math.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/world_material_uniform.hpp"
#include "halo2/presentation_format.hpp"
#include "halo2/sampler.hpp"
#include "halo2/texture_program.hpp"
#include "halo2/water.hpp"
#include "halo2/bsp_postprocess.hpp"
#include "halo2/texture.hpp"
#include "halo2/source_weather_depth_alias.hpp"
#include "halo2/scope_render.hpp"
#include "halo2/menu_projection.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/frontend_frame_sampling.hpp"
#include "halo2/frontend_post_raster.hpp"
#include "halo2/particle_renderer.hpp"
#include "halo2/source_decal_renderer.hpp"
#include "halo2/source_contrail_renderer.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_render_policy.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_shadow_render.hpp"
#include "halo2/source_shadow_draw_storage.hpp"
#include "resident_draw_storage.hpp"
#include "halo2/mainmenu_world_source.hpp"
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
#include "halo2/menu_world_native_packet.hpp"
#include "halo2/menu_world_depth.hpp"
#include "halo2/menu_world_entry_stencil.hpp"
#include "menu_world_vertex_catalog.generated.hpp"
#endif
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <deque>
#include <bit>
#include <charconv>
#include <cmath>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace halo2 {
namespace {
void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) throw std::runtime_error(std::string(operation) + ": Vulkan result " + std::to_string(result));
}
// Retail12490 and the observed128C0 integer-depth setting use D24S8.
// Packed weather depth cannot be supplied by a D32/D16 fallback.
constexpr VkFormat source_depth_format=VK_FORMAT_D24_UNORM_S8_UINT;
constexpr VkImageUsageFlags source_depth_usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|
    VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
void write_haze_trace(const std::filesystem::path &path,const void *data,std::size_t bytes) {
    std::ofstream file(path,std::ios::binary);
    if(!file.write(static_cast<const char *>(data),std::streamsize(bytes)))
        throw FormatError("cannot write actual live title haze trace");
}
std::array<float,49> haze_trace_frame(const PatchyFogFrame &frame) {
    std::array<float,49> words;
    for(unsigned i=0;i<4;++i)std::copy(frame.weights[i].begin(),frame.weights[i].end(),words.begin()+i*4);
    for(unsigned i=0;i<8;++i)std::copy(frame.rows[i].begin(),frame.rows[i].end(),words.begin()+16+i*4);
    words.back()=frame.vertical;
    return words;
}
bool source_depth_supported(VkPhysicalDevice physical,VkImageFormatProperties &image) {
    VkFormatProperties format;
    vkGetPhysicalDeviceFormatProperties(physical,source_depth_format,&format);
    constexpr VkFormatFeatureFlags required=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if((format.optimalTilingFeatures&required)!=required)return false;
    const auto result=vkGetPhysicalDeviceImageFormatProperties(physical,source_depth_format,
        VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,source_depth_usage,0,&image);
    if(result==VK_ERROR_FORMAT_NOT_SUPPORTED)return false;
    check(result,"query original integer-depth image support");
    return (image.sampleCounts&VK_SAMPLE_COUNT_1_BIT) && image.maxMipLevels>=1 && image.maxArrayLayers>=1;
}
std::vector<std::uint32_t> shader_file(const std::filesystem::path& directory,const char* name) {
    const auto path = directory / name;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("cannot open shader " + path.string());
    const auto end = file.tellg();
    if (end <= 0 || end % 4 != 0) throw std::runtime_error("invalid SPIR-V length");
    std::vector<std::uint32_t> words(static_cast<std::size_t>(end) / 4);
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(words.data()), end)) throw std::runtime_error("short SPIR-V read");
    return words;
}
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void*) {
    std::cerr << "Vulkan validation: " << data->pMessage << '\n';
    return VK_FALSE;
}
glm::mat4 view_projection(const Camera& camera, VkExtent2D extent, bool title = false,
                         std::optional<Vec2> clip_planes = {}) {
    const glm::vec3 eye(camera.position.x, camera.position.y, camera.position.z);
    const glm::vec3 forward = camera.oriented ? glm::vec3(camera.forward.x, camera.forward.y, camera.forward.z) : glm::vec3(std::cos(camera.pitch) * std::cos(camera.yaw),
                            std::cos(camera.pitch) * std::sin(camera.yaw), std::sin(camera.pitch));
    const glm::vec3 up = camera.oriented ? glm::vec3(camera.up.x, camera.up.y, camera.up.z) : glm::vec3(0, 0, 1);
    const float aspect = float(extent.width) / float(extent.height);
    const auto planes=clip_planes.value_or(title?Vec2{.015625f,1024.f}:Vec2{.05f,10000.f});
    if(!std::isfinite(planes.x) || !std::isfinite(planes.y) || planes.x<=0 || planes.y<=planes.x)
        throw FormatError("invalid camera clipping planes");
    auto projection = glm::perspective(camera_vertical_field_of_view(camera, aspect), aspect,
        planes.x,planes.y); // Retail 00467014/18 for title and authored UI cameras.
    projection[0][0] *= camera.projection_clip_scale.x;
    projection[1][1] *= camera.projection_clip_scale.y;
    projection[2][0] -= camera.projection_clip_offset.x;
    projection[2][1] += camera.projection_clip_offset.y;
    projection[1][1] *= -1;
    return projection * glm::lookAt(eye, eye + forward, up);
}
} // namespace

struct Renderer::Impl {
    std::filesystem::path shader_directory{HALO2_SHADER_DIR};
    std::vector<std::uint32_t> shader(const char* name)const{return shader_file(shader_directory,name);}
    // One platform window retains one Vulkan instance/surface across scene
    // devices. Destroying the old scene must not tear down the native surface
    // underlying the new scene's first image acquisition.
    struct Presentation {
        SDL_Window *window{};
        VkInstance instance{};
        VkDebugUtilsMessengerEXT debug{};
        VkSurfaceKHR surface{};
        std::uint32_t api_version{};
        bool validation{};
        ~Presentation() {
            if(surface)vkDestroySurfaceKHR(instance,surface,nullptr);
            if(debug) {
                const auto destroy=reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));
                if(destroy)destroy(instance,debug,nullptr);
            }
            if(instance)vkDestroyInstance(instance,nullptr);
        }
    };
    std::shared_ptr<Presentation> presentation;
    bool measure_frames{};
    RendererFrameTiming frame_timing;
    std::ofstream timing_report;
    unsigned timing_frame{};
    std::chrono::steady_clock::time_point timing_previous{};
    bool group_input_diagnostic_done{};
    std::vector<Vertex> dynamic_vertex_staging;
    struct DynamicImageUpdate {std::vector<std::byte> pixels;bool pending{};};
    std::map<unsigned,DynamicImageUpdate> dynamic_images;
    std::vector<unsigned> pending_images;
    VkBuffer dynamic_image_buffer{};
    VkDeviceMemory dynamic_image_memory{};
    VkDeviceSize dynamic_image_capacity{};
    SDL_Window* window{};
    std::unique_ptr<DebugPanel> debug_panel;
    bool debug_graphics{};
    ImDrawData* debug_draw_data{};
    VkInstance instance{};
    VkDebugUtilsMessengerEXT debug{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue queue{};
    std::uint32_t queue_family{};
    std::string gpu;
    SourceWorldDeviceState source_world_device;
    VkSwapchainKHR swapchain{};
    VkFormat format{}, depth_format{};
    VkExtent2D extent{};
    VkExtent2D presentation_extent{};
    std::vector<VkImage> presentation_images;
    unsigned active_internal_width{},active_internal_height{},active_aa{},active_detail{};
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    std::vector<VkFramebuffer> framebuffers;
    // Presentation completion is tied to each acquired image, per Vulkan's semaphore reuse rule.
    std::vector<VkSemaphore> present_ready;
    VkImage depth{};
    VkDeviceMemory depth_memory{};
    VkImageView depth_view{};
    VkImageView depth_attachment_view{};
    VkRenderPass render_pass{};
    VkImageLayout scene_layout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkRenderPass continuation_pass{}, fog_pass{};
    std::vector<VkFramebuffer> fog_framebuffers;
    VkPipeline fog_pipeline{};
    VkPipelineLayout fog_layout{};
    VkDescriptorSetLayout fog_texture_layout{};
    VkDescriptorPool fog_pool{};
    VkDescriptorSet fog_descriptor{};
    std::array<VkDescriptorSet,4> view_fog_descriptors{};
    std::array<VkBuffer,4> view_fog_buffers{};
    std::array<VkDeviceMemory,4> view_fog_memories{};
    SourceFogDrawControls fog_draw_controls{};
    VkSampler fog_depth_sampler{}, fog_gradient_sampler{};
    VkBuffer fog_buffer{};
    VkDeviceMemory fog_memory{};
    VkRenderPass patchy_pass{};
    VkFramebuffer patchy_framebuffer{};
    VkPipeline patchy_noise_pipeline{},patchy_composite_pipeline{};
    VkPipelineLayout patchy_layout{};
    VkDescriptorSetLayout patchy_texture_layout{};
    VkDescriptorPool patchy_pool{};
    std::array<VkDescriptorSet,2> patchy_descriptors{};
    std::array<VkBuffer,2> patchy_buffers{};
    std::array<VkDeviceMemory,2> patchy_memories{};
    std::array<VkBuffer,2> patchy_quad_buffers{};
    std::array<VkDeviceMemory,2> patchy_quad_memories{};
    VkSampler patchy_sampler{};
    VkExtent2D patchy_extent{};
    PatchyFogState patchy_state;
    std::uint32_t patchy_random{};
    std::uint64_t patchy_trace_frame{};
    VkPipelineLayout bloom_layout{};
    VkDescriptorSetLayout bloom_texture_layout{};
    VkDescriptorPool bloom_pool{};
    // Reserve two channel boundaries; the measured settled title/profile
    // and title/keyboard states each submit one. Every boundary owns four
    // constants until the GPU completes.
    static constexpr unsigned popup_channel_count=2;
    static constexpr unsigned postprocess_count=7+4*popup_channel_count;
    // 0..6: world bloom/filter; the remaining slots are full-size UI blur.
    std::array<VkDescriptorSet,postprocess_count> bloom_descriptors{};
    std::array<VkBuffer,postprocess_count> bloom_buffers{};
    std::array<VkDeviceMemory,postprocess_count> bloom_memories{};
    VkPipeline bloom_filter_pipeline{},bloom_composite_pipeline{},scene_filter_pipeline{};
    VkPipeline crt_pipeline{},display_pipeline{};
    VkPipelineLayout crt_layout{},display_layout{};
    std::array<VkFramebuffer,2> bloom_framebuffers{};
    VkExtent2D bloom_extent{};
    std::optional<float> bloom_threshold;
    std::optional<PixelProgram> scene_filter;
    bool bloom_scene_initialized{};
    VkRenderPass accumulation_pass{};
    VkRenderPass world_accumulation_pass{},foreground_accumulation_pass{};
    VkFramebuffer accumulation_framebuffer{};
    VkImage accumulation_image{};
    VkDeviceMemory accumulation_memory{};
    VkImageView accumulation_view{};
    VkRenderPass water_reflection_pass{},water_copy_pass{};
    VkFramebuffer water_reflection_framebuffer{},water_copy_framebuffer{};
    VkPipeline water_copy_pipeline{};
    VkDescriptorPool water_copy_pool{};
    std::array<VkDescriptorSet,4> water_copy_descriptors{};
    std::array<VkBuffer,4> water_copy_buffers{};
    std::array<VkDeviceMemory,4> water_copy_memories{};
    bool water_scene_initialized{};
    std::size_t material_count{};
    std::vector<unsigned> environment_materials;
    std::vector<std::optional<unsigned>> source_material_layers;
    std::vector<std::array<unsigned,2>> source_material_identities;
    std::vector<std::vector<unsigned>> source_layer15_passes;
    std::vector<std::vector<unsigned>> world_before, world_after, world_bloom;
    std::vector<bool> baked_materials;
    std::map<std::pair<unsigned,unsigned>,VkDescriptorSet> geometry_descriptors;
    VkPipelineLayout pipeline_layout{};
    VkPipeline pipeline{}, sky_pipeline{};
    VkPipeline light_volume_pipeline{};
    bool light_volume_materials{};
    VkPipeline overlay{},popup_tint_pipeline{},ui_backdrop_pipeline{};
    VkPipeline ui_pipeline{};
    VkPipeline ui_frame_pipeline{},ui_frame_backdrop_pipeline{};
    VkPipelineLayout ui_frame_layout{};
    VkPipelineLayout vista_loading_layout{};
    std::array<VkPipeline,3> vista_loading_pipelines{};
    std::vector<std::optional<VistaLoadingMaterial>> vista_loading_materials;
    std::array<VkPipeline,5> hud_pipelines{};
    std::vector<unsigned> hud_materials;
    std::vector<bool> ui_emblem_materials;
    std::vector<bool> ui_full_surfaces;
    std::vector<bool> ui_widescreen_backgrounds;
    std::vector<UiFrameExtension> ui_frame_extensions;
    std::vector<unsigned> ui_frame_widths;
    struct ForwardPipeline {
        unsigned blend{};
        ForwardRasterState state;
        VkPipeline handle{}, sky_handle{};
        bool extended_vertex_bank{};
        std::optional<std::array<std::uint32_t,60>> controls;
        bool emblem_w_depth{};
    };
    static std::array<std::uint32_t,60> combiner_controls(std::array<std::uint32_t,60> program) {
        std::fill(program.begin()+10,program.begin()+26,0);
        program[43]=program[44]=0;
        return program;
    }
    bool specialize_combiners=[] {
        const auto *reference=std::getenv("HALO2_REFERENCE_GENERIC_COMBINERS");
        return !reference || !*reference;
    }();
    std::vector<unsigned> specialized_pipeline_indices,active_forward_pipeline_indices;
    std::vector<ForwardPipeline> forward_pipelines;
    std::vector<unsigned> forward_pipeline_indices;
    VkPipelineLayout overlay_layout{};
    VkCommandPool command_pool{};
    VkCommandBuffer command{};
    VkSemaphore acquired{};
    VkFence fence{};
    VkBuffer vertex_buffer{};
    VkDeviceMemory vertex_memory{};
    VkBuffer debug_buffer{};
    VkDeviceMemory debug_memory{};
    VkBuffer light_volume_buffer{};
    VkDeviceMemory light_volume_memory{};
    std::size_t light_volume_capacity{};
    std::vector<LightVolumeInstance> light_volumes;
    std::vector<LightVolumeVertex> light_volume_staging;
    std::vector<bool> light_volume_slots;
    std::vector<ParticleDraw> particle_draws;
#ifdef HALO2_PARTICLE_GRAPHICS_AVAILABLE
    std::unique_ptr<ParticleRenderer> particle_renderer;
#endif
#if HALO2_SOURCE_DECALS_AVAILABLE
    std::unique_ptr<SourceDecalRenderer> decal_renderer;
#if HALO2_SOURCE_CONTRAILS_AVAILABLE
    std::unique_ptr<SourceContrailRenderer> contrail_renderer;
#endif
#endif
    std::shared_ptr<const ShaderFunctionTables> light_volume_tables;
    struct LightVolumeDraw { std::uint32_t first{},count{},material{}; float sort_depth{}; };
    std::size_t debug_capacity = 100000;
    static constexpr std::size_t dynamic_limit = 30000000;
    std::uint32_t vertex_count{};
    const SourceBspResidentPlan *bsp_resident_plan{};
    std::array<SourceBspMergedDrawPlan,4> bsp_frame_plans;
    struct Draw {
        std::uint32_t first{}, count{}, material{}, lightmap{};
        VkDescriptorSet descriptor{}; int cluster=-1;
        std::uint32_t source_section=0xffffffffu,source_part=0xffffffffu;
        bool source_resident{};
        // Index into this frame's owner-constant descriptor aliases.
        std::size_t object_alias=std::numeric_limits<std::size_t>::max();
    };
    using DrawKey=std::tuple<std::uint32_t,std::uint32_t,int,std::uint32_t,std::uint32_t>;
    static DrawKey draw_key(const Vertex &vertex) {
        return {vertex.material,vertex.lightmap,vertex.bsp_cluster,
            vertex.source_section,vertex.source_part};
    }
    struct SharedImage {
        VkDevice device{}; VkImage image{}; VkDeviceMemory memory{}; VkImageView view{},cube_view{};
        ~SharedImage() {
            if(view)vkDestroyImageView(device,view,nullptr);
            if(cube_view)vkDestroyImageView(device,cube_view,nullptr);
            if(image)vkDestroyImage(device,image,nullptr);
            if(memory)vkFreeMemory(device,memory,nullptr);
        }
    };
    struct GpuTexture { VkImage image{}; VkDeviceMemory memory{}; VkImageView view{}, cube_view{}; VkDescriptorSet descriptor{};
        std::shared_ptr<SharedImage> shared_image;
        std::uint32_t width{}, height{}, mip_count{}, depth{}; std::uint16_t format{};
        // Metadata for this exact uploaded page, never a fabricated fallback
        // binding. Pixel ownership remains in the source material bank.
        Texture source_shape;
        bool source_image{};
    };
    std::vector<Draw> draws;
    std::vector<GpuTexture> textures;
    std::vector<GpuTexture> scene_targets;
    GpuTexture ssaa_intermediate;
    struct TextureStaging { VkBuffer buffer{}; VkDeviceMemory memory{}; };
    std::vector<TextureStaging> texture_staging;
    VkDeviceSize texture_staging_bytes{};
    bool batch_texture_uploads{};
    std::vector<GpuTexture> detail_textures, bump_textures, lightmap_textures;
    std::vector<GpuTexture> auxiliary_textures;
    std::vector<std::array<std::vector<GpuTexture>,4>> animated_textures;
    std::vector<std::array<std::uint32_t,4>> wanted_frames, bound_frames;
    std::vector<std::array<VkSampler,4>> stage_samplers;
    std::vector<std::array<ShaderTextureState,4>> stage_sampler_states;
    std::vector<bool> scope_materials;
    std::vector<VkDescriptorSet> scope_descriptors;
    std::unique_ptr<ScopeRender> scope_render;
    std::vector<SourceShadowPlanMaterial> shadow_materials;
    std::unique_ptr<SourceShadowRender> shadow_render;
    std::unique_ptr<SourceShadowDrawStorage> shadow_storage;
    std::unique_ptr<SourceWorldDrawStorage> effect_light_storage;
    std::vector<bool> effect_light_materials;
    std::vector<ForwardRasterState> material_forward_states;
    std::vector<bool> accumulation_materials;
    std::unique_ptr<ResidentDrawStorage> object_draw_storage;
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
    std::unique_ptr<MenuWorldNativeDraw> menu_world_native;
    std::unique_ptr<MenuWorldNativeDraw> effect_light_native;
    bool menu_world_reported{};
    using MenuWorldDrawPair=std::pair<std::size_t,std::optional<std::size_t>>;
    using MenuWorldDrawMap=std::map<DrawKey,std::vector<MenuWorldDrawPair>>;
#endif
    GpuTexture cube_fallback, volume_fallback, image_fallback;
    GpuTexture fog_gradient;
    GpuTexture patchy_bitmap,patchy_target;
    GpuTexture bloom_scene;
    GpuTexture water_scene,water_reflection,water_refraction,water_depth;
    GpuTexture particle_scene;
    bool particle_scene_initialized{};
    SourceParticleSamplerBank particle_sampler_state{};
    const BspAtmosphere *bsp_bank{};
    std::unique_ptr<BspPostprocess> bsp_postprocess;
    std::optional<BspPostprocessState> bsp_postprocess_state;
    std::vector<GpuTexture> bsp_postprocess_textures;
    std::map<const Texture *,GpuTexture> weather_sprites;
    VkSampler weather_sprite_sampler{};
    bool weather_large_points{};
    bool weather_wide_lines{};
    std::vector<std::unique_ptr<SourceWeatherPointRender>> weather_point_renderers;
    std::vector<std::unique_ptr<SourceWeatherLineRender>> weather_line_renderers;
    std::vector<std::unique_ptr<SourceWeatherPlateRender>> weather_plate_renderers;
    std::vector<std::unique_ptr<SourceWeatherDepthAlias>> weather_aliases;
    std::vector<bool> weather_alias_pending;
    std::array<GpuTexture,2> bloom_targets{};
    std::vector<bool> after_fog;
    std::vector<unsigned> projected_decal_layers;
    std::vector<unsigned> planar_materials;
    std::map<unsigned,MaterialPass> planar_passes;
    std::vector<std::array<unsigned,3>> world_planar_materials;
    std::map<unsigned,MaterialPass> fog_material_passes;
    std::map<unsigned,ShaderAnimation> owner_material_animations;
    std::map<unsigned,std::array<std::int32_t,8>> fog_material_switches;
    std::vector<unsigned> material_roots;
    std::vector<std::vector<ShaderRemap>> view_direction_remaps,view_fog_remaps;
    std::array<std::vector<bool>,4> view_material_initialized;
    std::vector<std::array<float,2>> material_lighting_brightness;
    std::vector<int> water_material_definitions;
    std::vector<bool> water_materials;
    std::vector<std::array<bool,2>> water_texture_bindings;
    VkBuffer material_buffer{};
    VkDeviceMemory material_memory{};
    void *material_mapping{};
    bool base_camera_initialized{};
    std::optional<std::array<ShaderVector,7>> base_camera;
    VkBuffer vertex_bank_buffer{};
    VkDeviceMemory vertex_bank_memory{};
    VkDeviceSize vertex_bank_stride{};
    using VertexBankUniform=PackedVertexProgramBank;
    static_assert(sizeof(VertexBankUniform)==2544);
    static VertexBankUniform source_vertex_bank(const Material &material) {
        VertexBankUniform result;
        if(!material.vertex_bank)return result;
        if(!material.forward_pass)
            throw FormatError("draw vertex globals require an authored forward pass");
        return pack_vertex_program_bank(*material.vertex_bank);
    }
    std::vector<VertexBankUniform> vertex_bank_uniforms;
    // Native programs already address logical c0..191 through the explicit
    // camera/fog/material/extended banks. Vulkan injects no reserved constants.
    std::uint32_t source_constant_mode=0x11;
    // Existing world passes use integer W depth. Native source startup
    // selects that same mode; each pass retains its authored raster policy.
    std::uint32_t source_depth_mode=2;
    std::uint32_t source_depth_write=1,source_depth_function=0x203,source_depth_bias=0;
    std::array<std::uint32_t,8> source_output_state=renderer_cold_output_values;
    std::array<std::uint32_t,4> source_coordinate_registers=renderer_passthrough_registers;
    std::vector<bool> extended_vertex_banks;
    std::vector<std::size_t> changed_vertex_banks;
    using MaterialUniform=WorldMaterialUniform;
    std::vector<MaterialUniform> material_uniforms;
    std::vector<std::uint32_t> material_blend_colors;
    // Pixel remaps address fragments before the assembled combiner stages.
    // These CPU offsets are part of the active program, beyond its GPU words.
    std::vector<std::array<std::uint8_t,8>> material_fragment_offsets;
    std::vector<std::size_t> changed_materials;
    std::vector<std::uint8_t> material_pending,vertex_bank_pending;
    void mark_material_changed(std::size_t index) {
        if(!material_pending[index]) {material_pending[index]=1;changed_materials.push_back(index);}
    }
    void mark_vertex_bank_changed(std::size_t index) {
        if(!vertex_bank_pending[index]) {vertex_bank_pending[index]=1;changed_vertex_banks.push_back(index);}
    }
    VkDeviceSize material_stride{};
    VkDescriptorSetLayout texture_layout{};
    VkDescriptorPool texture_pool{};
    VkSampler sampler{};
    VkSampler lightmap_sampler{};
    std::map<std::pair<ShaderTextureState,unsigned>,VkSampler> authored_samplers;
    bool sampler_anisotropy{}, custom_border_color{}, sampler_mirror_clamp{}, sampler_non_seamless_cube{};
    VkPhysicalDeviceLimits sampler_limits{};
    bool dirty{}, can_capture{};
    std::uint64_t diagnostic_frame{};
    std::optional<Vec3> scene_clear;
    std::optional<MenuAtmosphere> atmosphere;
    std::optional<BspAtmosphere> title_atmosphere_source;
    bool bsp_atmosphere{};

    static bool shadow_texture_equal(const Texture &a,const Texture &b) {
        if(a.width!=b.width || a.height!=b.height || a.format!=b.format ||
           a.layers!=b.layers || a.depth!=b.depth || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
        for(std::size_t i=0;i<a.mips.size();++i) {
            const auto &x=a.mips[i],&y=b.mips[i];
            if(std::tie(x.width,x.height,x.depth,x.offset,x.size)!=
               std::tie(y.width,y.height,y.depth,y.offset,y.size))return false;
        }
        return true;
    }
    void shadow_residency(const Mesh &mesh,std::span<const SourceShadowPlanMaterial> resident) {
        std::set<unsigned> slots;
        for(const auto &entry:resident) {
            if(!entry.authored || entry.material_slot>=mesh.materials.size() ||
               !slots.insert(entry.material_slot).second)
                throw FormatError("invalid resident source shadow slot");
            const auto &authored=entry.authored->material;
            const auto &material=mesh.materials[entry.material_slot];
            if(!authored.forward_pass || authored.forward_pass_index>=authored.passes.size() ||
               !material.forward_pass || material.forward_pass_index!=authored.forward_pass_index ||
               material.source_shader_datum!=authored.source_shader_datum ||
               material.forward_vertex!=authored.forward_vertex ||
               material.forward_raster!=authored.forward_raster || material.forward_blend!=authored.forward_blend ||
               material.vertex_program!=authored.vertex_program ||
               material.vertex_declaration!=authored.vertex_declaration ||
               material.world_vertex_input_transform!=authored.world_vertex_input_transform ||
               material.texture_states!=authored.texture_states)
                throw FormatError("resident shadow slot differs from its authored pass");
            const Texture *actual[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
            const Texture *expected[]{&authored.diffuse,&authored.detail,&authored.bump,&authored.auxiliary};
            for(unsigned stage=0;stage<4;++stage) {
                if(entry.authored->external_targets[stage]==10)continue;
                const auto &frames=authored.animated_textures[stage];
                const auto &resident_frames=material.animated_textures[stage];
                if(frames.size()!=resident_frames.size() ||
                   !std::equal(frames.begin(),frames.end(),resident_frames.begin(),shadow_texture_equal) ||
                   (!shadow_texture_equal(*actual[stage],*expected[stage]) &&
                    std::none_of(frames.begin(),frames.end(),[&](const auto &frame){return shadow_texture_equal(*actual[stage],frame);})))
                    throw FormatError("resident shadow image differs from its authored bitmap/mip bank");
            }
            const auto &pass=authored.passes.at(authored.forward_pass_index);
            const auto phase=pass.vertex_shader.ends_with("\\shadow_buffer_generation")?
                SourceShadowPhase::Generate:SourceShadowPhase::Apply;
            validate_source_shadow_plan_material(*entry.authored,phase);
            const auto program=decode_vertex_program(material.vertex_program);
            if(program.relative_constants || !program.count)
                throw FormatError("resident shadow slot lacks admitted world program");
        }
        shadow_materials.assign(resident.begin(),resident.end());
    }

    ~Impl() {
        if (device) {
            vkDeviceWaitIdle(device);
            release_texture_staging();
            destroy_swapchain();
            if(bloom_pool)vkDestroyDescriptorPool(device,bloom_pool,nullptr);
            if(bloom_layout)vkDestroyPipelineLayout(device,bloom_layout,nullptr);
            if(bloom_texture_layout)vkDestroyDescriptorSetLayout(device,bloom_texture_layout,nullptr);
            for(unsigned i=0;i<bloom_buffers.size();++i) {
                if(bloom_buffers[i])vkDestroyBuffer(device,bloom_buffers[i],nullptr);
                if(bloom_memories[i])vkFreeMemory(device,bloom_memories[i],nullptr);
            }
            if(patchy_pool)vkDestroyDescriptorPool(device,patchy_pool,nullptr);
            if(patchy_layout)vkDestroyPipelineLayout(device,patchy_layout,nullptr);
            if(patchy_texture_layout)vkDestroyDescriptorSetLayout(device,patchy_texture_layout,nullptr);
            if(patchy_sampler)vkDestroySampler(device,patchy_sampler,nullptr);
            for(unsigned i=0;i<2;++i) {
                if(patchy_buffers[i])vkDestroyBuffer(device,patchy_buffers[i],nullptr);
                if(patchy_memories[i])vkFreeMemory(device,patchy_memories[i],nullptr);
                if(patchy_quad_buffers[i])vkDestroyBuffer(device,patchy_quad_buffers[i],nullptr);
                if(patchy_quad_memories[i])vkFreeMemory(device,patchy_quad_memories[i],nullptr);
            }
            if(patchy_bitmap.view)vkDestroyImageView(device,patchy_bitmap.view,nullptr);
            if(patchy_bitmap.image)vkDestroyImage(device,patchy_bitmap.image,nullptr);
            if(patchy_bitmap.memory)vkFreeMemory(device,patchy_bitmap.memory,nullptr);
            if(fog_pool)vkDestroyDescriptorPool(device,fog_pool,nullptr);
            if(fog_layout)vkDestroyPipelineLayout(device,fog_layout,nullptr);
            if(fog_texture_layout)vkDestroyDescriptorSetLayout(device,fog_texture_layout,nullptr);
            if(fog_depth_sampler)vkDestroySampler(device,fog_depth_sampler,nullptr);
            if(fog_gradient_sampler)vkDestroySampler(device,fog_gradient_sampler,nullptr);
            if(fog_buffer)vkDestroyBuffer(device,fog_buffer,nullptr);
            if(fog_memory)vkFreeMemory(device,fog_memory,nullptr);
            if(water_copy_pool)vkDestroyDescriptorPool(device,water_copy_pool,nullptr);
            for(unsigned i=0;i<4;++i) {
                if(water_copy_buffers[i])vkDestroyBuffer(device,water_copy_buffers[i],nullptr);
                if(water_copy_memories[i])vkFreeMemory(device,water_copy_memories[i],nullptr);
            }
            for(unsigned i=0;i<4;++i) {
                if(view_fog_buffers[i])vkDestroyBuffer(device,view_fog_buffers[i],nullptr);
                if(view_fog_memories[i])vkFreeMemory(device,view_fog_memories[i],nullptr);
            }
            if(fog_gradient.view)vkDestroyImageView(device,fog_gradient.view,nullptr);
            if(fog_gradient.image)vkDestroyImage(device,fog_gradient.image,nullptr);
            if(fog_gradient.memory)vkFreeMemory(device,fog_gradient.memory,nullptr);
            if (texture_pool) vkDestroyDescriptorPool(device, texture_pool, nullptr);
            for (auto* images : {&textures,&detail_textures,&bump_textures,&lightmap_textures,&auxiliary_textures,&bsp_postprocess_textures}) for (auto& texture : *images) {
                if(texture.shared_image){texture.shared_image.reset();continue;}
                if (texture.view) vkDestroyImageView(device, texture.view, nullptr);
                if (texture.cube_view) vkDestroyImageView(device, texture.cube_view, nullptr);
                if (texture.image) vkDestroyImage(device, texture.image, nullptr);
                if (texture.memory) vkFreeMemory(device, texture.memory, nullptr);
            }
            for(auto &material:animated_textures) for(auto &stage:material) for(auto &texture:stage) {
                if(texture.shared_image){texture.shared_image.reset();continue;}
                if(texture.view) vkDestroyImageView(device,texture.view,nullptr);
                if(texture.cube_view) vkDestroyImageView(device,texture.cube_view,nullptr);
                if(texture.image) vkDestroyImage(device,texture.image,nullptr);
                if(texture.memory) vkFreeMemory(device,texture.memory,nullptr);
            }
            if(cube_fallback.view) vkDestroyImageView(device,cube_fallback.view,nullptr);
            if(cube_fallback.cube_view) vkDestroyImageView(device,cube_fallback.cube_view,nullptr);
            if(cube_fallback.image) vkDestroyImage(device,cube_fallback.image,nullptr);
            if(cube_fallback.memory) vkFreeMemory(device,cube_fallback.memory,nullptr);
            for(const auto *texture:{&volume_fallback,&image_fallback}) {
                if(texture->view)vkDestroyImageView(device,texture->view,nullptr);
                if(texture->image)vkDestroyImage(device,texture->image,nullptr);
                if(texture->memory)vkFreeMemory(device,texture->memory,nullptr);
            }
            if (sampler) vkDestroySampler(device, sampler, nullptr);
            if (lightmap_sampler) vkDestroySampler(device, lightmap_sampler, nullptr);
            for(const auto &[state,handle]:authored_samplers)
                vkDestroySampler(device,handle,nullptr);
            if (material_buffer) vkDestroyBuffer(device, material_buffer, nullptr);
            if (material_mapping) vkUnmapMemory(device, material_memory);
            if (material_memory) vkFreeMemory(device, material_memory, nullptr);
            if (vertex_bank_buffer) vkDestroyBuffer(device, vertex_bank_buffer, nullptr);
            if (vertex_bank_memory) vkFreeMemory(device, vertex_bank_memory, nullptr);
            if (vertex_buffer) vkDestroyBuffer(device, vertex_buffer, nullptr);
            if (vertex_memory) vkFreeMemory(device, vertex_memory, nullptr);
            if (debug_buffer) vkDestroyBuffer(device, debug_buffer, nullptr);
            if (debug_memory) vkFreeMemory(device, debug_memory, nullptr);
            if(light_volume_buffer)vkDestroyBuffer(device,light_volume_buffer,nullptr);
            if(light_volume_memory)vkFreeMemory(device,light_volume_memory,nullptr);
            if(dynamic_image_buffer)vkDestroyBuffer(device,dynamic_image_buffer,nullptr);
            if(dynamic_image_memory)vkFreeMemory(device,dynamic_image_memory,nullptr);
            if (fence) vkDestroyFence(device, fence, nullptr);
            if (acquired) vkDestroySemaphore(device, acquired, nullptr);
            if (command_pool) vkDestroyCommandPool(device, command_pool, nullptr);
            if (pipeline_layout) vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
            if (ui_frame_layout) vkDestroyPipelineLayout(device, ui_frame_layout, nullptr);
            if (vista_loading_layout) vkDestroyPipelineLayout(device, vista_loading_layout, nullptr);
            if (overlay_layout) vkDestroyPipelineLayout(device, overlay_layout, nullptr);
            if (texture_layout) vkDestroyDescriptorSetLayout(device, texture_layout, nullptr);
            vkDestroyDevice(device, nullptr);
        }
    }

    std::uint32_t memory_type(std::uint32_t bits, VkMemoryPropertyFlags flags) const {
        VkPhysicalDeviceMemoryProperties properties;
        vkGetPhysicalDeviceMemoryProperties(physical, &properties);
        for (std::uint32_t i = 0; i < properties.memoryTypeCount; ++i)
            if ((bits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) return i;
        throw std::runtime_error("no compatible Vulkan memory type");
    }

    void make_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = size;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(device, &info, nullptr, &buffer), "create buffer");
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device, buffer, &requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device, &allocation, nullptr, &memory), "allocate buffer memory");
        check(vkBindBufferMemory(device, buffer, memory, 0), "bind buffer memory");
    }

    void initialize_presentation(SDL_Window* target, bool validation) {
        window = target;
        if(presentation) {
            if(presentation->window!=window || presentation->validation!=validation)
                throw FormatError("renderer handoff changed its platform presentation owner");
            instance=presentation->instance;debug=presentation->debug;surface=presentation->surface;
            source_world_device.instance_api_version=presentation->api_version;
            return;
        }
        presentation=std::make_shared<Presentation>();
        presentation->window=window;presentation->validation=validation;
        unsigned extension_count = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr)) throw std::runtime_error(SDL_GetError());
        std::vector<const char*> extensions(extension_count);
        if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) throw std::runtime_error(SDL_GetError());
        const char* layer = "VK_LAYER_KHRONOS_validation";
        VkDebugUtilsMessengerCreateInfoEXT debug_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_info.pfnUserCallback = debug_callback;
        if (validation) {
            std::uint32_t count;
            check(vkEnumerateInstanceLayerProperties(&count, nullptr), "enumerate instance layers");
            std::vector<VkLayerProperties> layers(count);
            check(vkEnumerateInstanceLayerProperties(&count, layers.data()), "enumerate instance layers");
            if (std::none_of(layers.begin(), layers.end(), [&](const auto& item) { return std::strcmp(item.layerName, layer) == 0; }))
                throw std::runtime_error("VK_LAYER_KHRONOS_validation is unavailable; install the layer to use --validation");
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        application.pApplicationName = "Halo 2 native workbench";
        std::uint32_t loader_version=VK_API_VERSION_1_0;
        const auto enumerate_version=reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
            vkGetInstanceProcAddr(nullptr,"vkEnumerateInstanceVersion"));
        if(enumerate_version)check(enumerate_version(&loader_version),"query Vulkan loader version");
        if(loader_version<VK_API_VERSION_1_1)
            throw FormatError("native renderer requires Vulkan 1.1 or newer");
        // The raw menu-world shaders are SPIR-V1.6, compiled for Vulkan1.3.
        // Preserve the ordinary renderer on older loaders; never admit those
        // shaders through an extension-only demote feature on a1.1 instance.
        application.apiVersion=loader_version>=VK_API_VERSION_1_3?
            VK_API_VERSION_1_3:VK_API_VERSION_1_1;
        source_world_device.instance_api_version=application.apiVersion;
        presentation->api_version=application.apiVersion;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        info.pApplicationInfo = &application;
        info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        info.enabledLayerCount = validation ? 1 : 0;
        info.ppEnabledLayerNames = validation ? &layer : nullptr;
        info.pNext = validation ? &debug_info : nullptr;
        check(vkCreateInstance(&info, nullptr, &instance), "create instance");
        presentation->instance=instance;
        if (validation) {
            auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
            if (!create) throw std::runtime_error("debug utils extension unavailable");
            check(create(instance, &debug_info, nullptr, &debug), "create debug messenger");
            presentation->debug=debug;
        }
        if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) throw std::runtime_error(SDL_GetError());
        presentation->surface=surface;
    }

    void initialize(SDL_Window* target, const Mesh& mesh, bool validation,
                    const std::function<void()> &before_presentation) {
        initialize_presentation(target,validation);
        std::uint32_t count = 0;
        check(vkEnumeratePhysicalDevices(instance, &count, nullptr), "enumerate GPUs");
        std::vector<VkPhysicalDevice> devices(count);
        check(vkEnumeratePhysicalDevices(instance, &count, devices.data()), "enumerate GPUs");
        int best = -1;
        for (const auto candidate : devices) {
            std::uint32_t queue_count;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_count, nullptr);
            std::vector<VkQueueFamilyProperties> queues(queue_count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_count, queues.data());
            std::uint32_t ext_count;
            check(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &ext_count, nullptr), "enumerate device extensions");
            std::vector<VkExtensionProperties> device_extensions(ext_count);
            check(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &ext_count, device_extensions.data()), "enumerate device extensions");
            if (std::none_of(device_extensions.begin(), device_extensions.end(), [](const auto& ext) {
                return std::strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
            })) continue;
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(candidate, &properties);
            VkImageFormatProperties depth_properties;
            if(!source_depth_supported(candidate,depth_properties))continue;
            for (std::uint32_t q = 0; q < queue_count; ++q) {
                VkBool32 present = VK_FALSE;
                check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate, q, surface, &present), "query presentation support");
                const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 100 : 10;
                constexpr auto required_queue=VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT;
                if (present && (queues[q].queueFlags & required_queue)==required_queue && score > best) {
                    best = score;
                    physical = candidate;
                    queue_family = q;
                    gpu = properties.deviceName;
                }
            }
        }
        if (!physical) throw std::runtime_error("no Vulkan GPU supports original D24S8 with graphics, compute and window presentation");
        float priority = 1;
        VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_info.queueFamilyIndex = queue_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        const char* device_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(physical, &features);
        VkPhysicalDeviceFeatures enabled{};
        enabled.textureCompressionBC = features.textureCompressionBC;
        if(!features.depthClamp)throw FormatError("original sky rasterization requires depth clamp support");
        enabled.depthClamp=VK_TRUE;
        enabled.geometryShader=features.geometryShader;
        enabled.shaderTessellationAndGeometryPointSize=features.shaderTessellationAndGeometryPointSize;
        enabled.largePoints=features.largePoints;
        enabled.wideLines=features.wideLines;
        weather_wide_lines=enabled.wideLines;
        weather_large_points=enabled.largePoints;
        enabled.samplerAnisotropy = features.samplerAnisotropy;
        sampler_anisotropy = features.samplerAnisotropy;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical,&properties);
        sampler_limits=properties.limits;
        source_world_device.physical_api_version=properties.apiVersion;
        source_world_device.geometry_shader_enabled=enabled.geometryShader;
        source_world_device.geometry_point_size_enabled=enabled.shaderTessellationAndGeometryPointSize;
        source_world_device.depth_clamp_enabled=enabled.depthClamp;
        source_world_device.anisotropy_enabled=enabled.samplerAnisotropy;
        source_world_device.maximum_anisotropy=properties.limits.maxSamplerAnisotropy;
        source_world_device.maximum_lod_bias=properties.limits.maxSamplerLodBias;
        VkPhysicalDeviceVulkan13Features world_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        if(source_world_device.instance_api_version>=VK_API_VERSION_1_3 && properties.apiVersion>=VK_API_VERSION_1_3) {
            VkPhysicalDeviceVulkan13Features available{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
            VkPhysicalDeviceFeatures2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
            query.pNext=&available;
            vkGetPhysicalDeviceFeatures2(physical,&query);
            world_features.shaderDemoteToHelperInvocation=available.shaderDemoteToHelperInvocation;
            source_world_device.demote_to_helper_enabled=world_features.shaderDemoteToHelperInvocation;
        }
        std::uint32_t device_extension_count{};
        check(vkEnumerateDeviceExtensionProperties(physical,nullptr,&device_extension_count,nullptr),"query sampler extensions");
        std::vector<VkExtensionProperties> available_extensions(device_extension_count);
        check(vkEnumerateDeviceExtensionProperties(physical,nullptr,&device_extension_count,available_extensions.data()),"read sampler extensions");
        std::vector<const char*> enabled_extensions{device_extension};
        if(std::any_of(available_extensions.begin(),available_extensions.end(),[](const auto &e){
            return std::strcmp(e.extensionName,VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME)==0;})) {
            sampler_mirror_clamp=true;
            enabled_extensions.push_back(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
        }
        VkPhysicalDeviceCustomBorderColorFeaturesEXT border_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT};
        if(std::any_of(available_extensions.begin(),available_extensions.end(),[](const auto &e){
            return std::strcmp(e.extensionName,VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME)==0;})) {
            VkPhysicalDeviceFeatures2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
            query.pNext=&border_features;vkGetPhysicalDeviceFeatures2(physical,&query);
            if(border_features.customBorderColors) {
                custom_border_color=true;
                border_features.customBorderColorWithoutFormat=VK_FALSE;
                enabled_extensions.push_back(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
                device_info.pNext=&border_features;
                source_world_device.custom_border_colors_enabled=true;
            }
        }
        VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT cube_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT};
        if(std::any_of(available_extensions.begin(),available_extensions.end(),[](const auto &e){
            return std::strcmp(e.extensionName,VK_EXT_NON_SEAMLESS_CUBE_MAP_EXTENSION_NAME)==0;})) {
            VkPhysicalDeviceFeatures2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
            query.pNext=&cube_features;vkGetPhysicalDeviceFeatures2(physical,&query);
            if(cube_features.nonSeamlessCubeMap) {
                sampler_non_seamless_cube=true;
                enabled_extensions.push_back(VK_EXT_NON_SEAMLESS_CUBE_MAP_EXTENSION_NAME);
                cube_features.pNext=const_cast<void*>(device_info.pNext);
                device_info.pNext=&cube_features;
            }
        }
        if(world_features.shaderDemoteToHelperInvocation) {
            world_features.pNext=const_cast<void*>(device_info.pNext);
            device_info.pNext=&world_features;
        }
        device_info.pEnabledFeatures = &enabled;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = std::uint32_t(enabled_extensions.size());
        device_info.ppEnabledExtensionNames = enabled_extensions.data();
        check(vkCreateDevice(physical, &device_info, nullptr, &device), "create device");
        vkGetDeviceQueue(device, queue_family, 0, &queue);
        depth_format=source_depth_format;
        VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = queue_family;
        check(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool), "create command pool");
        VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocate.commandPool = command_pool;
        allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate.commandBufferCount = 1;
        check(vkAllocateCommandBuffers(device, &allocate, &command), "allocate command buffer");
        VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        check(vkCreateSemaphore(device, &semaphore_info, nullptr, &acquired), "create acquire semaphore");
        VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        check(vkCreateFence(device, &fence_info, nullptr, &fence), "create fence");
        VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)+3*sizeof(glm::vec4)};
        VkDescriptorSetLayoutBinding texture_bindings[8]{};
        for(unsigned i=0;i<4;++i) texture_bindings[i]={i,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        texture_bindings[4]={4,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        texture_bindings[5]={5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        texture_bindings[6]={6,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        texture_bindings[7]={7,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo texture_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        texture_info.bindingCount = 8;
        texture_info.pBindings = texture_bindings;
        check(vkCreateDescriptorSetLayout(device, &texture_info, nullptr, &texture_layout), "create texture descriptor layout");
        VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &texture_layout;
        check(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout), "create pipeline layout");
        VkPushConstantRange frame_push{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(ShaderVector)};
        layout_info.pPushConstantRanges=&frame_push;
        check(vkCreatePipelineLayout(device,&layout_info,nullptr,&ui_frame_layout),"create frame sampling layout");
        VkPushConstantRange loading_push{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,32};
        layout_info.pPushConstantRanges=&loading_push;
        check(vkCreatePipelineLayout(device,&layout_info,nullptr,&vista_loading_layout),"create Vista loading layout");
        push = {VK_SHADER_STAGE_FRAGMENT_BIT, 0, 32};
        layout_info.pPushConstantRanges=&push;
        layout_info.setLayoutCount = 0;
        layout_info.pSetLayouts = nullptr;
        check(vkCreatePipelineLayout(device, &layout_info, nullptr, &overlay_layout), "create overlay layout");
        vertex_count = static_cast<std::uint32_t>(mesh.vertices.size());
        // Preserve authored parts as well as their material/lightmap bindings.
        // A shader shared by multiple parts does not make them one draw token.
        std::map<DrawKey,std::size_t> draw_indices;
        for (const auto& vertex : mesh.vertices) {
            if (vertex.material >= mesh.materials.size() || vertex.lightmap > mesh.lightmaps.size()) throw std::runtime_error("invalid native material/lightmap index");
            const auto key=draw_key(vertex);
            auto [it,inserted]=draw_indices.emplace(key,draws.size());
            if(inserted) draws.push_back({0,0,vertex.material,vertex.lightmap,{},vertex.bsp_cluster,
                vertex.source_section,vertex.source_part});
            ++draws[it->second].count;
        }
        std::uint32_t first = 0;
        for (auto& draw : draws) { draw.first = first; first += draw.count; }
        std::vector<std::uint32_t> cursor;
        for (const auto& draw : draws) cursor.push_back(draw.first);
        std::vector<Vertex> grouped(mesh.vertices.size());
        for (const auto& vertex : mesh.vertices) grouped[cursor[draw_indices.at(draw_key(vertex))]++] = vertex;
        if(bsp_resident_plan) {
            const auto &bank=bsp_resident_plan->resident_vertices();
            if(bank.size()!=grouped.size() ||
               (!bank.empty() && std::memcmp(bank.data(),grouped.data(),bank.size()*sizeof(Vertex))!=0))
                throw FormatError("source BSP resident atlas differs from exact renderer vertex bank");
            const auto &parts=bsp_resident_plan->parts();
            if(parts.size()!=draws.size())throw FormatError("source BSP resident catalogue differs from renderer draws");
            for(std::size_t i=0;i<parts.size();++i) {
                const auto &p=parts[i];const auto &d=draws[i];
                if(p.first!=d.first || p.count!=d.count ||
                   DrawKey{p.original.material,p.original.lightmap,p.original.geometry,p.original.section,p.original.part}!=
                     DrawKey{d.material,d.lightmap,d.cluster,d.source_section,d.source_part})
                    throw FormatError("source BSP resident physical range differs from renderer draw");
            }
        }
        const VkDeviceSize bytes = std::max<std::size_t>(1,grouped.size()) * sizeof(Vertex);
        make_buffer(bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_buffer, vertex_memory);
        void* mapped;
        check(vkMapMemory(device, vertex_memory, 0, bytes, 0, &mapped), "map vertices");
        if(!grouped.empty())std::memcpy(mapped,grouped.data(),grouped.size()*sizeof(Vertex));
        vkUnmapMemory(device, vertex_memory);
        make_buffer(debug_capacity * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, debug_buffer, debug_memory);
        make_textures(mesh);
        make_fog_bindings();
        make_water_bindings();
        if(before_presentation)before_presentation();
        create_swapchain();
    }

    void make_fog_bindings() {
        VkDescriptorSetLayoutBinding bindings[]{
            {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout.bindingCount=3;layout.pBindings=bindings;
        check(vkCreateDescriptorSetLayout(device,&layout,nullptr,&fog_texture_layout),"create fog descriptor layout");
        VkPipelineLayoutCreateInfo pipeline_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipeline_info.setLayoutCount=1;pipeline_info.pSetLayouts=&fog_texture_layout;
        check(vkCreatePipelineLayout(device,&pipeline_info,nullptr,&fog_layout),"create fog pipeline layout");
        VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,10},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,5}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets=5;pool.poolSizeCount=2;pool.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(device,&pool,nullptr,&fog_pool),"create fog descriptor pool");
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool=fog_pool;allocation.descriptorSetCount=1;allocation.pSetLayouts=&fog_texture_layout;
        check(vkAllocateDescriptorSets(device,&allocation,&fog_descriptor),"allocate fog descriptors");
        make_buffer(272,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,fog_buffer,fog_memory);
        VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler_info.magFilter=sampler_info.minFilter=VK_FILTER_NEAREST;
        sampler_info.addressModeU=sampler_info.addressModeV=sampler_info.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        check(vkCreateSampler(device,&sampler_info,nullptr,&fog_depth_sampler),"create fog depth sampler");
        sampler_info.magFilter=sampler_info.minFilter=VK_FILTER_LINEAR;
        check(vkCreateSampler(device,&sampler_info,nullptr,&fog_gradient_sampler),"create fog gradient sampler");
        VkDescriptorBufferInfo buffer{fog_buffer,0,272};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet=fog_descriptor;write.dstBinding=2;write.descriptorCount=1;
        write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&buffer;
        vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        for(unsigned i=0;i<4;++i) {
            check(vkAllocateDescriptorSets(device,&allocation,&view_fog_descriptors[i]),"allocate viewport fog descriptors");
            make_buffer(272,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,view_fog_buffers[i],view_fog_memories[i]);
            const VkDescriptorBufferInfo view_buffer{view_fog_buffers[i],0,272};
            write.dstSet=view_fog_descriptors[i];write.pBufferInfo=&view_buffer;
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
        make_patchy_bindings();
        make_bloom_bindings();
    }

    void make_water_bindings() {
        VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,8},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,4}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets=4;pool.poolSizeCount=2;pool.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(device,&pool,nullptr,&water_copy_pool),"create water copy descriptor pool");
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool=water_copy_pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&fog_texture_layout;
        for(unsigned i=0;i<4;++i) {
            check(vkAllocateDescriptorSets(device,&allocate,&water_copy_descriptors[i]),"allocate water copy descriptors");
            make_buffer(272,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,water_copy_buffers[i],water_copy_memories[i]);
            const VkDescriptorBufferInfo buffer{water_copy_buffers[i],0,272};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet=water_copy_descriptors[i];write.dstBinding=2;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&buffer;
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
    }

    void make_bloom_bindings() {
        VkDescriptorSetLayoutBinding bindings[]{
            {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout.bindingCount=2;layout.pBindings=bindings;
        check(vkCreateDescriptorSetLayout(device,&layout,nullptr,&bloom_texture_layout),"create bloom descriptor layout");
        VkPipelineLayoutCreateInfo pi{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pi.setLayoutCount=1;pi.pSetLayouts=&bloom_texture_layout;
        check(vkCreatePipelineLayout(device,&pi,nullptr,&bloom_layout),"create bloom pipeline layout");
        VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,postprocess_count},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,postprocess_count}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=postprocess_count;pool.poolSizeCount=2;pool.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(device,&pool,nullptr,&bloom_pool),"create bloom descriptor pool");
        std::array<VkDescriptorSetLayout,postprocess_count> layouts;layouts.fill(bloom_texture_layout);
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool=bloom_pool;allocation.descriptorSetCount=postprocess_count;allocation.pSetLayouts=layouts.data();
        check(vkAllocateDescriptorSets(device,&allocation,bloom_descriptors.data()),"allocate bloom descriptors");
        for(unsigned i=0;i<bloom_buffers.size();++i) {
            make_buffer(272,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,bloom_buffers[i],bloom_memories[i]);
            const VkDescriptorBufferInfo buffer{bloom_buffers[i],0,272};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet=bloom_descriptors[i];write.dstBinding=1;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&buffer;
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
    }
    void make_post_image(GpuTexture &target,VkExtent2D size,VkFormat image_format,VkImageUsageFlags usage) {
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType=VK_IMAGE_TYPE_2D;image.format=image_format;image.extent={size.width,size.height,1};
        image.mipLevels=image.arrayLayers=1;image.samples=VK_SAMPLE_COUNT_1_BIT;image.tiling=VK_IMAGE_TILING_OPTIMAL;
        image.usage=usage|VK_IMAGE_USAGE_SAMPLED_BIT;
        check(vkCreateImage(device,&image,nullptr,&target.image),"create bloom image");
        VkMemoryRequirements requirements;vkGetImageMemoryRequirements(device,target.image,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&target.memory),"allocate bloom image");
        check(vkBindImageMemory(device,target.image,target.memory,0),"bind bloom image");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image=target.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=image_format;
        view.subresourceRange={usage&VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT?VkImageAspectFlags(VK_IMAGE_ASPECT_DEPTH_BIT):VkImageAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT),0,1,0,1};
        if((usage&VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) && image_format==source_depth_format)
            view.subresourceRange.aspectMask|=VK_IMAGE_ASPECT_STENCIL_BIT;
        check(vkCreateImageView(device,&view,nullptr,&target.view),"create bloom image view");
    }

    void make_patchy_bindings() {
        std::array<VkDescriptorSetLayoutBinding,5> bindings{};
        for(unsigned i=0;i<4;++i)bindings[i]={i,i==3?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        bindings[4]={4,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout.bindingCount=bindings.size();layout.pBindings=bindings.data();
        check(vkCreateDescriptorSetLayout(device,&layout,nullptr,&patchy_texture_layout),"create patchy descriptor layout");
        VkPipelineLayoutCreateInfo pi{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pi.setLayoutCount=1;pi.pSetLayouts=&patchy_texture_layout;
        check(vkCreatePipelineLayout(device,&pi,nullptr,&patchy_layout),"create patchy pipeline layout");
        VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,6},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,4}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets=2;pool.poolSizeCount=2;pool.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(device,&pool,nullptr,&patchy_pool),"create patchy descriptor pool");
        const VkDescriptorSetLayout layouts[]{patchy_texture_layout,patchy_texture_layout};
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool=patchy_pool;allocation.descriptorSetCount=2;allocation.pSetLayouts=layouts;
        check(vkAllocateDescriptorSets(device,&allocation,patchy_descriptors.data()),"allocate patchy descriptors");
        for(unsigned i=0;i<2;++i) {
            make_buffer(sizeof(PatchyFogUniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,patchy_buffers[i],patchy_memories[i]);
            VkDescriptorBufferInfo buffer{patchy_buffers[i],0,sizeof(PatchyFogUniform)};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet=patchy_descriptors[i];write.dstBinding=3;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&buffer;
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
        for(unsigned i=0;i<2;++i) {
            make_buffer(sizeof(PatchyFogQuad),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,patchy_quad_buffers[i],patchy_quad_memories[i]);
            VkDescriptorBufferInfo quad_buffer{patchy_quad_buffers[i],0,sizeof(PatchyFogQuad)};
            VkWriteDescriptorSet quad_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            quad_write.dstSet=patchy_descriptors[i];quad_write.dstBinding=4;quad_write.descriptorCount=1;
            quad_write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;quad_write.pBufferInfo=&quad_buffer;
            vkUpdateDescriptorSets(device,1,&quad_write,0,nullptr);
        }
        VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler_info.magFilter=sampler_info.minFilter=VK_FILTER_LINEAR;
        // Original Vulkan draw 8005 binds all four noise stages with
        // minLOD=maxLOD=0. The bitmap has a mip chain, but the title pass
        // explicitly samples its top level even when the slices are minified.
        sampler_info.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;sampler_info.maxLod=0;
        sampler_info.addressModeU=sampler_info.addressModeV=sampler_info.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;
        check(vkCreateSampler(device,&sampler_info,nullptr,&patchy_sampler),"create patchy noise sampler");
    }
    void patchy_image(unsigned descriptor,unsigned binding,VkImageView view,VkSampler sampler) {
        const VkDescriptorImageInfo image{sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet=patchy_descriptors[descriptor];write.dstBinding=binding;write.descriptorCount=1;
        write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
        vkUpdateDescriptorSets(device,1,&write,0,nullptr);
    }

    std::vector<LightVolumeDraw> prepare_light_volumes(const Camera &camera,RenderViewport viewport,
                                                     Vec2 extension) {
        if(light_volumes.empty())return {};
        if(!light_volume_tables)throw FormatError("missing light-volume curve tables");
        const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
        const auto up=camera.oriented?camera.up:Vec3{0,0,1};
        auto lens=menu_projection_lens(camera.field_of_view);
        lens.scale={lens.scale.x*extension.x,lens.scale.y*extension.y};
        const auto projection=light_volume_title_projection(camera.position,forward,up,lens.scale,viewport);
        const LightVolumeView view{camera.position,projection.world_to_view,
            {lens.scale.x*viewport.width*.5f,lens.scale.y*viewport.height*.5f}};
        std::vector<const LightVolumeInstance*> ordered;
        for(const auto &instance:light_volumes)ordered.push_back(&instance);
        const auto depth=[&](const auto *instance) {
            const auto delta=instance->state.marker.position-camera.position;
            return 0.f-((forward.z*delta.z+forward.y*delta.y)+forward.x*delta.x);
        };
        // 42850's negative camera depth and 1E700's non-plane comparison.
        std::stable_sort(ordered.begin(),ordered.end(),[&](auto *a,auto *b){return depth(a)<depth(b);});
        auto &vertices=light_volume_staging;
        std::vector<LightVolumeDraw> draws;
        for(const auto *instance:ordered) {
            if(!instance->definition || instance->material>=material_count)
                throw FormatError("invalid title light-volume binding");
            const auto batches=light_volume_sprites(*instance->definition,instance->state,view,*light_volume_tables);
            for(const auto &batch:batches) {
                if(batch.fuzzy || batch.bitmap!=0xffffffffu)
                    throw FormatError("unsupported title light-volume texture/fuzzy path");
                LightVolumeDraw draw{std::uint32_t(vertices.size()),0,instance->material};
                draw.sort_depth=depth(instance);
                for(const auto &sprite:batch.sprites)
                    if(const auto quad=light_volume_quad(sprite,batch.cosine,batch.sine,projection))
                        for(const auto index:{0u,1u,2u,0u,2u,3u})vertices.push_back((*quad)[index]);
                draw.count=std::uint32_t(vertices.size())-draw.first;
                if(draw.count)draws.push_back(draw);
            }
        }
        return draws;
    }
    void upload_light_volumes() {
        const auto &vertices=light_volume_staging;
        if(vertices.empty())return;
        if(vertices.size()>dynamic_limit)throw FormatError("title light-volume geometry exceeds native limit");
        if(vertices.size()>light_volume_capacity) {
            if(light_volume_buffer)vkDestroyBuffer(device,light_volume_buffer,nullptr);
            if(light_volume_memory)vkFreeMemory(device,light_volume_memory,nullptr);
            light_volume_buffer=VK_NULL_HANDLE;light_volume_memory=VK_NULL_HANDLE;
            light_volume_capacity=vertices.size();
            make_buffer(light_volume_capacity*sizeof(LightVolumeVertex),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                light_volume_buffer,light_volume_memory);
        }
        void *mapped{};
        check(vkMapMemory(device,light_volume_memory,0,vertices.size()*sizeof(LightVolumeVertex),0,&mapped),"map title light volumes");
        std::memcpy(mapped,vertices.data(),vertices.size()*sizeof(LightVolumeVertex));
        vkUnmapMemory(device,light_volume_memory);
    }

    void set_atmosphere(MenuAtmosphere fog,BspAtmosphere source) {
        if(atmosphere)throw FormatError("title atmosphere is already bound");
        upload_texture(fog.gradient,fog_gradient);
        const VkDescriptorImageInfo image{fog_gradient_sampler,fog_gradient.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet=fog_descriptor;write.dstBinding=1;write.descriptorCount=1;
        write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
        vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        if(fog.patchy) {
            upload_texture(fog.patchy->noise.at(0),patchy_bitmap);
            patchy_image(0,2,patchy_bitmap.view,patchy_sampler);
            patchy_image(1,1,fog_gradient.view,fog_gradient_sampler);
            // 146240 mixes wall time, CRT random state and platform ticks.
            // Native startup has its own unsynchronized random stream; exact
            // original noise placement requires replaying the same RNG state.
            const auto crt_random=((214013u+2531011u)>>16)&32767u;
            patchy_random=std::uint32_t(std::time(nullptr))^crt_random^SDL_GetTicks();
            // Optional native screenshot diagnostic. Normal startup retains
            // the original seed derivation; comparison runs can pin it.
            if(const auto* seed=std::getenv("HALO2_MENU_FOG_SEED")) {
                const auto* end=seed+std::strlen(seed);
                const auto parsed=std::from_chars(seed,end,patchy_random);
                if(parsed.ec!=std::errc{} || parsed.ptr!=end)
                    throw FormatError("HALO2_MENU_FOG_SEED must be a uint32 decimal value");
            }
            std::cout<<"Retail patchy-fog random seed: "<<patchy_random<<'\n';
        }
        atmosphere=std::move(fog);
        title_atmosphere_source=std::move(source);
    }

    void set_bsp_atmosphere(const BspAtmosphere &fog) {
        if(atmosphere || bsp_atmosphere)throw FormatError("scene atmosphere is already bound");
        upload_texture(fog.gradient,fog_gradient);
        const VkDescriptorImageInfo image{fog_gradient_sampler,fog_gradient.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstBinding=1;write.descriptorCount=1;
        write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
        for(const auto descriptor:view_fog_descriptors) {
            write.dstSet=descriptor;vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
        bsp_atmosphere=true;
        bsp_bank=&fog;
        make_bsp_postprocess();
    }

    void make_bsp_postprocess() {
        if(!bsp_bank)return;
        if(!can_capture)throw FormatError("BSP postprocessing requires readable color targets");
        BspPostprocessContext context{device,physical,extent,format,depth_format,
            fog_pass,continuation_pass,depth,depth_view,shader_directory};
        context.scene_layout=scene_layout;
        std::size_t texture_index=0;
        bsp_postprocess=std::make_unique<BspPostprocess>(context,*bsp_bank,[&](const Texture &source) {
            if(texture_index==bsp_postprocess_textures.size()) {
                bsp_postprocess_textures.emplace_back();
                upload_texture(source,bsp_postprocess_textures.back());
            }
            return bsp_postprocess_textures.at(texture_index++).view;
        });
        if(bsp_postprocess_state)bsp_postprocess->restore_state(*bsp_postprocess_state);
    }

    void release_texture_staging() {
        for(const auto &staging:texture_staging) {
            vkDestroyBuffer(device,staging.buffer,nullptr);
            vkFreeMemory(device,staging.memory,nullptr);
        }
        texture_staging.clear();
        texture_staging_bytes=0;
    }

    void flush_texture_uploads() {
        if(texture_staging.empty())return;
        check(vkEndCommandBuffer(command), "end texture upload");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount=1;
        submit.pCommandBuffers=&command;
        check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE), "submit texture upload");
        check(vkQueueWaitIdle(queue), "wait texture upload");
        release_texture_staging();
    }

    void upload_texture(const Texture& source, GpuTexture& texture) {
        // Keep the original copies, mip/layer ranges and layout barriers, but
        // retire startup transfers together instead of stalling per image.
        // Bound both payload bytes and allocations (including tiny fallbacks).
        constexpr VkDeviceSize staging_limit=32u*1024u*1024u;
        if(!texture_staging.empty() && (texture_staging.size()>=128 ||
            source.pixels.size()>staging_limit-std::min(staging_limit,texture_staging_bytes)))
            flush_texture_uploads();
        texture.source_shape={source.width,source.height,source.format,{},{}};
        texture.source_shape.layers=source.layers;
        texture.source_shape.depth=source.depth;
        texture.source_image=!source.pixels.empty();
        if (source.layers != 1 && source.layers != 6) throw std::runtime_error("invalid texture layer count");
        if(source.depth && source.layers!=1)throw FormatError("volume texture cannot have cube layers");
        if (source.layers == 6 && (source.width != source.height || source.mips.empty() || source.mips.size()%6))
            throw std::runtime_error("invalid cube texture mip layout");
        const auto mip_count = source.mips.empty() ? 1u : static_cast<std::uint32_t>(source.mips.size()/source.layers);
        texture.width=source.width; texture.height=source.height;
        texture.depth=source.depth;
        texture.format=source.format; texture.mip_count=mip_count;
        VkFormat texture_format = VK_FORMAT_R8G8B8A8_UNORM;
        if (source.format == native_rgba32f) texture_format = VK_FORMAT_R32G32B32A32_SFLOAT;
        if (source.format == 14) texture_format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        if (source.format == 15) texture_format = VK_FORMAT_BC2_UNORM_BLOCK;
        if (source.format == 16) texture_format = VK_FORMAT_BC3_UNORM_BLOCK;
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical, texture_format, &properties);
        if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ||
            !(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            throw std::runtime_error("GPU does not support required diffuse texture format");
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType = source.depth?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D;
        image.flags = source.layers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        image.format = texture_format;
        image.extent = {source.width, source.height, source.depth?source.depth:1};
        image.mipLevels = mip_count;
        image.arrayLayers = source.layers;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateImage(device, &image, nullptr, &texture.image), "create diffuse image");
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device, texture.image, &requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device, &allocation, nullptr, &texture.memory), "allocate diffuse memory");
        check(vkBindImageMemory(device, texture.image, texture.memory, 0), "bind diffuse memory");
        VkBuffer staging{};
        VkDeviceMemory staging_memory{};
        struct Staging { VkDevice d; VkBuffer& b; VkDeviceMemory& m;
            ~Staging() { if (b) { vkDeviceWaitIdle(d); vkDestroyBuffer(d, b, nullptr); } if (m) vkFreeMemory(d, m, nullptr); }
        } staging_guard{device, staging, staging_memory};
        make_buffer(source.pixels.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging, staging_memory);
        void* mapped;
        check(vkMapMemory(device, staging_memory, 0, source.pixels.size(), 0, &mapped), "map diffuse upload");
        std::memcpy(mapped, source.pixels.data(), source.pixels.size());
        vkUnmapMemory(device, staging_memory);
        if(texture_staging.empty()) {
            check(vkResetCommandBuffer(command, 0), "reset texture commands");
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            check(vkBeginCommandBuffer(command, &begin), "begin texture upload");
        }
        const auto upload_buffer=staging;
        texture_staging.push_back({staging,staging_memory});
        texture_staging_bytes+=source.pixels.size();
        staging=VK_NULL_HANDLE;
        staging_memory=VK_NULL_HANDLE;
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_count, 0, source.layers};
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        std::vector<VkBufferImageCopy> copies(mip_count*source.layers);
        for (std::uint32_t layer = 0; layer < source.layers; ++layer) for (std::uint32_t level = 0; level < mip_count; ++level) {
            const auto index=layer*mip_count+level;
            auto& copy = copies[index];
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, layer, 1};
            copy.bufferOffset = source.mips.empty() ? 0 : source.mips[index].offset;
            copy.imageExtent = {source.mips.empty() ? source.width : source.mips[index].width,
                                source.mips.empty() ? source.height : source.mips[index].height,
                                source.depth?(source.mips.empty()?source.depth:source.mips[index].depth):1};
        }
        vkCmdCopyBufferToImage(command, upload_buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<unsigned>(copies.size()), copies.data());
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        if(!batch_texture_uploads)flush_texture_uploads();
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = texture.image;
        view.viewType = source.depth?VK_IMAGE_VIEW_TYPE_3D:VK_IMAGE_VIEW_TYPE_2D;
        view.format = texture_format;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_count, 0, 1};
        check(vkCreateImageView(device, &view, nullptr, &texture.view), "create diffuse view");
        if (source.layers == 6) {
            view.viewType=VK_IMAGE_VIEW_TYPE_CUBE;
            view.subresourceRange.layerCount=6;
            check(vkCreateImageView(device,&view,nullptr,&texture.cube_view),"create cube view");
        }
    }

    void update_image(unsigned material, const Texture& source) {

        if(material>=textures.size()) throw std::runtime_error("movie material outside renderer");
        auto& texture=textures[material];
        if(source.format!=11 || texture.format!=11 || texture.mip_count!=1 ||
           source.depth || texture.depth || source.layers!=1 || texture.source_shape.layers!=1 ||
           !source.width || !source.height ||
           !source.mips.empty() || source.pixels.size()!=std::size_t(source.width)*source.height*4)
            throw std::runtime_error("invalid dynamic image update");
        if(texture.shared_image || source.width!=texture.width || source.height!=texture.height) {
            // A mutable material image becomes private before upload. Other
            // hands/views and animated frames retain the resident artwork.
            check(vkQueueWaitIdle(queue),"retire shared material image before update");
            auto previous=std::move(texture.shared_image);
            if(!previous) {
                previous=std::make_shared<SharedImage>();
                previous->device=device;previous->image=texture.image;previous->memory=texture.memory;
                previous->view=texture.view;previous->cube_view=texture.cube_view;
            }
            // A queued frame for the old extent must not overwrite the new one.
            std::erase(pending_images,material);
            dynamic_images.erase(material);
            const auto descriptor=texture.descriptor;
            texture={};upload_texture(source,texture);texture.descriptor=descriptor;
            VkDescriptorImageInfo image{stage_samplers.at(material)[0],texture.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet=descriptor;write.dstBinding=0;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
            return;
        }
        const auto *serial=std::getenv("HALO2_REFERENCE_SERIAL_IMAGE_UPLOADS");
        if(!serial || !*serial) {
            auto &update=dynamic_images[material];
            if(!update.pending) {pending_images.push_back(material);update.pending=true;}
            update.pixels=source.pixels;
            return;
        }
        check(vkQueueWaitIdle(queue), "wait before movie upload");
        VkBuffer staging{}; VkDeviceMemory memory{};
        struct Staging { VkDevice device; VkBuffer& buffer; VkDeviceMemory& memory;
            ~Staging(){if(buffer) {vkDeviceWaitIdle(device);vkDestroyBuffer(device,buffer,nullptr);} if(memory) vkFreeMemory(device,memory,nullptr);}
        } guard{device,staging,memory};
        make_buffer(source.pixels.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT,staging,memory);
        void* pixels;
        check(vkMapMemory(device,memory,0,source.pixels.size(),0,&pixels),"map movie frame");
        std::memcpy(pixels,source.pixels.data(),source.pixels.size());
        vkUnmapMemory(device,memory);
        check(vkResetCommandBuffer(command,0),"reset movie upload");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command,&begin),"begin movie upload");
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=texture.image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        copy.imageExtent={source.width,source.height,1};
        vkCmdCopyBufferToImage(command,staging,texture.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        check(vkEndCommandBuffer(command),"end movie upload");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE),"submit movie frame");
        check(vkQueueWaitIdle(queue),"wait for movie frame");
    }

    void record_dynamic_images() {
        if(pending_images.empty())return;
        VkDeviceSize bytes{};
        for(const auto material:pending_images)bytes+=dynamic_images.at(material).pixels.size();
        // draw already waited for its previous frame fence, so its retained
        // transfer storage is no longer in use by the GPU.
        if(bytes>dynamic_image_capacity) {
            if(dynamic_image_buffer)vkDestroyBuffer(device,dynamic_image_buffer,nullptr);
            if(dynamic_image_memory)vkFreeMemory(device,dynamic_image_memory,nullptr);
            dynamic_image_buffer={};dynamic_image_memory={};dynamic_image_capacity=0;
            make_buffer(bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,dynamic_image_buffer,dynamic_image_memory);
            dynamic_image_capacity=bytes;
        }
        void *mapped{};
        check(vkMapMemory(device,dynamic_image_memory,0,bytes,0,&mapped),"map dynamic image updates");
        std::vector<VkImageMemoryBarrier> barriers;
        std::vector<VkBufferImageCopy> copies;
        barriers.reserve(pending_images.size());copies.reserve(pending_images.size());
        VkDeviceSize offset{};
        for(const auto material:pending_images) {
            const auto &texture=textures[material];const auto &pixels=dynamic_images.at(material).pixels;
            std::memcpy(static_cast<std::byte*>(mapped)+offset,pixels.data(),pixels.size());
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=texture.image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            barriers.push_back(barrier);
            VkBufferImageCopy copy{};copy.bufferOffset=offset;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
            copy.imageExtent={texture.width,texture.height,1};copies.push_back(copy);offset+=pixels.size();
        }
        vkUnmapMemory(device,dynamic_image_memory);
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,0,nullptr,unsigned(barriers.size()),barriers.data());
        for(std::size_t i=0;i<pending_images.size();++i)
            vkCmdCopyBufferToImage(command,dynamic_image_buffer,textures[pending_images[i]].image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copies[i]);
        for(auto &barrier:barriers) {
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,0,nullptr,0,nullptr,unsigned(barriers.size()),barriers.data());
        for(const auto material:pending_images)dynamic_images.at(material).pending=false;
        pending_images.clear();
    }

    void make_textures(const Mesh& mesh) {
        // Resident source shader alternatives share the original geometry.
        // All descriptor/resource tables below are sized from the actual bank;
        // a fixed2048 root-slot cap rejects legitimate four-player maps.
        if (mesh.materials.empty() || mesh.materials.size() >= std::numeric_limits<unsigned>::max())
            throw std::runtime_error("invalid native material count: "+std::to_string(mesh.materials.size()));
        material_count=mesh.materials.size();
        std::vector<const Material*> materials;
        for(const auto &m:mesh.materials) materials.push_back(&m);
        environment_materials.resize(material_count,~0u);
        for(std::size_t i=0;i<material_count;++i) if(mesh.materials[i].environment_pass) {
            environment_materials[i]=unsigned(materials.size());
            materials.push_back(mesh.materials[i].environment_pass.get());
        }
        world_before.resize(material_count);world_after.resize(material_count);
        world_bloom.resize(material_count);
        for(std::size_t i=0;i<material_count;++i) {
            for(const auto &pass:mesh.materials[i].world_overlays_before) {
                world_before[i].push_back(unsigned(materials.size()));materials.push_back(pass.get());
            }
            for(const auto &pass:mesh.materials[i].world_overlays_after) {
                world_after[i].push_back(unsigned(materials.size()));materials.push_back(pass.get());
            }
            for(const auto &pass:mesh.materials[i].world_bloom_passes) {
                world_bloom[i].push_back(unsigned(materials.size()));materials.push_back(pass.get());
            }
        }
        planar_materials.resize(material_count,~0u);
        for(std::size_t i=0;i<material_count;++i) if(mesh.materials[i].planar_fog_pass) {
            const auto &pass=*mesh.materials[i].planar_fog_pass;
            const auto index=unsigned(materials.size());
            planar_materials[i]=index;planar_passes.emplace(index,pass.passes.at(0));
            materials.push_back(&pass);
        }
        world_planar_materials.resize(material_count,{~0u,~0u,~0u});
        for(std::size_t i=0;i<material_count;++i)for(unsigned p=0;p<3;++p)
            if(const auto &pass=mesh.materials[i].world_planar_passes[p]) {
                world_planar_materials[i][p]=unsigned(materials.size());
                materials.push_back(pass.get());
            }
        light_volume_slots.clear();
        for(const auto *material:materials) {
            light_volume_materials|=material->light_volume_pass;
            light_volume_slots.push_back(material->light_volume_pass);
        }
        source_material_layers.reserve(materials.size());
        for(const auto *material:materials)
            source_material_layers.push_back(source_material_layer(*material));
        for(const auto *material:materials)
            source_material_identities.push_back({material->source_shader_datum,material->selected_shader_lod});
        source_layer15_passes.resize(materials.size());
        for(unsigned root=0;root<materials.size();++root) {
            auto &passes=source_layer15_passes[root];
            const auto add=[&](unsigned m) {
                const auto &material=*materials[m];
                if(source_material_layers[m]!=15 || material.selected_shader_lod!=0)return;
                if(material.source_shader_datum!=materials[root]->source_shader_datum ||
                   material.forward_pass_index>=material.passes.size() ||
                   material.passes[material.forward_pass_index].implementation!=0)
                    return;
                passes.push_back(m);
            };
            add(root);
            if(root<world_before.size())for(auto m:world_before[root])add(m);
            if(root<world_after.size())for(auto m:world_after[root])add(m);
            std::sort(passes.begin(),passes.end(),[&](auto a,auto b) {
                return materials[a]->forward_pass_index<materials[b]->forward_pass_index;
            });
        }
        for(unsigned i=0;i<materials.size();++i)if(materials[i]->forward_pass &&
            !materials[i]->hud_pass && !materials[i]->light_volume_pass &&
            materials[i]->forward_vertex!=11) {
            // Projected decals have their own procedural texture/fade pass;
            // they do not own a shader-template pass or its fog extern bank.
            fog_material_passes.emplace(i,materials[i]->passes.at(materials[i]->forward_pass_index));
            if(!materials[i]->animation.overlays.empty())owner_material_animations.emplace(i,materials[i]->animation);
            fog_material_switches.emplace(i,materials[i]->shader_switches);
        }
        // These authored mappings never change when animation updates the
        // live program words. Retain their exact order once per material.
        view_direction_remaps.resize(materials.size());view_fog_remaps.resize(materials.size());
        for(const auto &[index,pass]:fog_material_passes)
            for(const auto &mapping:pass.pixel_externs) {
                const auto component=mapping.component&7;
                if(component==4 && mapping.source==0)
                    view_direction_remaps[index].push_back(mapping);
                // Same admission as apply_bsp_fog_pixel_externs; retain the
                // authored mapping order and the original remap implementation.
                const bool scalar=component<4 && (mapping.source==3 ||
                    (mapping.source>=24 && mapping.source<=36));
                const bool color=component==4 && mapping.source>=25 && mapping.source<=32;
                if(scalar || color)view_fog_remaps[index].push_back(mapping);
            }
        material_roots.resize(materials.size());
        material_lighting_brightness.resize(materials.size());
        for(unsigned i=0;i<material_count;++i) {
            material_roots[i]=i;
            if(environment_materials[i]!=~0u)material_roots[environment_materials[i]]=i;
            for(const auto m:world_before[i])material_roots[m]=i;
            for(const auto m:world_after[i])material_roots[m]=i;
            for(const auto m:world_bloom[i])material_roots[m]=i;
            if(planar_materials[i]!=~0u)material_roots[planar_materials[i]]=i;
            for(const auto m:world_planar_materials[i])if(m!=~0u)material_roots[m]=i;
        }
        for(unsigned i=0;i<materials.size();++i)
            material_lighting_brightness[i]={materials[i]->specular_brightness,materials[i]->ambient_brightness};
        VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler_info.magFilter = sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.maxLod = VK_LOD_CLAMP_NONE;
        sampler_info.addressModeU = sampler_info.addressModeV = sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        check(vkCreateSampler(device, &sampler_info, nullptr, &sampler), "create material sampler");
        sampler_info.addressModeU = sampler_info.addressModeV = sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        check(vkCreateSampler(device, &sampler_info, nullptr, &lightmap_sampler), "create lightmap sampler");
        const auto count = static_cast<std::uint32_t>(materials.size());
        std::set<std::pair<unsigned,unsigned>> geometry_pairs;
        const auto add_geometry_pair=[&](unsigned root,unsigned lightmap) {
            if(root>=material_count || lightmap>mesh.lightmaps.size())
                throw FormatError("resident geometry material/lightmap outside source bank");
            const auto add=[&](unsigned material) {
                if(lightmap && (!materials[material]->forward_pass || materials[material]->baked_lightmap_texture))
                    geometry_pairs.emplace(material,lightmap);
            };
            add(root);
            if(environment_materials[root]!=~0u)add(environment_materials[root]);
            for(const auto material:world_before[root])add(material);
            for(const auto material:world_after[root])add(material);
        };
        for(const auto &draw:draws)add_geometry_pair(draw.material,draw.lightmap);
        for(const auto &binding:mesh.resident_material_lightmaps)
            add_geometry_pair(binding[0],binding[1]);
        const auto scope_count=std::count_if(materials.begin(),materials.end(),[](const auto *m){return m->scope_texture;});
        const auto resident_sets=std::uint64_t(count)+geometry_pairs.size()+std::uint64_t(scope_count);
        if(resident_sets>std::numeric_limits<std::uint32_t>::max()/12u)
            throw FormatError("resident material descriptors exceed Vulkan count range");
        const auto sets = static_cast<std::uint32_t>(resident_sets);
        VkDescriptorPoolSize pool_sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sets*12},
                                          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,sets},
                                          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,sets}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = sets; pool.poolSizeCount = 3; pool.pPoolSizes = pool_sizes;
        check(vkCreateDescriptorPool(device, &pool, nullptr, &texture_pool), "create material descriptor pool");
        textures.resize(count); detail_textures.resize(count); bump_textures.resize(count);
        auxiliary_textures.resize(count);after_fog.reserve(count);
        animated_textures.resize(count);wanted_frames.resize(count);bound_frames.resize(count);
        stage_samplers.resize(count);stage_sampler_states.resize(count);
        scope_materials.resize(count);scope_descriptors.resize(count);
        lightmap_textures.resize(mesh.lightmaps.size()+1);
        const Texture white{1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}, {}};
        const Texture flat_normal{1,1,11,{std::byte{128},std::byte{128},std::byte{255},std::byte{255}}, {}};
        Texture white_cube{1,1,11,std::vector<std::byte>(24,std::byte{255}),{},6};
        for(unsigned face=0;face<6;++face) white_cube.mips.push_back({1,1,face*4,4});
        batch_texture_uploads=true;
        upload_texture(white_cube,cube_fallback);
        upload_texture(white,image_fallback);
        auto white_volume=white;white_volume.depth=1;
        upload_texture(white_volume,volume_fallback);
        upload_texture(white,lightmap_textures[0]);
        for(std::size_t i=0;i<mesh.lightmaps.size();++i) upload_texture(mesh.lightmaps[i],lightmap_textures[i+1]);
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical,&properties);
        const auto alignment=properties.limits.minUniformBufferOffsetAlignment;
        const VkDeviceSize stride=(sizeof(MaterialUniform)+alignment-1)/alignment*alignment;
        material_stride=stride;
        vertex_bank_stride=(sizeof(VertexBankUniform)+alignment-1)/alignment*alignment;
        material_uniforms.resize(count);
        material_blend_colors.resize(count);
        vertex_bank_uniforms.resize(count);
        material_pending.resize(count);vertex_bank_pending.resize(count);
        extended_vertex_banks.resize(count);
        effect_light_materials.resize(count);
        material_forward_states.resize(count);
        accumulation_materials.resize(count);
        material_fragment_offsets.resize(count);
        baked_materials.resize(count);
        water_material_definitions.resize(count);water_materials.resize(count);water_texture_bindings.resize(count);
        // Bank zero belongs to the main scene/UI. Each local player's bank
        // retains its own original fog constants and planar pass program.
        make_buffer(stride*count*5,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,material_buffer,material_memory);
        make_buffer(vertex_bank_stride*count,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,vertex_bank_buffer,vertex_bank_memory);
        void* mapped;
        void* bank_mapped;
        // This fixed-size HOST_COHERENT allocation remains mapped. Writes still
        // occur only after the frame fence; mapping is not synchronization.
        check(vkMapMemory(device,material_memory,0,VK_WHOLE_SIZE,0,&material_mapping),"map material constants");
        mapped=material_mapping;
        check(vkMapMemory(device,vertex_bank_memory,0,vertex_bank_stride*count,0,&bank_mapped),"map draw vertex globals");
        for(std::size_t i=0;i<count;++i) {
            const auto& material=*materials[i];
            material_blend_colors[i]=material.forward_blend_color;
            material_forward_states[i]=material.forward_raster;
            accumulation_materials[i]=material.accumulation_texture;
            if(material.forward_blend==10) {
                source_effect_light_draw(unsigned(i),material,{});
                effect_light_materials[i]=true;
            }
            scope_materials[i]=material.scope_texture;
            water_material_definitions[i]=material.water_definition;
            water_materials[i]=material.water_pass && material.forward_vertex>=7;
            water_texture_bindings[i]={material.water_reflection_texture,material.water_refraction_texture};
            baked_materials[i]=!material.forward_pass || material.baked_lightmap_texture;
            if(material.hud_pass && (material.forward_blend<1 || material.forward_blend>4))
                throw FormatError("unsupported authored HUD blend");
            hud_materials.push_back(material.hud_pass?material.forward_blend:0);
            ui_emblem_materials.push_back(material.ui_emblem_pass);
            ui_full_surfaces.push_back(material.ui_full_surface);
            if(material.vista_loading && (!material.ui_full_surface || material.forward_pass || material.hud_pass ||
               material.vista_loading->shader>3 || unsigned(material.vista_loading->blend)>2))
                throw FormatError("invalid explicit Vista loading material");
            vista_loading_materials.push_back(material.vista_loading);
            ui_widescreen_backgrounds.push_back(material.ui_widescreen_background);
            ui_frame_extensions.push_back(material.ui_frame_extension);
            ui_frame_widths.push_back(material.ui_frame_center?material.diffuse.width:0);
            MaterialUniform uniform{material.forward_pass?material.forward_program.words:material.lightmap_program.words,
                                    material.texture_transforms,
                                    {material.static_lightmap_pass?1u:0u,material.forward_pass?1u:0u,
                                     material.per_vertex_lighting?4u:material.forward_vertex,0},
                                    material.forward_transforms,material.forward_diffuse,material.forward_specular};
            uniform.alpha_test={material.forward_pass?material.forward_raster.alpha_test:source_output_state[1],
                material.forward_pass?material.forward_raster.alpha_function:source_output_state[2],
                material.forward_pass?material.forward_raster.alpha_reference:source_output_state[3],0};
            for(unsigned stage=0;stage<4;++stage) {
                const auto &state=material.texture_states[stage];
                const Texture *stage_textures[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
                if(state.color_key_op || (state.color_sign && stage_textures[stage]->format!=native_rgba32f))
                    throw FormatError("authored texture state requires native texel decoding");
                // Bit zero selects pixel coordinates; bits 1..4 preserve
                // which channels have been signed before texture filtering.
                uniform.texture_effects[stage]={state.alpha_kill,
                    (stage==0 && material.accumulation_texture?1u:0u)|((state.color_sign>>28)<<1),stage_textures[stage]->depth?1u:0u,state.key_color};
            }
            const auto &pixel_program=material.forward_pass?material.forward_program:material.lightmap_program;
            material_fragment_offsets[i]=pixel_program.fragment_offsets;
            if((material.forward_pass || material.static_lightmap_pass) && !texture_program_supported(pixel_program))
                std::cerr << "Texture addressing fallback: " << material.name << " requires unsupported texture operations\n";
            if(texture_program_supported(pixel_program) && !material.vertex_program.empty() &&
                ((!material.forward_pass && material.static_lightmap_pass) ||
                 (material.forward_vertex>=1 && material.forward_vertex<=10) || material.forward_vertex==12)) {
                try {
                    const auto vertex=decode_vertex_program(material.vertex_program);
                    if(vertex.relative_constants &&
                       (!material.vertex_bank || material.vertex_bank->constants.empty()))
                        throw FormatError("relative vertex program has no explicit source constant bank: "+material.name);
                    uniform.vertex_instructions=vertex.instructions;
                    uniform.flags[3]=vertex.count;
                    uniform.alpha_test[3]=menu_world_vertex_default_texture_w_mask(vertex);
                    uniform.vertex_constants=material.program_constants[1];
                    uniform.vertex_attributes=material.program_constants[0];
                    uniform.vertex_declaration=pack_source_vertex_declaration(material.vertex_declaration,material.world_vertex_input_transform);
                    if(material.vertex_declaration &&
                       std::any_of(material.vertex_declaration->semantics.begin(),material.vertex_declaration->semantics.end(),
                           [](auto semantic){return semantic==19 || semantic==20;}) &&
                       (!material.vertex_bank || !material.vertex_bank->prt_inputs))
                        throw FormatError("source PRT declaration has no explicit vertex stream bank: "+material.name);
                } catch (const UnsupportedVertexProgram& error) {
                    // Retain the existing fixed vertex path for valid retail
                    // programs whose instructions are not implemented yet.
                    // Malformed lengths/markers still propagate as errors.
                    std::cerr << "Vertex program fallback: " << material.name
                              << ": " << error.what() << '\n';
                }
            }
            std::memcpy(static_cast<std::byte*>(mapped)+stride*i,&uniform,sizeof(uniform));
            material_uniforms[i]=uniform;
            const auto bank=source_vertex_bank(material);
            std::memcpy(static_cast<std::byte*>(bank_mapped)+vertex_bank_stride*i,&bank,sizeof(bank));
            vertex_bank_uniforms[i]=bank;
            extended_vertex_banks[i]=bool(material.vertex_bank);
        }
        vkUnmapMemory(device,vertex_bank_memory);
        // Action/frame constants and samplers remain per material. Identical
        // immutable image snapshots share allocations across hands, players,
        // stages and animated frames. update_image detaches mutable owners.
        std::map<std::uint64_t,std::vector<std::pair<const Texture*,GpuTexture>>> shared_images;
        const auto upload_shared_image=[&](const Texture &source,GpuTexture &target) {
            std::uint64_t hash=14695981039346656037ull;
            for(auto byte:source.pixels){hash^=std::to_integer<unsigned char>(byte);hash*=1099511628211ull;}
            auto &candidates=shared_images[hash];
            for(const auto &[other,resident]:candidates) {
                if(source.width!=other->width || source.height!=other->height || source.format!=other->format ||
                   source.layers!=other->layers || source.depth!=other->depth || source.pixels!=other->pixels ||
                   source.mips.size()!=other->mips.size())continue;
                if(!std::equal(source.mips.begin(),source.mips.end(),other->mips.begin(),[](const auto &a,const auto &b) {
                    return a.width==b.width && a.height==b.height && a.offset==b.offset && a.size==b.size && a.depth==b.depth;
                }))continue;
                target=resident;return;
            }
            upload_texture(source,target);
            target.shared_image=std::make_shared<SharedImage>();
            auto &owner=*target.shared_image;owner.device=device;owner.image=target.image;owner.memory=target.memory;
            owner.view=target.view;owner.cube_view=target.cube_view;
            candidates.emplace_back(&source,target);
        };
        for(std::size_t i=0;i<count;++i) {
            const auto& material=*materials[i];
            const auto &diffuse=material.diffuse.pixels.empty()?white:material.diffuse;
            if(!material.accumulation_texture && !material.scope_texture &&
               !material.water_reflection_texture && !material.water_refraction_texture)
                upload_shared_image(diffuse,textures[i]);
            else upload_texture(diffuse,textures[i]);
            upload_shared_image(material.detail.pixels.empty()?white:material.detail,detail_textures[i]);
            upload_shared_image(material.bump.pixels.empty()?flat_normal:material.bump,bump_textures[i]);
            upload_shared_image(material.auxiliary.pixels.empty()?white:material.auxiliary,auxiliary_textures[i]);
            textures[i].source_image=!material.diffuse.pixels.empty();
            detail_textures[i].source_image=!material.detail.pixels.empty();
            bump_textures[i].source_image=!material.bump.pixels.empty();
            auxiliary_textures[i].source_image=!material.auxiliary.pixels.empty();
            wanted_frames[i]=bound_frames[i]=material.texture_frames;
            for(unsigned stage=0;stage<4;++stage) {
                const auto &frames=material.animated_textures[stage];
                auto &images=animated_textures[i][stage];images.resize(frames.size());
                for(unsigned frame=0;frame<frames.size();++frame) upload_shared_image(frames[frame],images[frame]);
            }
            // Retail 2BCD0 composites atmospheric fog before layer 15.
            // Blending alone cannot identify that layer: the carrier's
            // additive self-illumination belongs to layer 7, before fog.
            bool late=material.forward_blend!=0;
            if(material.forward_pass && !material.lods.empty()) {
                if(material.selected_shader_lod>=material.lods.size())
                    throw FormatError("fog scheduling shader LOD outside authored template");
                const auto &lod=material.lods[material.selected_shader_lod];
                if(material.forward_pass_index>=material.passes.size())
                    throw FormatError("fog scheduling pass outside material");
                const auto pass=material.passes[material.forward_pass_index].template_pass;
                bool selected=false;
                for(unsigned layer=0;layer<lod.layers.count() && layer<32;++layer) {
                    if(!(lod.available_layers & (1u<<layer)))continue;
                    const auto index=lod.layers.first()+layer;
                    if(index>=material.layers.size())throw FormatError("fog scheduling layer outside template");
                    const auto range=material.layers[index];
                    if(pass>=range.first() && pass<range.first()+range.count()) {
                        if(selected && late!=(layer==15))
                            throw FormatError("fog scheduling pass belongs to incompatible layers");
                        late=layer==15;selected=true;
                    }
                }
            }
            after_fog.push_back(late);
            projected_decal_layers.push_back(material.forward_vertex==11?material.forward_pass_index:~0u);
            if(!material.hud_pass && !material.light_volume_pass &&
                (material.world_pass || material.forward_blend || material.accumulation_texture || material.environment_pass)) {
                const bool emblem_w_depth=material.template_name.ends_with("\\emblem_overlay_simple");
                const auto found=std::find_if(forward_pipelines.begin(),forward_pipelines.end(),
                    [&](const auto &p){return p.blend==material.forward_blend && p.state==material.forward_raster &&
                        p.extended_vertex_bank==bool(material.vertex_bank) && !p.controls && p.emblem_w_depth==emblem_w_depth;});
                const auto index=unsigned(found-forward_pipelines.begin());
                if(found==forward_pipelines.end())
                    forward_pipelines.push_back({material.forward_blend,material.forward_raster,{},{},bool(material.vertex_bank),{},emblem_w_depth});
                forward_pipeline_indices.push_back(index);
                unsigned specialized=~0u;
                if(specialize_combiners) {
                    const auto controls=combiner_controls(material_uniforms[i].program);
                    const auto found_specialized=std::find_if(forward_pipelines.begin(),forward_pipelines.end(),
                        [&](const auto &p){return p.blend==material.forward_blend && p.state==material.forward_raster &&
                            p.extended_vertex_bank==bool(material.vertex_bank) && p.controls==controls && p.emblem_w_depth==emblem_w_depth;});
                    specialized=unsigned(found_specialized-forward_pipelines.begin());
                    if(found_specialized==forward_pipelines.end())
                        forward_pipelines.push_back({material.forward_blend,material.forward_raster,{},{},bool(material.vertex_bank),controls,emblem_w_depth});
                }
                specialized_pipeline_indices.push_back(specialized);
            } else {
                forward_pipeline_indices.push_back(~0u);specialized_pipeline_indices.push_back(~0u);
            }
        }
        auto authored_sampler=[&](const ShaderTextureState &state,unsigned mip_levels=0) {
            const auto key=std::pair{state,mip_levels};
            if(const auto found=authored_samplers.find(key);found!=authored_samplers.end())return found->second;
            if(!sampler_mirror_clamp && (state.address_u==5 || state.address_v==5 || state.address_w==5))
                throw FormatError("retail mirror-once addressing requires native sampler support");
            auto description=native_sampler(state,sampler_limits,sampler_anisotropy,sampler_non_seamless_cube);
            // The captured default glow sampler explicitly limits LOD to the
            // image's final level (6 for its seven original mip levels).
            if(mip_levels)description.info.maxLod=std::min(description.info.maxLod,float(mip_levels-1));
            if(description.custom_border) {
                if(!custom_border_color)throw FormatError("authored texture border requires VK_EXT_custom_border_color");
                description.info.pNext=&description.border;
            }
            VkSampler handle{};
            check(vkCreateSampler(device,&description.info,nullptr,&handle),"create authored texture sampler");
            authored_samplers.emplace(key,handle);return handle;
        };
        auto descriptor=[&](std::uint32_t material,std::uint32_t lightmap,VkDescriptorSet& set) {
            stage_sampler_states[material]=materials[material]->texture_states;
            VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocate.descriptorPool=texture_pool; allocate.descriptorSetCount=1; allocate.pSetLayouts=&texture_layout;
            check(vkAllocateDescriptorSets(device,&allocate,&set),"allocate material descriptor");
            VkDescriptorImageInfo images[]{
                {authored_sampler(materials[material]->texture_states[0],
                    materials[material]->light_volume_pass?textures[material].mip_count:0),textures[material].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                {authored_sampler(materials[material]->texture_states[1]),detail_textures[material].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                {authored_sampler(materials[material]->texture_states[2]),bump_textures[material].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                {authored_sampler(materials[material]->texture_states[3]),baked_materials[material]?lightmap_textures[lightmap].view:auxiliary_textures[material].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
            VkDescriptorBufferInfo buffer{material_buffer,stride*material,sizeof(MaterialUniform)};
            VkDescriptorBufferInfo vertex_bank{vertex_bank_buffer,vertex_bank_stride*material,sizeof(VertexBankUniform)};
            const GpuTexture* stages[]{&textures[material],&detail_textures[material],&bump_textures[material],
                baked_materials[material]?&lightmap_textures[lightmap]:&auxiliary_textures[material]};
            for(unsigned i=0;i<4;++i) {
                stage_samplers[material][i]=images[i].sampler;
                const auto &frames=animated_textures[material][i];
                if(!frames.empty()) {
                    if(bound_frames[material][i]>=frames.size()) throw FormatError("animated texture frame outside GPU images");
                    stages[i]=&frames[bound_frames[material][i]];images[i].imageView=stages[i]->view;
                }
            }
            VkDescriptorImageInfo cubes[4]{},volumes[4]{};
            for(unsigned i=0;i<4;++i) {
                if(stages[i]->cube_view && !sampler_non_seamless_cube)
                    throw FormatError("original cube face filtering requires VK_EXT_non_seamless_cube_map");
                cubes[i]={images[i].sampler,stages[i]->cube_view?stages[i]->cube_view:cube_fallback.cube_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                volumes[i]={images[i].sampler,stages[i]->depth?stages[i]->view:volume_fallback.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                if(stages[i]->depth)images[i].imageView=image_fallback.view;
            }
            VkWriteDescriptorSet writes[8]{};
            for(unsigned i=0;i<5;++i) {
                writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}; writes[i].dstSet=set;
                writes[i].dstBinding=i; writes[i].descriptorCount=1;
                writes[i].descriptorType=i==4?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                if(i==4) writes[i].pBufferInfo=&buffer; else writes[i].pImageInfo=&images[i];
            }
            writes[5]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[5].dstSet=set;writes[5].dstBinding=5;
            writes[5].descriptorCount=4;writes[5].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[5].pImageInfo=cubes;
            writes[6]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[6].dstSet=set;writes[6].dstBinding=6;
            writes[6].descriptorCount=4;writes[6].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[6].pImageInfo=volumes;
            writes[7]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[7].dstSet=set;writes[7].dstBinding=7;
            writes[7].descriptorCount=1;writes[7].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;writes[7].pBufferInfo=&vertex_bank;
            vkUpdateDescriptorSets(device,8,writes,0,nullptr);
        };
        for(std::uint32_t i=0;i<count;++i) {
            descriptor(i,0,textures[i].descriptor);
            if(scope_materials[i])descriptor(i,0,scope_descriptors[i]);
        }
        for(const auto &[material,lightmap]:geometry_pairs)
            descriptor(material,lightmap,geometry_descriptors[{material,lightmap}]);
        for(auto& draw:draws)draw.descriptor=geometry_descriptor(draw.material,draw.lightmap);
        flush_texture_uploads();
        batch_texture_uploads=false;
    }

    VkDescriptorSet geometry_descriptor(unsigned material,unsigned lightmap) const {
        if(!lightmap || !baked_materials.at(material))return textures.at(material).descriptor;
        const auto found=geometry_descriptors.find({material,lightmap});
        if(found==geometry_descriptors.end())throw FormatError("dynamic geometry has an unbound baked-lightmap pair");
        return found->second;
    }

    void retire_weather_aliases() {
        for(std::size_t i=0;i<weather_aliases.size();++i)if(weather_alias_pending[i]) {
            weather_aliases[i]->retire(fence);weather_alias_pending[i]=false;
        }
    }
    void destroy_swapchain() {
        weather_point_renderers.clear();
        weather_line_renderers.clear();
        weather_plate_renderers.clear();weather_aliases.clear();weather_alias_pending.clear();
        for(auto &[source,texture]:weather_sprites) {
            if(texture.view)vkDestroyImageView(device,texture.view,nullptr);
            if(texture.image)vkDestroyImage(device,texture.image,nullptr);
            if(texture.memory)vkFreeMemory(device,texture.memory,nullptr);
        }
        weather_sprites.clear();
        if(weather_sprite_sampler)vkDestroySampler(device,weather_sprite_sampler,nullptr);
        weather_sprite_sampler={};
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
        menu_world_native.reset();menu_world_reported=false;
        effect_light_native.reset();
#endif
        if(debug_graphics) {
            ImGui::SetCurrentContext(debug_panel->context());
            ImGui_ImplVulkan_Shutdown();
            debug_graphics=false;
        }
        debug_draw_data=nullptr;
        if(display_pipeline)vkDestroyPipeline(device,display_pipeline,nullptr);
        if(display_layout)vkDestroyPipelineLayout(device,display_layout,nullptr);
        display_pipeline=VK_NULL_HANDLE;display_layout=VK_NULL_HANDLE;
        if(crt_pipeline)vkDestroyPipeline(device,crt_pipeline,nullptr);
        if(crt_layout)vkDestroyPipelineLayout(device,crt_layout,nullptr);
        crt_pipeline=VK_NULL_HANDLE;crt_layout=VK_NULL_HANDLE;
        // Their pipelines/passes refer to this swapchain's continuation;
        // the caller has retired GPU work before recreating these resources.
        shadow_storage.reset();
        effect_light_storage.reset();
        shadow_render.reset();
        object_draw_storage.reset();
#ifdef HALO2_PARTICLE_GRAPHICS_AVAILABLE
        particle_renderer.reset();
#endif
#if HALO2_SOURCE_DECALS_AVAILABLE
        decal_renderer.reset();
#if HALO2_SOURCE_CONTRAILS_AVAILABLE
        contrail_renderer.reset();
#endif
#endif
        scope_render.reset();
        if(bsp_postprocess)bsp_postprocess_state=bsp_postprocess->persistent_state();
        bsp_postprocess.reset();
        if(water_copy_pipeline)vkDestroyPipeline(device,water_copy_pipeline,nullptr);
        if(water_reflection_framebuffer)vkDestroyFramebuffer(device,water_reflection_framebuffer,nullptr);
        if(water_copy_framebuffer)vkDestroyFramebuffer(device,water_copy_framebuffer,nullptr);
        if(water_reflection_pass)vkDestroyRenderPass(device,water_reflection_pass,nullptr);
        if(water_copy_pass)vkDestroyRenderPass(device,water_copy_pass,nullptr);
        water_copy_pipeline=VK_NULL_HANDLE;water_reflection_framebuffer=water_copy_framebuffer=VK_NULL_HANDLE;
        water_reflection_pass=water_copy_pass=VK_NULL_HANDLE;water_scene_initialized=false;
        particle_scene_initialized=false;
        for(auto target:{&water_scene,&water_reflection,&water_refraction,&water_depth,&particle_scene}) {
            if(target->view)vkDestroyImageView(device,target->view,nullptr);
            if(target->image)vkDestroyImage(device,target->image,nullptr);
            if(target->memory)vkFreeMemory(device,target->memory,nullptr);
            *target={};
        }
        if(bloom_filter_pipeline)vkDestroyPipeline(device,bloom_filter_pipeline,nullptr);
        if(bloom_composite_pipeline)vkDestroyPipeline(device,bloom_composite_pipeline,nullptr);
        if(scene_filter_pipeline)vkDestroyPipeline(device,scene_filter_pipeline,nullptr);
        bloom_filter_pipeline=bloom_composite_pipeline=scene_filter_pipeline=VK_NULL_HANDLE;
        for(auto fb:bloom_framebuffers)if(fb)vkDestroyFramebuffer(device,fb,nullptr);
        bloom_framebuffers={};
        for(auto target:{&bloom_scene,&bloom_targets[0],&bloom_targets[1]}) {
            if(target->view)vkDestroyImageView(device,target->view,nullptr);
            if(target->image)vkDestroyImage(device,target->image,nullptr);
            if(target->memory)vkFreeMemory(device,target->memory,nullptr);
            *target={};
        }
        bloom_scene_initialized=false;
        if(patchy_noise_pipeline)vkDestroyPipeline(device,patchy_noise_pipeline,nullptr);
        if(patchy_composite_pipeline)vkDestroyPipeline(device,patchy_composite_pipeline,nullptr);
        if(patchy_framebuffer)vkDestroyFramebuffer(device,patchy_framebuffer,nullptr);
        if(patchy_pass)vkDestroyRenderPass(device,patchy_pass,nullptr);
        if(patchy_target.view)vkDestroyImageView(device,patchy_target.view,nullptr);
        if(patchy_target.image)vkDestroyImage(device,patchy_target.image,nullptr);
        if(patchy_target.memory)vkFreeMemory(device,patchy_target.memory,nullptr);
        patchy_noise_pipeline=patchy_composite_pipeline=VK_NULL_HANDLE;
        patchy_framebuffer=VK_NULL_HANDLE;patchy_pass=VK_NULL_HANDLE;patchy_target={};
        for(auto framebuffer:fog_framebuffers)vkDestroyFramebuffer(device,framebuffer,nullptr);
        fog_framebuffers.clear();
        if(fog_pipeline)vkDestroyPipeline(device,fog_pipeline,nullptr);
        if(fog_pass)vkDestroyRenderPass(device,fog_pass,nullptr);
        if(continuation_pass)vkDestroyRenderPass(device,continuation_pass,nullptr);
        fog_pipeline=VK_NULL_HANDLE;fog_pass=VK_NULL_HANDLE;continuation_pass=VK_NULL_HANDLE;
        if(accumulation_framebuffer) vkDestroyFramebuffer(device,accumulation_framebuffer,nullptr);
        if(world_accumulation_pass) vkDestroyRenderPass(device,world_accumulation_pass,nullptr);
        if(foreground_accumulation_pass) vkDestroyRenderPass(device,foreground_accumulation_pass,nullptr);
        if(accumulation_pass) vkDestroyRenderPass(device,accumulation_pass,nullptr);
        if(accumulation_view) vkDestroyImageView(device,accumulation_view,nullptr);
        if(accumulation_image) vkDestroyImage(device,accumulation_image,nullptr);
        if(accumulation_memory) vkFreeMemory(device,accumulation_memory,nullptr);
        accumulation_framebuffer=VK_NULL_HANDLE;accumulation_pass=VK_NULL_HANDLE;
        world_accumulation_pass=foreground_accumulation_pass=VK_NULL_HANDLE;
        accumulation_view=VK_NULL_HANDLE;accumulation_image=VK_NULL_HANDLE;accumulation_memory=VK_NULL_HANDLE;
        for (auto framebuffer : framebuffers) if (framebuffer) vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffers.clear();
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
        if(sky_pipeline)vkDestroyPipeline(device,sky_pipeline,nullptr);
        sky_pipeline=VK_NULL_HANDLE;
        if(light_volume_pipeline)vkDestroyPipeline(device,light_volume_pipeline,nullptr);
        light_volume_pipeline=VK_NULL_HANDLE;
        for(auto &hud:hud_pipelines){if(hud)vkDestroyPipeline(device,hud,nullptr);hud=VK_NULL_HANDLE;}
        for(auto &loading:vista_loading_pipelines){if(loading)vkDestroyPipeline(device,loading,nullptr);loading=VK_NULL_HANDLE;}
        if (ui_pipeline) vkDestroyPipeline(device, ui_pipeline, nullptr);
        ui_pipeline = VK_NULL_HANDLE;
        if(ui_backdrop_pipeline)vkDestroyPipeline(device,ui_backdrop_pipeline,nullptr);
        ui_backdrop_pipeline=VK_NULL_HANDLE;
        for(auto *frame:{&ui_frame_pipeline,&ui_frame_backdrop_pipeline}) {
            if(*frame)vkDestroyPipeline(device,*frame,nullptr);
            *frame=VK_NULL_HANDLE;
        }
        for(auto &p:forward_pipelines) {
            if(p.handle)vkDestroyPipeline(device,p.handle,nullptr);
            if(p.sky_handle)vkDestroyPipeline(device,p.sky_handle,nullptr);
            p.handle=p.sky_handle=VK_NULL_HANDLE;
        }
        if (overlay) vkDestroyPipeline(device, overlay, nullptr);
        overlay = VK_NULL_HANDLE;
        if(popup_tint_pipeline)vkDestroyPipeline(device,popup_tint_pipeline,nullptr);
        popup_tint_pipeline=VK_NULL_HANDLE;
        if (render_pass) vkDestroyRenderPass(device, render_pass, nullptr);
        render_pass = VK_NULL_HANDLE;
        if (depth_view) vkDestroyImageView(device, depth_view, nullptr);
        depth_view = VK_NULL_HANDLE;
        if(depth_attachment_view)vkDestroyImageView(device,depth_attachment_view,nullptr);
        depth_attachment_view=VK_NULL_HANDLE;
        if (depth) vkDestroyImage(device, depth, nullptr);
        depth = VK_NULL_HANDLE;
        if (depth_memory) vkFreeMemory(device, depth_memory, nullptr);
        depth_memory = VK_NULL_HANDLE;
        for (auto view : views) if (view) vkDestroyImageView(device, view, nullptr);
        views.clear();
        for(auto &target:scene_targets) {
            // views owns and has already released target.view.
            if(target.image)vkDestroyImage(device,target.image,nullptr);
            if(target.memory)vkFreeMemory(device,target.memory,nullptr);
        }
        scene_targets.clear();images.clear();presentation_images.clear();
        if(ssaa_intermediate.view)vkDestroyImageView(device,ssaa_intermediate.view,nullptr);
        if(ssaa_intermediate.image)vkDestroyImage(device,ssaa_intermediate.image,nullptr);
        if(ssaa_intermediate.memory)vkFreeMemory(device,ssaa_intermediate.memory,nullptr);
        ssaa_intermediate={};
        for (auto semaphore : present_ready) if (semaphore) vkDestroySemaphore(device, semaphore, nullptr);
        present_ready.clear();
        if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }

    std::string validate_display(const DisplaySettings &s) const {
        if(s.anti_aliasing>2 || s.brightness>9 || s.gamma>10 || s.detail>2 ||
           !valid_field_of_view(s.field_of_view) || !valid_field_of_view(s.vehicle_field_of_view) ||
           (s.two_player_split!=TwoPlayerSplit::Horizontal && s.two_player_split!=TwoPlayerSplit::Vertical))
            return "Invalid native video setting.";
        int w{},h{};SDL_Vulkan_GetDrawableSize(window,&w,&h);
        const std::uint64_t scale=1u<<s.anti_aliasing;
        const auto width=std::uint64_t(s.internal_width?s.internal_width:std::max(1,w))*scale;
        const auto height=std::uint64_t(s.internal_height?s.internal_height:std::max(1,h))*scale;
        VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical,&properties);
        if(width>properties.limits.maxImageDimension2D || height>properties.limits.maxImageDimension2D ||
           width>properties.limits.maxFramebufferWidth || height>properties.limits.maxFramebufferHeight)
            return "Internal resolution with anti-aliasing exceeds this GPU's image/framebuffer limit.";
        if(width*height>64u*1024u*1024u)
            return "Internal resolution with anti-aliasing exceeds the native 64-megapixel allocation limit.";
        VkFormatProperties features{};vkGetPhysicalDeviceFormatProperties(physical,format,&features);
        const auto needed=VK_FORMAT_FEATURE_BLIT_SRC_BIT|VK_FORMAT_FEATURE_BLIT_DST_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        if((s.internal_width || s.anti_aliasing) && (features.optimalTilingFeatures&needed)!=needed)
            return "This GPU format does not support filtered internal-resolution presentation.";
        return {};
    }
    bool create_swapchain(bool loading_only=false) {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        if (width <= 0 || height <= 0) return false;
        check(vkDeviceWaitIdle(device), "wait for swapchain recreation");
        destroy_swapchain();
        VkSurfaceCapabilitiesKHR capabilities;
        check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &capabilities), "query surface capabilities");
        std::uint32_t count{};
        std::vector<VkSurfaceFormatKHR> formats;
        VkResult format_result;
        do {
            check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, nullptr), "query surface formats");
            formats.resize(count);
            format_result = count ? vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, formats.data()) : VK_SUCCESS;
            // The platform's format list may change between count and fill,
            // including while the same window changes renderer at loading.
        } while (format_result == VK_INCOMPLETE);
        check(format_result, "query surface formats");
        formats.resize(count);
        const auto chosen = presentation_format(formats);
        if (!chosen) throw std::runtime_error("surface requires an unsupported color format");
        format = chosen->format;
        extent = capabilities.currentExtent;
        if (extent.width == std::numeric_limits<std::uint32_t>::max()) {
            extent.width = std::clamp(static_cast<std::uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(static_cast<std::uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
        if (!extent.width || !extent.height) return false;
        presentation_extent=extent;
        const auto settings=!loading_only && debug_panel?debug_panel->display_settings():DisplaySettings{};
        const auto error=validate_display(settings);
        if(!error.empty())throw FormatError(error);
        active_internal_width=settings.internal_width;active_internal_height=settings.internal_height;
        active_aa=settings.anti_aliasing;active_detail=settings.detail;
        const unsigned scale=1u<<active_aa;
        extent.width=(settings.internal_width?settings.internal_width:presentation_extent.width)*scale;
        extent.height=(settings.internal_height?settings.internal_height:presentation_extent.height)*scale;
        const bool offscreen=extent.width!=presentation_extent.width || extent.height!=presentation_extent.height;
        scene_layout=offscreen?VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        if(offscreen && !(capabilities.supportedUsageFlags&VK_IMAGE_USAGE_TRANSFER_DST_BIT))
            throw FormatError("Surface does not support native internal-resolution presentation.");
        std::uint32_t image_count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount) image_count = std::min(image_count, capabilities.maxImageCount);
        VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        for (const auto candidate : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
            if (capabilities.supportedCompositeAlpha & candidate) { alpha = candidate; break; }
        can_capture = capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        info.surface = surface;
        info.minImageCount = image_count;
        info.imageFormat = format;
        info.imageColorSpace = chosen->colorSpace;
        info.imageExtent = presentation_extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | (can_capture ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0) |
            (offscreen?VK_IMAGE_USAGE_TRANSFER_DST_BIT:0);
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform = capabilities.currentTransform;
        info.compositeAlpha = alpha;
        info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        info.clipped = VK_TRUE;
        check(vkCreateSwapchainKHR(device, &info, nullptr, &swapchain), "create swapchain");
        check(vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr), "get swapchain images");
        images.resize(count);
        check(vkGetSwapchainImagesKHR(device, swapchain, &count, images.data()), "get swapchain images");
        presentation_images=images;
        if(offscreen) {
            scene_targets.resize(count);
            for(unsigned i=0;i<count;++i) {
                make_post_image(scene_targets[i],extent,format,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
                images[i]=scene_targets[i].image;
                vkDestroyImageView(device,scene_targets[i].view,nullptr);
                scene_targets[i].view=VK_NULL_HANDLE;
            }
        }
        if(offscreen && active_aa==2)
            make_post_image(ssaa_intermediate,{extent.width/2,extent.height/2},format,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        views.resize(count);
        present_ready.resize(count);
        for (std::size_t i = 0; i < images.size(); ++i) {
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view.image = images[i];
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format = format;
            view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            check(vkCreateImageView(device, &view, nullptr, &views[i]), "create color view");
            VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            check(vkCreateSemaphore(device, &semaphore, nullptr, &present_ready[i]), "create image presentation semaphore");
        }
        make_depth();
        make_pipeline();
        framebuffers.resize(images.size());
        fog_framebuffers.resize(images.size());
        for (std::size_t i = 0; i < images.size(); ++i) {
            VkImageView attachments[]{views[i], depth_attachment_view};
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer.renderPass = render_pass;
            framebuffer.attachmentCount = 2;
            framebuffer.pAttachments = attachments;
            framebuffer.width = extent.width;
            framebuffer.height = extent.height;
            framebuffer.layers = 1;
            check(vkCreateFramebuffer(device, &framebuffer, nullptr, &framebuffers[i]), "create framebuffer");
            framebuffer.renderPass=fog_pass;framebuffer.attachmentCount=1;
            check(vkCreateFramebuffer(device,&framebuffer,nullptr,&fog_framebuffers[i]),"create fog framebuffer");
        }
        const VkDescriptorImageInfo image{fog_depth_sampler,depth_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet=fog_descriptor;write.dstBinding=0;write.descriptorCount=1;
        write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
        vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        for(const auto descriptor:view_fog_descriptors) {
            write.dstSet=descriptor;vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
        patchy_image(1,0,depth_view,fog_depth_sampler);
        if(!loading_only)make_bsp_postprocess();
        dirty = false;
        return true;
    }

    void make_depth() {
        VkImageFormatProperties support;
        if(!source_depth_supported(physical,support) || extent.width>support.maxExtent.width ||
           extent.height>support.maxExtent.height || support.maxExtent.depth<1)
            throw FormatError("original D24S8 image does not support this drawable extent");
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = depth_format;
        image.extent = {extent.width, extent.height, 1};
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = source_depth_usage;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateImage(device, &image, nullptr, &depth), "create depth image");
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device, depth, &requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device, &allocation, nullptr, &depth_memory), "allocate depth memory");
        check(vkBindImageMemory(device, depth, depth_memory, 0), "bind depth memory");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = depth;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = depth_format;
        view.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        check(vkCreateImageView(device, &view, nullptr, &depth_view), "create depth view");
        view.subresourceRange.aspectMask|=VK_IMAGE_ASPECT_STENCIL_BIT;
        check(vkCreateImageView(device,&view,nullptr,&depth_attachment_view),"create original depth/stencil attachment view");
    }

    void make_pipeline() {
        VkAttachmentDescription attachments[2]{};
        attachments[0].format = format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = scene_layout;
        attachments[1].format = depth_format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        // Retain the existing native frame initialization {depth1,stencil0}.
        // Original conditional stencil clears and per-draw writers are
        // separate source scheduling inputs; this is not that lifecycle.
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        const VkAttachmentReference depth_reference{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color;
        subpass.pDepthStencilAttachment = &depth_reference;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = dependency.srcStageMask;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        std::array<VkSubpassDependency,2> pass_dependencies{};
        pass_dependencies[0]=dependency;
        pass_dependencies[0].srcStageMask|=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        pass_dependencies[0].srcAccessMask|=VK_ACCESS_SHADER_READ_BIT;
        pass_dependencies[0].srcAccessMask|=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        pass_dependencies[0].dstAccessMask|=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        pass_dependencies[1].srcSubpass=0;pass_dependencies[1].dstSubpass=VK_SUBPASS_EXTERNAL;
        pass_dependencies[1].srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        pass_dependencies[1].dstStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        pass_dependencies[1].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        pass_dependencies[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        VkRenderPassCreateInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        render.attachmentCount = 2;
        render.pAttachments = attachments;
        render.subpassCount = 1;
        render.pSubpasses = &subpass;
        render.dependencyCount = unsigned(pass_dependencies.size());
        render.pDependencies = pass_dependencies.data();
        check(vkCreateRenderPass(device, &render, nullptr, &render_pass), "create render pass");
        auto loaded=std::array{attachments[0],attachments[1]};
        loaded[0].loadOp=loaded[1].loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;
        loaded[1].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_LOAD;
        loaded[0].initialLayout=scene_layout;
        loaded[1].initialLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        auto continuation=render;continuation.pAttachments=loaded.data();
        check(vkCreateRenderPass(device,&continuation,nullptr,&continuation_pass),"create fog continuation pass");
        if(!shadow_materials.empty()) {
            shadow_render=std::make_unique<SourceShadowRender>(SourceShadowRenderContext{
                device,physical,extent,continuation_pass,shader_directory});
            shadow_storage=std::make_unique<SourceShadowDrawStorage>(SourceShadowDrawStorageContext{
                device,physical,texture_layout,pipeline_layout,shadow_render->generation_pass(),
                continuation_pass,shader_directory,sizeof(MaterialUniform)});
        }
        if(std::any_of(scope_materials.begin(),scope_materials.end(),[](bool value){return value;})) {
            if(!can_capture)throw FormatError("original scope copy requires a readable scene target");
            scope_render=std::make_unique<ScopeRender>(ScopeRenderContext{
                device,physical,extent,format,continuation_pass,shader_directory,scene_layout});
            for(unsigned m=0;m<scope_materials.size();++m)if(scope_materials[m]) {
                const VkDescriptorImageInfo image{stage_samplers[m][0],scope_render->target_view(),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=scope_descriptors[m];
                write.dstBinding=0;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
                vkUpdateDescriptorSets(device,1,&write,0,nullptr);
            }
        }
        VkSubpassDescription fog_subpass{};fog_subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        fog_subpass.colorAttachmentCount=1;fog_subpass.pColorAttachments=&color;
        auto fog_render=render;fog_render.attachmentCount=1;fog_render.pAttachments=loaded.data();
        fog_render.pSubpasses=&fog_subpass;
        check(vkCreateRenderPass(device,&fog_render,nullptr,&fog_pass),"create fog composite pass");
        auto patchy_attachment=attachments[0];
        patchy_attachment.format=VK_FORMAT_R8G8B8A8_UNORM;
        patchy_attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        patchy_attachment.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        auto patchy_render=fog_render;patchy_render.pAttachments=&patchy_attachment;
        check(vkCreateRenderPass(device,&patchy_render,nullptr,&patchy_pass),"create patchy noise pass");
        // 1D770's retail target19 is320x240. Native aspect extension adds
        // samples outside the centered raster instead of stretching those
        // samples across a wider or taller frame. Retail4:3 stays320x240.
        const auto haze_raster=frontend_post_raster(extent.width,extent.height,320,240);
        patchy_extent={haze_raster.width,haze_raster.height};
        VkImageCreateInfo patchy_image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        patchy_image_info.imageType=VK_IMAGE_TYPE_2D;patchy_image_info.format=patchy_attachment.format;
        patchy_image_info.extent={patchy_extent.width,patchy_extent.height,1};
        patchy_image_info.mipLevels=patchy_image_info.arrayLayers=1;
        patchy_image_info.samples=VK_SAMPLE_COUNT_1_BIT;patchy_image_info.tiling=VK_IMAGE_TILING_OPTIMAL;
        patchy_image_info.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        check(vkCreateImage(device,&patchy_image_info,nullptr,&patchy_target.image),"create patchy noise target");
        VkMemoryRequirements patchy_requirements{};vkGetImageMemoryRequirements(device,patchy_target.image,&patchy_requirements);
        VkMemoryAllocateInfo patchy_allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        patchy_allocation.allocationSize=patchy_requirements.size;
        patchy_allocation.memoryTypeIndex=memory_type(patchy_requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&patchy_allocation,nullptr,&patchy_target.memory),"allocate patchy noise target");
        check(vkBindImageMemory(device,patchy_target.image,patchy_target.memory,0),"bind patchy noise target");
        VkImageViewCreateInfo patchy_view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        patchy_view.image=patchy_target.image;patchy_view.viewType=VK_IMAGE_VIEW_TYPE_2D;patchy_view.format=patchy_attachment.format;
        patchy_view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        check(vkCreateImageView(device,&patchy_view,nullptr,&patchy_target.view),"create patchy noise target view");
        VkFramebufferCreateInfo patchy_fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        patchy_fb.renderPass=patchy_pass;patchy_fb.attachmentCount=1;patchy_fb.pAttachments=&patchy_target.view;
        patchy_fb.width=patchy_extent.width;patchy_fb.height=patchy_extent.height;patchy_fb.layers=1;
        check(vkCreateFramebuffer(device,&patchy_fb,nullptr,&patchy_framebuffer),"create patchy noise framebuffer");
        patchy_image(1,2,patchy_target.view,fog_gradient_sampler);
        // Retail ping-pong targets 7/8 are fixed 160x120 images (1D770).
        const auto bloom_raster=frontend_post_raster(extent.width,extent.height,active_detail?80:160,active_detail?60:120);
        bloom_extent={bloom_raster.width,bloom_raster.height};
        make_post_image(bloom_scene,extent,format,VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        for(unsigned i=0;i<2;++i) {
            make_post_image(bloom_targets[i],bloom_extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            auto fb=patchy_fb;fb.pAttachments=&bloom_targets[i].view;fb.width=bloom_extent.width;fb.height=bloom_extent.height;
            check(vkCreateFramebuffer(device,&fb,nullptr,&bloom_framebuffers[i]),"create bloom framebuffer");
        }
        for(unsigned i=0;i<bloom_descriptors.size();++i) {
            const auto view=i && i<6?bloom_targets[(i-1)%2].view:bloom_scene.view;
            const VkDescriptorImageInfo image{fog_gradient_sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet=bloom_descriptors[i];write.dstBinding=0;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        }
        if(std::any_of(accumulation_materials.begin(),accumulation_materials.end(),[](bool present){return present;})) {
            auto accumulation_attachments=std::array{attachments[0],attachments[1]};
            accumulation_attachments[0].finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            auto target_pass=render;target_pass.pAttachments=accumulation_attachments.data();
            check(vkCreateRenderPass(device,&target_pass,nullptr,&accumulation_pass),"create base accumulation pass");
            // The full-surface W-depth path has already initialized the shared
            // depth/stencil in the main pass. Target 1 clears only color here;
            // first person also retains that view's accumulated color. These
            // compatible passes do not change legacy/clipped initialization.
            auto world_accumulation_attachments=accumulation_attachments;
            world_accumulation_attachments[1]=loaded[1];
            target_pass.pAttachments=world_accumulation_attachments.data();
            check(vkCreateRenderPass(device,&target_pass,nullptr,&world_accumulation_pass),"create retained world accumulation pass");
            auto foreground_accumulation_attachments=world_accumulation_attachments;
            foreground_accumulation_attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;
            foreground_accumulation_attachments[0].initialLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            target_pass.pAttachments=foreground_accumulation_attachments.data();
            check(vkCreateRenderPass(device,&target_pass,nullptr,&foreground_accumulation_pass),"create retained foreground accumulation pass");
            VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            image.imageType=VK_IMAGE_TYPE_2D;image.format=format;
            image.extent={extent.width,extent.height,1};image.mipLevels=image.arrayLayers=1;
            image.samples=VK_SAMPLE_COUNT_1_BIT;image.tiling=VK_IMAGE_TILING_OPTIMAL;
            image.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
            check(vkCreateImage(device,&image,nullptr,&accumulation_image),"create base accumulation image");
            VkMemoryRequirements requirements;vkGetImageMemoryRequirements(device,accumulation_image,&requirements);
            VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            allocation.allocationSize=requirements.size;
            allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            check(vkAllocateMemory(device,&allocation,nullptr,&accumulation_memory),"allocate base accumulation image");
            check(vkBindImageMemory(device,accumulation_image,accumulation_memory,0),"bind base accumulation image");
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view.image=accumulation_image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=format;
            view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            check(vkCreateImageView(device,&view,nullptr,&accumulation_view),"create base accumulation view");
            VkImageView target_views[]{accumulation_view,depth_attachment_view};
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer.renderPass=accumulation_pass;framebuffer.attachmentCount=2;framebuffer.pAttachments=target_views;
            framebuffer.width=extent.width;framebuffer.height=extent.height;framebuffer.layers=1;
            check(vkCreateFramebuffer(device,&framebuffer,nullptr,&accumulation_framebuffer),"create base accumulation framebuffer");
            // Extern3 also belongs to ordinary sphere-light passes. Bind all
            // authored consumers, including independently queued light draws.
            for(unsigned material=0;material<accumulation_materials.size();++material) if(accumulation_materials[material]) {
                const VkDescriptorImageInfo target{stage_samplers[material][0],accumulation_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                write.dstSet=textures[material].descriptor;write.dstBinding=0;write.descriptorCount=1;
                write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&target;
                vkUpdateDescriptorSets(device,1,&write,0,nullptr);
                for(const auto &[key,descriptor]:geometry_descriptors) if(key.first==material) {
                    write.dstSet=descriptor;
                    vkUpdateDescriptorSets(device,1,&write,0,nullptr);
                }
            }
        }
        if(std::any_of(water_texture_bindings.begin(),water_texture_bindings.end(),[](const auto &b){return b[0]||b[1];})) {
            if(!can_capture)throw FormatError("water refraction requires a readable color surface");
            constexpr VkExtent2D water_extent{512,256}; // 1D770 targets 21/22.
            make_post_image(water_scene,extent,format,VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            make_post_image(water_reflection,water_extent,format,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            make_post_image(water_refraction,water_extent,format,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            make_post_image(water_depth,water_extent,depth_format,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            auto water_attachments=std::array{attachments[0],attachments[1]};
            water_attachments[0].finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            auto water_render=render;water_render.pAttachments=water_attachments.data();
            check(vkCreateRenderPass(device,&water_render,nullptr,&water_reflection_pass),"create water sky reflection pass");
            VkImageView reflection_views[]{water_reflection.view,water_depth.view};
            VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fb.renderPass=water_reflection_pass;fb.attachmentCount=2;fb.pAttachments=reflection_views;
            fb.width=water_extent.width;fb.height=water_extent.height;fb.layers=1;
            check(vkCreateFramebuffer(device,&fb,nullptr,&water_reflection_framebuffer),"create water reflection framebuffer");
            water_render.attachmentCount=1;water_render.pSubpasses=&fog_subpass;
            check(vkCreateRenderPass(device,&water_render,nullptr,&water_copy_pass),"create water color downsample pass");
            fb.renderPass=water_copy_pass;fb.attachmentCount=1;fb.pAttachments=&water_refraction.view;
            check(vkCreateFramebuffer(device,&fb,nullptr,&water_copy_framebuffer),"create water refraction framebuffer");
            const VkDescriptorImageInfo source{fog_gradient_sampler,water_scene.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&source;
            for(const auto descriptor:water_copy_descriptors)for(unsigned binding=0;binding<2;++binding) {
                write.dstSet=descriptor;write.dstBinding=binding;vkUpdateDescriptorSets(device,1,&write,0,nullptr);
            }
            for(unsigned m=0;m<water_texture_bindings.size();++m)for(unsigned target=0;target<2;++target)
                if(water_texture_bindings[m][target]) {
                    const VkDescriptorImageInfo image{stage_samplers[m][target+1],target?water_refraction.view:water_reflection.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                    write.dstSet=textures[m].descriptor;write.dstBinding=target+1;write.pImageInfo=&image;
                    vkUpdateDescriptorSets(device,1,&write,0,nullptr);
                    for(const auto &[key,descriptor]:geometry_descriptors)if(key.first==m) {
                        write.dstSet=descriptor;vkUpdateDescriptorSets(device,1,&write,0,nullptr);
                    }
                }
        }
        const auto vertex = shader("world.vert.spv"), fragment = shader("world.frag.spv");
        VkShaderModule vert{}, frag{};
        struct Modules { VkDevice device; VkShaderModule& a; VkShaderModule& b;
            ~Modules() { if (a) vkDestroyShaderModule(device, a, nullptr); if (b) vkDestroyShaderModule(device, b, nullptr); }
        } modules{device, vert, frag};
        VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        module.codeSize = vertex.size() * 4;
        module.pCode = vertex.data();
        check(vkCreateShaderModule(device, &module, nullptr, &vert), "create vertex shader");
        module.codeSize = fragment.size() * 4;
        module.pCode = fragment.data();
        check(vkCreateShaderModule(device, &module, nullptr, &frag), "create fragment shader");
        VkPipelineShaderStageCreateInfo stages[3]{};
        stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vert, "main", nullptr};
        stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, frag, "main", nullptr};
        VkVertexInputBindingDescription binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[]{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
                                                      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
                                                      {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
                                                      {3,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,lightmap_uv)},
                                                      {4,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,normal)},
                                                      {5,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,tangent)},
                                                      {6,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,binormal)},
                                                      {7,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,incident)},
                                                      {8,0,VK_FORMAT_R32_UINT,offsetof(Vertex,lightmap)},
                                                      {11,0,VK_FORMAT_R32_UINT,offsetof(Vertex,source_stipple_level)},
                                                      {12,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,source_local_position)},
                                                      {13,0,VK_FORMAT_R32_SFLOAT,offsetof(Vertex,source_color_alpha)},
                                                      {9,0,VK_FORMAT_R32_SFLOAT,offsetof(Vertex,prt_cluster)},
                                                      {10,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(Vertex,prt_weights)}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        input.vertexBindingDescriptionCount = 1;
        input.pVertexBindingDescriptions = &binding;
        input.vertexAttributeDescriptionCount = 12;
        input.pVertexAttributeDescriptions = attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.lineWidth = 1;
        raster.depthBiasEnable=source_depth_bias!=0 && source_output_state[0]!=0;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth_state.depthTestEnable = source_depth_mode!=0;
        depth_state.depthWriteEnable = source_depth_write!=0;
        depth_state.depthCompareOp = VkCompareOp(source_depth_function-0x200);
        VkPipelineColorBlendAttachmentState blend{};
        blend.blendEnable=source_output_state[4]!=0;
        // The cold native interface admits ONE, ZERO and FUNC_ADD only.
        blend.srcColorBlendFactor=blend.srcAlphaBlendFactor=source_output_state[5]?VK_BLEND_FACTOR_ONE:VK_BLEND_FACTOR_ZERO;
        blend.dstColorBlendFactor=blend.dstAlphaBlendFactor=source_output_state[6]?VK_BLEND_FACTOR_ONE:VK_BLEND_FACTOR_ZERO;
        blend.colorBlendOp=blend.alphaBlendOp=VkBlendOp(source_output_state[7]-0x8006);
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo color_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        color_state.attachmentCount = 1;
        color_state.pAttachments = &blend;
        VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,VK_DYNAMIC_STATE_BLEND_CONSTANTS};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 3;
        dynamic.pDynamicStates = dynamic_states;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &input;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &samples;
        info.pDepthStencilState = &depth_state;
        info.pColorBlendState = &color_state;
        info.pDynamicState = &dynamic;
        info.layout = pipeline_layout;
        info.renderPass = render_pass;
        check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline), "create world pipeline");
        // Retail 4C690 enables NV097 ZCLAMP_EN=CLAMP and overrides sky
        // culling to NONE, then restores both states before world geometry.
        raster.depthClampEnable=VK_TRUE;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&sky_pipeline),"create original sky pipeline");
        raster.depthClampEnable=VK_FALSE;
        depth_state.depthWriteEnable=VK_FALSE;
        blend.blendEnable=VK_TRUE;blend.colorBlendOp=blend.alphaBlendOp=VK_BLEND_OP_ADD;
        VkShaderModule extended_vert{};
        struct ExtendedModule {
            VkDevice device;VkShaderModule &module;
            ~ExtendedModule(){if(module)vkDestroyShaderModule(device,module,nullptr);}
        } extended_module{device,extended_vert};
        VkShaderModule emblem_geometry{},emblem_fragment{};
        Modules emblem_modules{device,emblem_geometry,emblem_fragment};
        if(std::any_of(forward_pipelines.begin(),forward_pipelines.end(),
                      [](const auto &p){return p.emblem_w_depth;})) {
            if(!source_world_device.geometry_shader_enabled)
                throw FormatError("original armor emblem W-depth requires geometry shader support");
            const auto geometry=shader("world_emblem.geom.spv"),fragment=shader("world_emblem.frag.spv");
            module.codeSize=geometry.size()*4;module.pCode=geometry.data();
            check(vkCreateShaderModule(device,&module,nullptr,&emblem_geometry),"create armor emblem depth geometry shader");
            module.codeSize=fragment.size()*4;module.pCode=fragment.data();
            check(vkCreateShaderModule(device,&module,nullptr,&emblem_fragment),"create armor emblem depth fragment shader");
            stages[2]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_GEOMETRY_BIT,emblem_geometry,"main",nullptr};
        }
        if(std::any_of(forward_pipelines.begin(),forward_pipelines.end(),
                      [](const auto &p){return p.extended_vertex_bank;})) {
            const auto extended=shader("world_prt.vert.spv");
            module.codeSize=extended.size()*4;module.pCode=extended.data();
            check(vkCreateShaderModule(device,&module,nullptr,&extended_vert),"create extended source vertex shader");
        }
        for(auto &p:forward_pipelines) {
            std::array<std::uint32_t,61> constants{};
            std::array<VkSpecializationMapEntry,61> entries{};
            VkSpecializationInfo specialization{};
            if(p.controls) {
                constants[0]=1;
                std::copy(p.controls->begin(),p.controls->end(),constants.begin()+1);
                for(unsigned i=0;i<entries.size();++i)entries[i]={i,std::uint32_t(i*sizeof(constants[0])),sizeof(constants[0])};
                specialization={unsigned(entries.size()),entries.data(),sizeof(constants),constants.data()};
            }
            stages[1].pSpecializationInfo=p.controls?&specialization:nullptr;
            stages[1].module=p.emblem_w_depth?emblem_fragment:frag;
            info.stageCount=p.emblem_w_depth?3:2;
            stages[0].module=p.extended_vertex_bank?extended_vert:vert;
            input.vertexAttributeDescriptionCount=p.extended_vertex_bank?14:12;
            blend.blendEnable=p.blend!=0;
            blend.colorBlendOp=blend.alphaBlendOp=p.blend==12?VK_BLEND_OP_MAX:VK_BLEND_OP_ADD;
            blend.srcColorBlendFactor=p.blend==9?VK_BLEND_FACTOR_DST_COLOR:p.blend==8?VK_BLEND_FACTOR_DST_ALPHA:p.blend==7?VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:p.blend==6?VK_BLEND_FACTOR_DST_COLOR:p.blend==1?VK_BLEND_FACTOR_SRC_ALPHA:VK_BLEND_FACTOR_ONE;
            blend.dstColorBlendFactor=p.blend==11?VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:p.blend==9?VK_BLEND_FACTOR_ZERO:p.blend==10?VK_BLEND_FACTOR_ONE:p.blend==8?VK_BLEND_FACTOR_ONE:p.blend==7?VK_BLEND_FACTOR_DST_ALPHA:p.blend==6?VK_BLEND_FACTOR_SRC_COLOR:p.blend==3?VK_BLEND_FACTOR_ONE:VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            // Planar fog's CONSTANT_COLOR is packed 00FFFFFF: source RGB
            // factor one, source alpha factor zero (59/62/63/74/75 states).
            blend.srcAlphaBlendFactor=p.blend==8?VK_BLEND_FACTOR_DST_ALPHA:p.blend==7?VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:p.blend==6?VK_BLEND_FACTOR_DST_ALPHA:p.blend==5?VK_BLEND_FACTOR_ZERO:VK_BLEND_FACTOR_ONE;
            blend.dstAlphaBlendFactor=p.blend==11?VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:p.blend==10?VK_BLEND_FACTOR_ZERO:p.blend==6?VK_BLEND_FACTOR_SRC_ALPHA:blend.dstColorBlendFactor;
            blend.colorWriteMask=0;
            constexpr unsigned channel_shift[]{16,8,0,24}; // Retail ARGB mask.
            for(unsigned channel=0;channel<4;++channel)
                if((p.state.color_write>>channel_shift[channel])&1)blend.colorWriteMask|=1u<<channel;
            depth_state.depthTestEnable=p.state.depth_enable!=0;
            depth_state.depthWriteEnable=p.state.depth_write!=0;
            if(p.state.depth_function<0x200 || p.state.depth_function>0x207)
                throw FormatError("unsupported retail forward depth comparison");
            depth_state.depthCompareOp=VkCompareOp(p.state.depth_function-0x200);
            // D3D culls the named screen winding. Vk frontFace is CCW here.
            if(p.state.cull!=0 && p.state.cull!=0x900 && p.state.cull!=0x901)
                throw FormatError("unsupported retail forward cull state");
            raster.cullMode=p.state.cull==0x901?VK_CULL_MODE_FRONT_BIT:
                p.state.cull==0x900?VK_CULL_MODE_BACK_BIT:VK_CULL_MODE_NONE;
            check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&p.handle),"create forward material pipeline");
            raster.depthClampEnable=VK_TRUE;raster.cullMode=VK_CULL_MODE_NONE;
            check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&p.sky_handle),"create original sky material pipeline");
            raster.depthClampEnable=VK_FALSE;
        }
        stages[1].pSpecializationInfo=nullptr;
        stages[1].module=frag;
        info.stageCount=2;
        stages[0].module=vert;
        input.vertexAttributeDescriptionCount=12;
        raster.cullMode=VK_CULL_MODE_NONE;
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        vkDestroyShaderModule(device, vert, nullptr); vert = VK_NULL_HANDLE;
        vkDestroyShaderModule(device, frag, nullptr); frag = VK_NULL_HANDLE;
        if(light_volume_materials) {
        const auto volume_vertex=shader("light_volume.vert.spv"),volume_fragment=shader("light_volume.frag.spv");
        module.codeSize=volume_vertex.size()*4;module.pCode=volume_vertex.data();
        check(vkCreateShaderModule(device,&module,nullptr,&vert),"create light-volume vertex shader");
        module.codeSize=volume_fragment.size()*4;module.pCode=volume_fragment.data();
        check(vkCreateShaderModule(device,&module,nullptr,&frag),"create light-volume fragment shader");
        stages[0].module=vert;stages[1].module=frag;
        const VkVertexInputBindingDescription volume_binding{0,sizeof(LightVolumeVertex),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription volume_attributes[]{
            {0,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(LightVolumeVertex,screen)},
            {1,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(LightVolumeVertex,color)},
            {2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(LightVolumeVertex,uv)}};
        input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&volume_binding;
        input.vertexAttributeDescriptionCount=3;input.pVertexAttributeDescriptions=volume_attributes;
        depth_state.depthTestEnable=VK_TRUE;depth_state.depthWriteEnable=VK_FALSE;
        depth_state.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
        blend.blendEnable=VK_TRUE;blend.srcColorBlendFactor=VK_BLEND_FACTOR_ONE;
        blend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&light_volume_pipeline),"create light-volume pipeline");
        input.pVertexBindingDescriptions=&binding;input.pVertexAttributeDescriptions=attributes;
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        vkDestroyShaderModule(device,vert,nullptr);vert=VK_NULL_HANDLE;
        vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
        }
        const auto overlay_vertex = shader("overlay.vert.spv"), overlay_fragment = shader("overlay.frag.spv");
        module.codeSize = overlay_vertex.size() * 4; module.pCode = overlay_vertex.data();
        check(vkCreateShaderModule(device, &module, nullptr, &vert), "create overlay vertex shader");
        module.codeSize = overlay_fragment.size() * 4; module.pCode = overlay_fragment.data();
        check(vkCreateShaderModule(device, &module, nullptr, &frag), "create overlay fragment shader");
        stages[0].module = vert; stages[1].module = frag;
        input.vertexBindingDescriptionCount = input.vertexAttributeDescriptionCount = 0;
        input.pVertexBindingDescriptions = nullptr; input.pVertexAttributeDescriptions = nullptr;
        depth_state.depthTestEnable = depth_state.depthWriteEnable = VK_FALSE;
        blend.blendEnable = VK_TRUE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend.alphaBlendOp = VK_BLEND_OP_ADD;
        info.layout = overlay_layout;
        check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &overlay), "create overlay pipeline");
        // Original UI title and 147CDB tint draws preserve the lower scene's
        // alpha. The popup combiner consumes that alpha in its blur stages.
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&popup_tint_pipeline),"create original popup tint pipeline");
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        vkDestroyShaderModule(device,vert,nullptr);vert=VK_NULL_HANDLE;
        vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
        const auto fog_vertex=shader("fog.vert.spv"),fog_fragment=shader("fog.frag.spv");
        module.codeSize=fog_vertex.size()*4;module.pCode=fog_vertex.data();
        check(vkCreateShaderModule(device,&module,nullptr,&vert),"create fog vertex shader");
        module.codeSize=fog_fragment.size()*4;module.pCode=fog_fragment.data();
        check(vkCreateShaderModule(device,&module,nullptr,&frag),"create fog fragment shader");
        stages[0].module=vert;stages[1].module=frag;
        blend.srcColorBlendFactor=VK_BLEND_FACTOR_ONE;
        blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;
        blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        info.layout=fog_layout;info.renderPass=fog_pass;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&fog_pipeline),"create fog composite pipeline");
        if(water_copy_pass) {
            vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
            const auto copy=shader("water_copy.frag.spv");
            module.codeSize=copy.size()*4;module.pCode=copy.data();
            check(vkCreateShaderModule(device,&module,nullptr,&frag),"create water downsample shader");
            stages[1].module=frag;blend.blendEnable=VK_FALSE;info.renderPass=water_copy_pass;
            check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&water_copy_pipeline),"create original water downsample pipeline");
            blend.blendEnable=VK_TRUE;info.renderPass=fog_pass;
        }
        vkDestroyShaderModule(device,vert,nullptr);vert=VK_NULL_HANDLE;
        vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
        // Patchy and bloom lookups are relative to the active scene rectangle,
        // even when the presentation surface has black borders around it.
        const auto patchy_vertex=shader("patchy_noise.vert.spv");
        module.codeSize=patchy_vertex.size()*4;module.pCode=patchy_vertex.data();
        check(vkCreateShaderModule(device,&module,nullptr,&vert),"create original haze quad vertex shader");
        stages[0].module=vert;
        const auto patchy_noise=shader("patchy_noise.frag.spv");
        module.codeSize=patchy_noise.size()*4;module.pCode=patchy_noise.data();
        check(vkCreateShaderModule(device,&module,nullptr,&frag),"create patchy noise shader");
        stages[1].module=frag;blend.blendEnable=VK_FALSE;
        info.layout=patchy_layout;info.renderPass=patchy_pass;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&patchy_noise_pipeline),"create patchy noise pipeline");
        vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
        const auto patchy_composite=shader("patchy_composite.frag.spv");
        module.codeSize=patchy_composite.size()*4;module.pCode=patchy_composite.data();
        check(vkCreateShaderModule(device,&module,nullptr,&frag),"create patchy composite shader");
        stages[1].module=frag;blend.blendEnable=VK_TRUE;info.renderPass=fog_pass;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&patchy_composite_pipeline),"create patchy composite pipeline");
        vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
        vkDestroyShaderModule(device,vert,nullptr);vert=VK_NULL_HANDLE;
        const auto bloom_vertex=shader("bloom.vert.spv");
        module.codeSize=bloom_vertex.size()*4;module.pCode=bloom_vertex.data();
        check(vkCreateShaderModule(device,&module,nullptr,&vert),"create postprocess vertex shader");
        const auto bloom_fragment=shader("bloom.frag.spv");
        module.codeSize=bloom_fragment.size()*4;module.pCode=bloom_fragment.data();
        check(vkCreateShaderModule(device,&module,nullptr,&frag),"create bloom fragment shader");
        stages[0].module=vert;stages[1].module=frag;blend.blendEnable=VK_FALSE;
        info.layout=bloom_layout;info.renderPass=patchy_pass;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&bloom_filter_pipeline),"create bloom filter pipeline");
        blend.blendEnable=VK_TRUE;blend.srcColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        blend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE;blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE;info.renderPass=fog_pass;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&bloom_composite_pipeline),"create bloom composite pipeline");
        blend.blendEnable=VK_FALSE;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&scene_filter_pipeline),"create title color filter pipeline");
        info.layout=overlay_layout;info.renderPass=render_pass;blend.blendEnable=VK_TRUE;
        blend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
        blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;
        vkDestroyShaderModule(device, vert, nullptr); vert = VK_NULL_HANDLE;
        vkDestroyShaderModule(device, frag, nullptr); frag = VK_NULL_HANDLE;
        const auto ui_vertex = shader("ui.vert.spv"), ui_fragment = shader("ui.frag.spv");
        module.codeSize = ui_vertex.size() * 4; module.pCode = ui_vertex.data();
        check(vkCreateShaderModule(device, &module, nullptr, &vert), "create UI vertex shader");
        module.codeSize = ui_fragment.size() * 4; module.pCode = ui_fragment.data();
        check(vkCreateShaderModule(device, &module, nullptr, &frag), "create UI fragment shader");
        stages[0].module = vert; stages[1].module = frag;
        input.vertexBindingDescriptionCount = 1; input.pVertexBindingDescriptions = &binding;
        input.vertexAttributeDescriptionCount = 3; input.pVertexAttributeDescriptions = attributes;
        info.layout = pipeline_layout;
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&ui_backdrop_pipeline),"create original lower UI channel pipeline");
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &ui_pipeline), "create UI pipeline");
        vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
        const auto frame_fragment=shader("ui_frame.frag.spv");
        module.codeSize=frame_fragment.size()*4;module.pCode=frame_fragment.data();
        check(vkCreateShaderModule(device,&module,nullptr,&frag),"create frame boundary shader");
        stages[1].module=frag;info.layout=ui_frame_layout;
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&ui_frame_backdrop_pipeline),"create lower frame sampling pipeline");
        blend.colorWriteMask|=VK_COLOR_COMPONENT_A_BIT;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&ui_frame_pipeline),"create frame sampling pipeline");
        vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
        info.layout=pipeline_layout;
        const auto hud_fragment=shader("hud.frag.spv");
        module.codeSize=hud_fragment.size()*4;module.pCode=hud_fragment.data();
        check(vkCreateShaderModule(device,&module,nullptr,&frag),"create HUD fragment shader");stages[1].module=frag;
        for(unsigned i=1;i<hud_pipelines.size();++i){
            blend.srcColorBlendFactor=i==1?VK_BLEND_FACTOR_SRC_ALPHA:VK_BLEND_FACTOR_ONE;
            blend.dstColorBlendFactor=i==4?VK_BLEND_FACTOR_SRC_ALPHA:i==3?VK_BLEND_FACTOR_ONE:VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.srcAlphaBlendFactor=blend.srcColorBlendFactor;blend.dstAlphaBlendFactor=blend.dstColorBlendFactor;
            check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&hud_pipelines[i]),"create authored HUD blend pipeline");
        }
        vkDestroyShaderModule(device,vert,nullptr);vert=VK_NULL_HANDLE;
        vkDestroyShaderModule(device,frag,nullptr);frag=VK_NULL_HANDLE;
        const auto loading_vertex=shader("vista_loading.vert.spv"),loading_fragment=shader("vista_loading.frag.spv");
        module.codeSize=loading_vertex.size()*4;module.pCode=loading_vertex.data();
        check(vkCreateShaderModule(device,&module,nullptr,&vert),"create original Vista loading vertex shader");
        module.codeSize=loading_fragment.size()*4;module.pCode=loading_fragment.data();
        check(vkCreateShaderModule(device,&module,nullptr,&frag),"create original Vista loading pixel shaders");
        stages[0].module=vert;stages[1].module=frag;info.layout=vista_loading_layout;
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        for(unsigned i=0;i<vista_loading_pipelines.size();++i) {
            blend.srcColorBlendFactor=i==0?VK_BLEND_FACTOR_ONE:i==1?VK_BLEND_FACTOR_DST_COLOR:VK_BLEND_FACTOR_SRC_ALPHA;
            blend.dstColorBlendFactor=i==0?VK_BLEND_FACTOR_ONE:i==1?VK_BLEND_FACTOR_ZERO:VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.srcAlphaBlendFactor=blend.srcColorBlendFactor;blend.dstAlphaBlendFactor=blend.dstColorBlendFactor;
            check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&vista_loading_pipelines[i]),"create original Vista loading blend pipeline");
        }
    }

    void update_material_constants(std::span<const Material> materials,
        std::optional<std::span<const unsigned>> active = {}) {
        if(materials.size()!=material_count) throw FormatError("animated material table size changed");

        auto update=[&](std::size_t i,const Material &m) {
            if(bool(m.vista_loading)!=bool(vista_loading_materials[i]))
                throw FormatError("animated draw changed its Vista loading category");
            if(m.vista_loading) {
                if(m.vista_loading->shader>3 || unsigned(m.vista_loading->blend)>2)
                    throw FormatError("unsupported Vista loading material program/blend");
                vista_loading_materials[i]=m.vista_loading;
            }
            if(!m.forward_pass)return;
            material_blend_colors[i]=m.forward_blend_color;
            if(bool(m.vertex_bank)!=extended_vertex_banks[i])
                throw FormatError("animated draw changed its source vertex-bank category");
            // An absent extended bank is the immutable zero bank established
            // at construction. Category changes were rejected above.
            if(m.vertex_bank) {
                const auto bank=source_vertex_bank(m);
                if(bank!=vertex_bank_uniforms[i]) {
                    vertex_bank_uniforms[i]=bank;
                    mark_vertex_bank_changed(i);
                }
            }
            wanted_frames[i]=m.texture_frames;
            material_fragment_offsets[i]=m.forward_program.fragment_offsets;
            auto &u=material_uniforms[i];
            if(u.flags[3] && u.vertex_declaration!=pack_source_vertex_declaration(m.vertex_declaration,m.world_vertex_input_transform))
                throw FormatError("animated draw changed its source vertex declaration");
            if(u.program==m.forward_program.words && shader_constants_equal(u.vertex_constants,m.program_constants[1]) &&
               shader_constants_equal(u.vertex_attributes,m.program_constants[0])) return;
            u.program=m.forward_program.words;
            u.vertex_constants=m.program_constants[1];u.vertex_attributes=m.program_constants[0];
            u.forward_transforms=m.forward_transforms;u.diffuse=m.forward_diffuse;u.specular=m.forward_specular;
            mark_material_changed(i);
        };
        const auto update_root=[&](std::size_t i) {
            const auto &m=materials[i];update(i,m);
            if(m.environment_pass)update(environment_materials[i],*m.environment_pass);
            for(std::size_t p=0;p<m.world_overlays_before.size();++p)
                update(world_before[i].at(p),*m.world_overlays_before[p]);
            for(std::size_t p=0;p<m.world_overlays_after.size();++p)
                update(world_after[i].at(p),*m.world_overlays_after[p]);
            for(std::size_t p=0;p<m.world_bloom_passes.size();++p)
                update(world_bloom[i].at(p),*m.world_bloom_passes[p]);
            if(m.planar_fog_pass)update(planar_materials[i],*m.planar_fog_pass);
            for(unsigned p=0;p<3;++p)if(m.world_planar_passes[p])
                update(world_planar_materials[i][p],*m.world_planar_passes[p]);
        };
        if(active) {
            unsigned previous{};bool first=true;
            for(const auto i:*active) {
                if(i>=materials.size() || (!first && i<=previous))
                    throw FormatError("active material roots must be ordered unique resident slots");
                update_root(i);previous=i;first=false;
            }
        } else for(std::size_t i=0;i<materials.size();++i)update_root(i);
    }
    void make_crt_pipeline() {
        if(crt_pipeline)return;
        if(!can_capture)throw FormatError("CRT filter requires a readable color target");
        const VkPushConstantRange constants{VK_SHADER_STAGE_FRAGMENT_BIT,0,4*sizeof(float)};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        // Reuse the existing full-resolution scene copy and linear-clamp sampler.
        layout.setLayoutCount=1;layout.pSetLayouts=&bloom_texture_layout;
        layout.pushConstantRangeCount=1;layout.pPushConstantRanges=&constants;
        check(vkCreatePipelineLayout(device,&layout,nullptr,&crt_layout),"create CRT pipeline layout");
        struct Modules {
            VkDevice device;
            VkShaderModule vert{},frag{};
            ~Modules() {
                if(vert)vkDestroyShaderModule(device,vert,nullptr);
                if(frag)vkDestroyShaderModule(device,frag,nullptr);
            }
        } modules{device};
        const auto vertex=shader("bloom.vert.spv"),fragment=shader("crt_oled.frag.spv");
        VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        module.codeSize=vertex.size()*4;module.pCode=vertex.data();
        check(vkCreateShaderModule(device,&module,nullptr,&modules.vert),"create CRT vertex shader");
        module.codeSize=fragment.size()*4;module.pCode=fragment.data();
        check(vkCreateShaderModule(device,&module,nullptr,&modules.frag),"create CRT fragment shader");
        const VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,modules.vert,"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,modules.frag,"main",nullptr}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blend{};
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo color{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        color.attachmentCount=1;color.pAttachments=&blend;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline.stageCount=2;pipeline.pStages=stages;pipeline.pVertexInputState=&input;
        pipeline.pInputAssemblyState=&assembly;pipeline.pViewportState=&viewport;
        pipeline.pRasterizationState=&raster;pipeline.pMultisampleState=&samples;
        pipeline.pColorBlendState=&color;pipeline.pDynamicState=&dynamic;
        pipeline.layout=crt_layout;pipeline.renderPass=fog_pass;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipeline,nullptr,&crt_pipeline),"create CRT filter pipeline");
    }
    void make_display_pipeline() {
        if(display_pipeline)return;
        if(!can_capture)throw FormatError("display correction filter requires a readable color target");
        const VkPushConstantRange constants{VK_SHADER_STAGE_FRAGMENT_BIT,0,4*sizeof(float)};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        // Reuse the existing full-resolution scene copy and linear-clamp sampler.
        layout.setLayoutCount=1;layout.pSetLayouts=&bloom_texture_layout;
        layout.pushConstantRangeCount=1;layout.pPushConstantRanges=&constants;
        check(vkCreatePipelineLayout(device,&layout,nullptr,&display_layout),"create display correction pipeline layout");
        struct Modules {
            VkDevice device;
            VkShaderModule vert{},frag{};
            ~Modules() {
                if(vert)vkDestroyShaderModule(device,vert,nullptr);
                if(frag)vkDestroyShaderModule(device,frag,nullptr);
            }
        } modules{device};
        const auto vertex=shader("bloom.vert.spv"),fragment=shader("display.frag.spv");
        VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        module.codeSize=vertex.size()*4;module.pCode=vertex.data();
        check(vkCreateShaderModule(device,&module,nullptr,&modules.vert),"create display correction vertex shader");
        module.codeSize=fragment.size()*4;module.pCode=fragment.data();
        check(vkCreateShaderModule(device,&module,nullptr,&modules.frag),"create display correction fragment shader");
        const VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,modules.vert,"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,modules.frag,"main",nullptr}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blend{};
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo color{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        color.attachmentCount=1;color.pAttachments=&blend;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline.stageCount=2;pipeline.pStages=stages;pipeline.pVertexInputState=&input;
        pipeline.pInputAssemblyState=&assembly;pipeline.pViewportState=&viewport;
        pipeline.pRasterizationState=&raster;pipeline.pMultisampleState=&samples;
        pipeline.pColorBlendState=&color;pipeline.pDynamicState=&dynamic;
        pipeline.layout=display_layout;pipeline.renderPass=fog_pass;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipeline,nullptr,&display_pipeline),"create display correction filter pipeline");
    }
    void copy_scene_for_post(unsigned index) {
        vkCmdEndRenderPass(command);
        std::array<VkImageMemoryBarrier,2> copy_barriers{};
        for(auto &b:copy_barriers) {
            b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        }
        auto &color=copy_barriers[0],&target=copy_barriers[1];
        color.image=images[index];color.oldLayout=scene_layout;
        color.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        color.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;color.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        target.image=bloom_scene.image;
        target.oldLayout=bloom_scene_initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
        target.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        target.srcAccessMask=bloom_scene_initialized?VK_ACCESS_SHADER_READ_BIT:0;target.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,copy_barriers.data());
        VkImageCopy region{};region.srcSubresource=region.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        region.extent={extent.width,extent.height,1};
        vkCmdCopyImage(command,images[index],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            bloom_scene.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
        color.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;color.newLayout=scene_layout;
        color.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;color.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        target.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;target.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        target.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;target.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,0,nullptr,0,nullptr,2,copy_barriers.data());
        bloom_scene_initialized=true;
    }
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
    static DrawKey draw_key(const Draw &draw) {
        return {draw.material,draw.lightmap,draw.cluster,draw.source_section,draw.source_part};
    }
    MenuWorldDrawMap prepare_menu_world(const MainmenuWorldDraw *world,
        const Camera &camera,const VkViewport &viewport,bool title_scene,
        std::span<const ModelView> views) {
        MenuWorldDrawMap selected;
        // Retire the previous per-frame descriptors/UBOs only after draw's
        // fence wait. A wider frame needs its own source visibility producer.
        if(menu_world_native)menu_world_native->prepare({});
        if(!world || !title_scene || extent.width*3ull!=extent.height*4ull ||
            extent.width>4095 || extent.height>4095 ||
            std::any_of(views.begin(),views.end(),[](const auto &view){return view.full_surface;}))return selected;
        const auto &enabled=source_world_device;
        if(enabled.instance_api_version<VK_API_VERSION_1_3 ||
            enabled.physical_api_version<VK_API_VERSION_1_3 ||
            !enabled.geometry_shader_enabled || !enabled.geometry_point_size_enabled ||
            !enabled.depth_clamp_enabled || !enabled.demote_to_helper_enabled ||
            depth_format!=VK_FORMAT_D24_UNORM_S8_UINT)return selected;
        if(world->materials.size()!=material_count)
            throw FormatError("mainmenu raw animated material bank differs from resident GPU roots");
        MenuWorldSourceConstants constants;
        for(auto &row:constants)row.fill(std::numeric_limits<float>::quiet_NaN());
        const auto view=source_view_shader_bank(world->frame.render_camera,1024.f,camera.position,
            full_surface_viewport({0,0,640,480,1,false,false,0},extent.width,extent.height),false);
        std::copy(view.begin(),view.end(),constants.begin());
        const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
        const auto fog=bsp_fog_constants(world->frame.fog,camera.position,forward);
        std::copy(fog.begin(),fog.end(),constants.begin()+12);
        const auto frame=world->source.prepare(world->frame,world->materials,constants);
        MenuWorldRasterContext raster;
        raster.viewport_reset({extent.width,extent.height});raster.game_initialize_offset();
        const auto clip=menu_world_depth_clip_planes({.015625f,1024.f,
            MenuWorldDepthBoundary::none,0,false});
        const std::array<float,4> clip_range{0,16777215.f,clip.minimum_w,clip.maximum_w};
        const MenuWorldFragmentSamplerLimits limits{enabled.maximum_lod_bias,
            enabled.maximum_anisotropy,enabled.anisotropy_enabled,enabled.custom_border_colors_enabled};
        std::map<DrawKey,const Draw *> physical_draws;
        for(const auto &draw:draws)if(!physical_draws.emplace(draw_key(draw),&draw).second)
            throw FormatError("duplicate complete title-world physical draw key");
        std::vector<MenuWorldNativeDrawInput> inputs;
        std::vector<unsigned> input_slots;
        std::map<std::string,std::size_t> rejected;
        for(const auto &[reason,count]:frame.rejected)
            rejected[std::string(mainmenu_world_source_reject_name(reason))]+=count;
        const auto packet=[&](const MenuWorldSourceBinding &binding,const Material &material,
            unsigned slot,unsigned lightmap,MenuWorldRasterLayer layer,int geometry) {
            raster.layer_begin(layer);
            raster.entry_begin({menu_world_bsp_entry_stencil_marked(world->frame.admission,geometry),false,false});
            const auto state=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,
                MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},true});
            raster.entry_end();
            MenuWorldNativePacketInput input{binding,generated_menu_world_vertex_catalog::entries,
                material_uniforms.at(slot)};
            input.fragment_offsets=material_fragment_offsets.at(slot);
            input.forward=material.forward_raster;input.forward_blend=material.forward_blend;
            input.texture_states=stage_sampler_states.at(slot);input.sampler_limits=limits;
            input.raster=state;input.clip_range=clip_range;input.fog_parameters={1,1};
            input.surface_size={viewport.width,viewport.height};
            input.demote_to_helper_enabled=enabled.demote_to_helper_enabled;
            input.material_set=geometry_descriptor(slot,lightmap);input.material_offset=0;
            const GpuTexture *stages[]{&textures.at(slot),&detail_textures.at(slot),
                &bump_textures.at(slot),baked_materials.at(slot)?
                    &lightmap_textures.at(lightmap):&auxiliary_textures.at(slot)};
            for(unsigned stage=0;stage<4;++stage) {
                const auto &animated=animated_textures.at(slot)[stage];
                if(!animated.empty())stages[stage]=&animated.at(bound_frames.at(slot)[stage]);
                const auto &image=*stages[stage];
                const auto mode=(input.material.program[54]>>(5*stage))&31u;
                if(image.source_image && image.source_shape.format==11 && !image.depth &&
                    ((mode==1 && image.view && image.source_shape.layers==1) ||
                     (mode==3 && image.cube_view && image.source_shape.layers==6)))
                    input.images[stage]=&image.source_shape;
            }
            return menu_world_native_packet(input);
        };
        for(const auto &candidate:frame.candidates) {
            const auto &key=candidate.original;
            const DrawKey identity{candidate.selected_material,key.lightmap,key.geometry,key.section,key.part};
            // Until selected LOD bindings are also applied to the packed
            // title token, preserve its complete base/fog pair as fallback.
            if(candidate.selected_material!=key.material) {++rejected["packed_lod_binding"];continue;}
            const auto found=physical_draws.find(identity);
            const auto &part=world->resident.parts().at(candidate.catalogue_index);
            if(found==physical_draws.end() || found->second->first!=part.first || found->second->count!=part.count) {
                ++rejected["packed_physical_range"];continue;
            }
            const auto &material=world->materials[candidate.selected_material];
            if(after_fog[candidate.selected_material] || water_materials[candidate.selected_material] ||
                environment_materials[candidate.selected_material]!=~0u ||
                !world_before[candidate.selected_material].empty() || !world_after[candidate.selected_material].empty()) {
                ++rejected["additional_material_passes"];continue;
            }
            try {
                if(selected.contains(identity))
                    throw FormatError("raw title ranges collide with an admitted complete draw key");
                // Validate the complete source-ordered range list before
                // claiming its packed whole-part base/fog fallback. A failure
                // in any partner leaves no orphan input or partial claim.
                std::vector<MenuWorldNativeDrawInput> candidate_inputs;
                std::vector<unsigned> candidate_slots;
                std::vector<MenuWorldDrawPair> pairs;
                pairs.reserve(candidate.ranges.size());
                for(const auto &range:candidate.ranges) {
                    auto base=packet(range.base,material,candidate.selected_material,key.lightmap,
                        MenuWorldRasterLayer::base_3,key.geometry);
                    std::optional<MenuWorldNativeDrawInput> fog_packet;
                    unsigned fog_slot=~0u;
                    if(range.fog) {
                        const auto &child=*range.fog;
                        const auto slot=material.world_planar_passes[child.implementation]?
                            world_planar_materials[candidate.selected_material][child.implementation]:
                            planar_materials[candidate.selected_material];
                        if(slot==~0u)throw FormatError("source-selected planar fog has no resident descriptor");
                        fog_slot=slot;
                        fog_packet=packet(child.binding,*child.material,slot,0,
                            MenuWorldRasterLayer::planar_fog_16,key.geometry);
                    }
                    const auto base_index=inputs.size()+candidate_inputs.size();
                    candidate_inputs.push_back(std::move(base));
                    candidate_slots.push_back(candidate.selected_material);
                    std::optional<std::size_t> fog_index;
                    if(fog_packet) {
                        fog_index=inputs.size()+candidate_inputs.size();
                        candidate_inputs.push_back(std::move(*fog_packet));
                        candidate_slots.push_back(fog_slot);
                    }
                    pairs.emplace_back(base_index,fog_index);
                }
                for(auto &input:candidate_inputs)inputs.push_back(std::move(input));
                input_slots.insert(input_slots.end(),candidate_slots.begin(),candidate_slots.end());
                // Empty is a successful complete source claim: suppress both
                // packed fallbacks without preparing or recording GPU draws.
                selected.emplace(identity,std::move(pairs));
            } catch(const FormatError &error) {++rejected[error.what()];}
        }
        try {
            if(!menu_world_native && !inputs.empty())menu_world_native=std::make_unique<MenuWorldNativeDraw>(
                MenuWorldNativeDrawContext{device,physical,render_pass,texture_layout,
                    enabled.instance_api_version,enabled.geometry_shader_enabled,
                    enabled.geometry_point_size_enabled,enabled.depth_clamp_enabled,enabled.demote_to_helper_enabled,shader_directory});
            if(menu_world_native)menu_world_native->prepare(inputs);
        } catch(const FormatError &error) {
            // Storage/pipeline admission is also atomic. A failed preparation
            // invalidates the service's frame, so retain every whole CPU pair.
            ++rejected[error.what()];
            selected.clear();inputs.clear();input_slots.clear();
        }
        if(!menu_world_reported) {
            // Diagnostic export of actual live inputs, never an admission
            // selector or a source of rendering constants.
            if(const auto *directory=std::getenv("HALO2_MENU_WORLD_TRACE");directory && *directory) {
                const std::filesystem::path root(directory);
                std::filesystem::create_directories(root);
                std::ofstream manifest(root/"draws.tsv");
                manifest<<"draw\towner\tpart\tvertex_shader\tmaterial\tindex_count\tsource_index_first\tsource_index_count\n";
                const auto write=[&](const std::filesystem::path &path,const void *data,std::size_t bytes) {
                    std::ofstream file(path,std::ios::binary);
                    if(!file.write(static_cast<const char *>(data),bytes))
                        throw FormatError("cannot write actual source title diagnostic packet");
                };
                for(std::size_t i=0;i<inputs.size();++i) {
                    const auto &input=inputs[i];const auto prefix=root/std::to_string(i);
                    const auto indices=input.source.draw().indices();
                    const auto range=input.source.draw().index_range();
                    manifest<<i<<'\t'<<input.source.owner().ordinal<<'\t'<<input.source.draw().part<<'\t'
                        <<input.source.vertex_shader()<<'\t'<<input_slots[i]<<'\t'<<indices.size()<<'\t'
                        <<range.first<<'\t'<<range.count<<'\n';
                    write(prefix.string()+".vs.bin",&input.vertex,sizeof(input.vertex));
                    write(prefix.string()+".fs.bin",&input.fragment,sizeof(input.fragment));
                    write(prefix.string()+".material.bin",&material_uniforms[input_slots[i]],sizeof(MaterialUniform));
                    write(prefix.string()+".indices.bin",indices.data(),indices.size()*sizeof(indices[0]));
                    write(prefix.string()+".program.bin",input.source.program_bytes().data(),input.source.program_bytes().size());
                }
                if(!manifest)throw FormatError("cannot write actual source title diagnostic manifest");
            }
            std::size_t base_count{},fog_count{};
            for(const auto &[identity,pairs]:selected) {
                base_count+=pairs.size();
                fog_count+=std::count_if(pairs.begin(),pairs.end(),[](const auto &pair){return bool(pair.second);});
            }
            std::cout<<"Source title world: raw_base="<<base_count<<" raw_fog="<<fog_count
                <<" paired_cpu_suppressed="<<selected.size()<<" candidates="<<frame.candidates.size()<<'\n';
            for(const auto &[reason,count]:rejected)
                std::cout<<"Source title world excluded: "<<reason<<" count="<<count<<'\n';
            menu_world_reported=true;
        }
        return selected;
    }
#endif
    void prepare_loading() {
        check(vkDeviceWaitIdle(device),"retire ordinary frame before loading");
        // ImGui belongs to the UI thread. The loading thread neither reads its
        // context nor initializes/shuts down its Vulkan backend.
        if(debug_graphics) {
            ImGui::SetCurrentContext(debug_panel->context());
            ImGui_ImplVulkan_Shutdown();debug_graphics=false;
        }
        debug_draw_data=nullptr;
    }
    bool draw_loading(unsigned base,std::span<const Material> materials,
                      std::span<const Vertex> vertices,const std::filesystem::path& screenshot) {
        if(materials.empty() || base>textures.size() || materials.size()>textures.size()-base ||
           vertices.empty() || vertices.size()%6 || vertices.size()>debug_capacity)
            throw FormatError("invalid resident Vista loading submission");
        for(unsigned i=0;i<materials.size();++i)
            if(!materials[i].vista_loading || !vista_loading_materials.at(base+i) ||
               materials[i].vista_loading->shader>3 || unsigned(materials[i].vista_loading->blend)>2)
                throw FormatError("Vista loading submission lacks an admitted resident slot");
        for(const auto& v:vertices)if(v.material>=materials.size())
            throw FormatError("Vista loading triangle addresses an absent resident slot");
        if(debug_graphics)throw FormatError("loading transfer did not retire the UI graphics owner");
        if(SDL_GetWindowFlags(window)&SDL_WINDOW_MINIMIZED)return false;
        int width{},height{};SDL_Vulkan_GetDrawableSize(window,&width,&height);
        if(width<=0 || height<=0)return false;
        if(unsigned(width)!=presentation_extent.width || unsigned(height)!=presentation_extent.height ||
           active_internal_width || active_internal_height || active_aa)dirty=true;
        // A departed renderer may outlive its CPU scene. The loading resize
        // path skips BSP postprocess reconstruction and every borrowed bank.
        if((dirty || !swapchain) && !create_swapchain(true))return false;
        check(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX),"wait loading frame fence");
        retire_weather_aliases();
        void *mapped{};
        check(vkMapMemory(device,debug_memory,0,vertices.size_bytes(),0,&mapped),"map loading triangles");
        std::memcpy(mapped,vertices.data(),vertices.size_bytes());vkUnmapMemory(device,debug_memory);
        std::uint32_t index{};
        const auto acquired_status=vkAcquireNextImageKHR(device,swapchain,UINT64_MAX,acquired,VK_NULL_HANDLE,&index);
        if(acquired_status==VK_ERROR_OUT_OF_DATE_KHR){dirty=true;return false;}
        if(acquired_status==VK_SUBOPTIMAL_KHR)dirty=true;
        else check(acquired_status,"acquire loading surface");
        VkBuffer capture{};VkDeviceMemory capture_memory{};
        struct Readback {
            Impl& owner;VkBuffer& buffer;VkDeviceMemory& memory;
            ~Readback(){if(buffer){vkDeviceWaitIdle(owner.device);vkDestroyBuffer(owner.device,buffer,nullptr);}
                if(memory)vkFreeMemory(owner.device,memory,nullptr);}
        } readback{*this,capture,capture_memory};
        const VkDeviceSize capture_size=VkDeviceSize(extent.width)*extent.height*4;
        if(!screenshot.empty()) {
            if(!can_capture)throw FormatError("loading surface is not readable");
            make_buffer(capture_size,VK_BUFFER_USAGE_TRANSFER_DST_BIT,capture,capture_memory);
        }
        check(vkResetCommandBuffer(command,0),"reset loading commands");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command,&begin),"begin loading commands");
        const VkClearValue clears[]{VkClearValue{.color={{0,0,0,1}}},
                                    VkClearValue{.depthStencil={1,0}}};
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        render.renderPass=render_pass;render.framebuffer=framebuffers.at(index);
        render.renderArea={{0,0},extent};render.clearValueCount=2;render.pClearValues=clears;
        vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
        const VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
        const VkRect2D scissor{{0,0},extent};vkCmdSetScissor(command,0,1,&scissor);
        const std::uint32_t bank=0;
        for(std::size_t first=0;first<vertices.size();) {
            const auto slot=vertices[first].material;std::size_t end=first+1;
            while(end<vertices.size() && vertices[end].material==slot)++end;
            const auto& material=*materials[slot].vista_loading;
            struct Push {ShaderVector constant0;Vec2 reserved_offset;std::uint32_t program,reserved;};
            static_assert(sizeof(Push)==32);
            const Push push{material.constant0,{},material.shader,0};
            const VkViewport viewport{material.raster_offset_pixels.x,material.raster_offset_pixels.y,
                float(extent.width),float(extent.height),0,1};
            vkCmdSetViewport(command,0,1,&viewport);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,vista_loading_pipelines.at(unsigned(material.blend)));
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,vista_loading_layout,0,1,
                &textures.at(base+slot).descriptor,1,&bank);
            vkCmdPushConstants(command,vista_loading_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                0,sizeof(push),&push);
            vkCmdDraw(command,std::uint32_t(end-first),1,std::uint32_t(first),0);first=end;
        }
        vkCmdEndRenderPass(command);
        if(capture) {
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=images.at(index);barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
            copy.imageExtent={extent.width,extent.height,1};
            vkCmdCopyImageToBuffer(command,images[index],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,capture,1,&copy);
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=0;
            barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.buffer=capture;host.size=VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,
                0,0,nullptr,1,&host,0,nullptr);
        }
        check(vkEndCommandBuffer(command),"end loading commands");
        check(vkResetFences(device,1,&fence),"reset loading frame fence");
        const VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&acquired;submit.pWaitDstStageMask=&stage;
        submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&present_ready[index];
        check(vkQueueSubmit(queue,1,&submit,fence),"submit loading frame");
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount=1;present.pWaitSemaphores=&present_ready[index];
        present.swapchainCount=1;present.pSwapchains=&swapchain;present.pImageIndices=&index;
        const auto status=vkQueuePresentKHR(queue,&present);
        if(status==VK_ERROR_OUT_OF_DATE_KHR || status==VK_SUBOPTIMAL_KHR)dirty=true;
        else check(status,"present loading frame");
        if(capture) {
            check(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX),"wait loading readback");
            check(vkMapMemory(device,capture_memory,0,capture_size,0,&mapped),"map loading readback");
            struct Mapping {VkDevice device;VkDeviceMemory memory;~Mapping(){vkUnmapMemory(device,memory);}} mapping{device,capture_memory};
            if(!screenshot.parent_path().empty())std::filesystem::create_directories(screenshot.parent_path());
            std::ofstream file(screenshot,std::ios::binary);
            if(!file)throw FormatError("cannot write loading screenshot");
            file<<"P6\n"<<extent.width<<' '<<extent.height<<"\n255\n";
            const auto *pixels=static_cast<const unsigned char*>(mapped);
            const bool bgra=format==VK_FORMAT_B8G8R8A8_SRGB || format==VK_FORMAT_B8G8R8A8_UNORM;
            std::vector<char> rgb(std::size_t(extent.width)*extent.height*3);
            for(std::size_t i=0;i<std::size_t(extent.width)*extent.height;++i) {
                rgb[i*3]=char(pixels[i*4+(bgra?2:0)]);
                rgb[i*3+1]=char(pixels[i*4+1]);
                rgb[i*3+2]=char(pixels[i*4+(bgra?0:2)]);
            }
            file.write(rgb.data(),std::streamsize(rgb.size()));
            if(!file)throw FormatError("failed writing loading screenshot");
        }
        return true;
    }
    bool draw(const Camera& camera, const std::filesystem::path& screenshot, std::span<const Vertex> debug, bool reticle, std::span<const Vertex> first_person, ScreenEffects effects, std::span<const Vertex> ui, std::span<const ModelView> views, std::span<const Vertex> background, std::span<const UiBackdrop> backdrops, const BspFogSample *mainmenu_fog, const MainmenuWorldDraw *mainmenu_world, bool ui_full_surface_bounds) {
        // Reject unsupported captured resource/history states before recording
        // commands or resizing any attachments. Admitted plans have no
        // viewport mutation, so their operands do not depend on native scale.
        std::vector<std::vector<SourceClearEvent>> source_initializations(views.size());
        for(std::size_t i=0;i<views.size();++i)if(views[i].source_view_clear) {
            const auto &source=*views[i].source_view_clear;
            auto &events=source_initializations[i];
            events=source_view_initialization_plan(source.header,source.transition_byte,
                source.x87_control_word,{0,0,640,480,0,0x3f800000});
            if(!views[i].full_surface || events.size()!=2 ||
               events[0].words!=std::array<std::uint32_t,6>{0,1,0,0,0,0} ||
               events[1].kind!=SourceClearEvent::Kind::clear || events[1].words[0]!=0xf3)
                throw FormatError("Captured source initialization needs unsupported resource/history admission");
        }
        if(mainmenu_fog && !atmosphere)
            throw FormatError("current mainmenu fog requires the bound title atmosphere");
        if(mainmenu_world && mainmenu_fog!=&mainmenu_world->frame.fog)
            throw FormatError("title source world and fog do not share the current retained frame");
        std::optional<std::filesystem::path> haze_trace_path;
        frame_timing={};
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) return false;
        const auto display=debug_panel?debug_panel->display_settings():DisplaySettings{};
        if(display.internal_width!=active_internal_width || display.internal_height!=active_internal_height ||
           display.anti_aliasing!=active_aa || display.detail!=active_detail)dirty=true;
        if ((dirty || !swapchain) && !create_swapchain()) return false;
        using TimingClock=std::chrono::steady_clock;
        TimingClock::time_point timing_phase, timing_start;
        double timing_interval{};
        if(measure_frames) {
            timing_phase=TimingClock::now();timing_start=timing_phase;frame_timing.measured=true;
            if(timing_report.is_open()) {
                if(timing_frame)timing_interval=std::chrono::duration<double,std::milli>(timing_start-timing_previous).count();
                timing_previous=timing_start;
            }
            frame_timing.materials=material_uniforms.size();frame_timing.views=views.size();
            frame_timing.width=extent.width;frame_timing.height=extent.height;
        }
        const auto measure=[&](double &elapsed) {
            if(!measure_frames)return;
            const auto now=TimingClock::now();
            elapsed=std::chrono::duration<double,std::milli>(now-timing_phase).count();
            timing_phase=now;
        };
        const auto *serial_geometry=std::getenv("HALO2_REFERENCE_SERIAL_GEOMETRY");
        const bool overlap_geometry=!(serial_geometry && *serial_geometry);
        if(!overlap_geometry) {
            check(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX),"wait reference frame fence");
            measure(frame_timing.fence_ms);
        }
        // CPU-owned planning/staging may overlap the previous submission.
        // Descriptor writes, mapped GPU writes and resource lifetimes remain
        // protected by the frame fence below.
        if(views.size()>4)throw FormatError("too many simultaneous model viewports");
        std::vector<SourceShadowPlanStats> shadow_stats(views.size());
        std::size_t shadow_draw_count=0;
        for(std::size_t i=0;i<views.size();++i)if(views[i].source_shadows) {
            shadow_stats[i]=validate_source_shadow_plan(*views[i].source_shadows,shadow_materials);
            if(shadow_stats[i].roots && (!shadow_render || !shadow_storage))
                throw FormatError("source shadow plan has no resident render service");
            if(shadow_stats[i].roots) {
                if(!views[i].atmosphere || !views[i].source_view_shader_constants)
                    throw FormatError("source shadow receiver requires actual world W-depth camera context");
                const auto &world=views[i].source_view_shader_constants->world;
                if(std::memcmp(world.data(),views[i].source_shadows->camera.data(),sizeof(world))!=0)
                    throw FormatError("source shadow camera differs from submitted world W-depth bank");
            }
            shadow_draw_count+=shadow_stats[i].caster_draws+shadow_stats[i].receiver_draws;
        }
        std::size_t effect_light_draw_count=0;
        std::size_t raw_effect_light_draw_count=0;
        for(const auto &view:views) {
            if((!view.effect_lights.empty() || !view.raw_effect_lights.empty()) &&
                (!view.atmosphere || !view.source_view_shader_constants))
                throw FormatError("Sphere diffuse draw requires actual world W-depth camera context");
            if(!view.effect_lights.empty() && !view.raw_effect_lights.empty())
                throw FormatError("Sphere diffuse view mixes two unordered geometry representations");
            for(const auto &draw:view.raw_effect_lights) {
                if(draw.material_slot>=effect_light_materials.size() || !effect_light_materials[draw.material_slot] ||
                   draw.vertex_bank.range!=std::array<std::uint32_t,4>{34,6,0,0} ||
                   std::memcmp(draw.source.constants().data(),view.source_view_shader_constants->world.data(),
                       sizeof(view.source_view_shader_constants->world)))
                    throw FormatError("Raw sphere diffuse draw differs from its world camera/material binding");
                ++raw_effect_light_draw_count;
            }
            for(const auto &draw:view.effect_lights) {
                if(draw.material_slot>=effect_light_materials.size() || !effect_light_materials[draw.material_slot] ||
                   draw.vertices.size()%3 || draw.vertex_bank.range!=std::array<std::uint32_t,4>{34,6,0,0})
                    throw FormatError("Invalid explicit sphere diffuse draw binding");
                for(const auto &vertex:draw.vertices)if(vertex.material!=draw.material_slot)
                    throw FormatError("Sphere diffuse draw changed resident material identity");
                ++effect_light_draw_count;
            }
        }
        auto required = debug.size() + first_person.size() + ui.size() + background.size();
        std::array<std::optional<SourceBspMergedDrawPlan>,4> fresh_bsp_plans;
        std::array<const SourceBspMergedDrawPlan *,4> bsp_plans{};
        const auto *fresh_plans=std::getenv("HALO2_REFERENCE_FRESH_BSP_PLANS");
        const auto *copy_bsp_objects=std::getenv("HALO2_REFERENCE_COPY_BSP_OBJECTS");
        const bool borrow_bsp_objects=!(copy_bsp_objects && *copy_bsp_objects);
        std::size_t previous_ui = 0;
        for(std::size_t i=0;i<views.size();++i) {
            const auto &view=views[i];
            if(view.before_ui_vertex < previous_ui || view.before_ui_vertex > ui.size() || view.before_ui_vertex % 3 || view.vertices.size()%3)
                throw std::runtime_error("invalid UI model view ordering");
            previous_ui=view.before_ui_vertex;
            if(view.source_bsp_resident_plan) {
                if(view.source_bsp_resident_plan!=bsp_resident_plan)
                    throw FormatError("view BSP resident plan is not the validated renderer atlas");
                if(fresh_plans && *fresh_plans) {
                    fresh_bsp_plans[i]=bsp_resident_plan->merge(view.source_bsp_bindings,view.vertices,
                        unsigned(material_count),unsigned(lightmap_textures.size()));
                    bsp_plans[i]=&*fresh_bsp_plans[i];
                } else {
                    bsp_resident_plan->merge_into(bsp_frame_plans[i],view.source_bsp_bindings,view.vertices,
                        unsigned(material_count),unsigned(lightmap_textures.size()),0,0,borrow_bsp_objects);
                    bsp_plans[i]=&bsp_frame_plans[i];
                }
                required+=bsp_plans[i]->dynamic_vertices.size()+bsp_plans[i]->borrowed_object_vertices;
            } else {
                if(!view.source_bsp_bindings.empty())throw FormatError("view BSP bindings have no resident owner");
                required+=view.vertices.size();
            }
            for(const auto &object:view.objects) {
                if(object.vertices.size()%3)throw FormatError("incomplete owner draw triangles");
                required+=object.vertices.size();
            }
            const auto transparent_size=[&](const auto &bank,std::span<const ParticleDraw> particles) {
                if(!bank)return;
                if(bank->commands.size()>1024 || bank->first>bank->commands.size() ||
                   bank->bindings.size()!=bank->commands.size())
                    throw FormatError("source transparent drain has invalid command/binding range");
                for(unsigned j=bank->first;j<bank->commands.size();++j) {
                    const auto &command=bank->commands[j];if(!command.callback)continue;
                    std::array<std::uint32_t,2> payload;
                    std::memcpy(payload.data(),command.payload.data(),8);
                    const auto *shader_geometry=std::get_if<SourceTransparentShaderGeometryDraw>(&bank->bindings[j]);
                    if(command.callback==0x41020 || shader_geometry) {
                        const auto *geometry=shader_geometry?&shader_geometry->geometry:
                            std::get_if<SourceTransparentGeometryDraw>(&bank->bindings[j]);
                        if(shader_geometry && shader_geometry->callback!=command.callback)
                            throw FormatError("source sorted shader callback lost its actual adapter identity");
                        if(!geometry || geometry->shader!=payload[0] || geometry->draw_token!=payload[1] ||
                           geometry->vertices.size()%3)
                            throw FormatError("source transparent geometry callback lost its actual payload/triangles");
                        for(const auto &vertex:geometry->vertices) {
                            if(vertex.material>=material_count ||
                               source_material_identities[vertex.material]!=std::array<unsigned,2>{geometry->shader,0} ||
                               source_layer15_passes[vertex.material].empty())
                                throw FormatError("source transparent callback requires resident shader LOD0/layer15/implementation0");
                        }
                        required+=geometry->vertices.size();
                    } else if(const auto *binding=std::get_if<SourceTransparentContrailDraw>(&bank->bindings[j])) {
                        if(!binding->draw || std::memcmp(&command,&binding->draw->command,sizeof(command)))
                            throw FormatError("Source contrail callback lost its actual record owner");
                    } else if(command.callback==source_light_volume_callback) {
                        const auto *binding=std::get_if<SourceTransparentLightVolumeDraw>(&bank->bindings[j]);
                        if(!binding || !binding->draw || binding->draw->object!=payload[0] || binding->draw->definition!=payload[1] ||
                           binding->draw->material>=light_volume_slots.size() || !light_volume_slots[binding->draw->material] ||
                           std::memcmp(&command,&binding->draw->command,sizeof(command)))
                            throw FormatError("Source light-volume callback lost its actual record/material owner");
                    } else if(command.callback==0x173c90) {
                        const auto *range=std::get_if<SourceTransparentParticleRange>(&bank->bindings[j]);
                        if(!range || range->system!=payload[0] || range->location!=payload[1] ||
                           range->first>particles.size() || range->count>particles.size()-range->first)
                            throw FormatError("source transparent particle callback lost its actual location/range");
#ifndef HALO2_PARTICLE_GRAPHICS_AVAILABLE
                        if(range->count)throw FormatError("source transparent particle graphics unavailable");
#endif
                    } else throw FormatError("source transparent callback service is unresolved");
                }
            };
            for(const auto &draw:view.light_volumes) {
                if(draw.bitmap_materials.empty())throw FormatError("View light-volume submission has no actual bitmap bindings");
                for(const auto &binding:draw.bitmap_materials)
                    if(binding.material>=light_volume_slots.size() || !light_volume_slots[binding.material])
                        throw FormatError("View light-volume bitmap lost its resident original program");
                if(draw.material>=light_volume_slots.size() || !light_volume_slots[draw.material] ||
                   draw.command.callback!=source_light_volume_callback || draw.object==0xffffffffu || draw.definition==0xffffffffu)
                    throw FormatError("View light-volume submission lost its live owner/material");
            }
            transparent_size(view.world_transparent,view.world_particles);
            transparent_size(view.foreground_transparent,view.foreground_particles);
            required+=view.foreground.size()+view.background.size()+view.water_reflection_background.size();
            required+=shadow_stats[i].vertices;
            for(const auto &draw:view.effect_lights)required+=draw.vertices.size();
        }
        if (required > dynamic_limit) throw std::runtime_error("dynamic geometry capacity exceeded");
        std::vector<Draw> dynamic_draws, foreground_draws, background_draws;
        auto &dynamic=dynamic_vertex_staging;
        dynamic.clear();
        // The reference staging path reserves the validated upper bound once.
        // The default plan borrows the same ordered spans until GPU upload.
        const auto *copy_frame=std::getenv("HALO2_REFERENCE_COPY_FRAME_VERTICES");
        const bool borrowed_frame=!(copy_frame && *copy_frame);
        const bool verify_frame=std::getenv("HALO2_VERIFY_BORROWED_FRAME_VERTICES")!=nullptr;
        const bool verify_geometry=std::getenv("HALO2_VERIFY_FRAME_GEOMETRY")!=nullptr;
        const bool copy_vertices=!borrowed_frame || verify_frame;
        const bool plan_vertices=borrowed_frame || verify_frame;
        if(copy_vertices)dynamic.reserve(required);
        const auto *unshared=std::getenv("HALO2_REFERENCE_UNSHARED_VERTICES");
        FrameVertexReuse<DrawKey> vertex_reuse(dynamic,!(unshared && *unshared));
        FrameVertexUploadPlan<DrawKey> upload_plan(!(unshared && *unshared));
        const auto vertex_count=[&]{return copy_vertices?dynamic.size():upload_plan.size();};
        const auto append_frame_parts=[&](const DrawKey &key,const auto &parts) {
            if(verify_geometry)for(const auto part:parts) {
                if(part.size()%3)throw FormatError("Frame geometry splits a triangle between draw ranges");
                for(const auto &vertex:part) {
                    const auto p=vertex.position;
                    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
                        throw FormatError("Frame geometry contains a nonfinite position");
                    if(draw_key(vertex)!=key)
                        throw FormatError("Frame geometry groups a vertex under a different draw identity");
                }
            }
            std::size_t first{};
            if(copy_vertices)first=vertex_reuse.append_parts(key,parts);
            if(plan_vertices) {
                const auto planned=upload_plan.append_parts(key,parts);
                if(copy_vertices && planned!=first)throw FormatError("borrowed frame vertex offset differs from original staging");
                first=planned;
            }
            return first;
        };
        const auto append_frame=[&](const DrawKey &key,std::span<const Vertex> vertices) {
            return append_frame_parts(key,std::array{vertices});
        };
        const auto append_raw_frame=[&](std::span<const Vertex> vertices) {
            if(copy_vertices)dynamic.insert(dynamic.end(),vertices.begin(),vertices.end());
            if(plan_vertices)upload_plan.append_raw(vertices);
        };
        // The only edited upload source is the full-surface UI tail. Keep
        // its private copy alive through upload; other spans stay immutable.
        std::vector<Vertex> adjusted_ui;
        std::ofstream group_input_layout,group_input_vertices;
        if(!group_input_diagnostic_done)if(const auto *path=std::getenv("HALO2_GROUP_INPUT_DIAGNOSTICS")) {
            group_input_layout.open(std::string(path)+".tsv");
            group_input_vertices.open(std::string(path)+".vertices.bin",std::ios::binary);
            if(!group_input_layout || !group_input_vertices)throw FormatError("open source geometry grouping diagnostics");
            group_input_layout<<"stride\t"<<sizeof(Vertex)<<"\nmaterials\t"<<material_count
                <<"\nlightmaps\t"<<lightmap_textures.size()<<"\ncall\tpreserve_order\tfirst_input_vertex\tvertices\n";
            group_input_diagnostic_done=true;
        }
        std::size_t group_input_call{},group_input_first{};
        auto group = [&](std::span<const Vertex> vertices, std::vector<Draw>& list,
                         bool preserve_order=false) {

            if(group_input_layout.is_open()) {
                group_input_layout<<group_input_call++<<'\t'<<preserve_order<<'\t'<<group_input_first<<'\t'<<vertices.size()<<'\n';
                if(!vertices.empty())group_input_vertices.write(reinterpret_cast<const char*>(vertices.data()),
                    std::streamsize(vertices.size_bytes()));
                group_input_first+=vertices.size();
            }
            if(preserve_order) {
                // Preserve each contiguous authored alpha-layer run and every
                // draw. Only its identical vertex storage may be shared.
                for(std::size_t i=0;i<vertices.size();) {
                    const auto &v=vertices[i];
                    if(v.material>=material_count || v.lightmap>=lightmap_textures.size())
                        throw std::runtime_error("invalid dynamic material/lightmap index");
                    const auto key=draw_key(v);auto end=i+1;
                    while(end<vertices.size() && draw_key(vertices[end])==key)++end;
                    const auto first=append_frame(key,vertices.subspan(i,end-i));
                    list.push_back({std::uint32_t(first),std::uint32_t(end-i),
                        std::get<0>(key),std::get<1>(key),VK_NULL_HANDLE,
                        std::get<2>(key),std::get<3>(key),std::get<4>(key)});
                    i=end;
                }
                return;
            }
            struct Run {std::size_t first{},count{};};
            struct Pack {unsigned count{};std::vector<Run> runs;};
            std::map<DrawKey,Pack> packs;
            for(std::size_t i=0;i<vertices.size();) {
                const auto &v=vertices[i];
                if(v.material>=material_count || v.lightmap>=lightmap_textures.size())
                    throw std::runtime_error("invalid dynamic material/lightmap index");
                const auto key=draw_key(v);auto end=i+1;
                while(end<vertices.size() && draw_key(vertices[end])==key)++end;
                auto &pack=packs[key];pack.runs.push_back({i,end-i});pack.count+=unsigned(end-i);i=end;
            }
            for(const auto &[key,pack]:packs) {
                const auto parts=std::span{pack.runs} | std::views::transform([&](const Run &run){return vertices.subspan(run.first,run.count);});
                const auto first=append_frame_parts(key,parts);
                list.push_back({std::uint32_t(first),pack.count,std::get<0>(key),std::get<1>(key),
                    VK_NULL_HANDLE,std::get<2>(key),std::get<3>(key),std::get<4>(key)});
            }
        };
        group(background, background_draws,true);
        group(debug, dynamic_draws);
        group(first_person, foreground_draws);
        std::vector<std::vector<Draw>> view_draws;
        struct ObjectDrawRange {std::size_t view{},object{},first{},count{};};
        std::vector<ObjectDrawRange> object_ranges;
        for(std::size_t i=0;i<views.size();++i) {
            const auto &view=views[i];view_draws.emplace_back();
            if(!bsp_plans[i])group(view.vertices,view_draws.back());
            else {
            const auto &plan=*bsp_plans[i];
            if(group_input_layout.is_open()) {
                auto input=bsp_resident_plan->expand(view.source_bsp_bindings);
                input.insert(input.end(),view.vertices.begin(),view.vertices.end());
                group_input_layout<<group_input_call++<<"\t0\t"<<group_input_first<<'\t'<<input.size()<<'\n';
                if(!input.empty())group_input_vertices.write(reinterpret_cast<const char*>(input.data()),
                    std::streamsize(input.size()*sizeof(Vertex)));
                group_input_first+=input.size();
            }
            auto &list=view_draws.back();list.reserve(plan.draws.size());
            for(const auto &token:plan.draws) {
                const auto &key=token.key;
                const bool resident=token.storage==SourceBspVertexStorage::resident;
                const DrawKey identity{key.material,key.lightmap,key.geometry,key.section,key.part};
                const auto input=token.storage==SourceBspVertexStorage::objects?view.vertices:std::span<const Vertex>{plan.dynamic_vertices};
                auto first=token.first;
                if(token.storage==SourceBspVertexStorage::object_runs) {
                    const auto runs=std::span{plan.borrowed_runs}.subspan(token.first,token.run_count);
                    const auto spans=runs | std::views::transform([&](const auto &run) {
                        return view.vertices.subspan(run.first,run.count);
                    });
                    first=std::uint32_t(append_frame_parts(identity,spans));
                } else if(!resident)first=std::uint32_t(append_frame(identity,input.subspan(token.first,token.count)));
                list.push_back({first,token.count,key.material,key.lightmap,VK_NULL_HANDLE,
                    key.geometry,key.section,key.part,resident});
            }
            }
            for(const auto &object:view.objects) {
                auto &list=view_draws.back();const auto first=list.size();
                group(object.vertices,list);
                object_ranges.push_back({i,std::size_t(&object-view.objects.data()),first,list.size()-first});
            }
        }
        struct TransparentStep {
            std::vector<Draw> geometry;
            std::optional<SourceTransparentParticleRange> particles;
            std::vector<LightVolumeDraw> lights;
            const SourceLightVolumeDraw *light_source{};
            const SourceContrailDraw *contrail_source{};
            bool first_person_projection{};
            const ObjectLightingState *lighting{};
        };
        light_volume_staging.clear();
        std::vector<std::deque<SourceLightVolumeDraw>> projected_lights(views.size());
        std::vector<bool> sorted_world_effects(views.size());
        std::vector<std::array<std::vector<TransparentStep>,2>> transparent_views(views.size());
        for(std::size_t i=0;i<views.size();++i)for(unsigned phase=0;phase<2;++phase) {
            const auto &view=views[i];const auto &bank=phase?view.foreground_transparent:view.world_transparent;
            if(!bank && (phase || (view.light_volumes.empty() && view.contrails.empty())))continue;
            std::vector<SourceTransparentCommand> commands;
            std::vector<SourceTransparentDrawBinding> bindings;
            unsigned first{};
            if(bank) {
                commands.assign(bank->commands.begin(),bank->commands.end());
                bindings.assign(bank->bindings.begin(),bank->bindings.end());first=bank->first;
            }
            const auto append=[&](const SourceTransparentCommand &command,SourceTransparentDrawBinding binding) {
                if(commands.size()==1024)return;
                commands.push_back(command);bindings.push_back(std::move(binding));
            };
            if(!phase) {
                for(const auto &draw:view.contrails)append(draw.command,SourceTransparentContrailDraw{&draw});
                for(const auto &draw:view.light_volumes)append(draw.command,SourceTransparentLightVolumeDraw{&draw});
                if(!bank) {
                    // Native world effects retain their own system/location namespace.
                    // Their completed175EA0 center/depth becomes the original type3
                    // comparison key; GPU ranges still reference actual native draws.
                    for(unsigned n=0;n<view.world_particles.size();) {
                        const auto &p=view.world_particles[n];
                        if(!p.scene_sort_depth || !p.scene_sort_owner)
                            throw FormatError("Mixed world effect sorting requires its actual location/center producer");
                        const auto &owner=*p.scene_sort_owner;unsigned count=1;
                        while(n+count<view.world_particles.size()) {
                            const auto &next=view.world_particles[n+count];
                            if(!next.scene_sort_owner || next.scene_sort_owner->system!=owner.system || next.scene_sort_owner->location!=owner.location)break;
                            if(next.scene_sort_depth!=p.scene_sort_depth)throw FormatError("One particle location has inconsistent source depth");
                            ++count;
                        }
                        if(owner.system>0xffffffffu || owner.location>0xffffffffu)
                            throw FormatError("Native particle identity exceeds its transparent token adapter");
                        SourceTransparentCommand command{};command.priority=3;command.depth=*p.scene_sort_depth;
                        command.callback=0x173c90;command.center=owner.center;
                        const std::array<std::uint32_t,2> payload{std::uint32_t(owner.system),std::uint32_t(owner.location)};
                        std::memcpy(command.payload.data(),payload.data(),8);
                        append(command,SourceTransparentParticleRange{payload[0],payload[1],n,count});n+=count;
                    }
                }
                sorted_world_effects[i]=true;
            }
            const auto order=source_transparent_sort(commands,view.camera.position,first);
            for(auto ordinal:order) {
                if(!commands[ordinal].callback)continue;
                auto &step=transparent_views[i][phase].emplace_back();
                const auto &binding=bindings[ordinal];
                if(const auto *trail=std::get_if<SourceTransparentContrailDraw>(&binding)) {
                    if(phase || !trail->draw)throw FormatError("Contrail draw requires its actual ordinary world view");
                    step.contrail_source=trail->draw;continue;
                }
                if(const auto *light=std::get_if<SourceTransparentLightVolumeDraw>(&binding)) {
                    if(phase || !light->draw || !view.full_surface)
                        throw FormatError("Light-volume draw requires its actual ordinary gameplay view");
                    auto &draw=projected_lights[i].emplace_back(*light->draw);
                    if(!draw.volume || !draw.tables || draw.actual_w_distance!=1024.f ||
                       !(draw.viewport.width>0 && draw.viewport.height>0))
                        throw FormatError("Light-volume draw lost its completed projection/definition inputs");
                    const auto physical=full_surface_viewport({view.left,view.top,view.width,view.height},extent.width,extent.height);
                    if(physical.width!=draw.viewport.width)draw.camera.viewport_half_metrics.x*=physical.width/draw.viewport.width;
                    if(physical.height!=draw.viewport.height)draw.camera.viewport_half_metrics.y*=physical.height/draw.viewport.height;
                    draw.viewport=physical;
                    auto projected=source_light_volume_vertices(*draw.volume,draw.state,draw.camera,physical,draw.actual_w_distance,*draw.tables,draw.bitmap_materials);
                    draw.vertices=std::move(projected.vertices);draw.ranges=std::move(projected.ranges);
                    const auto first_vertex=std::uint32_t(light_volume_staging.size());
                    light_volume_staging.insert(light_volume_staging.end(),draw.vertices.begin(),draw.vertices.end());
                    for(const auto &range:draw.ranges)
                        step.lights.push_back({first_vertex+range.first,range.count,range.material,draw.command.depth});
                    step.light_source=&draw;continue;
                }
                const auto *shader_geometry=std::get_if<SourceTransparentShaderGeometryDraw>(&binding);
                const auto *geometry=shader_geometry?&shader_geometry->geometry:
                    std::get_if<SourceTransparentGeometryDraw>(&binding);
                if(geometry) {
                    group(geometry->vertices,step.geometry,true);
                    step.lighting=geometry->lighting?&*geometry->lighting:nullptr;
                    step.first_person_projection=geometry->render_cache_flags&0x800;
                } else step.particles=std::get<SourceTransparentParticleRange>(binding);
            }
        }
        std::vector<std::vector<Draw>> view_foreground;
        for(const auto& view:views) {view_foreground.emplace_back();group(view.foreground,view_foreground.back());}
        std::vector<std::vector<Draw>> view_background;
        for(const auto& view:views) {view_background.emplace_back();group(view.background,view_background.back(),true);}
        std::vector<std::vector<Draw>> view_reflection_background;
        for(const auto& view:views) {view_reflection_background.emplace_back();group(view.water_reflection_background,view_reflection_background.back(),true);}
        std::vector<std::uint32_t> shadow_vertex_first;
        shadow_vertex_first.reserve(shadow_draw_count);
        for(const auto &view:views)if(view.source_shadows)
            for(const auto &root:view.source_shadows->roots) {
                if(root.casters.empty() || root.receivers.empty())continue;
                for(unsigned phase=0;phase<2;++phase)for(const auto &draw:phase?root.receivers:root.casters) {
                    shadow_vertex_first.push_back(std::uint32_t(vertex_count()));
                    append_raw_frame(draw.vertices);
                }
            }
        std::vector<unsigned> light_vertex_first;light_vertex_first.reserve(effect_light_draw_count);
        for(const auto &view:views)for(const auto &draw:view.effect_lights) {
            light_vertex_first.push_back(unsigned(vertex_count()));
            append_raw_frame(draw.vertices);
        }
        const auto ui_first = static_cast<std::uint32_t>(vertex_count());
        if (ui.size() % 3) throw std::runtime_error("incomplete UI triangles");
        for (const auto& v : ui) if (v.material >= textures.size()) throw std::runtime_error("invalid UI material");
        if(plan_vertices && ui_full_surface_bounds)adjusted_ui.assign(ui.begin(),ui.end());
        if(copy_vertices)dynamic.insert(dynamic.end(),ui.begin(),ui.end());
        if(plan_vertices)upload_plan.append_raw(ui_full_surface_bounds?std::span<const Vertex>{adjusted_ui}:ui);
        const bool full_surface_scene=std::any_of(views.begin(),views.end(),[](const auto &v){return v.full_surface;});
        // Match the command recorder's global scene admission. Model views
        // prepare their own direction/combiner banks independently below.
        std::vector<bool> global_material_roots(material_count);
        const auto use_global=[&](unsigned material) {global_material_roots[material_roots.at(material)]=true;};
        if(!full_surface_scene)for(const auto &draw:draws)use_global(draw.material);
        for(const auto *bank:{&background_draws,&dynamic_draws,&foreground_draws})
            for(const auto &draw:*bank)use_global(draw.material);
        for(const auto &vertex:ui)use_global(vertex.material);
        measure(frame_timing.geometry_plan_ms);
        frame_timing.geometry_pack_ms=frame_timing.geometry_plan_ms;
        if(overlap_geometry) {
            check(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX),"wait frame fence");
            measure(frame_timing.fence_ms);
        }
        debug_draw_data=nullptr;
        if(debug_panel && debug_panel->visible()) {
            ImGui::SetCurrentContext(debug_panel->context());
            if(!debug_graphics) {
                ImGui_ImplVulkan_InitInfo info{};
                info.ApiVersion=VK_API_VERSION_1_1;
                info.Instance=instance;info.PhysicalDevice=physical;info.Device=device;
                info.QueueFamily=queue_family;info.Queue=queue;
                info.RenderPass=render_pass;
                info.MinImageCount=2;info.ImageCount=static_cast<std::uint32_t>(images.size());
                info.MSAASamples=VK_SAMPLE_COUNT_1_BIT;
                info.DescriptorPoolSize=64;
                info.CheckVkResultFn=[](VkResult result){check(result,"optional debug graphics");};
                if(!ImGui_ImplVulkan_Init(&info))throw FormatError("cannot initialize optional debug graphics");
                debug_graphics=true;
            }
            ImGui_ImplVulkan_NewFrame();
            debug_draw_data=debug_panel->build_frame();
        }
        const bool crt_enabled=debug_panel && debug_panel->display_settings().crt_filter;
        if(crt_enabled)make_crt_pipeline();
        const bool display_correction=display.brightness!=3 || display.gamma!=0;
        if(display_correction)make_display_pipeline();
        if(backdrops.size()>popup_channel_count)throw FormatError("modal UI channel count exceeds allocated blur slots");
        std::size_t previous_boundary=0;
        for(const auto &backdrop:backdrops) {
            if(backdrop.before_vertex<previous_boundary || backdrop.before_vertex>ui.size() ||
                backdrop.before_vertex%3 || !std::isfinite(backdrop.fade))
                throw FormatError("invalid modal UI backdrop boundary");
            previous_boundary=backdrop.before_vertex;
        }
        const glm::vec3 forward=camera.oriented?glm::vec3(camera.forward.x,camera.forward.y,camera.forward.z):
            glm::vec3(std::cos(camera.pitch)*std::cos(camera.yaw),std::cos(camera.pitch)*std::sin(camera.yaw),std::sin(camera.pitch));
        const auto menu_canvas=frontend_canvas(extent.width,extent.height);
        const bool title_scene=atmosphere && !background.empty();
        const auto menu_projection=title_scene?
            std::optional(menu_full_frame_projection(camera,extent.width,extent.height)):std::nullopt;
        const auto &scene_camera=menu_projection?menu_projection->camera:camera;
        VkViewport viewport{0,0,float(extent.width),float(extent.height),0,1};
        VkRect2D scissor{{0,0},extent};
        if(!ui.empty() && !title_scene) {
            viewport={menu_canvas.left,menu_canvas.top,menu_canvas.width,menu_canvas.height,0,1};
            scissor={{int(viewport.x),int(viewport.y)},{unsigned(viewport.width),unsigned(viewport.height)}};
        }
        // 2F970 ->2FB70 ->2FD90 ->12FA0 supplies the original mainmenu
        // c0..6 bank. Native aspect extension scales only clip X/Y to reveal
        // the map around its original center projection.
        // ModelView banks clone this base later and replace its camera.
        const auto title_camera=title_scene?
            std::optional(source_view_shader_constants(menu_projection->bank,1024.f,false)):
            std::nullopt;
        if(!base_camera_initialized || base_camera!=title_camera) {
        for(std::size_t i=0;i<material_uniforms.size();++i) {
            auto &u=material_uniforms[i];
            const std::array<std::uint32_t,4> control{title_camera?1u:0u,0,0,0};
            if(u.source_camera_control==control && (!title_camera ||
                (u.source_camera_world==*title_camera && u.source_camera_reflection==*title_camera)))continue;
            u.source_camera_control=control;
            if(title_camera)u.source_camera_world=u.source_camera_reflection=*title_camera;
            mark_material_changed(i);
        }
        base_camera=title_camera;base_camera_initialized=true;
        }
        std::array<PopupBlur,popup_channel_count> popups{};
        for(unsigned channel=0;channel<backdrops.size();++channel) {
            const auto &popup=popups[channel]=frontend_popup_blur(backdrops[channel].fade);
            const bool wide=backdrops[channel].widescreen_background;
            const ShaderVector source=menu_canvas.source(wide);
            for(unsigned i=0;i<popup.passes;++i) {
                // Keep the original physical tap spacing while sampling the
                // native expanded background rectangle. Centered callers keep
                // their unchanged 640x480 source rectangle.
                const ShaderVector controls{popup.radii[i],wide?menu_canvas.background_logical_width():640.f,480,0};
                const auto slot=7+channel*4+i;
                void *mapped{};check(vkMapMemory(device,bloom_memories[slot],0,272,0,&mapped),"map popup blur constants");
                std::memcpy(mapped,popup.program.words.data(),240);
                std::memcpy(static_cast<std::byte*>(mapped)+240,controls.data(),16);
                std::memcpy(static_cast<std::byte*>(mapped)+256,source.data(),16);
                vkUnmapMemory(device,bloom_memories[slot]);
            }
        }
        const auto volume_draws=title_scene?prepare_light_volumes(camera,
            {viewport.x,viewport.y,viewport.width,viewport.height},menu_projection->scale):std::vector<LightVolumeDraw>{};
        upload_light_volumes();
#ifdef HALO2_PARTICLE_GRAPHICS_AVAILABLE
        struct ViewParticleRange {std::size_t world{},world_count{},foreground{},foreground_count{};};
        std::vector<ViewParticleRange> view_particle_ranges;
        auto prepared_particles=particle_draws;
        if(title_scene && !prepared_particles.empty() &&
            std::all_of(prepared_particles.begin(),prepared_particles.end(),
                [](const auto &draw){return draw.scene_sort_depth.has_value();})) {
            // Particle locations and attached engine glows share original
            // type3 non-plane depth records (175EA0/1E4C0,42850,1E700).
            std::stable_sort(prepared_particles.begin(),prepared_particles.end(),
                [](const auto &a,const auto &b){return *a.scene_sort_depth<*b.scene_sort_depth;});
            unsigned boundary=0;
            for(auto &draw:prepared_particles) {
                while(boundary<volume_draws.size() && volume_draws[boundary].sort_depth<=*draw.scene_sort_depth)++boundary;
                draw.before_light_volume=boundary;
            }
        }
        for(const auto &view:views) {
            ViewParticleRange range;
            range.world=prepared_particles.size();range.world_count=view.world_particles.size();
            prepared_particles.insert(prepared_particles.end(),view.world_particles.begin(),view.world_particles.end());
            range.foreground=prepared_particles.size();range.foreground_count=view.foreground_particles.size();
            prepared_particles.insert(prepared_particles.end(),view.foreground_particles.begin(),view.foreground_particles.end());
            view_particle_ranges.push_back(range);
        }
        if(!prepared_particles.empty()) {
            if(!particle_draws.empty() && prepared_particles[particle_draws.size()-1].before_light_volume>volume_draws.size())
                throw FormatError("particle transparent order exceeds the supplied glow groups");
            if(!particle_renderer)particle_renderer=std::make_unique<ParticleRenderer>(
                physical,device,queue,queue_family,render_pass,shader_directory);
        }
        const bool scene_particles=std::any_of(prepared_particles.begin(),prepared_particles.end(),[](const auto &draw) {
            return draw.material && std::any_of(draw.material->pass.textures.begin(),draw.material->pass.textures.end(),
                [](const auto &binding){return binding.external==23;});
        });
        if(scene_particles && !particle_scene.view) {
            if(!can_capture)throw FormatError("particle external23 requires a readable current scene surface");
            make_post_image(particle_scene,extent,format,VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        }
        if(particle_renderer)particle_renderer->prepare(prepared_particles,{particle_scene.view});
#endif
        std::vector<std::pair<std::size_t,std::size_t>> decal_ranges(views.size());
#if HALO2_SOURCE_DECALS_AVAILABLE
        std::vector<SourceDecalDraw> prepared_decals;
        for(std::size_t i=0;i<views.size();++i){decal_ranges[i]={prepared_decals.size(),views[i].source_decals.size()};prepared_decals.insert(prepared_decals.end(),views[i].source_decals.begin(),views[i].source_decals.end());}
        if(!prepared_decals.empty() && !decal_renderer) {
            if(!source_world_device.geometry_shader_enabled || !source_world_device.depth_clamp_enabled)
                throw FormatError("Cached decal W-depth needs enabled geometry/depth-clamp features");
            decal_renderer=std::make_unique<SourceDecalRenderer>(physical,device,queue,queue_family,render_pass,shader_directory);
        }
        if(decal_renderer)decal_renderer->prepare(prepared_decals);
#endif
        std::vector<std::size_t> contrail_first(views.size());
#if HALO2_SOURCE_CONTRAILS_AVAILABLE
        std::vector<SourceContrailDraw> prepared_contrails;
        for(std::size_t i=0;i<views.size();++i){contrail_first[i]=prepared_contrails.size();prepared_contrails.insert(prepared_contrails.end(),views[i].contrails.begin(),views[i].contrails.end());}
        if(!prepared_contrails.empty() && !contrail_renderer) {
            if(!source_world_device.geometry_shader_enabled || !source_world_device.depth_clamp_enabled)
                throw FormatError("Contrail W-depth needs enabled geometry/depth-clamp features");
            contrail_renderer=std::make_unique<SourceContrailRenderer>(physical,device,queue,queue_family,render_pass,shader_directory);
        }
        if(contrail_renderer)contrail_renderer->prepare(prepared_contrails);
#else
        for(const auto &v:views)if(!v.contrails.empty())throw FormatError("Original contrail graphics unavailable");
#endif
        std::vector<std::vector<std::size_t>> weather_draw_ranges(views.size());
        std::size_t weather_draw_index=0;
        for(std::size_t view=0;view<views.size();++view)for(const auto &draw:views[view].weather_points) {
            if(!draw.sprite)throw FormatError("live weather point has no authored sprite");
            if(!weather_sprite_sampler) {
                VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
                sampler.minFilter=sampler.magFilter=VK_FILTER_LINEAR;
                sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler.maxLod=VK_LOD_CLAMP_NONE;
                check(vkCreateSampler(device,&sampler,nullptr,&weather_sprite_sampler),"create authored snow sampler");
            }
            auto [texture,inserted]=weather_sprites.try_emplace(draw.sprite);
            if(inserted)upload_texture(*draw.sprite,texture->second);
            if(weather_draw_index==weather_point_renderers.size())
                weather_point_renderers.push_back(std::make_unique<SourceWeatherPointRender>(
                    SourceWeatherPointRenderContext{physical,device,render_pass,shader_directory,
                        weather_large_points,source_world_device.depth_clamp_enabled}));
            SourceWeatherPointPacket packet{draw.vertex_program,draw.camera,draw.pixel,draw.points,
                texture->second.view,weather_sprite_sampler,VK_FORMAT_BC3_UNORM_BLOCK,
                {draw.sprite->width,draw.sprite->height},unsigned(draw.sprite->mips.size()),
                {draw.clip_range,{float(extent.width),float(extent.height),0,0},0,0,7,true,true,1,1,
                    false,SourceWeatherPointSpriteCoordinates::OriginalGlRawRowsUpperLeft}};
            weather_point_renderers[weather_draw_index]->prepare_world(packet);
            weather_draw_ranges[view].push_back(weather_draw_index++);
        }
        std::vector<std::vector<std::size_t>> weather_line_ranges(views.size());
        std::size_t weather_line_index=0;
        for(std::size_t view=0;view<views.size();++view)for(const auto &draw:views[view].weather_lines) {
            if(weather_line_index==weather_line_renderers.size())
                weather_line_renderers.push_back(std::make_unique<SourceWeatherLineRender>(
                    SourceWeatherLineRenderContext{physical,device,render_pass,shader_directory,
                        source_world_device.geometry_shader_enabled,weather_wide_lines,
                        source_world_device.depth_clamp_enabled}));
            SourceWeatherLinePacket packet{draw.vertex_program,draw.camera,draw.pixel,draw.vertices,
                {draw.clip_range,{float(extent.width),float(extent.height),0,0},0,0,
                    draw.line_width,7,true,true,false}};
            weather_line_renderers[weather_line_index]->prepare(packet);
            weather_line_ranges[view].push_back(weather_line_index++);
        }
        retire_weather_aliases();
        while(weather_aliases.size()<views.size()) {
            weather_aliases.push_back({});
            weather_alias_pending.push_back(false);
        }
        std::vector<std::vector<std::size_t>> weather_plate_ranges(views.size());
        std::size_t weather_plate_index=0;
        for(std::size_t view=0;view<views.size();++view)if(!views[view].weather_plates.empty()) {
            if(!weather_aliases[view])weather_aliases[view]=std::make_unique<SourceWeatherDepthAlias>(
                SourceWeatherDepthAliasContext{physical,device,queue_family,shader_directory});
            weather_aliases[view]->prepare(extent);
            const auto alias=weather_aliases[view]->prepared_binding();
            if(!weather_sprite_sampler) {
                VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
                sampler.minFilter=sampler.magFilter=VK_FILTER_LINEAR;sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler.maxLod=VK_LOD_CLAMP_NONE;
                check(vkCreateSampler(device,&sampler,nullptr,&weather_sprite_sampler),"create authored plate layer sampler");
            }
            for(const auto &draw:views[view].weather_plates) {
                if(!draw.submission.result.program)throw FormatError("live weather plate queued a source no-draw return");
                if(weather_plate_index==weather_plate_renderers.size())
                    weather_plate_renderers.push_back(std::make_unique<SourceWeatherPlateRender>(
                        SourceWeatherPlateRenderContext{physical,device,render_pass,shader_directory,
                            source_world_device.depth_clamp_enabled}));
                SourceWeatherPlatePacket packet{};packet.vertex_program=draw.vertex_program;
                packet.pixel=*draw.submission.result.program;packet.scroll=draw.submission.scroll;
                for(unsigned vertex=0;vertex<4;++vertex)packet.vertices[vertex].registers=draw.submission.vertices[vertex].registers;
                packet.clip_range=draw.clip_range;packet.surface={float(extent.width),float(extent.height),0,0};
                packet.clip_regions[0]={draw.rectangle[1],draw.rectangle[0],draw.rectangle[3],draw.rectangle[2]};
                packet.depth_clamp=true;packet.depth_clipping=true;
                packet.textures[0]={alias.view,alias.sampler,alias.format,alias.extent,alias.mips};
                for(unsigned layer=0;layer<3;++layer) {
                    const auto *source=draw.layers[layer];
                    if(!source||source->layers!=1||source->depth||source->format!=11)
                        throw FormatError("live weather plate needs owned decoded DXT45 RGBA layers");
                    auto [texture,inserted]=weather_sprites.try_emplace(source);
                    if(inserted)upload_texture(*source,texture->second);
                    packet.textures[layer+1]={texture->second.view,weather_sprite_sampler,VK_FORMAT_R8G8B8A8_UNORM,
                        {source->width,source->height},texture->second.mip_count};
                }
                weather_plate_renderers[weather_plate_index]->prepare(draw.submission.caller,packet);
                weather_plate_ranges[view].push_back(weather_plate_index++);
            }
        }
        flush_texture_uploads();
        if(title_scene && bloom_threshold) {
            const auto bloom=menu_bloom(*bloom_threshold);
            const auto raster=frontend_post_raster(extent.width,extent.height,160,120);
            for(unsigned i=0;i<6;++i) {
                const auto &program=i==0?bloom.threshold:i==5?bloom.composite:bloom.blur;
                const ShaderVector controls{i<5?bloom.radii[i]:0,i==5?viewport.width:float(bloom_extent.width),
                    i==5?viewport.height:float(bloom_extent.height),0};
                // Threshold fills the padded raster at the original texel
                // spacing. Blur operates on it; composite crops its margins.
                const ShaderVector source=i==0?raster.source:i==5?raster.crop:ShaderVector{0,0,1,1};
                void *mapped{};check(vkMapMemory(device,bloom_memories[i],0,272,0,&mapped),"map bloom constants");
                std::memcpy(mapped,program.words.data(),240);std::memcpy(static_cast<std::byte*>(mapped)+240,controls.data(),16);
                std::memcpy(static_cast<std::byte*>(mapped)+256,source.data(),16);
                vkUnmapMemory(device,bloom_memories[i]);
            }
        }
        if(title_scene && scene_filter) {
            const ShaderVector controls{0,viewport.width,viewport.height,0};
            const ShaderVector source{viewport.x/extent.width,viewport.y/extent.height,
                viewport.width/extent.width,viewport.height/extent.height};
            void *mapped{};check(vkMapMemory(device,bloom_memories[6],0,272,0,&mapped),"map title color filter");
            std::memcpy(mapped,scene_filter->words.data(),240);
            std::memcpy(static_cast<std::byte*>(mapped)+240,controls.data(),16);
            std::memcpy(static_cast<std::byte*>(mapped)+256,source.data(),16);
            vkUnmapMemory(device,bloom_memories[6]);
        }
        if(atmosphere) {
            if(mainmenu_fog)update_menu_fog_inputs(*atmosphere,*mainmenu_fog);
            else update_menu_fog_inputs(*atmosphere,
                title_atmosphere_source->sample(camera.position,{forward.x,forward.y,forward.z}));
        }
        const auto menu_fog=atmosphere?menu_fog_draw(*atmosphere,camera.position):std::nullopt;
        if(atmosphere) {
            if(menu_fog) {
                void *mapped{};check(vkMapMemory(device,fog_memory,0,272,0,&mapped),"map fog constants");
                std::memcpy(mapped,menu_fog->program.words.data(),240);
                std::memcpy(static_cast<std::byte*>(mapped)+240,menu_fog->lookup_coordinates.data(),32);
                vkUnmapMemory(device,fog_memory);
            }
            if(title_scene && atmosphere->patchy) {
                const Vec3 f{forward.x,forward.y,forward.z};
                const auto &patch=*atmosphere->patchy;
                const auto state_before=patchy_state;
                const auto seed_before=patchy_random;
                const float haze_fov=camera_vertical_field_of_view(camera,4.f/3.f);
                const float haze_time=float(SDL_GetTicks64())*.001f;
                // Preserve the original haze state/scroll and noise scale in
                // the centered image. Extend its clip-space rows to cover the
                // added scenery rather than stretching the original noise.
                // 2BB1C ->2FD90 ->132D0 copies this inverse view directly to
                // 48568C, consumed by27AA0. Reconstructing a look basis here
                // changes the original normalization and rounding order.
                auto frame=patchy_fog_frame(patch,patchy_state,patchy_random,menu_projection->bank.inverse_view,f.z,
                    haze_fov,3.f/4.f,haze_time);
                const auto raster=frontend_post_raster(extent.width,extent.height,320,240);
                // Observation only: preserve all float/seed bits, including
                // both sides of retained history and the pre-extension rows.
                // No captured value is read back into rendering.
                if(const auto *directory=std::getenv("HALO2_MENU_HAZE_TRACE");directory && *directory) {
                    static_assert(sizeof(PatchyFogState)==136);
                    static_assert(sizeof(Transform)==52);
                    const auto ordinal=patchy_trace_frame++;
                    haze_trace_path=std::filesystem::path(directory)/("frame-"+std::to_string(ordinal));
                    std::filesystem::create_directories(*haze_trace_path);
                    const std::array<float,6> arguments{f.z,haze_fov,haze_time,3.f/4.f,.015625f,1024.f};
                    const std::array<float,19> definition{
                        patch.color.x,patch.color.y,patch.color.z,
                        patch.density[0],patch.density[1],patch.distance[0],patch.distance[1],
                        std::bit_cast<float>(patch.flags),patch.rotation,patch.strafe,patch.zoom,patch.noise_scale,
                        patch.vertical_forward,patch.vertical_up,patch.opacity_up,patch.animation_period,
                        patch.constant_wind.x,patch.constant_wind.y,patch.constant_wind.z};
                    const std::array<std::uint32_t,2> seeds{seed_before,patchy_random};
                    const auto source_frame=haze_trace_frame(frame);
                    std::array<std::uint32_t,180> programs;
                    std::copy(frame.noise.words.begin(),frame.noise.words.end(),programs.begin());
                    std::copy(frame.cached.words.begin(),frame.cached.words.end(),programs.begin()+60);
                    std::copy(frame.direct.words.begin(),frame.direct.words.end(),programs.begin()+120);
                    const auto write=[&](const char *name,const auto &value) {
                        write_haze_trace(*haze_trace_path/name,&value,sizeof(value));
                    };
                    write("view.bin",menu_projection->bank.inverse_view);
                    write("arguments.bin",arguments);write("patch.bin",definition);
                    write("state.before.bin",state_before);write("state.after.bin",patchy_state);
                    write("seed.bin",seeds);write("source-frame.bin",source_frame);write("programs.bin",programs);
                    std::ofstream metadata(*haze_trace_path/"metadata.tsv");
                    metadata<<"version\tH2LIVEHAZEV1\nsequence\t"<<ordinal
                        <<"\nsurface\t"<<extent.width<<'\t'<<extent.height
                        <<"\nviewport\t"<<viewport.x<<'\t'<<viewport.y<<'\t'<<viewport.width<<'\t'<<viewport.height
                        <<"\nnoise\t"<<patchy_extent.width<<'\t'<<patchy_extent.height
                        <<"\nnormal_reference_rectangle\t0\t0\t480\t640\npatchy_datum\t"
                        <<(mainmenu_fog?mainmenu_fog->patchy_datum:0xffffffffu)<<'\n';
                    if(!metadata)throw FormatError("cannot write live title haze trace metadata");
                }
                for(auto& row:frame.rows) {
                    if(raster.scale[0]!=1)row[0]*=raster.scale[0];
                    if(raster.scale[1]!=1)row[1]*=raster.scale[1];
                }
                std::array<PatchyFogQuad,2> quads{{
                    patchy_fog_noise_quad(frame,float(patchy_extent.width),float(patchy_extent.height)),
                    patchy_fog_direct_quad(patch,viewport.width,viewport.height,
                        float(patchy_extent.width),float(patchy_extent.height))}};
                if(raster.crop!=ShaderVector{0,0,1,1}) {
                    for(unsigned corner=0;corner<4;++corner)for(unsigned axis=0;axis<2;++axis) {
                        auto& coordinate=quads[1].attributes[16+corner][axis];
                        const auto size=axis?raster.height:raster.width;
                        coordinate=float((double(size)-raster.required[axis])*.5)+
                            (coordinate==0?0.f:float(raster.required[axis]));
                    }
                }
                for(unsigned i=0;i<quads.size();++i) {
                    void *mapped{};
                    check(vkMapMemory(device,patchy_quad_memories[i],0,sizeof(PatchyFogQuad),0,&mapped),"map completed haze quad");
                    std::memcpy(mapped,&quads[i],sizeof(PatchyFogQuad));vkUnmapMemory(device,patchy_quad_memories[i]);
                    if(haze_trace_path)write_haze_trace(*haze_trace_path/(i?"direct.quad.bin":"noise.quad.bin"),
                        &quads[i],sizeof(PatchyFogQuad));
                }
                if(haze_trace_path) {
                    const auto extended_frame=haze_trace_frame(frame);
                    write_haze_trace(*haze_trace_path/"extended-frame.bin",&extended_frame,sizeof(extended_frame));
                }
                const float inverse=1.f/std::max(.0001f,patch.distance[1]-patch.distance[0]);
                for(unsigned i=0;i<2;++i) {
                    const auto &program=i?frame.direct:frame.noise;
                    PatchyFogUniform uniform;
                    uniform.program=program.words;uniform.rows=frame.rows;
                    uniform.controls={frame.vertical,float(i?extent.width:patchy_extent.width),
                        float(i?extent.height:patchy_extent.height),0};
                    uniform.lookup={1024.f*inverse,0,0.f-patch.distance[0]*inverse,0};
                    uniform.combiner_constants=patchy_fog_combiner_constants(program);
                    void *mapped{};check(vkMapMemory(device,patchy_memories[i],0,sizeof(uniform),0,&mapped),"map patchy fog constants");
                    std::memcpy(mapped,&uniform,sizeof(uniform));
                    vkUnmapMemory(device,patchy_memories[i]);
                    if(haze_trace_path)write_haze_trace(*haze_trace_path/(i?"direct.uniform.bin":"noise.uniform.bin"),
                        &uniform,sizeof(uniform));
                }
            }
            const auto constants=menu_fog_constants(*atmosphere,camera.position,
                {forward.x,forward.y,forward.z});
            for(std::size_t i=0;i<material_uniforms.size();++i) {
                auto &u=material_uniforms[i];
                if(!u.flags[3] || u.fog_constants==constants)continue;
                u.fog_constants=constants;
                mark_material_changed(i);
            }
            for(const auto &[index,pass]:planar_passes) {
                auto &u=material_uniforms[index];
                const auto program=menu_planar_fog_program(pass,*atmosphere,camera.position);
                if(u.program==program.words)continue;
                u.program=program.words;
                mark_material_changed(index);
            }
            // The source-selected implementation children have independent
            // descriptors. Complete their live fog externs in the title bank,
            // just as the per-view bank below does for ordinary world views.
            if(mainmenu_fog)for(const auto &children:world_planar_materials)
                for(const auto index:children)if(index!=~0u) {
                    auto &u=material_uniforms[index];
                    const auto program=bsp_planar_fog_program(
                        fog_material_passes.at(index),*mainmenu_fog);
                    if(u.program==program.words)continue;
                    u.program=program.words;
                    mark_material_changed(index);
                }
        }
        // 33980's view-direction RGB extern also belongs to the main camera
        // path. Per-view banks below replace only these selected channels.
        for(const auto &[index,pass]:fog_material_passes) {
            if(!global_material_roots[material_roots[index]])continue;
            const auto &mappings=view_direction_remaps[index];
            if(mappings.empty())continue;
            auto &uniform=material_uniforms[index];
            PixelProgram program{uniform.program,material_fragment_offsets[index]};
            const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
            const std::array<ShaderVector,1> colors{{{forward.x,forward.y,forward.z,0}}};
            remap_pixel_externs(program,{},colors,mappings,pass.fragments,fog_material_switches.at(index));
            if(uniform.program==program.words)continue;
            uniform.program=program.words;
            mark_material_changed(index);
        }
        for(std::size_t i=0;i<animated_textures.size();++i) for(unsigned stage=0;stage<4;++stage) {
            if(wanted_frames[i][stage]==bound_frames[i][stage]) continue;
            const auto &frames=animated_textures[i][stage];
            if(wanted_frames[i][stage]>=frames.size()) throw FormatError("animated texture frame outside GPU images");
            const auto &texture=frames[wanted_frames[i][stage]];
            if(texture.cube_view && !sampler_non_seamless_cube)
                throw FormatError("original animated cube face filtering requires VK_EXT_non_seamless_cube_map");
            const VkDescriptorImageInfo image{stage_samplers[i][stage],texture.depth?image_fallback.view:texture.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            const VkDescriptorImageInfo cube{stage_samplers[i][stage],texture.cube_view?texture.cube_view:cube_fallback.cube_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            const VkDescriptorImageInfo volume{stage_samplers[i][stage],texture.depth?texture.view:volume_fallback.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet writes[3]{};
            for(unsigned k=0;k<3;++k) {
                writes[k]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[k].dstSet=textures[i].descriptor;
                writes[k].dstBinding=k?k+4:stage;writes[k].dstArrayElement=k?stage:0;
                writes[k].descriptorCount=1;writes[k].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[k].pImageInfo=k==2?&volume:k==1?&cube:&image;
            }
            vkUpdateDescriptorSets(device,3,writes,0,nullptr);
            for(const auto &[key,descriptor]:geometry_descriptors) if(key.first==i) {
                for(auto &write:writes)write.dstSet=descriptor;
                vkUpdateDescriptorSets(device,3,writes,0,nullptr);
            }
            if(scope_descriptors[i]) {
                for(auto &write:writes)write.dstSet=scope_descriptors[i];
                // Extern27 has a live copy producer during FP, never an
                // animated stage0 bitmap. Other authored stages still move.
                if(stage!=0)vkUpdateDescriptorSets(device,3,writes,0,nullptr);
            }
            bound_frames[i][stage]=wanted_frames[i][stage];
        }
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
        const bool source_title_first=!menu_world_reported;
        const auto source_title_draws=prepare_menu_world(mainmenu_world,camera,viewport,title_scene,views);
        std::size_t source_base_recorded{},source_fog_recorded{};
#endif
        if(!changed_materials.empty()) {
            void *mapped;
            mapped=material_mapping;
            for(const auto i:changed_materials)
                std::memcpy(static_cast<std::byte*>(mapped)+material_stride*i,
                            &material_uniforms[i],sizeof(MaterialUniform));
            for(const auto index:changed_materials)material_pending[index]=0;
            changed_materials.clear();
        }
        if(!changed_vertex_banks.empty()) {
            void *mapped;
            check(vkMapMemory(device,vertex_bank_memory,0,VK_WHOLE_SIZE,0,&mapped),"map changed draw vertex globals");
            for(const auto i:changed_vertex_banks)
                std::memcpy(static_cast<std::byte*>(mapped)+vertex_bank_stride*i,
                            &vertex_bank_uniforms[i],sizeof(VertexBankUniform));
            vkUnmapMemory(device,vertex_bank_memory);
            for(const auto index:changed_vertex_banks)vertex_bank_pending[index]=0;
            changed_vertex_banks.clear();
        }
        measure(frame_timing.global_constants_ms);
        if(views.size()>4)throw FormatError("too many simultaneous model viewports");
        if(bsp_postprocess && !views.empty()) {
            std::vector<BspPostprocessView> post_views;
            const float scale=std::min(float(extent.width)/640.f,float(extent.height)/480.f);
            for(const auto &view:views) {
                if(!view.atmosphere)throw FormatError("BSP postprocess view has no atmosphere");
                const float sx=view.full_surface?float(extent.width)/640.f:scale;
                const float sy=view.full_surface?float(extent.height)/480.f:scale;
                const int left=int((view.full_surface?0.f:(float(extent.width)-640.f*scale)*.5f)+view.left*sx);
                const int top=int((view.full_surface?0.f:(float(extent.height)-480.f*scale)*.5f)+view.top*sy);
                VkRect2D rectangle{{left,top},{unsigned(view.width*sx),unsigned(view.height*sy)}};
                if(view.full_surface) {
                    const auto physical=full_surface_viewport({view.left,view.top,view.width,view.height},extent.width,extent.height);
                    rectangle={{int(physical.left),int(physical.top)},{unsigned(physical.width),unsigned(physical.height)}};
                }
                post_views.push_back({view.camera,*view.atmosphere,rectangle,
                    source_player_bloom_screen_context(std::int16_t(post_views.size())),
                    view.projection_clip_planes.value_or(Vec2{.015625f,1024.f})});
            }
            bsp_postprocess->prepare(post_views,views.front().effect_time);
        }
        if(scope_render) {
            std::vector<std::optional<ScopeCopyFrame>> frames;
            frames.reserve(views.size());
            for(const auto &view:views)frames.push_back(view.scope);
            scope_render->prepare(frames);
        }
        struct PreparedViewMaterialInputs {
            std::array<ShaderVector,6> fog{};
            std::array<ShaderVector,1> direction{};
            MenuFogExterns fog_externs{};
            std::vector<const ObjectLightingState *> lighting;
        };
        std::vector<PreparedViewMaterialInputs> prepared_views;
        prepared_views.reserve(views.size());
        for(const auto &view:views) {
            auto &prepared=prepared_views.emplace_back();
            const auto forward=view.camera.oriented?view.camera.forward:direction(view.camera.yaw,view.camera.pitch);
            prepared.direction={{{forward.x,forward.y,forward.z,0}}};
            if(view.atmosphere) {
                prepared.fog=bsp_fog_constants(*view.atmosphere,view.camera.position,forward);
                prepared.fog_externs=bsp_fog_externs(*view.atmosphere);
            }
            // Overlay children share their root's first matching owner range.
            // Rebuild from the current view every frame; no lighting/pose state
            // is retained across frames or shared between local players.
            prepared.lighting.resize(material_count);
            std::size_t unbound=material_count;
            for(const auto &binding:view.object_lighting) {
                // Once all roots have their first match, the old root-major
                // search also stops before examining any later bindings.
                if(!unbound)break;
                if(binding.first_material>material_count || binding.material_count>material_count-binding.first_material)
                    throw FormatError("object lighting material range outside renderer");
                for(unsigned root=binding.first_material;root<binding.first_material+binding.material_count;++root)
                    if(!prepared.lighting[root]) {prepared.lighting[root]=&binding.state;--unbound;}
            }
        }
        active_forward_pipeline_indices=forward_pipeline_indices;
        for(std::size_t m=0;m<material_uniforms.size();++m) {
            const auto specialized=specialized_pipeline_indices[m];
            if(specialized!=~0u)active_forward_pipeline_indices[m]=specialized;
        }
        const auto admit_combiner=[&](std::size_t m,const MaterialUniform &uniform) {
            const auto active=active_forward_pipeline_indices[m];
            if(active!=~0u && forward_pipelines[active].controls &&
               combiner_controls(uniform.program)!=*forward_pipelines[active].controls)
                active_forward_pipeline_indices[m]=forward_pipeline_indices[m];
        };
        for(std::size_t m=0;m<material_uniforms.size();++m)
            if(global_material_roots[material_roots[m]])admit_combiner(m,material_uniforms[m]);
        const auto *uncached_bindings=std::getenv("HALO2_REFERENCE_UNCACHED_VIEW_BINDINGS");
        const bool cache_view_bindings=!(uncached_bindings && *uncached_bindings);
        const auto view_material_uniform=[&](const ModelView &view,std::size_t m,
                                             const ObjectLightingState *owner_light=nullptr,
                                             const ObjectColors *owner_colors=nullptr,const ShaderInput *owner_input=nullptr) {

            const auto &prepared=prepared_views[std::size_t(&view-views.data())];
            const auto pass=fog_material_passes.find(unsigned(m));
            auto uniform=material_uniforms[m];
            if(view.source_view_shader_constants) {
                const auto &source=*view.source_view_shader_constants;
                uniform.source_camera_world=source.world;
                uniform.source_camera_reflection=source.reflection;
                uniform.source_camera_foreground=source.foreground;
                uniform.source_camera_control[0]=1;
                const auto layer=source_material_layers[m];
                uniform.source_camera_control[1]=layer && source_shader_layer_stipple(
                    *layer,source_shader_gameplay_frame_type);
                // Preserve the original observed physical raster phase.
                // Every supplied standard target height is divisible by4.
                uniform.source_camera_control[2]=31;
            }
            const auto apply_light=[&](const ObjectLightingState &state) {
                if(pass!=fog_material_passes.end()) {
                    PixelProgram program{uniform.program,material_fragment_offsets[m]};
                    ShaderConstants object_constants{uniform.vertex_attributes,uniform.vertex_constants};
                    const auto brightness=material_lighting_brightness[m];
                    apply_object_lighting_bindings(program,object_constants,pass->second,state,
                        fog_material_switches.at(unsigned(m)),brightness[0],brightness[1]);
                    uniform.program=program.words;
                    uniform.vertex_attributes=object_constants[0];uniform.vertex_constants=object_constants[1];
                }
            };
            if(owner_light)apply_light(*owner_light);
            else if(const auto *state=prepared.lighting[material_roots[m]])apply_light(*state);
            if(owner_colors && pass!=fog_material_passes.end()) {
                PixelProgram program{uniform.program,material_fragment_offsets[m]};
                ShaderConstants constants{uniform.vertex_attributes,uniform.vertex_constants};
                apply_object_render_color_bindings(program,constants,pass->second,
                    source_object_render_colors(*owner_colors),fog_material_switches.at(unsigned(m)));
                uniform.program=program.words;
                uniform.vertex_attributes=constants[0];uniform.vertex_constants=constants[1];
            }
            if(owner_input && pass!=fog_material_passes.end()) {
                if(const auto animation=owner_material_animations.find(unsigned(m));animation!=owner_material_animations.end()) {
                    PixelProgram program{uniform.program,material_fragment_offsets[m]};
                    ShaderConstants constants{uniform.vertex_attributes,uniform.vertex_constants};
                    animate_forward_constants(animation->second,pass->second,fog_material_switches.at(unsigned(m)),
                        view.effect_time,*owner_input,program,constants);
                    uniform.program=program.words;uniform.vertex_attributes=constants[0];uniform.vertex_constants=constants[1];
                    uniform.diffuse=constants[0][10];uniform.specular=constants[0][11];
                    std::copy_n(constants[1].begin(),8,uniform.forward_transforms.begin());
                }
            }
            if(pass!=fog_material_passes.end()) {
                PixelProgram program{uniform.program,material_fragment_offsets[m]};
                remap_pixel_externs(program,{},prepared.direction,view_direction_remaps[m],
                    pass->second.fragments,fog_material_switches.at(unsigned(m)));
                uniform.program=program.words;
            }
            if(view.atmosphere) {
                uniform.fog_constants=prepared.fog;
                if(pass!=fog_material_passes.end()) {
                    PixelProgram program{uniform.program,material_fragment_offsets[m]};
                    if(cache_view_bindings)
                        remap_pixel_externs(program,prepared.fog_externs.scalars,prepared.fog_externs.colors,
                            view_fog_remaps[m],pass->second.fragments,fog_material_switches.at(unsigned(m)));
                    else apply_bsp_fog_pixel_externs(program,pass->second,*view.atmosphere,fog_material_switches.at(unsigned(m)));
                    uniform.program=program.words;
                }
            }
            if(const int definition=water_material_definitions[m];definition>=0) {
                if(std::size_t(definition)>=view.water.size())throw FormatError("water material has no viewport state");
                const auto &water=view.water[definition];
                std::copy(water.constants.begin(),water.constants.end(),uniform.vertex_constants.begin()+3);
                uniform.bump_matrices=water.bump_matrices;
            }
            admit_combiner(m,uniform);
            return uniform;
        };
        std::array<bool,4> view_fog_enabled{};
        if(!views.empty()) {
            void *mapped{};
            for(std::size_t i=0;i<views.size();++i) if(views[i].atmosphere) {
                if(!bsp_atmosphere)throw FormatError("viewport atmosphere gradient is unbound");
                const auto selected=bsp_fog_draw(*views[i].atmosphere,1024.f,fog_draw_controls);
                if(!selected)continue;
                view_fog_enabled[i]=true;const auto &composite=*selected;
                check(vkMapMemory(device,view_fog_memories[i],0,272,0,&mapped),"map viewport fog composite");
                std::memcpy(mapped,composite.program.words.data(),240);
                std::memcpy(static_cast<std::byte*>(mapped)+240,composite.lookup_coordinates.data(),32);
                vkUnmapMemory(device,view_fog_memories[i]);
            }
            if(water_copy_pass)for(std::size_t i=0;i<views.size();++i) {
                const auto &v=views[i];
                ShaderVector rectangle{v.left/640.f,v.top/480.f,v.width/640.f,v.height/480.f};
                if(v.full_surface) {
                    const auto physical=full_surface_viewport({v.left,v.top,v.width,v.height},extent.width,extent.height);
                    rectangle={physical.left/extent.width,physical.top/extent.height,
                               physical.width/extent.width,physical.height/extent.height};
                }
                const auto source_taps=water_copy_taps(rectangle[2],rectangle[3]);
                const ShaderVector taps{source_taps[0][0],source_taps[0][1],source_taps[1][0],source_taps[2][1]};
                check(vkMapMemory(device,water_copy_memories[i],0,272,0,&mapped),"map water source rectangle");
                const auto program=water_copy_program();std::memcpy(mapped,program.words.data(),240);
                std::memcpy(static_cast<std::byte*>(mapped)+240,rectangle.data(),16);
                std::memcpy(static_cast<std::byte*>(mapped)+256,taps.data(),16);
                vkUnmapMemory(device,water_copy_memories[i]);
            }
        }
        measure(frame_timing.view_constants_ms);
        if (required > debug_capacity) {
            // The frame fence above retires all reads from the previous buffer.
            vkDestroyBuffer(device, debug_buffer, nullptr);
            vkFreeMemory(device, debug_memory, nullptr);
            debug_buffer = VK_NULL_HANDLE;
            debug_memory = VK_NULL_HANDLE;
            debug_capacity = std::min(dynamic_limit, std::max(required, debug_capacity * 2));
            make_buffer(debug_capacity * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, debug_buffer, debug_memory);
        }
        const auto bind_geometry=[&](auto &draws) {
            for(auto &draw:draws)draw.descriptor=geometry_descriptor(draw.material,draw.lightmap);
        };
        bind_geometry(background_draws);bind_geometry(dynamic_draws);bind_geometry(foreground_draws);
        for(auto &draws:view_draws)bind_geometry(draws);
        for(auto &draws:view_foreground)bind_geometry(draws);
        for(auto &draws:view_background)bind_geometry(draws);
        for(auto &draws:view_reflection_background)bind_geometry(draws);
        std::vector<std::map<std::pair<unsigned,unsigned>,std::size_t>> object_aliases;
        std::vector<MaterialUniform> object_uniforms;
        std::vector<std::optional<VertexBankUniform>> object_vertex_banks;
        std::vector<VkDescriptorSet> object_residents;
        const auto alias_owner=[&](const ModelView &view,std::span<Draw> list,
                                   const ObjectLightingState *lighting,const ObjectColors *colors=nullptr,const ShaderInput *input=nullptr,
                                   std::span<const SourcePrtMaterialBank> prt_banks=std::span<const SourcePrtMaterialBank>{}) {
            const auto owner=object_aliases.size();object_aliases.emplace_back();
            auto &aliases=object_aliases.back();
            std::map<unsigned,VertexBankUniform> banks;
            for(const auto &binding:prt_banks) {
                if(binding.material>=material_count || !binding.bank || !binding.bank->prt_inputs ||
                   binding.bank->camera_inputs || binding.bank->first_constant!=68 ||
                   (binding.bank->constants.size()!=42 && binding.bank->constants.size()!=82))
                    throw FormatError("owner PRT bank has invalid source material/range");
                const auto target=environment_materials[binding.material];
                if(target==~0u || !extended_vertex_banks[target] ||
                   vertex_bank_uniforms[target].range!=pack_vertex_program_bank(*binding.bank).range ||
                   !banks.emplace(target,pack_vertex_program_bank(*binding.bank)).second)
                    throw FormatError("owner PRT bank differs from resident layer5 declaration");
            }
            const auto add=[&](unsigned material,unsigned lightmap) {
                if(material==~0u || aliases.contains({material,lightmap}))return;
                if(water_materials[material] || scope_materials[material])
                    throw FormatError("owner draw cannot supply water or first-person external targets");
                aliases.emplace(std::pair{material,lightmap},object_uniforms.size());
                object_uniforms.push_back(view_material_uniform(view,material,lighting,colors,input));
                const auto bank=banks.find(material);
                object_vertex_banks.push_back(bank==banks.end()?std::nullopt:std::optional{bank->second});
                object_residents.push_back(geometry_descriptor(material,lightmap));
            };
            for(auto &draw:list) {
                draw.object_alias=owner;add(draw.material,draw.lightmap);
                add(environment_materials[draw.material],draw.lightmap);
                for(auto m:world_before[draw.material])add(m,draw.lightmap);
                for(auto m:world_after[draw.material])add(m,draw.lightmap);
                for(auto m:world_bloom[draw.material])add(m,draw.lightmap);
                add(planar_materials[draw.material],draw.lightmap);
                for(auto m:world_planar_materials[draw.material])add(m,draw.lightmap);
            }
        };
        for(const auto &range:object_ranges) {
            const auto &view=views[range.view];const auto &object=view.objects[range.object];
            alias_owner(view,std::span{view_draws[range.view]}.subspan(range.first,range.count),
                object.lighting?&*object.lighting:nullptr,object.colors?&*object.colors:nullptr,
                object.shader_input?&object.shader_input:nullptr,object.prt_banks);
        }
        for(std::size_t i=0;i<views.size();++i)for(auto &phase:transparent_views[i])for(auto &step:phase) {
            bind_geometry(step.geometry);
            if(!step.particles && !step.light_source)alias_owner(views[i],step.geometry,step.lighting);
        }
        std::vector<ResidentDrawUpload> object_uploads;object_uploads.reserve(object_uniforms.size());
        for(std::size_t i=0;i<object_uniforms.size();++i)
            object_uploads.push_back({object_residents[i],{reinterpret_cast<const std::byte*>(&object_uniforms[i]),sizeof(MaterialUniform)},object_vertex_banks[i]});
        if(!object_draw_storage && !object_uploads.empty())
            object_draw_storage=std::make_unique<ResidentDrawStorage>(device,physical,texture_layout,sizeof(MaterialUniform));
        const auto object_descriptors=object_draw_storage?object_draw_storage->upload(object_uploads):std::span<const VkDescriptorSet>{};
        if(!views.empty()) {
            void *mapped{};
            mapped=material_mapping;
            for(std::size_t i=0;i<views.size();++i) {
                const auto &view=views[i];
                std::vector<bool> used(material_uniforms.size());
                std::vector<unsigned> materials;
                const auto add=[&](unsigned material) {
                    if(material==~0u || used.at(material))return;
                    used[material]=true;materials.push_back(material);
                };
                const auto collect=[&](const auto &draws) {
                    for(const auto &draw:draws) {
                        const auto m=draw.material;
                        add(m);add(environment_materials[m]);add(planar_materials[m]);
                        for(const auto child:world_before[m])add(child);
                        for(const auto child:world_after[m])add(child);
                        for(const auto child:world_bloom[m])add(child);
                        for(const auto child:world_planar_materials[m])add(child);
                    }
                };
                // Retain every possible pass of every packed draw, including
                // sky/reflections, FP, water and owner objects. Visibility and
                // source layer admission remain in the command recorder.
                collect(view_draws[i]);collect(view_foreground[i]);
                collect(view_background[i]);collect(view_reflection_background[i]);
                for(const auto &phase:transparent_views[i])for(const auto &step:phase)collect(step.geometry);
                auto &initialized=view_material_initialized[i];
                initialized.resize(material_uniforms.size());
                for(const auto m:materials) {
                    const auto uniform=view_material_uniform(view,m);
                    auto *destination=static_cast<std::byte*>(mapped)+
                        material_stride*(material_uniforms.size()*(i+1)+m);
                    if(!initialized[m]) {
                        std::memcpy(destination,&uniform,sizeof(uniform));initialized[m]=true;
                    } else {
                        // The decoded vertex instruction stream is immutable
                        // after initialization. Upload every mutable field with
                        // unchanged bytes, including animated programs, object
                        // lighting, fog, water and this player's camera.
                        constexpr auto first=offsetof(MaterialUniform,vertex_instructions);
                        constexpr auto tail=offsetof(MaterialUniform,vertex_constants);
                        std::memcpy(destination,&uniform,first);
                        std::memcpy(destination+tail,reinterpret_cast<const std::byte*>(&uniform)+tail,
                            sizeof(uniform)-tail);
                    }
                }
            }
        }
        struct ShadowDrawRecord {
            const SourceShadowMaterial *authored{};
            std::uint32_t first{},count{},binding{};
            VkPipeline pipeline{};
        };
        struct ShadowRootRecord {
            SourceShadowUniforms uniforms;
            std::array<std::vector<ShadowDrawRecord>,2> draws;
        };
        std::vector<std::vector<ShadowRootRecord>> shadow_views(views.size());
        std::vector<MaterialUniform> shadow_uniforms;shadow_uniforms.reserve(shadow_draw_count);
        std::vector<SourceShadowDrawUpload> shadow_uploads;shadow_uploads.reserve(shadow_draw_count);
        for(std::size_t i=0;i<views.size();++i)if(views[i].source_shadows) {
            const auto &plan=*views[i].source_shadows;
            for(const auto &root:plan.roots) {
                if(root.casters.empty() || root.receivers.empty())continue;
                auto &record=shadow_views[i].emplace_back();record.uniforms=root.uniforms;
                for(unsigned phase=0;phase<2;++phase)for(const auto &draw:phase?root.receivers:root.casters) {
                    const auto resident=std::find_if(shadow_materials.begin(),shadow_materials.end(),
                        [&](const auto &entry){return entry.material_slot==draw.material_slot;});
                    if(resident==shadow_materials.end())throw FormatError("validated shadow resident disappeared");
                    const auto bound=source_shadow_plan_bindings(plan,root,draw,*resident);
                    auto &uniform=shadow_uniforms.emplace_back(material_uniforms.at(draw.material_slot));
                    if(!uniform.flags[3])throw FormatError("shadow slot requires its authored vertex program");
                    uniform.program=bound.program.words;
                    uniform.vertex_attributes=bound.constants[0];uniform.vertex_constants=bound.constants[1];
                    std::copy_n(bound.camera.begin(),7,uniform.source_camera_world.begin());
                    uniform.source_camera_control={1,0,0,phase};
                    // Extern8 is linear target10. Preserve this only in the
                    // queued shadow uniform, never in accumulation metadata.
                    if(bound.external_targets[0])uniform.texture_effects[0][1]=1;
                    SourceShadowDrawUpload upload;
                    upload.resident=textures.at(draw.material_slot).descriptor;
                    upload.material_uniform={reinterpret_cast<const std::byte *>(&uniform),sizeof(uniform)};
                    upload.vertex_bank=pack_vertex_program_bank(source_shadow_plan_vertex_bank(bound));
                    if(bound.external_targets[0]) {
                        upload.target_view=shadow_render->target_view();
                        upload.target_sampler=stage_samplers.at(draw.material_slot)[0];
                    }
                    record.draws[phase].push_back({resident->authored,shadow_vertex_first.at(shadow_uploads.size()),
                        std::uint32_t(draw.vertices.size()),std::uint32_t(shadow_uploads.size()),
                        shadow_storage->pipeline(*resident->authored,phase?SourceShadowPhase::Apply:
                            SourceShadowPhase::Generate,*draw.raster)});
                    shadow_uploads.push_back(upload);
                }
            }
        }
        const auto shadow_descriptors=shadow_storage?shadow_storage->upload(shadow_uploads):std::span<const VkDescriptorSet>{};
        struct EffectLightRecord {unsigned material{},first{},count{},binding{};};
        std::vector<std::vector<EffectLightRecord>> light_views(views.size());
        std::vector<MaterialUniform> light_uniforms;light_uniforms.reserve(effect_light_draw_count+raw_effect_light_draw_count);
        std::vector<SourceWorldDrawUpload> light_uploads;light_uploads.reserve(effect_light_draw_count+raw_effect_light_draw_count);
        for(std::size_t i=0;i<views.size();++i)for(const auto &draw:views[i].effect_lights) {
            auto &uniform=light_uniforms.emplace_back(view_material_uniform(views[i],draw.material_slot));
            if(!uniform.flags[3])throw FormatError("Sphere diffuse draw lacks authored vertex program");
            uniform.program=draw.program.words;
            uniform.vertex_attributes=draw.constants[0];uniform.vertex_constants=draw.constants[1];
            light_views[i].push_back({draw.material_slot,light_vertex_first.at(light_uploads.size()),unsigned(draw.vertices.size()),unsigned(light_uploads.size())});
            light_uploads.push_back({textures.at(draw.material_slot).descriptor,
                {reinterpret_cast<const std::byte*>(&uniform),sizeof(uniform)},draw.vertex_bank});
        }
        std::vector<std::vector<std::size_t>> raw_light_views(views.size());
        for(std::size_t i=0;i<views.size();++i)for(const auto &draw:views[i].raw_effect_lights) {
            auto &uniform=light_uniforms.emplace_back(view_material_uniform(views[i],draw.material_slot));
            uniform.program=draw.program.words;
            uniform.vertex_attributes=draw.constants[0];uniform.vertex_constants=draw.constants[1];
            raw_light_views[i].push_back(light_uploads.size());
            light_uploads.push_back({textures.at(draw.material_slot).descriptor,
                {reinterpret_cast<const std::byte*>(&uniform),sizeof(uniform)},draw.vertex_bank});
        }
        if(!effect_light_storage && !light_uploads.empty())
            effect_light_storage=std::make_unique<SourceWorldDrawStorage>(SourceWorldDrawStorageContext{
                device,physical,texture_layout,sizeof(MaterialUniform)});
        const auto light_descriptors=effect_light_storage?effect_light_storage->upload(light_uploads):std::span<const VkDescriptorSet>{};
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
        std::vector<MenuWorldNativeDrawInput> raw_light_inputs;
        raw_light_inputs.reserve(raw_effect_light_draw_count);
        Texture accumulation_shape;
        accumulation_shape.width=extent.width;accumulation_shape.height=extent.height;
        accumulation_shape.format=11;
        const auto &enabled=source_world_device;
        if(raw_effect_light_draw_count && (enabled.instance_api_version<VK_API_VERSION_1_3 ||
           enabled.physical_api_version<VK_API_VERSION_1_3 || !enabled.geometry_shader_enabled ||
           !enabled.geometry_point_size_enabled || !enabled.depth_clamp_enabled ||
           !enabled.demote_to_helper_enabled || depth_format!=VK_FORMAT_D24_UNORM_S8_UINT))
            throw FormatError("Raw sphere diffuse requires its enabled Vulkan source-world features");
        for(std::size_t i=0;i<views.size();++i) {
            const auto &view=views[i];
            for(std::size_t j=0;j<view.raw_effect_lights.size();++j) {
                const auto &draw=view.raw_effect_lights[j];const auto slot=draw.material_slot;
                const auto upload=raw_light_views[i][j];
                MenuWorldNativePacketInput input{draw.source,generated_menu_world_vertex_catalog::entries,
                    light_uniforms[upload]};
                input.fragment_offsets=draw.program.fragment_offsets;
                input.forward=material_forward_states.at(slot);input.forward_blend=10;
                input.texture_states=stage_sampler_states.at(slot);
                input.sampler_limits={enabled.maximum_lod_bias,enabled.maximum_anisotropy,
                    enabled.anisotropy_enabled,enabled.custom_border_colors_enabled};
                input.raster=draw.raster;input.clip_range=draw.clip_range;input.fog_parameters=draw.fog_parameters;
                // Original c8..11 already encode the view's screen offset and
                // size. The raw shader converts those framebuffer coordinates
                // against the whole surface; use a full Vulkan viewport below.
                input.surface_size={float(extent.width),float(extent.height)};
                input.demote_to_helper_enabled=enabled.demote_to_helper_enabled;
                input.material_set=light_descriptors[upload];input.material_offset=0;
                const GpuTexture *stages[]{&textures.at(slot),&detail_textures.at(slot),
                    &bump_textures.at(slot),&auxiliary_textures.at(slot)};
                for(unsigned stage=0;stage<4;++stage) {
                    if(stage==0 && accumulation_materials.at(slot)) {
                        if(!accumulation_view || !accumulation_pass)
                            throw FormatError("Raw sphere diffuse lacks its live texture-mapping target");
                        input.images[0]=&accumulation_shape;
                        input.texture_coordinates[0]=MenuWorldTextureCoordinates::pixel;
                        continue;
                    }
                    const auto &animated=animated_textures.at(slot)[stage];
                    if(!animated.empty())stages[stage]=&animated.at(bound_frames.at(slot)[stage]);
                    const auto &image=*stages[stage];
                    if(image.source_image && (image.view || image.cube_view))input.images[stage]=&image.source_shape;
                }
                raw_light_views[i][j]=raw_light_inputs.size();
                raw_light_inputs.push_back(menu_world_native_packet(input));
            }
        }
        if(!effect_light_native && !raw_light_inputs.empty())effect_light_native=std::make_unique<MenuWorldNativeDraw>(
            MenuWorldNativeDrawContext{device,physical,render_pass,texture_layout,enabled.instance_api_version,
                enabled.geometry_shader_enabled,enabled.geometry_point_size_enabled,
                enabled.depth_clamp_enabled,enabled.demote_to_helper_enabled,shader_directory});
        if(effect_light_native)effect_light_native->prepare(raw_light_inputs);
#else
        if(raw_effect_light_draw_count)throw FormatError("Raw sphere diffuse catalog was not built");
#endif
        if (ui_full_surface_bounds) {
            const auto adjust=[&](Vertex &vertex) {
                // Retain the original full-surface UI placement arithmetic.
                auto &p=vertex.position;
                p.x = (menu_canvas.left + p.x * menu_canvas.scale) * 640 / extent.width;
                p.y = (menu_canvas.top + p.y * menu_canvas.scale) * 480 / extent.height;
            };
            if(copy_vertices)for(auto i=ui_first;i<dynamic.size();++i)adjust(dynamic[i]);
            if(plan_vertices)for(auto &vertex:adjusted_ui)adjust(vertex);
        }
        if(verify_frame) {
            std::vector<Vertex> actual(upload_plan.size());upload_plan.copy_to(actual.data());
            if(actual.size()!=dynamic.size() || (!actual.empty() &&
               std::memcmp(actual.data(),dynamic.data(),actual.size()*sizeof(Vertex))))
                throw FormatError("borrowed frame upload bytes differ from original staging");
        }
        measure(frame_timing.geometry_bind_ms);
        frame_timing.geometry_pack_ms+=frame_timing.geometry_bind_ms;
        if (vertex_count()) {
            void* mapped;
            check(vkMapMemory(device,debug_memory,0,vertex_count()*sizeof(Vertex),0,&mapped),"map dynamic vertices");
            if(borrowed_frame)upload_plan.copy_to(mapped);
            else std::memcpy(mapped,dynamic.data(),dynamic.size()*sizeof(Vertex));
            vkUnmapMemory(device,debug_memory);
        }
        measure(frame_timing.geometry_upload_ms);
        if(const auto *path=std::getenv("HALO2_VIEW_DIAGNOSTICS")) {
            const auto stem=std::string(path)+"-"+std::to_string(diagnostic_frame++);
            std::ofstream layout(stem+".tsv");
            layout<<"material_count\t"<<material_uniforms.size()<<"\nstride\t"<<material_stride<<"\nvertices\t"<<vertex_count()<<"\n";
            void *mapped{};
            const auto bytes=material_stride*material_uniforms.size()*5;
            mapped=material_mapping;
            std::ofstream banks(stem+".uniforms.bin",std::ios::binary);
            banks.write(static_cast<const char *>(mapped),static_cast<std::streamsize>(bytes));
            for(unsigned bank=0;bank<=views.size();++bank)for(unsigned m=0;m<material_uniforms.size();++m) {
                const auto *uniform=reinterpret_cast<const MaterialUniform *>(static_cast<const std::byte *>(mapped)+material_stride*(material_uniforms.size()*bank+m));
                layout<<bank<<'\t'<<m;
                for(const auto flag:uniform->flags)layout<<'\t'<<flag;
                layout<<'\n';
            }
            std::ofstream geometry(stem+".vertices.bin",std::ios::binary);
            if(borrowed_frame)upload_plan.write_to(geometry);
            else geometry.write(reinterpret_cast<const char *>(dynamic.data()),static_cast<std::streamsize>(dynamic.size()*sizeof(Vertex)));
        }
        frame_timing.dynamic_vertices=vertex_count();
        measure(frame_timing.geometry_diagnostics_ms);
        frame_timing.geometry_ms=frame_timing.geometry_pack_ms+frame_timing.geometry_upload_ms+
            frame_timing.geometry_diagnostics_ms;
        std::uint32_t index;
        const auto status = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquired, VK_NULL_HANDLE, &index);
        measure(frame_timing.acquire_ms);
        if (status == VK_ERROR_OUT_OF_DATE_KHR) { dirty = true; return false; }
        if (status != VK_SUBOPTIMAL_KHR) check(status, "acquire swapchain image");
        else dirty = true;
        VkBuffer capture{};
        VkDeviceMemory capture_memory{};
        VkBuffer depth_capture{};
        VkDeviceMemory depth_capture_memory{};
        struct Capture { VkDevice device; VkBuffer& buffer; VkDeviceMemory& memory;
            ~Capture() { if (buffer) { vkDeviceWaitIdle(device); vkDestroyBuffer(device, buffer, nullptr); } if (memory) vkFreeMemory(device, memory, nullptr); }
        } capture_guard{device, capture, capture_memory};
        Capture depth_capture_guard{device,depth_capture,depth_capture_memory};
        const VkDeviceSize depth_capture_size = VkDeviceSize(extent.width) * extent.height * 4;
        const VkDeviceSize capture_size = VkDeviceSize(presentation_extent.width) * presentation_extent.height * 4;
        const VkDeviceSize stencil_capture_size=VkDeviceSize(extent.width)*extent.height;
        if (!screenshot.empty()) {
            if (!can_capture) throw std::runtime_error("surface does not support frame readback");
            make_buffer(capture_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, capture, capture_memory);
            if(std::getenv("HALO2_DEPTH_DIAGNOSTICS"))
                make_buffer(depth_capture_size+stencil_capture_size,VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    depth_capture,depth_capture_memory);
        }
        check(vkResetCommandBuffer(command, 0), "reset commands");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command, &begin), "begin commands");
        // Every successful alias record is covered by this frame's actual
        // fence. If recording fails before queue submission, reset the never
        // submitted command and restore each alias's previous layout history.
        struct WeatherFrameSubmission {
            Impl &renderer;bool fence_reset{},submitted{};
            ~WeatherFrameSubmission() {
                if(submitted||!fence_reset)return;
                try {
                    check(vkResetCommandBuffer(renderer.command,0),"discard unsubmitted weather frame");
                    for(std::size_t i=0;i<renderer.weather_aliases.size();++i)if(renderer.weather_alias_pending[i]) {
                        renderer.weather_aliases[i]->discard_unsubmitted(renderer.command);
                        renderer.weather_alias_pending[i]=false;
                    }
                    VkSubmitInfo empty{VK_STRUCTURE_TYPE_SUBMIT_INFO};
                    check(vkQueueSubmit(renderer.queue,1,&empty,renderer.fence),"restore discarded weather frame fence");
                    check(vkWaitForFences(renderer.device,1,&renderer.fence,VK_TRUE,UINT64_MAX),"retire discarded weather frame");
                }catch(...){std::terminate();}
            }
        } weather_submission{*this};
        record_dynamic_images();
        VkClearValue clear[2]{};
        clear[0].color = ui.empty()?VkClearColorValue{{0.018f, 0.026f, 0.044f, 1}}:VkClearColorValue{{0,0,0,1}};
        clear[1].depthStencil = {1, 0};
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        render.renderPass = render_pass;
        render.framebuffer = framebuffers[index];
        render.renderArea = {{0, 0}, extent};
        render.clearValueCount = 2;
        render.pClearValues = clear;
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdSetViewport(command, 0, 1, &viewport);
        vkCmdSetScissor(command, 0, 1, &scissor);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(command, 0, 1, &vertex_buffer, &offset);
#ifdef HALO2_PARTICLE_GRAPHICS_AVAILABLE
        const auto record_particles=[&](VkViewport particle_view,VkRect2D clip,std::size_t first,std::size_t count) {
            if(first>prepared_particles.size() || count>prepared_particles.size()-first)
                throw FormatError("ordered particle draw escapes its prepared frame");
            const auto *hide_value=std::getenv("HALO2_REFERENCE_PARTICLE_DISTORTION_HIDE");
            const bool hide=hide_value && *hide_value;
            std::size_t copies{},hidden{};
            const auto end=first+count;
            while(first<end) {
                if(!particle_renderer->reads_scene_texture(first)) {
                    auto next=first+1;while(next<end && !particle_renderer->reads_scene_texture(next))++next;
                    particle_renderer->record(command,particle_view,clip,first,next-first);first=next;continue;
                }
                const auto &draw=prepared_particles.at(first);
                source_apply_particle_sampler_writes(particle_sampler_state,draw.material->sampler_writes);
                const auto width=draw.vertex.surface_fog[0],height=draw.vertex.surface_fog[1];
                if(!(width>0 && height>0))throw FormatError("particle external23 has no completed surface dimensions");
                particle_renderer->resolve_scene_lookup(first,{source_particle_bump_matrix(particle_sampler_state,1),{1.f/width,1.f/height,0,0}});
                if(hide){++first;++hidden;continue;} // Final presentation control; source state writes still occur.
                vkCmdEndRenderPass(command);
                std::array<VkImageMemoryBarrier,2> barriers{};
                for(auto &barrier:barriers) {
                    barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
                    barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
                }
                auto &color=barriers[0],&target=barriers[1];
                color.image=images[index];color.oldLayout=scene_layout;color.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                color.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;color.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
                target.image=particle_scene.image;target.oldLayout=particle_scene_initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
                target.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                target.srcAccessMask=particle_scene_initialized?VK_ACCESS_SHADER_READ_BIT:0;target.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,barriers.size(),barriers.data());
                VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.extent={extent.width,extent.height,1};
                vkCmdCopyImage(command,images[index],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,particle_scene.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
                color.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;color.newLayout=scene_layout;
                color.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;color.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                target.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;target.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                target.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;target.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,0,nullptr,0,nullptr,barriers.size(),barriers.data());particle_scene_initialized=true;
                auto resumed=render;resumed.renderPass=continuation_pass;resumed.clearValueCount=0;resumed.pClearValues=nullptr;
                vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
                particle_renderer->record(command,particle_view,clip,first,1);++first;++copies;
            }
            if((copies || hidden) && std::getenv("HALO2_REFERENCE_PARTICLE_DISTORTION_TRACE"))
                std::cout<<"Source distortion: copies="<<copies<<" hidden="<<hidden
                    <<" sdk22="<<particle_sampler_state[1][0]<<" sdk23="<<particle_sampler_state[1][1]
                    <<" sdk24="<<particle_sampler_state[1][2]<<" sdk25="<<particle_sampler_state[1][3]<<'\n';
        };
#endif
        const auto matrix = view_projection(scene_camera,scissor.extent,bool(atmosphere));
        struct CameraUniform { glm::mat4 matrix; glm::vec4 eye, position, viewport; };
        const CameraUniform camera_uniform{matrix,glm::vec4(forward,atmosphere && source_depth_mode==2?1.f:0.f),
            glm::vec4(camera.position.x,camera.position.y,camera.position.z,1),
            glm::vec4(viewport.x,viewport.y,viewport.width,viewport.height)};
        vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(camera_uniform),&camera_uniform);
        std::uint32_t material_bank_offset=0;
        bool sky_raster=false,first_person_draw=false;
        const auto material_descriptor=[&](unsigned material,unsigned lightmap) {
            return first_person_draw && scope_descriptors[material]?scope_descriptors[material]:geometry_descriptor(material,lightmap);
        };
        const auto bind_material=[&](const Draw &draw,unsigned material) {
            const bool owner=draw.object_alias!=std::numeric_limits<std::size_t>::max();
            const auto descriptor=owner?object_descriptors[object_aliases.at(draw.object_alias).at({material,draw.lightmap})]:
                material_descriptor(material,draw.lightmap);
            const std::uint32_t bank=owner?0u:material_bank_offset;
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,&descriptor,1,&bank);
        };
        auto material_pipeline=[&](unsigned m) {const auto index=active_forward_pipeline_indices[m];
            const auto handle=index!=~0u?(sky_raster?forward_pipelines.at(index).sky_handle:forward_pipelines.at(index).handle):
                (sky_raster?sky_pipeline:pipeline);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,handle);
            const auto color=material_blend_colors[m];
            const float constants[]{float((color>>16)&255)/255.f,float((color>>8)&255)/255.f,
                float(color&255)/255.f,float(color>>24)/255.f};
            vkCmdSetBlendConstants(command,constants);};
        const BspAtmosphere *visibility_bank=nullptr;
        const SourceBspAdmission *bsp_admission=nullptr;
        const SourceGeometryLodPolicy *source_policy=nullptr;
        bool source_transparency_active=false;
        const auto layer_enabled=[&](unsigned material) {
            const auto layer=source_material_layers.at(material);
            if(source_transparency_active && layer==15)return false;
            return !source_policy || !layer || source_render_layer_enabled(*source_policy,*layer);
        };
        std::vector<std::uint32_t> visible_clusters;
        auto draw_visible=[&](const Draw &draw) {
            if(bsp_admission)return bsp_admission->geometry_visible(draw.cluster);
            return !visibility_bank || visibility_bank->geometry_visible(draw.cluster,visible_clusters);
        };
        const auto bind_draw_vertices=[&](const Draw &draw,VkBuffer ordinary) {
            const auto buffer=draw.source_resident?vertex_buffer:ordinary;
            vkCmdBindVertexBuffers(command,0,1,&buffer,&offset);
        };
        const auto texture_mapping_overlay=[&](unsigned material) {
            return source_material_layers[material]==2;
        };
        auto draw_dynamic = [&](const std::vector<Draw>& list,int phase=-1) {
            vkCmdBindVertexBuffers(command, 0, 1, &debug_buffer, &offset);
            for (const auto &draw:list) if(draw.count && draw_visible(draw) && layer_enabled(draw.material) && !water_materials[draw.material] &&
                projected_decal_layers[draw.material]==~0u &&
                !texture_mapping_overlay(draw.material) &&
                (phase<0 || after_fog[draw.material]==bool(phase))) {
                material_pipeline(draw.material);
                bind_material(draw,draw.material);
                bind_draw_vertices(draw,debug_buffer);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
        };
        const auto draw_decals=[&](const std::vector<Draw>& list) {
            // TU1.5 2C04A..2C068: layers2,3,4,5 follow dynamic lights and
            // texture mapping, before25D30 fog. Do not sort them by texture.
            for(unsigned layer=2;layer<=5;++layer)for(const auto &draw:list)
                if(draw.count && draw_visible(draw) &&
                   projected_decal_layers[draw.material]==layer) {
                    material_pipeline(draw.material);bind_material(draw,draw.material);
                    bind_draw_vertices(draw,debug_buffer);
                    vkCmdDraw(command,draw.count,1,draw.first,0);
                }
        };
        auto draw_texture_mapping_overlays = [&](const std::vector<Draw>& list,VkBuffer buffer) {
            // 2C052 submits layer2 after lighting/environment/self illumination
            // and before fog. 44370's layer2 target is the current final target,
            // not layer1's accumulation target. Emblems are standalone parts.
            for(const auto &draw:list)if(draw.count && draw_visible(draw) &&
                texture_mapping_overlay(draw.material) && layer_enabled(draw.material)) {
                material_pipeline(draw.material);bind_material(draw,draw.material);
                bind_draw_vertices(draw,buffer);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
        };
        auto draw_overlays = [&](const std::vector<Draw>& list,bool before,VkBuffer buffer) {
            vkCmdBindVertexBuffers(command,0,1,&buffer,&offset);
            for(const auto &draw:list)if(draw_visible(draw))for(const auto material:(before?world_before:world_after)[draw.material]) {
                if(!layer_enabled(material))continue;
                material_pipeline(material);bind_material(draw,material);
                bind_draw_vertices(draw,buffer);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
        };
        auto draw_environment = [&](const std::vector<Draw>& list) {
            vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
            for(const auto &draw:list)if(draw.count && draw_visible(draw) && environment_materials[draw.material]!=~0u) {
                const auto material=environment_materials[draw.material];
                if(!layer_enabled(material))continue;
                material_pipeline(material);
                bind_material(draw,material);
                bind_draw_vertices(draw,debug_buffer);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
            draw_overlays(list,false,debug_buffer);
            draw_texture_mapping_overlays(list,debug_buffer);
            draw_overlays(list,true,debug_buffer);
        };
        auto base_dynamic=[&](const std::vector<Draw>& list) {
            vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
            for(const auto &draw:list)if(draw.count && draw_visible(draw) && environment_materials[draw.material]!=~0u) {
                if(!layer_enabled(draw.material))continue;
                material_pipeline(draw.material);
                bind_material(draw,draw.material);
                bind_draw_vertices(draw,debug_buffer);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
        };
        auto composite_view_fog=[&](VkDescriptorSet descriptor) {
            vkCmdEndRenderPass(command);
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=depth;barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};
            if(depth_format==VK_FORMAT_D24_UNORM_S8_UINT)barrier.subresourceRange.aspectMask|=VK_IMAGE_ASPECT_STENCIL_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            auto composite=render;composite.renderPass=fog_pass;composite.framebuffer=fog_framebuffers[index];
            composite.clearValueCount=0;composite.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&composite,VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,fog_pipeline);
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,fog_layout,0,1,&descriptor,0,nullptr);
            vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
            barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                0,0,nullptr,0,nullptr,1,&barrier);
            auto resumed=render;resumed.renderPass=continuation_pass;
            resumed.clearValueCount=0;resumed.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
        };
        const bool empty_full_surface_accumulation=full_surface_scene &&
            background_draws.empty() && dynamic_draws.empty() && foreground_draws.empty();
        if(accumulation_pass && !empty_full_surface_accumulation) {
            // Extern 3 is the texture-mapping accumulation surface. The same
            // camera, posed vertices and viewport feed both authored passes.
            auto target=render;target.renderPass=accumulation_pass;target.framebuffer=accumulation_framebuffer;
            std::array<VkClearValue,2> target_clear{};target_clear[1].depthStencil={1,0};
            target.pClearValues=target_clear.data();
            vkCmdBeginRenderPass(command,&target,VK_SUBPASS_CONTENTS_INLINE);
            sky_raster=true;base_dynamic(background_draws);sky_raster=false;
            vkCmdBindVertexBuffers(command,0,1,&vertex_buffer,&offset);
            for(const auto &draw:draws) if(!full_surface_scene && environment_materials[draw.material]!=~0u) {
                material_pipeline(draw.material);
                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,&draw.descriptor,1,&material_bank_offset);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
            base_dynamic(dynamic_draws);
            VkClearAttachment depth_clear{};depth_clear.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
            depth_clear.clearValue.depthStencil={1,0};VkClearRect area{scissor,0,1};
            vkCmdClearAttachments(command,1,&depth_clear,1,&area);
            base_dynamic(foreground_draws);
            vkCmdEndRenderPass(command);
        }
        vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
        if(scene_clear) {
            VkClearAttachment attachment{};attachment.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            attachment.clearValue.color={{scene_clear->x,scene_clear->y,scene_clear->z,1}};
            VkClearRect rectangle{scissor,0,1};vkCmdClearAttachments(command,1,&attachment,1,&rectangle);
        }
        vkCmdBindVertexBuffers(command,0,1,&vertex_buffer,&offset);
        if (!background.empty()) {
            sky_raster=true;
            draw_dynamic(background_draws);
            draw_environment(background_draws);
            sky_raster=false;
            VkClearAttachment attachment{};
            attachment.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
            attachment.clearValue.depthStencil={1,0};
            VkClearRect rectangle{scissor,0,1};
            vkCmdClearAttachments(command,1,&attachment,1,&rectangle);
            vkCmdBindVertexBuffers(command,0,1,&vertex_buffer,&offset);
        }
        auto draw_world_phase=[&](unsigned phase) {
            if(full_surface_scene || (phase && !atmosphere))return;
            vkCmdBindVertexBuffers(command,0,1,&vertex_buffer,&offset);
            for(const auto &draw:draws) if(draw.count && !water_materials[draw.material] &&
                !texture_mapping_overlay(draw.material) &&
                (!atmosphere || after_fog[draw.material]==bool(phase))) {
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
                if(!phase)if(const auto raw=source_title_draws.find(draw_key(draw));raw!=source_title_draws.end()) {
                    for(const auto &pair:raw->second) {
                        menu_world_native->record(command,pair.first);
                        ++source_base_recorded;
                    }
                    vkCmdBindVertexBuffers(command,0,1,&vertex_buffer,&offset);
                    continue;
                }
#endif
                material_pipeline(draw.material);
                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,&draw.descriptor,1,&material_bank_offset);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
        };
        draw_world_phase(0);
        for(const auto &draw:draws) if(!full_surface_scene && environment_materials[draw.material]!=~0u) {
            const auto material=environment_materials[draw.material];material_pipeline(material);
            const auto descriptor=geometry_descriptor(material,draw.lightmap);
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,&descriptor,1,&material_bank_offset);
            vkCmdDraw(command,draw.count,1,draw.first,0);
        }
        if(!full_surface_scene) {
            draw_overlays(draws,false,vertex_buffer);
            draw_texture_mapping_overlays(draws,vertex_buffer);
            draw_overlays(draws,true,vertex_buffer);
        }
        if (!debug.empty()) {draw_dynamic(dynamic_draws,atmosphere?0:-1);draw_environment(dynamic_draws);}
        if(!debug.empty())draw_decals(dynamic_draws);
        if(atmosphere) {
            // 2BCD0 submits 25D50's selected pass before layer 16 and 15.
            // The plasma programs already attenuate
            // their own colors with c12..17; do not fog them a second time.
            if(menu_fog) {
                vkCmdEndRenderPass(command);
                VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
                barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
                barrier.image=depth;barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};
                if(depth_format==VK_FORMAT_D24_UNORM_S8_UINT)barrier.subresourceRange.aspectMask|=VK_IMAGE_ASPECT_STENCIL_BIT;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
                auto fog_render=render;fog_render.renderPass=fog_pass;fog_render.framebuffer=fog_framebuffers[index];
                fog_render.clearValueCount=0;fog_render.pClearValues=nullptr;
                vkCmdBeginRenderPass(command,&fog_render,VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,fog_pipeline);
                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,fog_layout,0,1,&fog_descriptor,0,nullptr);
                vkCmdDraw(command,3,1,0,0);
                vkCmdEndRenderPass(command);
                barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    0,0,nullptr,0,nullptr,1,&barrier);
                auto resumed=render;resumed.renderPass=continuation_pass;
                resumed.clearValueCount=0;resumed.pClearValues=nullptr;
                vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
                vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(camera_uniform),&camera_uniform);
            }
            // 2BCD0: 25D50 selected fog, then layer 16 at 2C0DB, then layer 15
            // at 2C173. Only the audited BSP admissions are submitted here.
            vkCmdBindVertexBuffers(command,0,1,&vertex_buffer,&offset);
            const auto &plane=atmosphere->planar;
            const float height=((camera.position.z*plane.plane[2]+camera.position.y*plane.plane[1])+
                camera.position.x*plane.plane[0])-plane.plane[3];
            const bool submerged=(0.f-height/plane.opaque_depth)>.9999f && height<1023.f;
            for(const auto &draw:draws) {
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
                if(const auto raw=source_title_draws.find(draw_key(draw));raw!=source_title_draws.end()) {
                    for(const auto &pair:raw->second)if(pair.second) {
                        menu_world_native->record(command,*pair.second);
                        ++source_fog_recorded;
                    }
                    vkCmdBindVertexBuffers(command,0,1,&vertex_buffer,&offset);
                    continue;
                }
#endif
                if(submerged || !draw.count || planar_materials[draw.material]==~0u)continue;
                const auto material=planar_materials[draw.material];material_pipeline(material);
                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,&textures[material].descriptor,1,&material_bank_offset);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
        }
        // Authored material order is retained within the transparent phase.
        // Per-view retail transparent sorting remains a separate task.
        draw_world_phase(1);
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
        std::size_t expected_base{},expected_fog{};
        for(const auto &[identity,pairs]:source_title_draws) {
            expected_base+=pairs.size();
            expected_fog+=std::count_if(pairs.begin(),pairs.end(),
                [](const auto &pair){return bool(pair.second);});
        }
        if(source_base_recorded!=expected_base || source_fog_recorded!=expected_fog)
            throw FormatError("source title replacement did not submit its complete base/fog pairs");
        if(source_title_first && !source_title_draws.empty())
            std::cout<<"Source title recorded: base="<<source_base_recorded<<" fog="<<source_fog_recorded
                <<" duplicate_cpu_base=0 duplicate_cpu_fog=0\n";
#endif
        if(atmosphere && !debug.empty())draw_dynamic(dynamic_draws,1);
        if(!volume_draws.empty() || !particle_draws.empty()) {
            // Original captured glow groups occur after body/beam layer 15,
            // before the direct patchy-haze composition and bloom.
            const VkViewport full_viewport{0,0,float(extent.width),float(extent.height),0,1};
            auto volume_camera=camera_uniform;
            volume_camera.viewport={0,0,float(extent.width),float(extent.height)};
            vkCmdSetViewport(command,0,1,&full_viewport);
            vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(volume_camera),&volume_camera);
            std::size_t particle_cursor=0;
            for(std::size_t group=0;group<=volume_draws.size();++group) {
#ifdef HALO2_PARTICLE_GRAPHICS_AVAILABLE
                const auto first=particle_cursor;
                while(particle_cursor<particle_draws.size() &&
                      prepared_particles[particle_cursor].before_light_volume==group)++particle_cursor;
                if(particle_cursor>first)record_particles(full_viewport,scissor,
                    first,particle_cursor-first);
#endif
                if(group==volume_draws.size())break;
                const auto &draw=volume_draws[group];
                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,light_volume_pipeline);
                vkCmdBindVertexBuffers(command,0,1,&light_volume_buffer,&offset);
                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,
                    &textures[draw.material].descriptor,1,&material_bank_offset);
                vkCmdDraw(command,draw.count,1,draw.first,0);
            }
            vkCmdSetViewport(command,0,1,&viewport);
            vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(camera_uniform),&camera_uniform);
            vkCmdBindVertexBuffers(command,0,1,&vertex_buffer,&offset);
        }
        if(title_scene && atmosphere->patchy) {
            // 2BCD0's non-sort-behind branch runs 25DC0 after material layer 15.
            // The original fpch separates the four slice depths in target 19.
            vkCmdEndRenderPass(command);
            VkClearValue clear_noise{};
            VkRenderPassBeginInfo noise_render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            noise_render.renderPass=patchy_pass;noise_render.framebuffer=patchy_framebuffer;
            noise_render.renderArea={{0,0},patchy_extent};noise_render.clearValueCount=1;noise_render.pClearValues=&clear_noise;
            vkCmdBeginRenderPass(command,&noise_render,VK_SUBPASS_CONTENTS_INLINE);
            const VkViewport noise_view{0,0,float(patchy_extent.width),float(patchy_extent.height),0,1};
            const VkRect2D noise_clip{{0,0},patchy_extent};
            vkCmdSetViewport(command,0,1,&noise_view);vkCmdSetScissor(command,0,1,&noise_clip);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,patchy_noise_pipeline);
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,patchy_layout,0,1,&patchy_descriptors[0],0,nullptr);
            vkCmdDraw(command,6,1,0,0);vkCmdEndRenderPass(command);
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=depth;barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};
            if(depth_format==VK_FORMAT_D24_UNORM_S8_UINT)barrier.subresourceRange.aspectMask|=VK_IMAGE_ASPECT_STENCIL_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
            auto patchy_render=render;patchy_render.renderPass=fog_pass;patchy_render.framebuffer=fog_framebuffers[index];
            patchy_render.clearValueCount=0;patchy_render.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&patchy_render,VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,patchy_composite_pipeline);
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,patchy_layout,0,1,&patchy_descriptors[1],0,nullptr);
            vkCmdDraw(command,6,1,0,0);vkCmdEndRenderPass(command);
            barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            auto resumed=render;resumed.renderPass=continuation_pass;resumed.clearValueCount=0;resumed.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
            vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(camera_uniform),&camera_uniform);
        }
        if (!first_person.empty()) {
            VkClearAttachment depth_clear{};
            depth_clear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depth_clear.clearValue.depthStencil = {1,0};
            VkClearRect area{{{0,0},extent},0,1};
            vkCmdClearAttachments(command,1,&depth_clear,1,&area);
            draw_dynamic(foreground_draws);
            draw_environment(foreground_draws);
        }
        if(title_scene && bloom_threshold) {
            if(!can_capture)throw FormatError("title bloom requires swapchain transfer-source support");
            copy_scene_for_post(index);
            VkClearValue clear_bloom{};
            const VkViewport quarter_view{0,0,float(bloom_extent.width),float(bloom_extent.height),0,1};
            const VkRect2D quarter_clip{{0,0},bloom_extent};
            vkCmdSetViewport(command,0,1,&quarter_view);vkCmdSetScissor(command,0,1,&quarter_clip);
            for(unsigned i=0;i<5;++i) {
                VkRenderPassBeginInfo blur{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
                blur.renderPass=patchy_pass;blur.framebuffer=bloom_framebuffers[i%2];blur.renderArea=quarter_clip;
                blur.clearValueCount=1;blur.pClearValues=&clear_bloom;
                vkCmdBeginRenderPass(command,&blur,VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,bloom_filter_pipeline);
                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,bloom_layout,0,1,&bloom_descriptors[i],0,nullptr);
                vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
            }
            vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
            auto composite=render;composite.renderPass=fog_pass;composite.framebuffer=fog_framebuffers[index];
            composite.clearValueCount=0;composite.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&composite,VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,bloom_composite_pipeline);
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,bloom_layout,0,1,&bloom_descriptors[5],0,nullptr);
            vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
            auto resumed=render;resumed.renderPass=continuation_pass;resumed.clearValueCount=0;resumed.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
            vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(camera_uniform),&camera_uniform);
        }
        if(title_scene && scene_filter) {
            if(!can_capture)throw FormatError("title color filter requires transfer-source support");
            // The original desaturation pass samples the world AFTER bloom,
            // with blending disabled, before any title bitmap or text.
            copy_scene_for_post(index);
            auto filtered=render;filtered.renderPass=fog_pass;filtered.framebuffer=fog_framebuffers[index];
            filtered.clearValueCount=0;filtered.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&filtered,VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,scene_filter_pipeline);
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,bloom_layout,0,1,&bloom_descriptors[6],0,nullptr);
            vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
            auto resumed=render;resumed.renderPass=continuation_pass;resumed.clearValueCount=0;resumed.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
            vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(camera_uniform),&camera_uniform);
        }
        if (reticle && effects.hud > 0 && extent.width > 16 && extent.height > 16) {
            VkClearAttachment color{};
            color.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            color.clearValue.color = {{0.9f * effects.hud,0.9f * effects.hud,0.9f * effects.hud,1}};
            // The workbench marker uses the same 640x480 layout scale as UI;
            // its size/thickness must not shrink as display resolution grows.
            const float hud_scale=std::min(float(extent.width)/640.f,float(extent.height)/480.f);
            const auto thickness=std::max(1u,unsigned(std::lround(hud_scale)));
            const auto length=std::max(1u,unsigned(std::lround(11.f*hud_scale)));
            const int x=int(extent.width/2),y=int(extent.height/2);
            VkClearRect marks[2]{{{{x-int(length/2),y-int(thickness/2)}, {length,thickness}},0,1},
                                 {{{x-int(thickness/2),y-int(length/2)}, {thickness,length}},0,1}};
            vkCmdClearAttachments(command, 1, &color, 2, marks);
        }
        if (effects.fade > 0 || effects.letterbox > 0) {
            vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, overlay);
            if (effects.letterbox > 0) {
                const std::array<float, 8> values{0,0,0,0,float(extent.width),float(extent.height),effects.letterbox * .125f,effects.letterbox};
                vkCmdPushConstants(command, overlay_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(values), values.data());
                vkCmdDraw(command, 3, 1, 0, 0);
            }
            if (effects.fade > 0) {
                const std::array<float, 8> values{effects.fade_color.x,effects.fade_color.y,effects.fade_color.z,effects.fade,float(extent.width),float(extent.height),0,0};
                vkCmdPushConstants(command, overlay_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(values), values.data());
                vkCmdDraw(command, 3, 1, 0, 0);
            }
        }
        if (!ui.empty() || !views.empty() || !backdrops.empty()) {
            const float scale = menu_canvas.scale;
            const float x = menu_canvas.left;
            const float y = menu_canvas.top;
            VkViewport ui_view{x,y,640.f*scale,480.f*scale,0,1};
            VkViewport hud_view{0,0,float(extent.width),float(extent.height),0,1};
            VkViewport wide_ui_view{0,y,float(extent.width),480.f*scale,0,1};
            const int canvas_left=int(std::ceil(x-.5f));
            const int canvas_right=int(std::ceil(x+menu_canvas.width-.5f));
            const int canvas_top=int(std::ceil(y-.5f));
            const int canvas_bottom=int(std::ceil(y+menu_canvas.height-.5f));
            VkRect2D wide_ui_clip{{0,canvas_top},{extent.width,unsigned(canvas_bottom-canvas_top)}};
            VkRect2D ui_clip{{canvas_left,canvas_top},
                {unsigned(canvas_right-canvas_left),unsigned(canvas_bottom-canvas_top)}};
            if(std::any_of(views.begin(),views.end(),[](const auto &v){return v.full_surface;}))
                ui_clip={{0,0},extent};
            vkCmdSetViewport(command,0,1,&ui_view);
            vkCmdSetScissor(command,0,1,&ui_clip);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,ui_pipeline);
            vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
            auto draw_view = [&](std::size_t i) {
                const auto& v=views[i];
                // Preserve intra-view native W-depth history after the existing
                // full-frame/per-view initialization. This is not admission of
                // the original132D0 conditional clear or caller profile.
                const bool retain_world_history=v.full_surface && v.atmosphere.has_value();
                source_policy=v.source_render_policy?&*v.source_render_policy:nullptr;
                source_transparency_active=v.world_transparent.has_value();
                if(v.width<=0 || v.height<=0)return;
                if(v.camera.projection_vertical_angle) {
                    const float angle=*v.camera.projection_vertical_angle;
                    if(!std::isfinite(angle) || angle<=0 || angle>=3.14159265358979323846f)return;
                } else if(v.camera.field_of_view<=0 || v.camera.field_of_view>=180)return;
                const float sx=v.full_surface?float(extent.width)/640.f:scale;
                const float sy=v.full_surface?float(extent.height)/480.f:scale;
                VkViewport model_view{(v.full_surface?0.f:x)+v.left*sx,(v.full_surface?0.f:y)+v.top*sy,v.width*sx,v.height*sy,0,1};
                if(v.full_surface) {
                    const auto physical=full_surface_viewport({v.left,v.top,v.width,v.height},extent.width,extent.height);
                    model_view={physical.left,physical.top,physical.width,physical.height,0,1};
                }
                if(v.source_ui_viewport) {
                    const auto &r=*v.source_ui_viewport;
                    model_view={float(r[1]),float(r[0]),float(r[3]-r[1]),float(r[2]-r[0]),0,1};
                }
                const auto left=std::max(0,int(model_view.x)),top=std::max(0,int(model_view.y));
                const auto right=std::min(int(extent.width),int(model_view.x+model_view.width));
                const auto bottom=std::min(int(extent.height),int(model_view.y+model_view.height));
                if(right<=left || bottom<=top)return;
                VkRect2D clip{{left,top},{unsigned(right-left),unsigned(bottom-top)}};
                VkClearAttachment depth_clear{};depth_clear.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;depth_clear.clearValue.depthStencil={1,0};
                VkClearRect area{clip,0,1};
                if(v.source_view_clear) {
                    const auto &events=source_initializations[i];
                    const auto argb=events[1].words[1];constexpr float byte_scale=1.f/255.f;
                    VkClearAttachment source_clear[2]{};
                    source_clear[0].aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;source_clear[0].colorAttachment=0;
                    source_clear[0].clearValue.color={{float((argb>>16)&255)*byte_scale,float((argb>>8)&255)*byte_scale,
                        float(argb&255)*byte_scale,float(argb>>24)*byte_scale}};
                    source_clear[1].aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;
                    source_clear[1].clearValue.depthStencil={std::bit_cast<float>(events[1].words[2]),events[1].words[3]};
                    vkCmdClearAttachments(command,2,source_clear,1,&area);
                } else {
                  vkCmdClearAttachments(command,1,&depth_clear,1,&area);
                  if(v.full_surface) {
                    VkClearAttachment color_clear{};color_clear.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
                    const auto color=v.atmosphere?v.atmosphere->clear.value_or(Vec3{}):scene_clear.value_or(Vec3{.02f,.025f,.035f});
                    color_clear.colorAttachment=0;color_clear.clearValue.color={{color.x,color.y,color.z,v.atmosphere?0.f:1.f}};
                    vkCmdClearAttachments(command,1,&color_clear,1,&area);
                  }
                }
                vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
                const auto model_matrix=view_projection(v.camera,{unsigned(model_view.width),unsigned(model_view.height)},bool(v.atmosphere),v.projection_clip_planes);
                auto foreground_camera=v.camera;
                if(v.foreground_projection_vertical_angle)
                    foreground_camera.projection_vertical_angle=v.foreground_projection_vertical_angle;
                const auto foreground_matrix=v.foreground_projection_vertical_angle?
                    view_projection(foreground_camera,{unsigned(model_view.width),unsigned(model_view.height)},bool(v.atmosphere),v.projection_clip_planes):model_matrix;
                visibility_bank=v.atmosphere_source;
                bsp_admission=v.source_bsp_admission;
                visible_clusters=visibility_bank && v.atmosphere?
                    visibility_bank->potentially_visible_clusters(v.atmosphere->cluster):std::vector<std::uint32_t>{};
                const auto view_forward=v.camera.oriented?v.camera.forward:direction(v.camera.yaw,v.camera.pitch);
                const CameraUniform model_uniform{model_matrix,glm::vec4(view_forward.x,view_forward.y,view_forward.z,v.atmosphere && source_depth_mode==2?1.f:0.f),
                    glm::vec4(v.camera.position.x,v.camera.position.y,v.camera.position.z,1),
                    glm::vec4(model_view.x,model_view.y,model_view.width,model_view.height)};
                const auto draw_transparent=[&](unsigned phase) {
                    for(const auto &step:transparent_views[i][phase]) {
                        if(step.contrail_source) {
#if HALO2_SOURCE_CONTRAILS_AVAILABLE
                            const auto index=std::size_t(step.contrail_source-v.contrails.data());
                            if(!contrail_renderer || index>=v.contrails.size())throw FormatError("Contrail transparent range lost its prepared owner");
                            contrail_renderer->record(command,model_view,clip,contrail_first[i]+index,1);
#endif
                            continue;
                        }
                        if(step.light_source) {
                            for(const auto &draw:step.lights)if(draw.count) {
                                auto push=model_uniform;push.viewport={0,0,float(extent.width),float(extent.height)};
                                const VkViewport full{0,0,float(extent.width),float(extent.height),0,1};
                                vkCmdSetViewport(command,0,1,&full);vkCmdSetScissor(command,0,1,&clip);
                                vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(push),&push);
                                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,light_volume_pipeline);
                                vkCmdBindVertexBuffers(command,0,1,&light_volume_buffer,&offset);
                                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,
                                    &textures[draw.material].descriptor,1,&material_bank_offset);
                                vkCmdDraw(command,draw.count,1,draw.first,0);
                            }
                            if(!step.lights.empty() && v.light_volume_observer)v.light_volume_observer(*step.light_source);
                            continue;
                        }
                        if(step.particles) {
#ifdef HALO2_PARTICLE_GRAPHICS_AVAILABLE
                            const auto &range=view_particle_ranges[i];const auto &particles=*step.particles;
                            if(particles.count)record_particles(
                                {0,0,float(extent.width),float(extent.height),0,1},clip,
                                (phase?range.foreground:range.world)+particles.first,particles.count);
#endif
                            continue;
                        }
                        if(source_policy && !source_render_layer_enabled(*source_policy,15))continue;
                        auto push=model_uniform;
                        if(step.first_person_projection) {
                            push.matrix=foreground_matrix;
                            push.position.w=2;
                            if(v.atmosphere){push.matrix*=.15f;push.eye.w=.15f;}
                        }
                        vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                        vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(push),&push);
                        for(const auto &draw:step.geometry)if(draw.count)
                            for(auto material:source_layer15_passes[draw.material]) {
                                material_pipeline(material);bind_material(draw,material);
                                bind_draw_vertices(draw,debug_buffer);
                                vkCmdDraw(command,draw.count,1,draw.first,0);
                            }
                    }
                    vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                };
                vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                material_bank_offset=std::uint32_t(material_stride*material_uniforms.size()*(i+1));
                if(accumulation_pass) {
                    // Each local player and clipped model owns its projected
                    // coordinates. Generate extern 3 with that view's camera
                    // before its lighting layer samples the image.
                    vkCmdEndRenderPass(command);
                    auto target=render;target.renderPass=retain_world_history?world_accumulation_pass:accumulation_pass;
                    target.framebuffer=accumulation_framebuffer;target.renderArea=clip;
                    std::array<VkClearValue,2> target_clear{};target_clear[1].depthStencil={1,0};
                    target.pClearValues=target_clear.data();
                    if(retain_world_history)target.clearValueCount=1;
                    vkCmdBeginRenderPass(command,&target,VK_SUBPASS_CONTENTS_INLINE);
                    sky_raster=true;base_dynamic(view_background[i]);sky_raster=false;
                    if(!retain_world_history)vkCmdClearAttachments(command,1,&depth_clear,1,&area);
                    base_dynamic(view_draws[i]);
                    vkCmdEndRenderPass(command);
                    auto resumed=render;resumed.renderPass=continuation_pass;
                    resumed.clearValueCount=0;resumed.pClearValues=nullptr;
                    vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
                    if(!retain_world_history)vkCmdClearAttachments(command,1,&depth_clear,1,&area);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                }
                if(!retain_world_history && !v.background.empty()) {
                    sky_raster=true;
                    draw_dynamic(view_background[i]);draw_environment(view_background[i]);
                    sky_raster=false;
                    vkCmdClearAttachments(command,1,&depth_clear,1,&area);
                }
                draw_dynamic(view_draws[i],v.atmosphere?0:-1);
                draw_environment(view_draws[i]);
#if HALO2_SOURCE_DECALS_AVAILABLE
                if(decal_renderer) {
                    const auto [first,count]=decal_ranges[i];
                    decal_renderer->record(command,model_view,clip,first,count,1u);
                    vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                }
#endif
                if(!shadow_views[i].empty()) {
                    // Original335E0 follows world/environment layer8/20 and
                    // precedes fog, late transparency and first person.
                    std::vector<SourceShadowSubmission> submissions;
                    std::vector<std::array<std::vector<SourceShadowDraw>,2>> roots(shadow_views[i].size());
                    submissions.reserve(roots.size());
                    for(std::size_t r=0;r<roots.size();++r) {
                        for(unsigned phase=0;phase<2;++phase)for(const auto &draw:shadow_views[i][r].draws[phase]) {
                            auto push=model_uniform;
                            push.eye.w=1;push.position.w=1;
                            if(!phase)push.viewport=glm::vec4(0,0,128,128);
                            const auto descriptor=shadow_descriptors[draw.binding];
                            roots[r][phase].push_back({draw.authored,[&,draw,descriptor,push](VkCommandBuffer buffer,const SourceShadowUniforms &) {
                                vkCmdBindPipeline(buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.pipeline);
                                vkCmdBindVertexBuffers(buffer,0,1,&debug_buffer,&offset);
                                constexpr std::uint32_t dynamic_offset=0;
                                vkCmdBindDescriptorSets(buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,
                                    0,1,&descriptor,1,&dynamic_offset);
                                vkCmdPushConstants(buffer,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(push),&push);
                                vkCmdDraw(buffer,draw.count,1,draw.first,0);
                            }});
                        }
                        submissions.push_back({shadow_views[i][r].uniforms,roots[r][0],roots[r][1]});
                    }
                    shadow_render->record(command,{framebuffers[index],model_view,clip},submissions);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                    vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                }
                // Original2BCD0 runs31C80 after335E0, retaining the same world
                // depth. Explicit submissions already contain source-cull and
                // shader selections; preserve their complete light/pass order.
                for(const auto &draw:light_views[i]) {
                    material_pipeline(draw.material);
                    vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
                    const auto descriptor=light_descriptors[draw.binding];
                    constexpr std::uint32_t dynamic_offset=0;
                    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,
                        0,1,&descriptor,1,&dynamic_offset);
                    vkCmdDraw(command,draw.count,1,draw.first,0);
                }
#ifdef HALO2_MENU_WORLD_NATIVE_AVAILABLE
                const VkViewport raw_light_viewport{0,0,float(extent.width),float(extent.height),0,1};
                if(!raw_light_views[i].empty())vkCmdSetViewport(command,0,1,&raw_light_viewport);
                for(const auto draw:raw_light_views[i])effect_light_native->record(command,draw);
                if(!raw_light_views[i].empty()) {
                    vkCmdSetViewport(command,0,1,&model_view);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                    vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
                }
#endif
#if HALO2_SOURCE_DECALS_AVAILABLE
                if(decal_renderer) {
                    const auto [first,count]=decal_ranges[i];
                    decal_renderer->record(command,model_view,clip,first,count,0x3eu);
                    vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                }
#endif
                if(retain_world_history && !v.background.empty()) {
                    // The named ordinary source schedule places main sky after
                    // world/shadows and before fog without clearing shared DS.
                    // Retain the earlier base pass for authored extern3 users
                    // and each sky material's raster state; owner/part admission
                    // for dedicated4C690 remains a separate source input.
                    sky_raster=true;
                    draw_dynamic(view_background[i]);draw_environment(view_background[i]);
                    sky_raster=false;
                }
                draw_decals(view_draws[i]);
                if(v.atmosphere) {
                    if(view_fog_enabled[i])composite_view_fog(view_fog_descriptors[i]);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                    vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
                    for(const auto &draw:view_draws[i]) {
                        if(!draw_visible(draw))continue;
                        const bool containing=bsp_admission?
                            bsp_admission->geometry_contains_fog(draw.cluster):
                            (v.atmosphere_source && v.atmosphere_source->contains_planar(
                                draw.cluster,v.atmosphere->planar_datum,visible_clusters));
                        const auto implementation=bsp_planar_fog_implementation(*v.atmosphere,containing);
                        if(!implementation)continue;
                        const auto material=world_planar_materials[draw.material][*implementation];
                        if(material==~0u)continue;
                        material_pipeline(material);bind_material(draw,material);
                        bind_draw_vertices(draw,debug_buffer);
                        vkCmdDraw(command,draw.count,1,draw.first,0);
                    }
                }
                if(water_copy_pass && !v.water.empty()) {
                    // 3BD00 renders sky alone into 21; 3BEA0/352E0 preserve
                    // the current viewport's pre-water color in target 22.
                    vkCmdEndRenderPass(command);
                    constexpr VkExtent2D target_extent{512,256};
                    VkViewport target_view{0,0,512,256,0,1};VkRect2D target_clip{{0,0},target_extent};
                    vkCmdSetViewport(command,0,1,&target_view);vkCmdSetScissor(command,0,1,&target_clip);
                    std::array<VkClearValue,2> water_clear{};water_clear[1].depthStencil={1,0};
                    const auto horizon=v.water.front().horizon_argb;
                    water_clear[0].color={{float((horizon>>16)&255)/255,float((horizon>>8)&255)/255,float(horizon&255)/255,0}};
                    VkRenderPassBeginInfo reflection{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
                    reflection.renderPass=water_reflection_pass;reflection.framebuffer=water_reflection_framebuffer;
                    reflection.renderArea=target_clip;reflection.clearValueCount=2;reflection.pClearValues=water_clear.data();
                    vkCmdBeginRenderPass(command,&reflection,VK_SUBPASS_CONTENTS_INLINE);
                    const auto &c=v.water_reflection_camera;
                    // 3BD00 changes the raster rectangle, retaining the
                    // player's projection coefficients and aspect ratio.
                    const CameraUniform reflected{view_projection(c,{unsigned(model_view.width),unsigned(model_view.height)},true),
                        glm::vec4(c.forward.x,c.forward.y,c.forward.z,0),glm::vec4(c.position.x,c.position.y,c.position.z,3),
                        glm::vec4(0,0,512,256)};
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(reflected),&reflected);
                    sky_raster=true;
                    draw_dynamic(view_reflection_background[i]);draw_environment(view_reflection_background[i]);
                    sky_raster=false;
                    vkCmdEndRenderPass(command);
                    std::array<VkImageMemoryBarrier,2> barriers{};
                    for(auto &barrier:barriers) {
                        barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
                        barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
                    }
                    auto &color=barriers[0],&target=barriers[1];
                    color.image=images[index];color.oldLayout=scene_layout;
                    color.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    color.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;color.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
                    target.image=water_scene.image;target.oldLayout=water_scene_initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
                    target.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    target.srcAccessMask=water_scene_initialized?VK_ACCESS_SHADER_READ_BIT:0;target.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
                    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,barriers.data());
                    VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
                    copy.extent={extent.width,extent.height,1};
                    vkCmdCopyImage(command,images[index],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,water_scene.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
                    color.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;color.newLayout=scene_layout;
                    color.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;color.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    target.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;target.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    target.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;target.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
                    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,0,nullptr,0,nullptr,2,barriers.data());water_scene_initialized=true;
                    auto downsample=reflection;downsample.renderPass=water_copy_pass;downsample.framebuffer=water_copy_framebuffer;
                    downsample.clearValueCount=1;
                    vkCmdBeginRenderPass(command,&downsample,VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,water_copy_pipeline);
                    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,fog_layout,0,1,&water_copy_descriptors[i],0,nullptr);
                    vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
                    auto resumed=render;resumed.renderPass=continuation_pass;resumed.clearValueCount=0;resumed.pClearValues=nullptr;
                    vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                }
                vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
                for(const auto &draw:view_draws[i])if(draw_visible(draw) && water_materials[draw.material]) {
                    if(v.water.empty())throw FormatError("water draw has no authored viewport state");
                    const int definition=water_material_definitions[draw.material];
                    if(definition<0 || std::size_t(definition)>=v.water.size())throw FormatError("water sampler state has no source definition");
                    // Source3C640 writes every logical SDK bump component.
                    // Retain those raw values for later18540 particle writes.
                    for(unsigned stage=1;stage<4;++stage)for(unsigned component=0;component<4;++component)
                        particle_sampler_state[stage][component]=std::bit_cast<std::uint32_t>(v.water[definition].bump_matrices[stage][component]);
                    material_pipeline(draw.material);
                    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,&draw.descriptor,1,&material_bank_offset);
                    bind_draw_vertices(draw,debug_buffer);
                    vkCmdDraw(command,draw.count,1,draw.first,0);
                }
                // 2BCE0 -> 3C3C0 -> 2C340: independent water precedes the
                // authored transparent queue (highest-LOD layer 15).
                if(v.atmosphere)draw_dynamic(view_draws[i],1);
                if(v.world_transparent || sorted_world_effects[i])draw_transparent(0);
#ifdef HALO2_PARTICLE_GRAPHICS_AVAILABLE
                const auto &particle_range=view_particle_ranges[i];
                if(!v.world_transparent && !sorted_world_effects[i] && particle_range.world_count)
                    record_particles({0,0,float(extent.width),float(extent.height),0,1},clip,
                        particle_range.world,particle_range.world_count);
                vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
#endif
                const BspPostprocessTarget post_target{command,images[index],fog_framebuffers[index],framebuffers[index]};
                if(v.atmosphere && bsp_postprocess) {
                    bsp_postprocess->record_patchy(i,post_target);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                }
                //36B50 follows late world effects and precedes first person.
                const auto *hide_weather=std::getenv("HALO2_REFERENCE_WEATHER_HIDE");
                if(!(hide_weather && *hide_weather)) {
                    const auto *hide_plates=std::getenv("HALO2_REFERENCE_WEATHER_PLATES_HIDE");
                    if(!(hide_plates&&*hide_plates)&&!weather_plate_ranges[i].empty()) {
                        vkCmdEndRenderPass(command);
                        if(!weather_submission.fence_reset) {
                            check(vkResetFences(device,1,&fence),"reset current weather frame fence");
                            weather_submission.fence_reset=true;
                        }
                        SourceWeatherDepthAliasRecord alias_record{};
                        alias_record.source={device,depth,depth_format,extent,source_depth_usage,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_SAMPLE_COUNT_1_BIT,1,1,queue_family,VK_SHARING_MODE_EXCLUSIVE};
                        alias_record.command_owner=device;alias_record.command=command;alias_record.completion=fence;
                        alias_record.command_queue_family=queue_family;alias_record.outside_render_pass=true;alias_record.exclusive_access=true;
                        weather_aliases[i]->record(alias_record);weather_alias_pending[i]=true;
                        if(const auto *abort=std::getenv("HALO2_REFERENCE_WEATHER_RECORD_ABORT");abort&&*abort)
                            throw FormatError("controlled weather alias frame recording abort");
                        auto resumed=render;resumed.renderPass=continuation_pass;resumed.clearValueCount=0;resumed.pClearValues=nullptr;
                        vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
                        for(const auto weather:weather_plate_ranges[i])weather_plate_renderers[weather]->record(command,
                            {0,0,float(extent.width),float(extent.height),0,1},clip);
                    }
                    for(const auto weather:weather_draw_ranges[i])
                        weather_point_renderers[weather]->record(command,
                            {0,0,float(extent.width),float(extent.height),0,1},clip);
                    for(const auto weather:weather_line_ranges[i])
                        weather_line_renderers[weather]->record(command,
                            {0,0,float(extent.width),float(extent.height),0,1},clip);
                }
                if(!weather_draw_ranges[i].empty() || !weather_line_ranges[i].empty() || !weather_plate_ranges[i].empty()) {
                    vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                }
                source_transparency_active=v.foreground_transparent.has_value();
                if(!v.foreground.empty()) {
                    if(scope_render && v.scope) {
                        scope_render->record(unsigned(i),command,images[index],framebuffers[index]);
                        vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                    }
                    first_person_draw=bool(v.scope);
                    auto foreground_uniform=model_uniform;
                    foreground_uniform.matrix=foreground_matrix;
                    foreground_uniform.position.w=2;
                    if(v.atmosphere) {
                        // 4D640 draw flag 0x800 selects 12D50's 0.15
                        // projection/W-depth range. World depth survives FP.
                        foreground_uniform.matrix*=.15f;
                        foreground_uniform.eye.w=.15f;
                    }
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(foreground_uniform),&foreground_uniform);
                    if(accumulation_pass) {
                        vkCmdEndRenderPass(command);
                        auto target=render;target.renderPass=retain_world_history?foreground_accumulation_pass:accumulation_pass;
                        target.framebuffer=accumulation_framebuffer;target.renderArea=clip;
                        std::array<VkClearValue,2> target_clear{};target_clear[1].depthStencil={1,0};
                        target.pClearValues=target_clear.data();
                        if(retain_world_history) {target.clearValueCount=0;target.pClearValues=nullptr;}
                        vkCmdBeginRenderPass(command,&target,VK_SUBPASS_CONTENTS_INLINE);
                        base_dynamic(view_foreground[i]);vkCmdEndRenderPass(command);
                        auto resumed=render;resumed.renderPass=continuation_pass;
                        resumed.clearValueCount=0;resumed.pClearValues=nullptr;
                        vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
                    }
                    if(!v.atmosphere)vkCmdClearAttachments(command,1,&depth_clear,1,&area);
                    draw_dynamic(view_foreground[i]);draw_environment(view_foreground[i]);
                    first_person_draw=false;
                }
#ifdef HALO2_PARTICLE_GRAPHICS_AVAILABLE
                if(!v.foreground_transparent && particle_range.foreground_count)
                    record_particles({0,0,float(extent.width),float(extent.height),0,1},clip,
                        particle_range.foreground,particle_range.foreground_count);
                vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
#endif
                if(v.foreground_transparent)draw_transparent(1);
                if(v.atmosphere && bsp_postprocess) {
                    // 2C2F8 submits alpha-only layer 17 after FP; 2C322
                    // consumes that mask in 13870 before the authored HUD.
                    vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                    vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
                    for(const auto &draw:view_draws[i])if(draw_visible(draw))for(const auto material:world_bloom[draw.material]) {
                        if(!layer_enabled(material))continue;
                        material_pipeline(material);
                        bind_material(draw,material);
                        bind_draw_vertices(draw,debug_buffer);
                        vkCmdDraw(command,draw.count,1,draw.first,0);
                    }
                    bsp_postprocess->record_bloom(i,post_target);
                }
                material_bank_offset=0;
                source_policy=nullptr;
                source_transparency_active=false;
                visibility_bank=nullptr;visible_clusters.clear();
                bsp_admission=nullptr;
                vkCmdSetViewport(command,0,1,&ui_view);
                vkCmdSetScissor(command,0,1,v.full_surface?&clip:&ui_clip);
                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,ui_pipeline);
                vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
            };
            auto draw_backdrop = [&](unsigned channel) {
                const auto &backdrop=backdrops[channel];
                const auto &popup=popups[channel];
                if(!can_capture)throw FormatError("popup blur requires a readable color target");
                vkCmdSetViewport(command,0,1,backdrop.widescreen_background?&wide_ui_view:&ui_view);
                vkCmdSetScissor(command,0,1,backdrop.widescreen_background?&wide_ui_clip:&ui_clip);
                // 234F8C tints the completed lower channels BEFORE 235756
                // samples them. Byte packing is part of the original path.
                const auto tint=frontend_popup_tint(backdrop.fade,backdrop.tint);
                const std::array<float,8> color{float((tint>>16)&255)/255.f,
                    float((tint>>8)&255)/255.f,float(tint&255)/255.f,float(tint>>24)/255.f,
                    float(extent.width),float(extent.height),0,0};
                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,popup_tint_pipeline);
                vkCmdPushConstants(command,overlay_layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(color),color.data());
                vkCmdDraw(command,3,1,0,0);
                for(unsigned i=0;i<popup.passes;++i) {
                    copy_scene_for_post(index);
                    auto filtered=render;filtered.renderPass=fog_pass;filtered.framebuffer=fog_framebuffers[index];
                    filtered.clearValueCount=0;filtered.pClearValues=nullptr;
                    vkCmdBeginRenderPass(command,&filtered,VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,scene_filter_pipeline);
                    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,bloom_layout,0,1,&bloom_descriptors[7+channel*4+i],0,nullptr);
                    vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
                    auto resumed=render;resumed.renderPass=continuation_pass;resumed.clearValueCount=0;resumed.pClearValues=nullptr;
                    vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
                }
                vkCmdSetViewport(command,0,1,&ui_view);vkCmdSetScissor(command,0,1,&ui_clip);
                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,ui_pipeline);
                vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
            };
            // Keep model viewports, channel backdrops and alpha-composited
            // widgets in authored order. Foreground popup views at the same
            // boundary are submitted after their lower channels are blurred.
            std::size_t next_view=0;
            unsigned next_backdrop=0;
            for (std::size_t first = 0; first < ui.size() || next_view<views.size() || next_backdrop<backdrops.size();) {
                while(next_backdrop<backdrops.size() && first==backdrops[next_backdrop].before_vertex)
                    draw_backdrop(next_backdrop++);
                while(next_view<views.size() && views[next_view].before_ui_vertex==first)draw_view(next_view++);
                if(first==ui.size())break;
                const auto material = ui[first].material;
                const auto hud_blend=material<hud_materials.size()?hud_materials[material]:0;
                const bool full_surface_ui=material<ui_full_surfaces.size() && ui_full_surfaces[material];
                const bool wide_background=material<ui_widescreen_backgrounds.size() && ui_widescreen_backgrounds[material];
                const bool centered_emblem=material<ui_emblem_materials.size() && ui_emblem_materials[material];
                const bool full_surface=ui_full_surface_bounds||(hud_blend&&!centered_emblem)||full_surface_ui;
                const auto frame_edge=ui_frame_extensions.at(material);
                VkViewport edge_view=ui_view;
                if(frame_edge==UiFrameExtension::Left) {
                    edge_view.x=0;edge_view.width=menu_canvas.left;
                } else if(frame_edge==UiFrameExtension::Right) {
                    edge_view.x=menu_canvas.left+menu_canvas.width;
                    edge_view.width=std::max(0.f,float(extent.width)-edge_view.x);
                }
                auto end = first + 1;
                auto boundary=next_view<views.size()?views[next_view].before_ui_vertex:ui.size();
                if(next_backdrop<backdrops.size())boundary=std::min(boundary,backdrops[next_backdrop].before_vertex);
                while (end < boundary && ui[end].material == material) ++end;
                if(frame_edge!=UiFrameExtension::None && edge_view.width<=0) {first=end;continue;}
                const VkRect2D edge_clip{{frame_edge==UiFrameExtension::Left?0:canvas_right,canvas_top},
                    {frame_edge==UiFrameExtension::Left?unsigned(canvas_left):extent.width-unsigned(canvas_right),
                     unsigned(canvas_bottom-canvas_top)}};
                vkCmdSetViewport(command,0,1,frame_edge!=UiFrameExtension::None?&edge_view:
                    full_surface?&hud_view:wide_background?&wide_ui_view:&ui_view);
                const VkRect2D full_ui_clip{{0,0},extent};
                vkCmdSetScissor(command,0,1,frame_edge!=UiFrameExtension::None?&edge_clip:
                    full_surface?&full_ui_clip:wide_background?&wide_ui_clip:&ui_clip);
                if(const auto& loading=vista_loading_materials.at(material)) {
                    struct LoadingPush { ShaderVector constant0;Vec2 clip_offset;std::uint32_t program,reserved; };
                    static_assert(sizeof(LoadingPush)==32);
                    const LoadingPush push{loading->constant0,{},loading->shader,0};
                    auto loading_view=hud_view;
                    loading_view.x=loading->raster_offset_pixels.x;loading_view.y=loading->raster_offset_pixels.y;
                    vkCmdSetViewport(command,0,1,&loading_view);
                    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,vista_loading_pipelines.at(unsigned(loading->blend)));
                    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,vista_loading_layout,0,1,
                        &textures[material].descriptor,1,&material_bank_offset);
                    vkCmdPushConstants(command,vista_loading_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push),&push);
                    vkCmdDraw(command,static_cast<std::uint32_t>(end-first),1,ui_first+static_cast<std::uint32_t>(first),0);
                    first=end;continue;
                }
                // Only the native horizontal extension needs this local
                // sampling limit. The authored quad/UVs and4:3 route stay
                // untouched, including all interior texel footprints.
                if(ui_frame_widths.at(material) && menu_canvas.left>0) {
                    if((end-first)%6)throw FormatError("frame center is not an authored quad stream");
                    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,
                        next_backdrop<backdrops.size()?ui_frame_backdrop_pipeline:ui_frame_pipeline);
                    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,ui_frame_layout,0,1,
                        &textures[material].descriptor,1,&material_bank_offset);
                    for(auto quad=first;quad<end;quad+=6) {
                        const auto &a=ui[quad],&b=ui[quad+1];
                        const auto bounds=frontend_frame_sampling(a.position.x,b.position.x-a.position.x,
                            a.uv.x,b.uv.x,ui_frame_widths[material]);
                        const ShaderVector sampling{bounds.left_u.value_or(0),bounds.right_u.value_or(0),
                            float(bounds.left_u.has_value()),float(bounds.right_u.has_value())};
                        vkCmdPushConstants(command,ui_frame_layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(sampling),sampling.data());
                        vkCmdDraw(command,6,1,ui_first+static_cast<std::uint32_t>(quad),0);
                    }
                    first=end;continue;
                }
                vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,hud_blend?hud_pipelines.at(hud_blend):
                    next_backdrop<backdrops.size()?ui_backdrop_pipeline:ui_pipeline);
                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,&textures[material].descriptor,1,&material_bank_offset);
                vkCmdDraw(command,static_cast<std::uint32_t>(end-first),1,ui_first+static_cast<std::uint32_t>(first),0);
                first = end;
            }
        }
        if(display_correction) {
            copy_scene_for_post(index);
            auto filtered=render;filtered.renderPass=fog_pass;filtered.framebuffer=fog_framebuffers[index];
            filtered.clearValueCount=0;filtered.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&filtered,VK_SUBPASS_CONTENTS_INLINE);
            const VkViewport full{0,0,float(extent.width),float(extent.height),0,1};
            const VkRect2D clip{{0,0},extent};
            vkCmdSetViewport(command,0,1,&full);vkCmdSetScissor(command,0,1,&clip);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,display_pipeline);
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,display_layout,0,1,&bloom_descriptors[6],0,nullptr);
            // Original PC663D0E/663E28 scalar outputs; the composition is a native display adapter.
            constexpr float brightness[]{-.15f,-.10f,-.05f,0,.05f,.10f,.15f,.20f,.25f,.30f};
            constexpr float gamma[]{1.f,.833333313f,.769230783f,.714285731f,.625f,.555555582f,
                .5f,.454545438f,.416666657f,.384615391f,.357142866f};
            const std::array<float,4> values{brightness[display.brightness],gamma[display.gamma],0,0};
            vkCmdPushConstants(command,display_layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(values),values.data());
            vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
            auto resumed=render;resumed.renderPass=continuation_pass;
            resumed.clearValueCount=0;resumed.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
        }
        if(crt_enabled) {
            // Phase-Distorter's direct-scene path filters the completed image
            // on the GPU, before ImGui, without quantizing scene positions.
            copy_scene_for_post(index);
            auto filtered=render;filtered.renderPass=fog_pass;filtered.framebuffer=fog_framebuffers[index];
            filtered.clearValueCount=0;filtered.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&filtered,VK_SUBPASS_CONTENTS_INLINE);
            const VkViewport full{0,0,float(extent.width),float(extent.height),0,1};
            const VkRect2D clip{{0,0},extent};
            vkCmdSetViewport(command,0,1,&full);vkCmdSetScissor(command,0,1,&clip);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,crt_pipeline);
            vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,crt_layout,0,1,&bloom_descriptors[6],0,nullptr);
            // Halo's source canvas has 480 rows; widen its logical raster with
            // the output aspect, keeping the same beam density on resize.
            const std::array<float,4> sizes{480.f*float(extent.width)/float(extent.height),480.f,
                float(extent.width),float(extent.height)};
            vkCmdPushConstants(command,crt_layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(sizes),sizes.data());
            vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
            auto resumed=render;resumed.renderPass=continuation_pass;
            resumed.clearValueCount=0;resumed.pClearValues=nullptr;
            vkCmdBeginRenderPass(command,&resumed,VK_SUBPASS_CONTENTS_INLINE);
        }
        if(debug_draw_data) {
            const auto previous=debug_draw_data->FramebufferScale;
            debug_draw_data->FramebufferScale=ImVec2(previous.x*float(extent.width)/presentation_extent.width,
                previous.y*float(extent.height)/presentation_extent.height);
            ImGui_ImplVulkan_RenderDrawData(debug_draw_data,command);
            debug_draw_data->FramebufferScale=previous;
        }
        vkCmdEndRenderPass(command);
        if(depth_capture) {
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=depth;
            barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
            constexpr auto depth_stages=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            vkCmdPipelineBarrier(command,depth_stages,VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy copies[2]{};
            copies[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};
            copies[1].bufferOffset=depth_capture_size;
            copies[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};
            for(auto &copy:copies)copy.imageExtent={extent.width,extent.height,1};
            vkCmdCopyImageToBuffer(command,depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,depth_capture,2,copies);
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,depth_stages,
                0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            host.buffer=depth_capture;host.size=VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,
                0,0,nullptr,1,&host,0,nullptr);
        }
        VkImageLayout presentation_layout=scene_layout;
        if(!scene_targets.empty()) {
            std::array<VkImageMemoryBarrier,2> barriers{};
            for(auto &b:barriers){b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
                b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
            barriers[0].image=images[index];barriers[0].oldLayout=scene_layout;
            barriers[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barriers[0].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            barriers[1].image=presentation_images[index];barriers[1].oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
            barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,0,nullptr,0,nullptr,2,barriers.data());
            VkImageBlit blit{};blit.srcSubresource=blit.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
            blit.srcOffsets[1]={int(extent.width),int(extent.height),1};
            blit.dstOffsets[1]={int(presentation_extent.width),int(presentation_extent.height),1};
            if(ssaa_intermediate.image) {
                auto target=barriers[1];target.image=ssaa_intermediate.image;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,0,nullptr,0,nullptr,1,&target);
                auto half=blit;half.dstOffsets[1]={int(extent.width/2),int(extent.height/2),1};
                vkCmdBlitImage(command,images[index],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    ssaa_intermediate.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&half,VK_FILTER_LINEAR);
                target.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;target.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                target.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;target.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,0,nullptr,0,nullptr,1,&target);
                blit.srcOffsets[1]=half.dstOffsets[1];
                vkCmdBlitImage(command,ssaa_intermediate.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    presentation_images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,VK_FILTER_LINEAR);
            } else vkCmdBlitImage(command,images[index],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                presentation_images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,VK_FILTER_LINEAR);
            presentation_layout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        }
        if (capture) {
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask = scene_targets.empty()?VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.oldLayout = presentation_layout;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = presentation_images[index];
            barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            VkBufferImageCopy copy{};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy.imageExtent = {presentation_extent.width, presentation_extent.height, 1};
            vkCmdCopyImageToBuffer(command, presentation_images[index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, capture, 1, &copy);
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = 0;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            host.srcQueueFamilyIndex = host.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            host.buffer = capture;
            host.size = VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &host, 0, nullptr);
        }
        if(!capture && presentation_layout!=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask=scene_targets.empty()?VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.oldLayout=presentation_layout;barrier.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.image=presentation_images[index];barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,0,nullptr,0,nullptr,1,&barrier);
        }
        check(vkEndCommandBuffer(command), "end commands");
        measure(frame_timing.commands_ms);
        if(!weather_submission.fence_reset)check(vkResetFences(device, 1, &fence), "reset frame fence");
        const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &acquired;
        submit.pWaitDstStageMask = &wait_stage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &present_ready[index];
        check(vkQueueSubmit(queue, 1, &submit, fence), "submit frame");
        weather_submission.submitted=true;
        measure(frame_timing.submit_ms);
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &present_ready[index];
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &index;
        const auto presented = vkQueuePresentKHR(queue, &present);
        if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR) dirty = true;
        else check(presented, "present frame");
        if(haze_trace_path) {
            std::ofstream submission(*haze_trace_path/"submission.tsv");
            submission<<"submitted\t1\npresent_result\t"<<int(presented)<<'\n';
            if(!submission)throw FormatError("cannot write live title haze submission trace");
        }
        measure(frame_timing.present_ms);
        if (capture) {
            check(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "wait screenshot");
            void* mapped;
            check(vkMapMemory(device, capture_memory, 0, capture_size, 0, &mapped), "map screenshot");
            struct Mapping { VkDevice d; VkDeviceMemory m; ~Mapping() { vkUnmapMemory(d, m); } } mapping{device, capture_memory};
            if (!screenshot.parent_path().empty()) std::filesystem::create_directories(screenshot.parent_path());
            std::ofstream file(screenshot, std::ios::binary);
            if (!file) throw std::runtime_error("cannot write screenshot " + screenshot.string());
            file << "P6\n" << presentation_extent.width << ' ' << presentation_extent.height << "\n255\n";
            const auto pixels = static_cast<const unsigned char*>(mapped);
            const bool bgra = format == VK_FORMAT_B8G8R8A8_SRGB || format == VK_FORMAT_B8G8R8A8_UNORM;
            for (std::size_t i = 0; i < std::size_t(presentation_extent.width) * presentation_extent.height; ++i) {
                const char rgb[]{char(pixels[i * 4 + (bgra ? 2 : 0)]), char(pixels[i * 4 + 1]), char(pixels[i * 4 + (bgra ? 0 : 2)])};
                file.write(rgb, 3);
            }
            if (!file) throw std::runtime_error("failed writing screenshot");
            std::cout << "Captured " << screenshot.string() << " (" << presentation_extent.width << 'x' << presentation_extent.height << ")\n";
            if(depth_capture) {
                void *depth_mapped;
                check(vkMapMemory(device,depth_capture_memory,0,depth_capture_size+stencil_capture_size,0,&depth_mapped),
                    "map original integer-depth diagnostic");
                Mapping depth_mapping{device,depth_capture_memory};
                const auto *bytes=static_cast<const char *>(depth_mapped);
                const auto write=[&](const char *suffix,const char *data,VkDeviceSize size) {
                    std::ofstream output(screenshot.string()+suffix,std::ios::binary);
                    if(!output || !output.write(data,std::streamsize(size)))
                        throw std::runtime_error("failed writing integer-depth diagnostic");
                };
                // Vulkan aspect-copy layout: one low24 DWORD per depth texel,
                // one byte per stencil texel. No original-word reinterpretation.
                write(".depth-d24-low24.bin",bytes,depth_capture_size);
                write(".stencil-u8.bin",bytes+depth_capture_size,stencil_capture_size);
                std::cout<<"Captured D24S8 depth/stencil aspects ("<<extent.width<<'x'<<extent.height<<")\n";
            }
        }
        measure(frame_timing.screenshot_ms);
        if(timing_report.is_open()) {
            timing_report<<timing_frame++<<'\t'<<timing_interval<<'\t'
                <<std::chrono::duration<double,std::milli>(TimingClock::now()-timing_start).count();
            for(const auto value:{frame_timing.fence_ms,frame_timing.global_constants_ms,
                frame_timing.view_constants_ms,frame_timing.geometry_pack_ms,frame_timing.geometry_upload_ms,
                frame_timing.geometry_diagnostics_ms,frame_timing.acquire_ms,frame_timing.commands_ms,
                frame_timing.submit_ms,frame_timing.present_ms,frame_timing.screenshot_ms})timing_report<<'\t'<<value;
            timing_report<<'\t'<<frame_timing.materials<<'\t'<<frame_timing.dynamic_vertices
                <<'\t'<<extent.width<<'\t'<<extent.height
                <<'\t'<<presentation_extent.width<<'\t'<<presentation_extent.height
                <<'\t'<<frame_timing.geometry_plan_ms<<'\t'<<frame_timing.geometry_bind_ms<<'\n';
            if(!timing_report)throw FormatError("Write renderer frame timings");
        }
        return true;
    }
};

Renderer::Renderer(SDL_Window* window, const Mesh& mesh, bool validation,
    std::span<const SourceShadowPlanMaterial> shadow_materials,
    const SourceBspResidentPlan *bsp_resident_plan,
    const std::function<void()> &before_presentation,
    const Renderer *presentation_owner,const std::filesystem::path &shader_directory) : impl_(std::make_shared<Impl>()) {
    if(!shader_directory.empty()) {
        impl_->shader_directory=shader_directory;
        std::cout<<"Pinned renderer shaders: "<<impl_->shader_directory.string()<<'\n';
    }
    if(presentation_owner)impl_->presentation=presentation_owner->impl_->presentation;
    if(const auto *path=std::getenv("HALO2_RENDER_TIMINGS");path && *path) {
        impl_->timing_report.open(path);
        if(!impl_->timing_report)throw FormatError("Open renderer frame timings");
        impl_->timing_report<<"frame\tinterval_ms\trenderer_ms\tfence_ms\tglobal_ms\tview_ms\tpack_ms\tupload_ms\tdiagnostics_ms\tacquire_ms\tcommands_ms\tsubmit_ms\tpresent_ms\tscreenshot_ms\tmaterials\tvertices\trender_width\trender_height\tpresent_width\tpresent_height\tgeometry_plan_ms\tgeometry_bind_ms\n";
        impl_->measure_frames=true;
    }
    impl_->bsp_resident_plan=bsp_resident_plan;
    impl_->shadow_residency(mesh,shadow_materials);
    impl_->initialize(window, mesh, validation,before_presentation);
}
Renderer::~Renderer() {
    device_lease_.retire();
}
bool Renderer::draw_loading(unsigned base,std::span<const Material> materials,
    std::span<const Vertex> vertices,const std::filesystem::path& screenshot) {
    return impl_->draw_loading(base,materials,vertices,screenshot);
}
void Renderer::prepare_loading(){impl_->prepare_loading();}
void Renderer::release_presentation(){
    device_lease_.retire();
    check(vkDeviceWaitIdle(impl_->device),"retire presentation before renderer handoff");
    impl_->destroy_swapchain();impl_->dirty=true;
}
bool Renderer::draw(const Camera& camera, const std::filesystem::path& screenshot, std::span<const Vertex> debug, bool reticle, std::span<const Vertex> first_person, ScreenEffects effects, std::span<const Vertex> ui, std::span<const ModelView> views, std::span<const Vertex> background, std::span<const UiBackdrop> backdrops, const BspFogSample *mainmenu_fog, const MainmenuWorldDraw *mainmenu_world, bool ui_full_surface_bounds) {
    if(impl_->source_coordinate_registers!=renderer_passthrough_registers)
        throw FormatError("Native texture coordinates differ from the retained programmable shader contract");
    return impl_->draw(camera, screenshot, debug, reticle, first_person, effects, ui, views, background, backdrops, mainmenu_fog, mainmenu_world, ui_full_surface_bounds);
}
void Renderer::resized() { impl_->dirty = true; }
DebugPanel& Renderer::debug_panel() {
    if(!impl_->debug_panel)impl_->debug_panel=std::make_unique<DebugPanel>(impl_->window);
    return *impl_->debug_panel;
}
void Renderer::measure_frames(bool enabled) { impl_->measure_frames=enabled || impl_->timing_report.is_open(); }
RendererFrameTiming Renderer::frame_timing() const { return impl_->frame_timing; }
std::size_t Renderer::resident_material_images() const {
    std::set<VkImage> images;
    const auto add=[&](const auto &bank) {for(const auto &texture:bank)if(texture.image)images.insert(texture.image);};
    add(impl_->textures);add(impl_->detail_textures);add(impl_->bump_textures);add(impl_->auxiliary_textures);
    for(const auto &material:impl_->animated_textures)for(const auto &stage:material)add(stage);
    return images.size();
}
void Renderer::update_image(unsigned material, const Texture& image) { impl_->update_image(material,image); }
void Renderer::update_palettized_image(unsigned material,unsigned stage,std::span<const std::byte> raw,
    unsigned width,unsigned height,bool swizzled,unsigned mips) {
    auto image=palette_factory().bound(stage).decode(raw,width,height,swizzled,mips);
    // update_image represents its sole resident level without a mip table.
    // Remove only that redundant descriptor; multi-level chains stay intact
    // and retain update_image's existing admission rules.
    if(image.mips.size()==1)image.mips.clear();
    update_image(material,image);
}
RendererPaletteFactory Renderer::palette_factory() {
    if(!palette_factory_.valid())palette_factory_=RendererPaletteFactory::issue(device_lease());
    return palette_factory_;
}
RendererConstantMode Renderer::constant_mode_control() {
    const std::weak_ptr<Impl> owner=impl_;
    return RendererConstantMode::issue(device_lease(),[owner](std::uint32_t mode) {
        auto impl=owner.lock();
        if(!impl || mode!=0x11)throw FormatError("Native renderer constant mode is unsupported");
        VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(impl->physical,&properties);
        if(!impl->material_buffer || !impl->vertex_bank_buffer || !impl->material_mapping ||
           impl->material_uniforms.empty() || impl->vertex_bank_uniforms.empty() ||
           properties.limits.maxUniformBufferRange<sizeof(Impl::MaterialUniform) ||
           properties.limits.maxUniformBufferRange<sizeof(Impl::VertexBankUniform))
            throw FormatError("Native renderer lacks its live vertex constant storage");
        impl->source_constant_mode=mode;
        return RendererConstantConfiguration{impl->source_constant_mode,192,true,
            sizeof(Impl::MaterialUniform),sizeof(Impl::VertexBankUniform),
            NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(impl->material_mapping))},
            NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(impl->vertex_bank_uniforms.data()))}};
    });
}
RendererDepthMode Renderer::depth_mode_control() {
    const std::weak_ptr<Impl> owner=impl_;
    return RendererDepthMode::issue(device_lease(),[owner](RendererDepthSetting setting,std::uint32_t value) {
        auto impl=owner.lock();
        if(!impl || (setting==RendererDepthSetting::Mode && value!=2) ||
           (setting==RendererDepthSetting::WriteEnable && value!=1) ||
           (setting==RendererDepthSetting::Function && value!=0x203) ||
           (setting==RendererDepthSetting::Bias && value!=0))
            throw FormatError("Native renderer cold depth setting is unsupported");
        VkImageFormatProperties properties;
        if(impl->depth_format!=source_depth_format || !impl->depth || !impl->depth_memory ||
           !impl->depth_attachment_view || !impl->pipeline ||
           !source_depth_supported(impl->physical,properties))
            throw FormatError("Native renderer lacks its live integer W-depth attachment");
        // Only already-realized cold defaults are admitted. Nondefault
        // authored passes retain their existing independent pipeline states.
        switch(setting) {
            case RendererDepthSetting::Mode:impl->source_depth_mode=value;break;
            case RendererDepthSetting::WriteEnable:impl->source_depth_write=value;break;
            case RendererDepthSetting::Function:impl->source_depth_function=value;break;
            case RendererDepthSetting::Bias:impl->source_depth_bias=value;break;
        }
        return RendererDepthConfiguration{impl->source_depth_mode,std::uint32_t(impl->depth_format),24,8,
            std::bit_cast<std::uint64_t>(impl->depth),
            NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(&impl->source_depth_mode))},
            impl->source_depth_write,impl->source_depth_function,impl->source_depth_bias};
    });
}
RendererOutputState Renderer::output_state_control() {
    const std::weak_ptr<Impl> owner=impl_;
    return RendererOutputState::issue(device_lease(),[owner](RendererOutputSetting setting,std::uint32_t value) {
        auto impl=owner.lock();const auto index=unsigned(setting);
        if(!impl || index>=renderer_cold_output_values.size() || value!=renderer_cold_output_values[index])
            throw FormatError("Native renderer cold output setting is unsupported");
        if(!impl->pipeline || !impl->material_buffer || !impl->material_mapping || impl->material_uniforms.empty())
            throw FormatError("Native renderer lacks its live pipeline or material output storage");
        // These defaults are already realized in the main pipeline and shader
        // constants. Authored material passes keep their own output policies.
        impl->source_output_state[index]=value;
        return RendererOutputConfiguration{impl->source_output_state,std::bit_cast<std::uint64_t>(impl->pipeline),
            NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(impl->source_output_state.data()))},
            NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(impl->material_mapping))}};
    });
}
RendererTextureCoordinates Renderer::texture_coordinate_control() {
    const std::weak_ptr<Impl> owner=impl_;
    return RendererTextureCoordinates::issue(device_lease(),[owner](unsigned stage,std::uint32_t value) {
        auto impl=owner.lock();
        if(!impl || stage>=4 || value!=stage)
            throw FormatError("Native renderer cold texture-coordinate selection is unsupported");
        if(!impl->pipeline || !impl->material_buffer || !impl->material_mapping || impl->material_uniforms.empty())
            throw FormatError("Native renderer lacks its live programmable coordinate pipeline or storage");
        // Existing world_vertex.glsl reads vpOut[9+i] for programmable stages;
        // texgen is absent. Admit only that realized identity contract.
        impl->source_coordinate_registers[stage]=9+value;
        return RendererTextureCoordinateConfiguration{impl->source_coordinate_registers,
            std::bit_cast<std::uint64_t>(impl->pipeline),sizeof(Impl::MaterialUniform),
            NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(impl->source_coordinate_registers.data()))},
            NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(impl->material_mapping))}};
    });
}
void Renderer::update_material_constants(std::span<const Material> materials) { impl_->update_material_constants(materials); }
void Renderer::update_material_constants(std::span<const Material> materials,std::span<const unsigned> active) {
    impl_->update_material_constants(materials,active);
}
void Renderer::scene_clear_color(Vec3 color) {
    for(auto value:{color.x,color.y,color.z})
        if(!std::isfinite(value) || value<0 || value>1)
            throw std::runtime_error("invalid scene clear color");
    impl_->scene_clear=color;
}
void Renderer::scene_atmosphere(MenuAtmosphere fog,BspAtmosphere source) {
    impl_->set_atmosphere(std::move(fog),std::move(source));
}
void Renderer::scene_bsp_atmosphere(const BspAtmosphere &fog) { impl_->set_bsp_atmosphere(fog); }
void Renderer::source_fog_controls(SourceFogDrawControls controls) { impl_->fog_draw_controls=controls; }
std::optional<BspPostprocessState> Renderer::bsp_postprocess_state() const {
    if(impl_->bsp_postprocess)return impl_->bsp_postprocess->persistent_state();
    return impl_->bsp_postprocess_state;
}
void Renderer::restore_bsp_postprocess_state(const BspPostprocessState &state) {
    impl_->bsp_postprocess_state=state;
    if(impl_->bsp_postprocess)impl_->bsp_postprocess->restore_state(state);
}
void Renderer::scene_color_filter(std::optional<PixelProgram> program) {
    impl_->scene_filter=std::move(program);
}
void Renderer::scene_light_volumes(std::vector<LightVolumeInstance> instances,
    std::shared_ptr<const ShaderFunctionTables> tables) {
    if(instances.size()>1024 || (!instances.empty() && !tables))
        throw FormatError("invalid title light-volume scene");
    impl_->light_volumes=std::move(instances);impl_->light_volume_tables=std::move(tables);
}
void Renderer::scene_particles(std::vector<ParticleDraw> draws) {
#ifndef HALO2_PARTICLE_GRAPHICS_AVAILABLE
    if(!draws.empty())throw FormatError("native particle graphics requires the original disc vertex program at build time");
#endif
    unsigned boundary=0;
    for(const auto &draw:draws) {
        if(!draw.material || draw.before_light_volume<boundary)
            throw FormatError("particle scene must supply original ordered draw state");
        boundary=draw.before_light_volume;
    }
    impl_->particle_draws=std::move(draws);
}
void Renderer::scene_bloom_threshold(float threshold) {
    if(!std::isfinite(threshold))throw FormatError("nonfinite title bloom threshold");
    impl_->bloom_threshold=threshold;
}
std::string Renderer::device_name() const { return impl_->gpu; }
SourceWorldDeviceState Renderer::source_world_device_state() const { return impl_->source_world_device; }
RendererDeviceLease Renderer::device_lease() {
    if(!impl_->device || !impl_->physical || !impl_->queue || !impl_->render_pass || !impl_->depth_view ||
       impl_->depth_format!=VK_FORMAT_D24_UNORM_S8_UINT ||
       (impl_->format!=VK_FORMAT_B8G8R8A8_UNORM && impl_->format!=VK_FORMAT_R8G8B8A8_UNORM))
        throw FormatError("Source device binding requires the live native classic color/depth renderer");
    if(!device_lease_.valid())
        device_lease_=RendererDeviceLease::create(NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(impl_->device)},impl_);
    return device_lease_;
}
} // namespace halo2

namespace halo2 { std::string Renderer::validate_display_settings(const DisplaySettings &s) const { return impl_->validate_display(s); } }
