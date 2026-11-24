#include "halo2/path_tracing/render.hpp"
#include "halo2/path_tracing/scene.hpp"
#include "halo2/path_tracing/lighting_storage.hpp"
#include "halo2/path_tracing/surface.hpp"
#include "rendering/renderer/scene_pass_dependencies.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace halo2::path_tracing {
namespace {
void check(VkResult result,const char* operation) {
    if(result!=VK_SUCCESS)
        throw FormatError(std::string("Path Tracing ")+operation+": "+std::to_string(result));
}
struct Buffer {
    VkDevice device{};
    VkBuffer buffer{};
    VkDeviceMemory memory{};
    void* mapped{};
    VkDeviceSize capacity{};
    VkDeviceSize populated{};
    VkDeviceSize allocation_bytes{};
    VkMemoryPropertyFlags memory_flags{};
    VkDeviceAddress address{};
    Buffer()=default;
    Buffer(const Buffer&)=delete;
    Buffer& operator=(const Buffer&)=delete;
    ~Buffer(){clear();}
    void clear()noexcept {
        if(mapped)vkUnmapMemory(device,memory);
        if(buffer)vkDestroyBuffer(device,buffer,nullptr);
        if(memory)vkFreeMemory(device,memory,nullptr);
        buffer=VK_NULL_HANDLE;memory=VK_NULL_HANDLE;mapped=nullptr;capacity=populated=allocation_bytes=0;memory_flags=0;
        address=0;
    }
    void swap(Buffer& other)noexcept {
        std::swap(device,other.device);std::swap(buffer,other.buffer);std::swap(memory,other.memory);
        std::swap(mapped,other.mapped);std::swap(capacity,other.capacity);std::swap(populated,other.populated);
        std::swap(allocation_bytes,other.allocation_bytes);
        std::swap(memory_flags,other.memory_flags);
        std::swap(address,other.address);
    }
};
// Device entry points of VK_KHR_acceleration_structure, resolved per device.
struct AccelerationFunctions {
    PFN_vkGetBufferDeviceAddress buffer_address{};
    PFN_vkGetAccelerationStructureBuildSizesKHR build_sizes{};
    PFN_vkCreateAccelerationStructureKHR create{};
    PFN_vkDestroyAccelerationStructureKHR destroy{};
    PFN_vkCmdBuildAccelerationStructuresKHR build{};
    PFN_vkGetAccelerationStructureDeviceAddressKHR structure_address{};
    void load(VkDevice device) {
        const auto get=[&](const char* name) {
            const auto function=vkGetDeviceProcAddr(device,name);
            if(!function)throw FormatError(std::string("Path Tracing hardware rays lack ")+name);
            return function;
        };
        buffer_address=reinterpret_cast<PFN_vkGetBufferDeviceAddress>(get("vkGetBufferDeviceAddress"));
        build_sizes=reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(get("vkGetAccelerationStructureBuildSizesKHR"));
        create=reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(get("vkCreateAccelerationStructureKHR"));
        destroy=reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(get("vkDestroyAccelerationStructureKHR"));
        build=reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(get("vkCmdBuildAccelerationStructuresKHR"));
        structure_address=reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
            get("vkGetAccelerationStructureDeviceAddressKHR"));
    }
};
// One bottom- or top-level structure and the device memory that holds it.
struct Acceleration {
    Buffer storage;
    VkAccelerationStructureKHR handle{};
    VkDeviceSize size{};
    VkDeviceAddress address{};
    std::uint32_t first{},count{}; // Triangle entries in the ray index list.
};
// Bottom-level slots. Static BSP geometry keeps its structure across frames;
// dynamic objects are rebuilt every frame. Alpha-tested triangles live in
// separate structures so opaque hits never return to the shader; light-only
// emitters (glow_material) live in structures that shadow rays mask out.
enum BottomSlot : unsigned {static_opaque,static_alpha,static_light,dynamic_opaque,dynamic_alpha,dynamic_light,bottom_slots};
constexpr unsigned slot_kinds=3; // opaque, alpha, light
bool static_slot(unsigned slot){return slot<dynamic_opaque;}
bool alpha_slot(unsigned slot){return slot%slot_kinds==1u;}
bool light_slot(unsigned slot){return slot%slot_kinds==2u;}
// Instance masks: shadow rays test only occluder instances; static and
// dynamic occluders are separate so a ray can ask what only objects block.
constexpr std::uint8_t static_occluder_mask=0x01,light_mask=0x02,dynamic_occluder_mask=0x04;
struct ShaderModules {
    VkDevice device{};
    std::array<VkShaderModule,3> modules{};
    ~ShaderModules(){for(auto shader:modules)if(shader)vkDestroyShaderModule(device,shader,nullptr);}
};
template<class T> std::span<const std::byte> bytes(const std::vector<T>& value) {
    return std::as_bytes(std::span<const T>(value));
}
std::uint64_t temporal_lighting_key(const Scene& scene,const GpuSceneUniform& uniform) {
    std::uint64_t key=14695981039346656037ull;
    const auto add=[&](std::span<const std::byte> value) {
        for(const auto byte:value){key^=std::to_integer<unsigned>(byte);key*=1099511628211ull;}
    };
    add(bytes(scene.color_guide_image_keys));
    for(std::size_t n=0;n<scene.materials.size();++n) {
        // Owner aliases carry current radiance and relocatable atlas headers.
        // Their consumed state invalidates only affected triangle bounds below.
        if(n<scene.material_source_roots.size() && scene.material_source_roots[n]!=n)continue;
        add(std::as_bytes(std::span(scene.materials[n].color_guide)));
    }
    add(std::as_bytes(std::span(uniform.environment).first(3)));
    // Emitters are deliberately absent. The ray scene holds the objects of the
    // current view, so emissive triangles appear, disappear and change LOD
    // every few frames as the camera moves: keying on them reset every cell's
    // history continuously in motion. A triangle that appears, moves or
    // changes material already invalidates only receivers near its bounds
    // (update_shadow_changes), and animated emission is time-varying radiance.
    return key;
}
std::uint64_t direct_lighting_key(const Scene& scene) {
    // Image/UV changes can alter alpha-tested shadow visibility even when
    // the source triangle positions and light bank are unchanged.
    std::uint64_t key=14695981039346656037ull;
    const auto add=[&](std::uint32_t word){key^=word;key*=1099511628211ull;};
    std::vector<bool> referenced(scene.materials.size());
    for(const auto& triangle:scene.triangles) {
        if(triangle.metadata[0]>=referenced.size())throw FormatError("Path Tracing alpha history lost its material");
        referenced[triangle.metadata[0]]=true;
    }
    // Only alpha-tested materials can change the shadow shape through their
    // authored image/UV animation without moving any vertices.
    for(std::size_t index=0;index<scene.materials.size();++index) {
        if(!referenced[index])continue;
        if(index<scene.material_source_roots.size() && scene.material_source_roots[index]!=index)continue;
        const auto& material=scene.materials[index];
        if(!(material.texture[3]&(1u|8u)))continue;
        add(std::uint32_t(index));
        add(material.texture[3]);add(std::bit_cast<std::uint32_t>(material.response[3]));
        for(const auto value:material.transform)add(std::bit_cast<std::uint32_t>(value));
        if(index<scene.material_image_keys.size())for(const auto value:scene.material_image_keys[index]) {
            add(std::uint32_t(value));add(std::uint32_t(value>>32));
        }
    }
    return key;
}
}
struct Render::Impl {
    RenderContext context;
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceMemoryProperties memory_properties{};
    VkDescriptorSetLayout scene_layout{};
    VkDescriptorPool pool{};
    VkDescriptorSet descriptor{};
    VkPipelineLayout pipeline_layout{};
    VkPipeline graphics_pipeline{};
    VkPipeline glow_layer_pipeline{}; // Created on first use (bind_glow_layer).
    std::map<std::pair<std::string,unsigned>,VkPipeline> source_depth_pipelines;
    bool source_depth_prepared{};
    VkImage environment_image{},environment_depth{};
    VkDeviceMemory environment_memory{},environment_depth_memory{};
    VkImageView environment_view{},environment_depth_view{};
    std::array<VkImageView,6> environment_faces{};
    std::array<VkFramebuffer,6> environment_framebuffers{};
    VkSampler environment_sampler{};
    VkRenderPass environment_pass{};
    VkFormat environment_color_format{},environment_depth_format{};
    unsigned environment_side{};
    VkDeviceSize environment_allocation_bytes{};
    bool environment_recording{},environment_source_available{},environment_fallback_available{};
    unsigned environment_levels=1,environment_face=0;
    VkRenderPass visibility_pass{};
    VkPipeline visibility_pipeline{};
    VkRenderPass lighting_pass{};
    VkPipeline lighting_pipeline{};
    VkImage visibility_depth{};
    VkImageView visibility_view{};
    VkDeviceMemory visibility_memory{};
    VkDeviceSize visibility_allocation_bytes{};
    VkFramebuffer visibility_framebuffer{};
    VkFramebuffer lighting_framebuffer{};
    VkExtent2D visibility_extent{};
    std::array<Buffer,10> buffers;
    // Hardware rays: acceleration structures over the triangle bank (binding1)
    // and the ray index list (binding12) that maps hits back to triangles.
    AccelerationFunctions rt{};
    PFN_vkCmdSetCullMode set_cull_mode{};
    VkCullModeFlags draw_cull=VK_CULL_MODE_NONE;
    std::array<Acceleration,bottom_slots> bottom;
    Acceleration top;
    Buffer ray_indices,instances,scratch;
    VkDeviceSize scratch_alignment=256;
    std::vector<std::uint32_t> ray_index_words;
    bool acceleration_pending{},static_acceleration_dirty=true,acceleration_descriptor_written{};
    std::array<VkDeviceSize,bottom_slots> bottom_scratch{};
    VkDeviceSize top_scratch{};
    std::uint32_t top_instances{},acceleration_triangles{};
    bool static_geometry_changed=true,static_payload_changed=true,static_indices_written{};
    std::uint32_t previous_static_triangles=~0u;
    // Compute tracing of the lighting grid, pushed the lighting draws' camera.
    VkPipelineLayout compute_layout{};
    VkPipeline trace_pipeline{},denoise_pipeline{},calibrate_pipeline{};
    // Running gains fitted to the baked lightmaps by calibrate.comp (see
    // update_calibration): sun and grid diffuse at the resolve, and the
    // emission scale of the surfaces that lit the bake.
    std::array<float,3> calibration{1.f,1.f,1.f};
    // The emitter scale in the uniform this frame traces with: calibrate.comp
    // removes that much emitter light from the grid it samples.
    float uploaded_uniform_emitter_scale=1.f;
    std::uint32_t calibration_frames{};
    // Least-squares sums pooled over every frame on this map, so the gains
    // describe the map rather than whatever the camera faces now: per-view
    // fits differ by 50% between a sunlit cliff and a shaded valley.
    std::array<double,12> calibration_sums{};
    // A-trous passes after each trace (HALO2_PATH_TRACING_DENOISE_PASSES, 0..5).
    unsigned denoise_passes=3;
    // Optional GPU pass timings (HALO2_PATH_TRACING_TIMINGS=report.tsv): one
    // row per frame of labelled intervals, read back after the frame fence.
    static constexpr unsigned timing_slots=32;
    VkQueryPool timing_pool{};
    std::ofstream timing_report;
    std::array<const char*,timing_slots> timing_labels{};
    unsigned timing_count{};
    std::uint64_t timing_frame{};
    double timing_period{};
    void timestamp(VkCommandBuffer command,const char* label) {
        if(!timing_pool || timing_count>=timing_slots)return;
        vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,timing_pool,timing_count);
        timing_labels[timing_count++]=label;
    }
    void harvest_timings() {
        if(!timing_pool || timing_count<2)return;
        std::array<std::uint64_t,timing_slots> values{};
        if(vkGetQueryPoolResults(context.device,timing_pool,0,timing_count,sizeof(values),values.data(),
            sizeof(std::uint64_t),VK_QUERY_RESULT_64_BIT)!=VK_SUCCESS)return;
        timing_report<<timing_frame++;
        for(unsigned n=1;n<timing_count;++n)
            timing_report<<'\t'<<timing_labels[n]<<'='<<double(values[n]-values[n-1])*timing_period*1.e-6;
        timing_report<<'\n'<<std::flush;
        timing_count=0;
    }
    std::array<std::byte,camera_bytes> lighting_push{};
    bool lighting_push_valid{};
    // Binding9 is an immutable previous-frame snapshot. Keeping its read bank
    // separate avoids races when several current pixels reproject to the same
    // old cell. Only the two bounded22-word histories are copied on the GPU.
    struct alignas(16) HistoryCamera {
        std::array<std::array<float,16>,2> matrix{};
        std::array<std::array<float,4>,2> viewport{};
        ShaderVector changed_occluder_min{},changed_occluder_max{}; // Union; max.w: local box count.
        ShaderVector changed_light_min{},changed_light_max{};
        // Local changed-geometry boxes (min with change flags in w, max).
        // One union box spanning distant changes invalidated whole maps.
        std::array<ShaderVector,2*lighting_change_boxes> changed_boxes{};
    };
    static_assert(sizeof(HistoryCamera)==lighting_previous_header_bytes);
    struct ShadowTriangle {
        std::array<float,9> positions{};
        std::array<float,6> alpha_uv{};
        std::uint32_t material{},alpha_flags{};
        std::uint64_t shading_key{},normal_key{};
        bool operator==(const ShadowTriangle&)const=default;
    };
    std::vector<ShadowTriangle> previous_shadow_geometry;
    std::vector<GpuLight> previous_shadow_lights;
    HistoryCamera current_history_camera{},previous_history_camera{};
    std::array<float,2> current_history_eye{},previous_history_eye{};
    std::uint32_t current_history_camera_mask{},previous_history_camera_mask{};
    VkViewport visibility_viewport{};
    bool previous_history_prepared{};
    const Scene* uploaded_color_guide_source{};
    std::vector<std::array<std::uint64_t,2>> uploaded_color_guide_keys;
    bool uploaded{},visibility_ready{},visibility_recording{},lighting_ready{},lighting_recording{};
    bool history_reset_pending=true,lighting_key_valid{};
    std::uint32_t frame_serial{};
    std::uint64_t lighting_key{},direct_key{};
    bool direct_history_reset=true;
    static constexpr unsigned history_words=lighting_history_words;
    const Scene* uploaded_source{};
    unsigned lighting_scale=2;
    unsigned samples=1,bounces=1;
    bool update_shadow_changes(const Scene& scene) {
        const float limit=std::numeric_limits<float>::max();
        auto& header=previous_history_camera;
        header.changed_occluder_min={limit,limit,limit,0};header.changed_occluder_max={-limit,-limit,-limit,0};
        header.changed_light_min={limit,limit,limit,0};header.changed_light_max={-limit,-limit,-limit,0};
        const auto point=[](ShaderVector& minimum,ShaderVector& maximum,const float* position) {
            for(unsigned axis=0;axis<3;++axis) {
                minimum[axis]=std::min(minimum[axis],position[axis]);
                maximum[axis]=std::max(maximum[axis],position[axis]);
            }
            minimum[3]=1;
        };
        // Consecutive changed triangles usually belong to one object: grow
        // the current local box while it stays small, then start another.
        struct ChangeBox {std::array<float,3> minimum,maximum;unsigned flags;};
        std::vector<ChangeBox> boxes;
        constexpr float run_extent=4.f; // WORLD units (~12 m).
        const auto triangle_bounds=[&](const ShadowTriangle& triangle,unsigned changes=1u) {
            const auto combined=unsigned(header.changed_occluder_min[3])|changes;
            for(unsigned vertex=0;vertex<3;++vertex)
                point(header.changed_occluder_min,header.changed_occluder_max,triangle.positions.data()+vertex*3);
            header.changed_occluder_min[3]=float(combined);
            ChangeBox box{{limit,limit,limit},{-limit,-limit,-limit},changes};
            for(unsigned vertex=0;vertex<3;++vertex)for(unsigned axis=0;axis<3;++axis) {
                box.minimum[axis]=std::min(box.minimum[axis],triangle.positions[vertex*3+axis]);
                box.maximum[axis]=std::max(box.maximum[axis],triangle.positions[vertex*3+axis]);
            }
            if(!boxes.empty()) {
                auto& last=boxes.back();bool fits=true;
                for(unsigned axis=0;axis<3;++axis)
                    fits=fits && std::max(last.maximum[axis],box.maximum[axis])-std::min(last.minimum[axis],box.minimum[axis])<=run_extent;
                if(fits) {
                    for(unsigned axis=0;axis<3;++axis) {
                        last.minimum[axis]=std::min(last.minimum[axis],box.minimum[axis]);
                        last.maximum[axis]=std::max(last.maximum[axis],box.maximum[axis]);
                    }
                    last.flags|=changes;return;
                }
            }
            boxes.push_back(box);
        };
        std::vector<std::uint64_t> material_keys(scene.materials.size());
        for(std::size_t n=0;n<scene.materials.size();++n)
            if(n<scene.material_source_roots.size() && scene.material_source_roots[n]!=n &&
               n<scene.material_image_keys.size()) {
                // Alias keys contain only consumed constants, UVs, image
                // identities and responses; atlas relocation is excluded.
                // The alias's second lane is only a salted copy of this key.
                material_keys[n]=scene.material_image_keys[n][0];
            }
        // A reused static prefix is bitwise identical to the previous frame's:
        // compare and bound only the dynamic triangles after it.
        const std::size_t first=scene.static_reused && previous_static_triangles==scene.static_triangles &&
            previous_shadow_geometry.size()>=scene.static_triangles?scene.static_triangles:0;
        const bool resized=previous_shadow_geometry.size()!=scene.triangles.size();
        if(resized)for(std::size_t n=first;n<previous_shadow_geometry.size();++n)triangle_bounds(previous_shadow_geometry[n]);
        // Static triangles keep their leading ordinals while dynamic objects
        // come and go, so their acceleration structure survives a resize.
        static_geometry_changed=!first && (scene.static_triangles!=previous_static_triangles ||
            previous_shadow_geometry.size()<scene.static_triangles);
        static_payload_changed=static_geometry_changed;
        previous_static_triangles=scene.static_triangles;
        previous_shadow_geometry.resize(scene.triangles.size());
        for(const auto& triangle:std::span(scene.triangles).subspan(first)) {
            const auto ordinal=std::bit_cast<std::uint32_t>(triangle.p0[3]);
            if(ordinal>=previous_shadow_geometry.size())throw FormatError("Path Tracing shadow history lost source triangle identity");
            ShadowTriangle current;current.material=triangle.metadata[0];
            if(current.material>=scene.materials.size())throw FormatError("Path Tracing shadow history lost its material");
            current.alpha_flags=scene.materials[current.material].texture[3]&(1u|8u);
            if(current.alpha_flags)
                current.alpha_uv={triangle.uv01[0],triangle.uv01[1],triangle.uv01[2],triangle.uv01[3],triangle.uv2[0],triangle.uv2[1]};
            current.shading_key=material_keys[current.material];
            const auto shade=[&](std::uint32_t value){current.shading_key^=value;current.shading_key*=1099511628211ull;};
            for(const auto* values:{&triangle.uv01,&triangle.uv2,&triangle.color_guide})
                for(const auto value:*values)shade(std::bit_cast<std::uint32_t>(value));
            for(unsigned n=1;n<4;++n)shade(triangle.metadata[n]);
            for(const auto* values:{&triangle.n0,&triangle.n1,&triangle.n2})for(unsigned axis=0;axis<3;++axis) {
                current.normal_key^=std::bit_cast<std::uint32_t>((*values)[axis]);current.normal_key*=1099511628211ull;
            }
            for(unsigned axis=0;axis<3;++axis) {
                current.positions[axis]=triangle.p0[axis];current.positions[axis+3]=triangle.p1[axis];
                current.positions[axis+6]=triangle.p2[axis];
            }
            auto& previous=previous_shadow_geometry[ordinal];
            if(!first && ordinal<scene.static_triangles && !static_payload_changed) {
                if(previous.positions!=current.positions || previous.alpha_flags!=current.alpha_flags)
                    static_geometry_changed=true;
                if(static_geometry_changed || !(previous==current))static_payload_changed=true;
            }
            const bool moved=resized || previous.positions!=current.positions || previous.material!=current.material ||
                previous.alpha_uv!=current.alpha_uv || previous.alpha_flags!=current.alpha_flags;
            // Moving skinning already changes reflected-geometry bounds. Do
            // not additionally restart broad diffuse history for every posed
            // normal; stationary normal-only animation still changes radiance.
            const bool shaded=previous.shading_key!=current.shading_key || (!moved && previous.normal_key!=current.normal_key);
            if(moved || shaded) {
                unsigned changes=(moved?1u:0u)|(shaded?2u:0u);
                if(shaded && (current.alpha_flags|previous.alpha_flags))changes|=1u;
                if(!resized)triangle_bounds(previous,changes);
                triangle_bounds(current,changes);previous=current;
            }
        }
        // At most lighting_change_boxes boxes: merge boxes whose centres share
        // a grid cell, doubling the cell until few enough remain.
        for(float cell=run_extent;boxes.size()>lighting_change_boxes;cell*=2.f) {
            std::map<std::array<std::int64_t,3>,ChangeBox> merged;
            for(const auto& box:boxes) {
                std::array<std::int64_t,3> key{};
                for(unsigned axis=0;axis<3;++axis)
                    key[axis]=std::int64_t(std::floor((box.minimum[axis]+box.maximum[axis])*.5f/cell));
                const auto [at,inserted]=merged.try_emplace(key,box);
                if(inserted)continue;
                for(unsigned axis=0;axis<3;++axis) {
                    at->second.minimum[axis]=std::min(at->second.minimum[axis],box.minimum[axis]);
                    at->second.maximum[axis]=std::max(at->second.maximum[axis],box.maximum[axis]);
                }
                at->second.flags|=box.flags;
            }
            boxes.clear();for(const auto& [key,box]:merged)boxes.push_back(box);
        }
        header.changed_boxes={};
        for(std::size_t n=0;n<boxes.size();++n) {
            header.changed_boxes[2*n]={boxes[n].minimum[0],boxes[n].minimum[1],boxes[n].minimum[2],float(boxes[n].flags)};
            header.changed_boxes[2*n+1]={boxes[n].maximum[0],boxes[n].maximum[1],boxes[n].maximum[2],0};
        }
        header.changed_occluder_max[3]=float(boxes.size());
        bool distant_changed=false;
        const auto light_bounds=[&](const GpuLight& light) {
            if(light.position_radius[3]<0){distant_changed=true;return;}
            ShaderVector minimum{},maximum{};
            for(unsigned axis=0;axis<3;++axis) {
                minimum[axis]=float(std::max(-double(limit),double(light.position_radius[axis])-light.position_radius[3]));
                maximum[axis]=float(std::min(double(limit),double(light.position_radius[axis])+light.position_radius[3]));
            }
            point(header.changed_light_min,header.changed_light_max,minimum.data());
            point(header.changed_light_min,header.changed_light_max,maximum.data());
        };
        for(std::size_t index=0;index<std::max(scene.lights.size(),previous_shadow_lights.size());++index) {
            const bool old=index<previous_shadow_lights.size(),current=index<scene.lights.size();
            if(old && current && !std::memcmp(&scene.lights[index],&previous_shadow_lights[index],sizeof(GpuLight)))continue;
            if(old)light_bounds(previous_shadow_lights[index]);
            if(current)light_bounds(scene.lights[index]);
        }
        previous_shadow_lights=scene.lights;
        return distant_changed;
    }
    void clear_environment()noexcept {
        for(auto& framebuffer:environment_framebuffers) {
            if(framebuffer)vkDestroyFramebuffer(context.device,framebuffer,nullptr);
            framebuffer=VK_NULL_HANDLE;
        }
        for(auto& view:environment_faces) {
            if(view)vkDestroyImageView(context.device,view,nullptr);
            view=VK_NULL_HANDLE;
        }
        if(environment_sampler)vkDestroySampler(context.device,environment_sampler,nullptr);
        if(environment_view)vkDestroyImageView(context.device,environment_view,nullptr);
        if(environment_depth_view)vkDestroyImageView(context.device,environment_depth_view,nullptr);
        if(environment_image)vkDestroyImage(context.device,environment_image,nullptr);
        if(environment_depth)vkDestroyImage(context.device,environment_depth,nullptr);
        if(environment_memory)vkFreeMemory(context.device,environment_memory,nullptr);
        if(environment_depth_memory)vkFreeMemory(context.device,environment_depth_memory,nullptr);
        if(environment_pass)vkDestroyRenderPass(context.device,environment_pass,nullptr);
        environment_image=environment_depth=VK_NULL_HANDLE;
        environment_memory=environment_depth_memory=VK_NULL_HANDLE;
        environment_view=environment_depth_view=VK_NULL_HANDLE;
        environment_sampler=VK_NULL_HANDLE;environment_pass=VK_NULL_HANDLE;
        environment_allocation_bytes=0;environment_side=0;environment_recording=false;
    }
    void clear_visibility_target()noexcept {
        if(lighting_framebuffer)vkDestroyFramebuffer(context.device,lighting_framebuffer,nullptr);
        if(visibility_framebuffer)vkDestroyFramebuffer(context.device,visibility_framebuffer,nullptr);
        if(visibility_view)vkDestroyImageView(context.device,visibility_view,nullptr);
        if(visibility_depth)vkDestroyImage(context.device,visibility_depth,nullptr);
        if(visibility_memory)vkFreeMemory(context.device,visibility_memory,nullptr);
        visibility_framebuffer=VK_NULL_HANDLE;visibility_view=VK_NULL_HANDLE;
        lighting_framebuffer=VK_NULL_HANDLE;
        visibility_depth=VK_NULL_HANDLE;visibility_memory=VK_NULL_HANDLE;
        visibility_allocation_bytes=0;visibility_extent={};visibility_ready=lighting_ready=false;
        history_reset_pending=true;
        current_history_camera_mask=previous_history_camera_mask=0;
        previous_history_prepared=false;
    }
    void clear_acceleration(Acceleration& value)noexcept {
        if(value.handle && rt.destroy)rt.destroy(context.device,value.handle,nullptr);
        value.handle=VK_NULL_HANDLE;value.size=0;value.address=0;value.count=0;value.storage.clear();
    }
    ~Impl() {
        clear_environment();
        clear_visibility_target();
        for(auto& value:bottom)clear_acceleration(value);
        clear_acceleration(top);
        ray_indices.clear();instances.clear();scratch.clear();
        if(trace_pipeline)vkDestroyPipeline(context.device,trace_pipeline,nullptr);
        if(denoise_pipeline)vkDestroyPipeline(context.device,denoise_pipeline,nullptr);
        if(calibrate_pipeline)vkDestroyPipeline(context.device,calibrate_pipeline,nullptr);
        if(timing_pool)vkDestroyQueryPool(context.device,timing_pool,nullptr);
        if(compute_layout)vkDestroyPipelineLayout(context.device,compute_layout,nullptr);
        for(auto& buffer:buffers)buffer.clear();
        for(const auto& [key,pipeline]:source_depth_pipelines)if(pipeline)vkDestroyPipeline(context.device,pipeline,nullptr);
        if(visibility_pipeline)vkDestroyPipeline(context.device,visibility_pipeline,nullptr);
        if(visibility_pass)vkDestroyRenderPass(context.device,visibility_pass,nullptr);
        if(lighting_pipeline)vkDestroyPipeline(context.device,lighting_pipeline,nullptr);
        if(lighting_pass)vkDestroyRenderPass(context.device,lighting_pass,nullptr);
        if(graphics_pipeline)vkDestroyPipeline(context.device,graphics_pipeline,nullptr);
        if(glow_layer_pipeline)vkDestroyPipeline(context.device,glow_layer_pipeline,nullptr);
        if(pipeline_layout)vkDestroyPipelineLayout(context.device,pipeline_layout,nullptr);
        if(pool)vkDestroyDescriptorPool(context.device,pool,nullptr);
        if(scene_layout)vkDestroyDescriptorSetLayout(context.device,scene_layout,nullptr);
    }
    unsigned memory_type(std::uint32_t mask,VkMemoryPropertyFlags required=
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)const {
        // Random BVH and texture reads must stay in GPU memory when a mapped
        // device-local type is available. The first compatible host type on
        // discrete devices can instead live in system RAM across PCIe.
        for(const auto preferred:{required|VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  required|VK_MEMORY_PROPERTY_HOST_CACHED_BIT,required})
            for(unsigned i=0;i<memory_properties.memoryTypeCount;++i)
                if((mask&(1u<<i)) && (memory_properties.memoryTypes[i].propertyFlags&preferred)==preferred)return i;
        throw FormatError("Path Tracing has no memory type for the requested resource");
    }
    // host: mapped for CPU writes (else device-local only). device_address:
    // usable as acceleration-structure build input or storage by address.
    void allocate(Buffer& destination,VkDeviceSize required,VkBufferUsageFlags usage,VkDeviceSize limit,
                  bool device_address=false,bool host=true) {
        if(required<=destination.capacity)return;
        Buffer replacement;replacement.device=context.device;
        replacement.capacity=std::min(limit,std::max(required,destination.capacity+destination.capacity/2));
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size=replacement.capacity;info.usage=usage|(device_address?VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT:0u);
        info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(context.device,&info,nullptr,&replacement.buffer),"create scene buffer");
        VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(context.device,replacement.buffer,&requirements);
        VkMemoryAllocateInfo memory{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        memory.allocationSize=requirements.size;
        memory.memoryTypeIndex=host?memory_type(requirements.memoryTypeBits):
            memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkMemoryAllocateFlagsInfo flags{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
        flags.flags=VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        if(device_address)memory.pNext=&flags;
        replacement.memory_flags=memory_properties.memoryTypes[memory.memoryTypeIndex].propertyFlags;
        check(vkAllocateMemory(context.device,&memory,nullptr,&replacement.memory),"allocate scene buffer");
        replacement.allocation_bytes=memory.allocationSize;
        check(vkBindBufferMemory(context.device,replacement.buffer,replacement.memory,0),"bind scene buffer");
        if(host)check(vkMapMemory(context.device,replacement.memory,0,replacement.capacity,0,&replacement.mapped),"map scene buffer");
        if(device_address) {
            VkBufferDeviceAddressInfo address{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};address.buffer=replacement.buffer;
            replacement.address=rt.buffer_address(context.device,&address);
        }
        destination.swap(replacement);
    }
    VkShaderModule shader(const char* name,bool catalog=false)const {
        const auto path=catalog?context.shaders/name:context.shaders/"path_tracing"/name;
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        const auto length=file.tellg();
        if(length<=0 || length%4 || std::uint64_t(length)>64u*1024u*1024u)
            throw FormatError("Path Tracing shader unavailable or invalid: "+path.string());
        std::vector<std::uint32_t> words(std::size_t(length)/4);file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(words.data()),length))
            throw FormatError("Path Tracing shader truncated: "+path.string());
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize=words.size()*sizeof(std::uint32_t);info.pCode=words.data();
        VkShaderModule result{};check(vkCreateShaderModule(context.device,&info,nullptr,&result),"create shader module");return result;
    }
    void create() {
        if(!context.device || !context.physical || !context.render_pass || !context.material_layout)
            throw FormatError("Path Tracing requires a device, render pass, and material descriptor layout");
        vkGetPhysicalDeviceProperties(context.physical,&properties);
        vkGetPhysicalDeviceMemoryProperties(context.physical,&memory_properties);
        if(properties.limits.maxPerStageDescriptorStorageBuffers<8 || properties.limits.maxDescriptorSetStorageBuffers<8 ||
           properties.limits.maxPushConstantsSize<camera_bytes)
            throw FormatError("Path Tracing scene resources exceed this Vulkan device's descriptor or push-constant limits");
        if(context.dynamic_cull) {
            set_cull_mode=reinterpret_cast<PFN_vkCmdSetCullMode>(vkGetDeviceProcAddr(context.device,"vkCmdSetCullMode"));
            if(!set_cull_mode)throw FormatError("Path Tracing dynamic cull mode requires a Vulkan 1.3 device");
        }
        if(context.hardware_rays) {
            rt.load(context.device);
            VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
            VkPhysicalDeviceProperties2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};query.pNext=&acceleration;
            vkGetPhysicalDeviceProperties2(context.physical,&query);
            scratch_alignment=std::max<VkDeviceSize>(1,acceleration.minAccelerationStructureScratchOffsetAlignment);
        }
        // Bindings 11 (TLAS) and 12 (ray index list) exist only with hardware
        // rays. Compute tracing reads every scene binding.
        std::array<VkDescriptorSetLayoutBinding,13> bindings{};
        const unsigned binding_count=context.hardware_rays?13u:11u;
        for(unsigned i=0;i<bindings.size();++i) {
            bindings[i].binding=i;bindings[i].descriptorCount=1;
            bindings[i].stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT;
            bindings[i].descriptorType=i==7?VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                i==11?VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                i==5 || i==10?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            if(i==10)bindings[i].stageFlags|=VK_SHADER_STAGE_VERTEX_BIT;
        }
        VkDescriptorSetLayoutCreateInfo scene{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        scene.bindingCount=binding_count;scene.pBindings=bindings.data();
        check(vkCreateDescriptorSetLayout(context.device,&scene,nullptr,&scene_layout),"create scene descriptor layout");
        const std::array layouts{context.material_layout,scene_layout};
        const VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,camera_bytes};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout.setLayoutCount=layouts.size();layout.pSetLayouts=layouts.data();layout.pushConstantRangeCount=1;layout.pPushConstantRanges=&push;
        check(vkCreatePipelineLayout(context.device,&layout,nullptr,&pipeline_layout),"create pipeline layout");
        const VkPushConstantRange compute_push{VK_SHADER_STAGE_COMPUTE_BIT,0,camera_bytes};
        layout.pPushConstantRanges=&compute_push;
        check(vkCreatePipelineLayout(context.device,&layout,nullptr,&compute_layout),"create compute pipeline layout");
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,9},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1}};
        VkDescriptorPoolCreateInfo descriptor_pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptor_pool.maxSets=1;descriptor_pool.poolSizeCount=context.hardware_rays?4u:3u;descriptor_pool.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(context.device,&descriptor_pool,nullptr,&pool),"create descriptor pool");
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool=pool;allocation.descriptorSetCount=1;allocation.pSetLayouts=&scene_layout;
        check(vkAllocateDescriptorSets(context.device,&allocation,&descriptor),"allocate scene descriptor");
        graphics_pipeline=make_pipeline(context.render_pass,false);
        trace_pipeline=make_compute_pipeline(context.hardware_rays?"trace.rt.comp.spv":"trace.comp.spv");
        denoise_pipeline=make_compute_pipeline("denoise.comp.spv");
        calibrate_pipeline=make_compute_pipeline("calibrate.comp.spv");
        if(const auto* passes=std::getenv("HALO2_PATH_TRACING_DENOISE_PASSES");passes && *passes)
            denoise_passes=unsigned(std::clamp(std::atoi(passes),0,5));
        if(const auto* path=std::getenv("HALO2_PATH_TRACING_TIMINGS");path && *path) {
            VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
            info.queryType=VK_QUERY_TYPE_TIMESTAMP;info.queryCount=timing_slots;
            check(vkCreateQueryPool(context.device,&info,nullptr,&timing_pool),"create timing query pool");
            timing_period=properties.limits.timestampPeriod;
            timing_report.open(path,std::ios::app);
        }
    }
    VkPipeline make_compute_pipeline(const char* name) {
        ShaderModules modules;modules.device=context.device;modules.modules[0]=shader(name);
        VkComputePipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        pipeline.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,
            modules.modules[0],"main",nullptr};
        pipeline.layout=compute_layout;
        VkPipeline result{};
        check(vkCreateComputePipelines(context.device,VK_NULL_HANDLE,1,&pipeline,nullptr,&result),"create compute pipeline");
        return result;
    }
    // glow_layer: an additive self-illumination layer over a resolved surface
    // (glow_layer.frag), depth EQUAL to that surface, adding RGB only.
    VkPipeline make_pipeline(VkRenderPass pass,bool visibility,bool lighting=false,const NativeWorldKernelEntry* native=nullptr,
                             bool glow_layer=false) {
        ShaderModules modules;modules.device=context.device;
        const auto path=native?std::string(native->path_tracing_source_depth_vertex_path):std::string{};
        if(native && path.empty())throw FormatError("Path Tracing native depth lost its compiled authored vertex kernel");
        modules.modules[0]=native?shader(path.c_str(),true):shader("world.vert.spv");
        modules.modules[1]=shader(native?(visibility?"source_depth_visibility.frag.spv":lighting?"source_depth_lighting.frag.spv":"source_depth.frag.spv"):
            glow_layer?"glow_layer.frag.spv":
            visibility?"depth.frag.spv":lighting?"lighting.frag.spv":context.hardware_rays?"world.rt.frag.spv":"world.frag.spv");
        if(native)modules.modules[2]=shader("source_depth.geom.spv");
        const VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,modules.modules[0],"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,modules.modules[1],"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_GEOMETRY_BIT,modules.modules[2],"main",nullptr}};
        const VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]{
            {0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,position)},
            {1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,color)},
            {2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,uv)},
            {3,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,normal)},
            {4,0,VK_FORMAT_R32_UINT,offsetof(Vertex,material)},
            {5,0,VK_FORMAT_R32_UINT,offsetof(Vertex,source_stipple_level)},
            {6,0,VK_FORMAT_R32_UINT,offsetof(Vertex,lightmap)},
            {7,0,VK_FORMAT_R32_SINT,offsetof(Vertex,bsp_cluster)},
            {8,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,lightmap_uv)},
            {9,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,tangent)},
            {10,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,binormal)},
            {11,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,incident)}};
        const VkVertexInputAttributeDescription native_attributes[]{
            {0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,position)},
            {1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,color)},
            {2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,uv)},
            {3,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,lightmap_uv)},
            {4,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,normal)},
            {5,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,tangent)},
            {6,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,binormal)},
            {7,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,incident)},
            {8,0,VK_FORMAT_R32_UINT,offsetof(Vertex,lightmap)},
            {11,0,VK_FORMAT_R32_UINT,offsetof(Vertex,source_stipple_level)},
            {12,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,source_local_position)},
            {13,0,VK_FORMAT_R32_SFLOAT,offsetof(Vertex,source_color_alpha)},
            {14,0,VK_FORMAT_R32_SINT,offsetof(Vertex,bsp_cluster)},
            {9,0,VK_FORMAT_R32_SFLOAT,offsetof(Vertex,prt_cluster)},
            {10,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(Vertex,prt_weights)}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;
        input.vertexAttributeDescriptionCount=std::size(attributes);input.pVertexAttributeDescriptions=attributes;
        if(native) {input.vertexAttributeDescriptionCount=native->extended_vertex_bank?15:13;input.pVertexAttributeDescriptions=native_attributes;}
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=context.cull_mode;
        raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
        raster.depthClampEnable=native?VK_TRUE:VK_FALSE;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable=VK_TRUE;depth.depthWriteEnable=lighting || glow_layer?VK_FALSE:VK_TRUE;
        depth.depthCompareOp=glow_layer?VK_COMPARE_OP_EQUAL:VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        if(glow_layer) {
            attachment.blendEnable=VK_TRUE;
            attachment.srcColorBlendFactor=attachment.dstColorBlendFactor=VK_BLEND_FACTOR_ONE;
            attachment.colorBlendOp=VK_BLEND_OP_ADD;
            attachment.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        }
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount=visibility || lighting?0u:1u;
        blend.pAttachments=visibility || lighting?nullptr:&attachment;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR,VK_DYNAMIC_STATE_CULL_MODE};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount=set_cull_mode?3u:2u;dynamic.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline.stageCount=native?3:2;pipeline.pStages=stages;pipeline.pVertexInputState=&input;
        pipeline.pInputAssemblyState=&assembly;pipeline.pViewportState=&viewport;pipeline.pRasterizationState=&raster;
        pipeline.pMultisampleState=&samples;pipeline.pDepthStencilState=&depth;pipeline.pColorBlendState=&blend;
        pipeline.pDynamicState=&dynamic;pipeline.layout=pipeline_layout;pipeline.renderPass=pass;
        VkPipeline result{};
        check(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&pipeline,nullptr,&result),"create graphics pipeline");
        return result;
    }
    void prepare_environment(VkFormat color,VkFormat depth,unsigned side) {
        if(environment_recording)throw FormatError("Path Tracing environment resized while recording");
        if(environment_side==side && environment_color_format==color && environment_depth_format==depth)return;
        if(!side || side>properties.limits.maxImageDimensionCube || side>properties.limits.maxFramebufferWidth ||
            side>properties.limits.maxFramebufferHeight)
            throw FormatError("Path Tracing environment has an unsupported extent");
        const auto supported=[&](VkFormat format,VkImageUsageFlags usage,VkImageCreateFlags flags,unsigned layers) {
            VkImageFormatProperties image{};
            return vkGetPhysicalDeviceImageFormatProperties(context.physical,format,VK_IMAGE_TYPE_2D,
                VK_IMAGE_TILING_OPTIMAL,usage,flags,&image)==VK_SUCCESS && image.maxArrayLayers>=layers &&
                image.maxExtent.width>=side && image.maxExtent.height>=side;
        };
        if(!supported(color,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,6) ||
            !supported(depth,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,0,1))
            throw FormatError("Path Tracing environment requires compatible native color/depth attachments");
        VkFormatProperties filtering{};vkGetPhysicalDeviceFormatProperties(context.physical,color,&filtering);
        if(!(filtering.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            throw FormatError("Path Tracing environment requires filtered native sky samples");
        clear_environment();environment_color_format=color;environment_depth_format=depth;
        // A mip chain lets diffuse rays read a blurred sky (see scene.glsl);
        // it is regenerated by blits after the six faces are drawn.
        constexpr VkFormatFeatureFlags blit=VK_FORMAT_FEATURE_BLIT_SRC_BIT|VK_FORMAT_FEATURE_BLIT_DST_BIT|
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        environment_levels=(filtering.optimalTilingFeatures&blit)==blit?unsigned(std::bit_width(side)):1u;
        const auto image=[&](VkFormat format,VkImageUsageFlags usage,VkImageCreateFlags flags,unsigned layers,
                             VkImage& target,VkDeviceMemory& memory,unsigned levels=1) {
            VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.flags=flags;
            create.imageType=VK_IMAGE_TYPE_2D;create.format=format;create.extent={side,side,1};
            create.mipLevels=levels;create.arrayLayers=layers;create.samples=VK_SAMPLE_COUNT_1_BIT;
            create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=usage;create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
            check(vkCreateImage(context.device,&create,nullptr,&target),"create environment image");
            VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(context.device,target,&requirements);
            VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=requirements.size;
            allocate.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            check(vkAllocateMemory(context.device,&allocate,nullptr,&memory),"allocate environment image");
            environment_allocation_bytes+=requirements.size;
            check(vkBindImageMemory(context.device,target,memory,0),"bind environment image");
        };
        image(color,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|
            (environment_levels>1?VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT:0u),
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,6,environment_image,environment_memory,environment_levels);
        image(depth,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,0,1,environment_depth,environment_depth_memory);
        const auto view=[&](VkImage target,VkFormat format,VkImageViewType type,VkImageAspectFlags aspect,
                            unsigned first,unsigned count,VkImageView& result,unsigned levels=1) {
            VkImageViewCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};create.image=target;
            create.viewType=type;create.format=format;create.subresourceRange={aspect,0,levels,first,count};
            check(vkCreateImageView(context.device,&create,nullptr,&result),"create environment image view");
        };
        view(environment_image,color,VK_IMAGE_VIEW_TYPE_CUBE,VK_IMAGE_ASPECT_COLOR_BIT,0,6,environment_view,environment_levels);
        const bool stencil=depth==VK_FORMAT_D24_UNORM_S8_UINT || depth==VK_FORMAT_D32_SFLOAT_S8_UINT || depth==VK_FORMAT_D16_UNORM_S8_UINT;
        view(environment_depth,depth,VK_IMAGE_VIEW_TYPE_2D,VK_IMAGE_ASPECT_DEPTH_BIT|(stencil?VK_IMAGE_ASPECT_STENCIL_BIT:0),0,1,environment_depth_view);
        for(unsigned face=0;face<6;++face)
            view(environment_image,color,VK_IMAGE_VIEW_TYPE_2D,VK_IMAGE_ASPECT_COLOR_BIT,face,1,environment_faces[face]);
        std::array<VkAttachmentDescription,2> attachments{};
        attachments[0].format=color;attachments[0].samples=VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[0].finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachments[1].format=depth;attachments[1].samples=VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp=stencil?VK_ATTACHMENT_LOAD_OP_CLEAR:VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference color_reference{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        const VkAttachmentReference depth_reference{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color_reference;subpass.pDepthStencilAttachment=&depth_reference;
        const auto dependencies=detail::scene_pass_dependencies();
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.attachmentCount=attachments.size();
        pass.pAttachments=attachments.data();pass.subpassCount=1;pass.pSubpasses=&subpass;
        pass.dependencyCount=dependencies.size();pass.pDependencies=dependencies.data();
        check(vkCreateRenderPass(context.device,&pass,nullptr,&environment_pass),"create environment render pass");
        for(unsigned face=0;face<6;++face) {
            const std::array views{environment_faces[face],environment_depth_view};
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};framebuffer.renderPass=environment_pass;
            framebuffer.attachmentCount=views.size();framebuffer.pAttachments=views.data();
            framebuffer.width=framebuffer.height=side;framebuffer.layers=1;
            check(vkCreateFramebuffer(context.device,&framebuffer,nullptr,&environment_framebuffers[face]),"create environment framebuffer");
        }
        VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler.magFilter=sampler.minFilter=VK_FILTER_LINEAR;
        sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.maxLod=float(environment_levels-1);
        check(vkCreateSampler(context.device,&sampler,nullptr,&environment_sampler),"create environment sampler");
        const VkDescriptorImageInfo info{environment_sampler,environment_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.dstBinding=7;
        write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&info;
        vkUpdateDescriptorSets(context.device,1,&write,0,nullptr);environment_side=side;
    }
    // Downsample the six freshly drawn faces into the rest of the mip chain.
    void generate_environment_mips(VkCommandBuffer command) {
        if(environment_levels<2)return;
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=environment_image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,6};
        // Mip0 was written by the face render passes (final layout: read-only).
        barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        std::array<VkImageMemoryBarrier,2> barriers{barrier,barrier};
        barriers[1].subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,1,environment_levels-1,0,6};
        barriers[1].srcAccessMask=0;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[1].oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,
            std::uint32_t(barriers.size()),barriers.data());
        for(unsigned level=1;level<environment_levels;++level) {
            const auto source=std::int32_t(std::max(1u,environment_side>>(level-1)));
            const auto target=std::int32_t(std::max(1u,environment_side>>level));
            VkImageBlit region{};
            region.srcSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level-1,0,6};region.srcOffsets[1]={source,source,1};
            region.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level,0,6};region.dstOffsets[1]={target,target,1};
            vkCmdBlitImage(command,environment_image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                environment_image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region,VK_FILTER_LINEAR);
            // The new level becomes the next blit's source.
            VkImageMemoryBarrier next=barrier;next.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,level,1,0,6};
            next.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;next.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            next.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;next.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,0,nullptr,0,nullptr,1,&next);
        }
        barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,environment_levels,0,6};
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    }
    static VkDeviceSize lighting_cells(VkExtent2D extent,unsigned scale) {
        return VkDeviceSize((extent.width+scale-1)/scale)*((extent.height+scale-1)/scale);
    }
    // Depth words, padded to whole 16-byte lanes so every grid record and
    // packed denoiser cell starts on one (denoise.comp reads uvec4 lanes).
    VkDeviceSize depth_words()const {
        return (VkDeviceSize(visibility_extent.width)*visibility_extent.height+3)/4*4;
    }
    VkDeviceSize lighting_offset()const {return 32+depth_words()*sizeof(float);}
    VkDeviceSize lighting_bytes()const {
        return lighting_cells(visibility_extent,lighting_scale)*lighting_record_words*sizeof(std::uint32_t);
    }
    VkDeviceSize visibility_bytes()const {
        //32-byte immutable frame header, full-resolution D32, current records,
        // separate world/foreground histories including direct diffuse/specular
        // channels whose visibility can change independently, then the
        // denoiser's two ping-pong buffers.
        return calibration_offset()+(calibration_groups()+1)*calibration_terms*sizeof(std::uint32_t);
    }
    // Calibration partial sums (one twelve-term row per 8x8 workgroup of the
    // lighting grid) followed by their total, after the denoiser buffers.
    static constexpr unsigned calibration_terms=12;
    VkDeviceSize calibration_offset()const {
        return lighting_offset()+lighting_cells(visibility_extent,lighting_scale)*
            (lighting_record_words+2*history_words+lighting_filter_words)*sizeof(std::uint32_t);
    }
    VkDeviceSize calibration_groups()const {
        const auto scale=std::max(1u,lighting_scale);
        const VkDeviceSize width=(visibility_extent.width+scale-1)/scale,height=(visibility_extent.height+scale-1)/scale;
        return ((width+7)/8)*((height+7)/8);
    }
    // Host side, after the frame fence: fold the previous frame's sums into
    // the map's cumulative sums and solve least squares for
    // L = ks*S + ki*I + c*E (calibrate.comp), with a small ridge toward the
    // current gains for views without sun, shade or emitters. The emitters
    // reach the screen through the grid, which the resolve scales by ki, so
    // their emission scale is c/ki. Gains stay bounded and ease toward the
    // solution, so turning the camera never pumps the exposure.
    void update_calibration() {
        const auto& buffer=buffers[6];
        const auto at=calibration_offset()+calibration_groups()*calibration_terms*sizeof(float);
        if(!buffer.mapped || !visibility_extent.width || at+calibration_terms*sizeof(float)>buffer.capacity)return;
        std::array<float,calibration_terms> sum{};
        std::memcpy(sum.data(),static_cast<const std::byte*>(buffer.mapped)+at,sizeof(sum));
        if(!std::all_of(sum.begin(),sum.end(),[](float value){return std::isfinite(value);}) || sum[5]<64.f)return;
        for(unsigned n=0;n<calibration_terms;++n)calibration_sums[n]+=sum[n];
        const auto& pooled=calibration_sums;
        const double ss=pooled[0],si=pooled[1],ii=pooled[2],sl=pooled[3],il=pooled[4],count=pooled[5];
        const double se=pooled[7],ie=pooled[8],ee=pooled[9],el=pooled[10];
        if(count<4096)return;
        // Normal equations, each unknown ridged toward its current value.
        const double previous_emitters=double(calibration[2])*calibration[1];
        const double sun_ridge=.05*ss+1.e-4*count,indirect_ridge=.05*ii+1.e-4*count,emitter_ridge=.05*ee+1.e-6*count;
        // A map without lightmap emitters in view keeps the two-gain fit.
        const bool emitters=ee>1.e-4*count;
        std::array<std::array<double,4>,3> m{{
            {ss+sun_ridge,si,se,sl+sun_ridge},
            {si,ii+indirect_ridge,ie,il+indirect_ridge},
            {se,ie,ee+emitter_ridge,el+emitter_ridge*previous_emitters}}};
        const unsigned unknowns=emitters?3:2;
        // Gaussian elimination with partial pivoting.
        for(unsigned column=0;column<unknowns;++column) {
            unsigned pivot=column;
            for(unsigned row=column+1;row<unknowns;++row)if(std::abs(m[row][column])>std::abs(m[pivot][column]))pivot=row;
            if(!(std::abs(m[pivot][column])>1.e-12))return;
            std::swap(m[column],m[pivot]);
            for(unsigned row=0;row<unknowns;++row)if(row!=column) {
                const double factor=m[row][column]/m[column][column];
                for(unsigned k=column;k<4;++k)m[row][k]-=factor*m[column][k];
            }
        }
        const auto solution=[&](unsigned n){return m[n][3]/m[n][n];};
        const auto sun=std::clamp(solution(0),.5,3.),indirect=std::clamp(solution(1),.5,3.);
        const auto emitter_scale=emitters?std::clamp(solution(2)/indirect,.05,100.):double(calibration[2]);
        // The first solution applies at once; later ones ease in over a few
        // seconds, and move less and less as the sums grow.
        const float rate=++calibration_frames==1?1.f:.01f;
        calibration[0]+=(float(sun)-calibration[0])*rate;
        calibration[1]+=(float(indirect)-calibration[1])*rate;
        calibration[2]+=(float(emitter_scale)-calibration[2])*rate;
        if(std::getenv("HALO2_PATH_TRACING_CALIBRATION_LOG") && calibration_frames%30==0)
            std::fprintf(stderr,"PT calibration frame=%u n=%.0f ss=%.4g ii=%.4g ee=%.4g sl=%.4g il=%.4g el=%.4g -> sun=%.3f indirect=%.3f emitters=%.3f (smoothed %.3f %.3f %.3f)\n",
                calibration_frames,count,ss,ii,ee,sl,il,el,sun,indirect,emitter_scale,
                double(calibration[0]),double(calibration[1]),double(calibration[2]));
    }
    VkDeviceSize history_offset()const {return lighting_offset()+lighting_bytes();}
    VkDeviceSize history_bytes()const {
        return lighting_cells(visibility_extent,lighting_scale)*2*history_words*sizeof(std::uint32_t);
    }
    void write_previous_history_mask() {
        const auto mask=history_reset_pending?0u:previous_history_camera_mask;
        std::memcpy(static_cast<std::byte*>(buffers[6].mapped)+6*sizeof(std::uint32_t),&mask,sizeof(mask));
    }
    bool environment_nee_available()const {
        return environment_source_available || environment_fallback_available;
    }
    void update_visibility_binding() {
        const auto size=visibility_bytes();
        if(size>properties.limits.maxStorageBufferRange)
            throw FormatError("Path Tracing visibility exceeds maxStorageBufferRange");
        auto& buffer=buffers[6];
        allocate(buffer,size,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 properties.limits.maxStorageBufferRange);
        const std::array<std::uint32_t,8> dimensions{visibility_extent.width,visibility_extent.height,
            std::uint32_t(depth_words()),lighting_scale,frame_serial,
            environment_nee_available()?1u:0u,history_reset_pending?0u:previous_history_camera_mask,
            direct_history_reset?1u:0u};
        std::memcpy(buffer.mapped,dimensions.data(),sizeof(dimensions));buffer.populated=sizeof(dimensions);
        const auto previous_size=sizeof(HistoryCamera)+history_bytes();
        if(previous_size>properties.limits.maxStorageBufferRange)
            throw FormatError("Path Tracing previous history exceeds maxStorageBufferRange");
        auto& previous=buffers[8];
        allocate(previous,previous_size,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 properties.limits.maxStorageBufferRange);
        std::memcpy(previous.mapped,&previous_history_camera,sizeof(previous_history_camera));
        previous.populated=previous_size;
        const VkDescriptorBufferInfo infos[]{{buffer.buffer,0,size},{previous.buffer,0,previous_size}};
        std::array<VkWriteDescriptorSet,2> writes{};
        for(unsigned i=0;i<writes.size();++i) {
            auto& write=writes[i];write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet=descriptor;write.dstBinding=i?9:6;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&infos[i];
        }
        vkUpdateDescriptorSets(context.device,writes.size(),writes.data(),0,nullptr);
    }
    // Host side, after the previous frame's fence: write the ray index list
    // and size every acceleration structure this frame will build.
    void prepare_acceleration(const Scene& scene) {
        const auto triangles=std::uint32_t(scene.triangles.size());
        const auto static_count=std::min(scene.static_triangles,triangles);
        if(static_geometry_changed)static_acceleration_dirty=true;
        // Slot of a triangle: static/dynamic, then opaque, alpha-tested or light-only.
        const auto slot_of=[&](std::uint32_t index) {
            const auto flags=scene.materials[scene.triangles[index].metadata[0]].texture[3];
            const unsigned kind=(flags&light_only_flag)?2u:(flags&9u)?1u:0u;
            return (index<static_count?static_opaque:dynamic_opaque)+kind;
        };
        std::array<std::uint32_t,bottom_slots> counts{};
        // An unchanged static prefix keeps its split.
        const bool keep_static=!static_acceleration_dirty && static_indices_written;
        if(keep_static)for(unsigned slot=static_opaque;slot<dynamic_opaque;++slot)counts[slot]=bottom[slot].count;
        for(std::uint32_t n=keep_static?static_count:0u;n<triangles;++n)++counts[slot_of(n)];
        std::uint32_t first=0;
        for(unsigned slot=0;slot<bottom_slots;++slot){bottom[slot].first=first;bottom[slot].count=counts[slot];first+=counts[slot];}
        const auto previous_capacity=ray_indices.capacity;
        allocate(ray_indices,std::max<VkDeviceSize>(16,VkDeviceSize(triangles)*3*sizeof(std::uint32_t)),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            properties.limits.maxStorageBufferRange,true);
        if(ray_indices.capacity!=previous_capacity)static_indices_written=false;
        // Three vertex indices per triangle: lanes 0..2 of its ten 16-byte lanes.
        auto* words=static_cast<std::uint32_t*>(ray_indices.mapped);
        std::array<std::uint32_t,bottom_slots> cursor{};
        for(unsigned slot=0;slot<bottom_slots;++slot)cursor[slot]=bottom[slot].first;
        const bool write_static=static_acceleration_dirty || !static_indices_written;
        for(std::uint32_t n=write_static?0u:static_count;n<triangles;++n) {
            const auto slot=slot_of(n);
            auto* entry=words+std::size_t(cursor[slot]++)*3;
            entry[0]=n*10;entry[1]=n*10+1;entry[2]=n*10+2;
        }
        static_indices_written=true;
        ray_indices.populated=VkDeviceSize(triangles)*3*sizeof(std::uint32_t);
        // Size and (re)create each structure that will be built this frame.
        acceleration_triangles=triangles;
        for(unsigned slot=0;slot<bottom_slots;++slot) {
            const bool is_static=static_slot(slot);
            if(is_static && !static_acceleration_dirty)continue;
            auto& target=bottom[slot];
            if(!target.count)continue;
            const auto geometry=triangle_geometry(slot);
            const auto info=bottom_build(slot,geometry);
            VkAccelerationStructureBuildSizesInfoKHR sizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
            rt.build_sizes(context.device,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,&info,&target.count,&sizes);
            ensure_structure(target,sizes.accelerationStructureSize,VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
            bottom_scratch[slot]=sizes.buildScratchSize;
        }
        // Instances for the top level. Write them now; addresses are stable.
        allocate(instances,bottom_slots*sizeof(VkAccelerationStructureInstanceKHR),
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,~VkDeviceSize{0},true);
        std::array<VkAccelerationStructureInstanceKHR,bottom_slots> rows{};
        std::uint32_t instance_count=0;
        for(unsigned slot=0;slot<bottom_slots;++slot) {
            const auto& source=bottom[slot];
            if(!source.count || !source.handle)continue;
            auto& row=rows[instance_count++];
            row.transform.matrix[0][0]=row.transform.matrix[1][1]=row.transform.matrix[2][2]=1.f;
            row.instanceCustomIndex=source.first;
            row.mask=light_slot(slot)?light_mask:static_slot(slot)?static_occluder_mask:dynamic_occluder_mask;
            const bool opaque=!alpha_slot(slot);
            row.flags=VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR|
                (opaque?VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR:VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR);
            row.accelerationStructureReference=source.address;
        }
        std::memcpy(instances.mapped,rows.data(),sizeof(rows));
        top_instances=instance_count;
        VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.data.deviceAddress=instances.address;
        VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type=VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        info.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        info.geometryCount=1;info.pGeometries=&geometry;
        const std::uint32_t maximum_instances=bottom_slots;
        VkAccelerationStructureBuildSizesInfoKHR sizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        rt.build_sizes(context.device,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,&info,&maximum_instances,&sizes);
        const auto previous_top=top.handle;
        ensure_structure(top,sizes.accelerationStructureSize,VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
        top_scratch=sizes.buildScratchSize;
        // Scratch covers every bottom-level build plus the top level.
        VkDeviceSize scratch_bytes=align_scratch(top_scratch);
        for(unsigned slot=0;slot<bottom_slots;++slot)
            if(bottom_scratch_needed(slot))scratch_bytes+=align_scratch(bottom_scratch[slot]);
        allocate(scratch,scratch_bytes+scratch_alignment,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,~VkDeviceSize{0},true,false);
        if(previous_top!=top.handle || !acceleration_descriptor_written) {
            VkWriteDescriptorSetAccelerationStructureKHR structure{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
            structure.accelerationStructureCount=1;structure.pAccelerationStructures=&top.handle;
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.pNext=&structure;
            write.dstSet=descriptor;write.dstBinding=11;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            vkUpdateDescriptorSets(context.device,1,&write,0,nullptr);
            acceleration_descriptor_written=true;
        }
        const VkDescriptorBufferInfo index_info{ray_indices.buffer,0,VK_WHOLE_SIZE};
        VkWriteDescriptorSet index_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        index_write.dstSet=descriptor;index_write.dstBinding=12;index_write.descriptorCount=1;
        index_write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;index_write.pBufferInfo=&index_info;
        vkUpdateDescriptorSets(context.device,1,&index_write,0,nullptr);
        acceleration_pending=true;
    }
    VkAccelerationStructureGeometryKHR triangle_geometry(unsigned slot)const {
        VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType=VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags=alpha_slot(slot)?0u:VK_GEOMETRY_OPAQUE_BIT_KHR;
        auto& data=geometry.geometry.triangles;
        data.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        // Each 160-byte GpuTriangle begins with p0/p1/p2 in 16-byte lanes.
        data.vertexFormat=VK_FORMAT_R32G32B32_SFLOAT;data.vertexData.deviceAddress=buffers[1].address;
        data.vertexStride=16;data.maxVertex=acceleration_triangles?acceleration_triangles*10-1:0;
        data.indexType=VK_INDEX_TYPE_UINT32;data.indexData.deviceAddress=ray_indices.address;
        return geometry;
    }
    VkAccelerationStructureBuildGeometryInfoKHR bottom_build(unsigned slot,const VkAccelerationStructureGeometryKHR& geometry)const {
        const bool is_static=static_slot(slot);
        VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type=VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        info.flags=is_static?VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR:
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
        info.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        info.dstAccelerationStructure=bottom[slot].handle;
        info.geometryCount=1;info.pGeometries=&geometry;
        return info;
    }
    // Device side: build the structures prepare_acceleration sized.
    void record_acceleration(VkCommandBuffer command) {
        if(timing_pool) {
            vkCmdResetQueryPool(command,timing_pool,0,timing_slots);
            timing_count=0;timestamp(command,"start");
        }
        if(!context.hardware_rays || !acceleration_pending)return;
        acceleration_pending=false;
        const auto base=(scratch.address+scratch_alignment-1)/scratch_alignment*scratch_alignment;
        VkDeviceSize offset=align_scratch(top_scratch);
        std::array<VkAccelerationStructureGeometryKHR,bottom_slots> geometries{};
        std::array<VkAccelerationStructureBuildGeometryInfoKHR,bottom_slots> infos{};
        std::array<VkAccelerationStructureBuildRangeInfoKHR,bottom_slots> ranges{};
        std::array<const VkAccelerationStructureBuildRangeInfoKHR*,bottom_slots> range_pointers{};
        unsigned builds=0;
        for(unsigned slot=0;slot<bottom_slots;++slot) {
            if(!bottom_scratch_needed(slot))continue;
            geometries[builds]=triangle_geometry(slot);
            infos[builds]=bottom_build(slot,geometries[builds]);
            infos[builds].pGeometries=&geometries[builds];
            infos[builds].scratchData.deviceAddress=base+offset;offset+=align_scratch(bottom_scratch[slot]);
            ranges[builds]={bottom[slot].count,bottom[slot].first*3u*std::uint32_t(sizeof(std::uint32_t)),0,0};
            range_pointers[builds]=&ranges[builds];++builds;
        }
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        if(builds) {
            rt.build(command,builds,infos.data(),range_pointers.data());
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,0,1,&barrier,0,nullptr,0,nullptr);
        }
        static_acceleration_dirty=false;
        VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.data.deviceAddress=instances.address;
        VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type=VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        info.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        info.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        info.dstAccelerationStructure=top.handle;info.geometryCount=1;info.pGeometries=&geometry;
        info.scratchData.deviceAddress=base;
        const VkAccelerationStructureBuildRangeInfoKHR range{top_instances,0,0,0};
        const auto* range_pointer=&range;
        rt.build(command,1,&info,&range_pointer);
        barrier.dstAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR|VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        timestamp(command,"acceleration");
    }
    VkDeviceSize align_scratch(VkDeviceSize value)const {
        return (value+scratch_alignment-1)/scratch_alignment*scratch_alignment;
    }
    bool bottom_scratch_needed(unsigned slot)const {
        const bool is_static=static_slot(slot);
        return bottom[slot].count && (!is_static || static_acceleration_dirty);
    }
    // Creates the structure object when absent or too small. The caller has
    // waited for the previous frame, so a replaced structure is idle.
    void ensure_structure(Acceleration& target,VkDeviceSize size,VkAccelerationStructureTypeKHR type) {
        if(target.handle && target.size>=size)return;
        if(target.handle)rt.destroy(context.device,target.handle,nullptr);
        target.handle=VK_NULL_HANDLE;
        allocate(target.storage,size,VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,~VkDeviceSize{0},true,false);
        VkAccelerationStructureCreateInfoKHR create{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        create.buffer=target.storage.buffer;create.size=target.storage.capacity;create.type=type;
        check(rt.create(context.device,&create,nullptr,&target.handle),"create acceleration structure");
        target.size=target.storage.capacity;
        VkAccelerationStructureDeviceAddressInfoKHR address{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        address.accelerationStructure=target.handle;target.address=rt.structure_address(context.device,&address);
    }
    void snapshot_previous_history(VkCommandBuffer command) {
        if(previous_history_prepared)return;
        previous_history_prepared=true;
        if(history_reset_pending)previous_history_camera_mask=0;
        write_previous_history_mask();
        if(!previous_history_camera_mask || !history_bytes())return;
        std::array<VkBufferMemoryBarrier,2> barriers{};
        for(auto& barrier:barriers) {
            barrier.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.size=history_bytes();
        }
        barriers[0].buffer=buffers[6].buffer;barriers[0].offset=history_offset();
        barriers[0].srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        barriers[1].buffer=buffers[8].buffer;barriers[1].offset=sizeof(HistoryCamera);
        barriers[1].srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|
            VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,barriers.size(),barriers.data(),0,nullptr);
        const VkBufferCopy copy{history_offset(),sizeof(HistoryCamera),history_bytes()};
        vkCmdCopyBuffer(command,buffers[6].buffer,buffers[8].buffer,1,&copy);
        barriers[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        barriers[0].dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barriers[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,barriers.size(),barriers.data(),0,nullptr);
    }
    void capture_history_camera(std::span<const std::byte> camera) {
        float eye;std::memcpy(&eye,camera.data()+23*sizeof(float),sizeof(eye));
        const unsigned role=eye==2.f?1:0;const auto bit=1u<<role;
        if((previous_history_camera_mask&bit) &&
           std::bit_cast<std::uint32_t>(eye)!=std::bit_cast<std::uint32_t>(previous_history_eye[role])) {
            previous_history_camera_mask&=~bit;
            write_previous_history_mask();
        }
        std::memcpy(current_history_camera.matrix[role].data(),camera.data(),16*sizeof(float));
        current_history_camera.viewport[role]={visibility_viewport.x,visibility_viewport.y,
            visibility_viewport.width,visibility_viewport.height};
        current_history_eye[role]=eye;current_history_camera_mask|=bit;
    }
    void prepare_visibility(VkExtent2D extent) {
        if(visibility_recording || lighting_recording)throw FormatError("Path Tracing visibility cannot resize while recording");
        visibility_ready=lighting_ready=false;
        if(!extent.width || !extent.height ||
           extent.width>properties.limits.maxFramebufferWidth || extent.height>properties.limits.maxFramebufferHeight ||
           extent.width>properties.limits.maxImageDimension2D || extent.height>properties.limits.maxImageDimension2D ||
           std::uint64_t(extent.width)*extent.height>std::numeric_limits<std::uint32_t>::max())
            throw FormatError("Path Tracing visibility has an unsupported extent");
        if(extent.width==visibility_extent.width && extent.height==visibility_extent.height)return;
        const auto plan=lighting_storage_plan(extent.width,extent.height,properties.limits.maxStorageBufferRange);
        if(!plan.scale)throw FormatError("Path Tracing visibility exceeds maxStorageBufferRange");
        lighting_scale=plan.scale;
        constexpr auto usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkImageFormatProperties support{};
        if(vkGetPhysicalDeviceImageFormatProperties(context.physical,VK_FORMAT_D32_SFLOAT,VK_IMAGE_TYPE_2D,
            VK_IMAGE_TILING_OPTIMAL,usage,0,&support)!=VK_SUCCESS ||
           extent.width>support.maxExtent.width || extent.height>support.maxExtent.height)
            throw FormatError("Path Tracing visibility requires a transferable D32 depth attachment");
        if(!visibility_pass) {
            VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_D32_SFLOAT;
            attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            const VkAttachmentReference depth{0,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.pDepthStencilAttachment=&depth;
            std::array<VkSubpassDependency,2> dependencies{};
            dependencies[0].srcSubpass=VK_SUBPASS_EXTERNAL;dependencies[0].dstSubpass=0;
            dependencies[0].srcStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT|
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[0].dstStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT|
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[0].dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].srcSubpass=0;dependencies[1].dstSubpass=VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;
            dependencies[1].srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            pass.attachmentCount=1;pass.pAttachments=&attachment;pass.subpassCount=1;pass.pSubpasses=&subpass;
            pass.dependencyCount=dependencies.size();pass.pDependencies=dependencies.data();
            check(vkCreateRenderPass(context.device,&pass,nullptr,&visibility_pass),"create visibility render pass");
        }
        if(!visibility_pipeline)visibility_pipeline=make_pipeline(visibility_pass,true);
        if(!lighting_pass) {
            VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_D32_SFLOAT;
            attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;
            attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout=attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            const VkAttachmentReference depth{0,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.pDepthStencilAttachment=&depth;
            std::array<VkSubpassDependency,2> dependencies{};
            dependencies[0].srcSubpass=VK_SUBPASS_EXTERNAL;dependencies[0].dstSubpass=0;
            dependencies[0].srcStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;
            dependencies[0].dstStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
            // STORE still writes the attachment when the lighting pipeline
            // disables depth writes. Order that store and its final layout
            // transition as well as the depth tests and HDR storage writes.
            dependencies[0].dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|
                VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
            dependencies[1].srcSubpass=0;dependencies[1].dstSubpass=VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].dstStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT;
            dependencies[1].srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_TRANSFER_READ_BIT;
            VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            pass.attachmentCount=1;pass.pAttachments=&attachment;pass.subpassCount=1;pass.pSubpasses=&subpass;
            pass.dependencyCount=dependencies.size();pass.pDependencies=dependencies.data();
            check(vkCreateRenderPass(context.device,&pass,nullptr,&lighting_pass),"create HDR lighting render pass");
        }
        if(!lighting_pipeline)lighting_pipeline=make_pipeline(lighting_pass,false,true);
        clear_visibility_target();
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType=VK_IMAGE_TYPE_2D;image.format=VK_FORMAT_D32_SFLOAT;
        image.extent={extent.width,extent.height,1};image.mipLevels=image.arrayLayers=1;
        image.samples=VK_SAMPLE_COUNT_1_BIT;image.tiling=VK_IMAGE_TILING_OPTIMAL;
        image.usage=usage;image.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateImage(context.device,&image,nullptr,&visibility_depth),"create visibility depth image");
        VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(context.device,visibility_depth,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(context.device,&allocation,nullptr,&visibility_memory),"allocate visibility depth memory");
        visibility_allocation_bytes=requirements.size;
        check(vkBindImageMemory(context.device,visibility_depth,visibility_memory,0),"bind visibility depth memory");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image=visibility_depth;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=VK_FORMAT_D32_SFLOAT;
        view.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};
        check(vkCreateImageView(context.device,&view,nullptr,&visibility_view),"create visibility depth view");
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass=visibility_pass;framebuffer.attachmentCount=1;framebuffer.pAttachments=&visibility_view;
        framebuffer.width=extent.width;framebuffer.height=extent.height;framebuffer.layers=1;
        check(vkCreateFramebuffer(context.device,&framebuffer,nullptr,&visibility_framebuffer),"create visibility framebuffer");
        framebuffer.renderPass=lighting_pass;
        check(vkCreateFramebuffer(context.device,&framebuffer,nullptr,&lighting_framebuffer),"create HDR lighting framebuffer");
        visibility_extent=extent;update_visibility_binding();
    }
    void bind(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
              std::span<const std::byte> camera,std::uint32_t material_index,bool visibility,bool lighting=false,
              const NativeWorldKernelEntry* native=nullptr,std::optional<std::uint32_t> glow_layer_program={}) {
        const bool glow_layer=glow_layer_program.has_value();
        if(!uploaded)throw FormatError("Path Tracing scene must be uploaded before binding");
        if(camera.size()!=camera_bytes || (material_index!=~0u && material_index>=0x80000000u))
            throw FormatError("Path Tracing draw lost its camera or material identity");
        if(visibility!=visibility_recording || lighting!=lighting_recording || (visibility && lighting))
            throw FormatError("Path Tracing material pipeline does not match the visibility phase");
        if(visibility || lighting)capture_history_camera(camera);
        std::array<std::byte,camera_bytes> push;std::copy(camera.begin(),camera.end(),push.begin());
        const std::array<float,2> quality{float(samples),float(bounces)};
        std::memcpy(push.data()+camera_bytes-4*sizeof(float),quality.data(),sizeof(quality));
        // RT does not otherwise consume viewport.z. A push-constant flag is
        // immutable for each recorded draw, even when another camera phase
        // overwrites the visibility pixels later in this command buffer.
        const float guarded=lighting?2.f:!visibility && lighting_ready?3.f:!visibility && visibility_ready?1.f:0.f;
        std::memcpy(push.data()+camera_bytes-2*sizeof(float),&guarded,sizeof(guarded));
        if(material_index!=~0u) {
            const auto binding=material_index|0x80000000u;
            std::memcpy(push.data()+camera_bytes-sizeof(binding),&binding,sizeof(binding));
        }
        // glow_layer.frag reads its program's control words at this atlas
        // offset instead of the visibility guard.
        if(glow_layer)std::memcpy(push.data()+camera_bytes-2*sizeof(float),&*glow_layer_program,sizeof(std::uint32_t));
        if(lighting){lighting_push=push;lighting_push_valid=true;}
        if(glow_layer && !glow_layer_pipeline)glow_layer_pipeline=make_pipeline(context.render_pass,false,false,nullptr,true);
        auto pipeline=glow_layer?glow_layer_pipeline:visibility?visibility_pipeline:lighting?lighting_pipeline:graphics_pipeline;
        if(native) {
            if(!source_depth_prepared || material_index==~0u)throw FormatError("Path Tracing source depth requires its current raster and material owner");
            const auto key=std::pair{std::string(native->path_tracing_source_depth_vertex_path),unsigned(visibility?1:lighting?2:0)};
            auto found=source_depth_pipelines.find(key);
            if(found==source_depth_pipelines.end())found=source_depth_pipelines.emplace(key,
                make_pipeline(visibility?visibility_pass:lighting?lighting_pass:context.render_pass,visibility,lighting,native)).first;
            pipeline=found->second;
        }
        const VkDescriptorSet sets[]{material,descriptor};
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        if(set_cull_mode)set_cull_mode(command,draw_cull);
        draw_cull=VK_CULL_MODE_NONE;
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,2,sets,1,&dynamic_bank_offset);
        vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,camera_bytes,push.data());
    }
};
Render::Render(RenderContext context):impl_(std::make_unique<Impl>()) {
    impl_->context=std::move(context);impl_->create();
}
Render::~Render()=default;
void Render::set_quality(unsigned samples,unsigned bounces) {
    if(samples<1 || samples>2 || bounces<1 || bounces>2)
        throw FormatError("Path Tracing sample and bounce budgets must be 1 or 2");
    if(impl_->samples!=samples || impl_->bounces!=bounces)impl_->history_reset_pending=true;
    impl_->samples=samples;impl_->bounces=bounces;
}
VkPipeline Render::pipeline()const noexcept{return impl_->graphics_pipeline;}
VkPipelineLayout Render::layout()const noexcept{return impl_->pipeline_layout;}
std::size_t Render::allocated_bytes()const noexcept {
    std::size_t total=std::size_t(impl_->visibility_allocation_bytes+impl_->environment_allocation_bytes);
    for(const auto& buffer:impl_->buffers)total+=std::size_t(buffer.allocation_bytes);
    return total;
}
std::size_t Render::cpu_bytes()const noexcept {
    return impl_->previous_shadow_geometry.capacity()*sizeof(Impl::ShadowTriangle)+
        impl_->previous_shadow_lights.capacity()*sizeof(GpuLight);
}
void Render::record_temporal_stats(VkCommandBuffer command)const {
    const auto& p=*impl_;if(!p.buffers[6].buffer || !p.visibility_extent.width)return;
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer=p.buffers[6].buffer;barrier.offset=p.lighting_offset()+p.lighting_bytes();
    barrier.size=p.visibility_bytes()-barrier.offset;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|
        VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&barrier,0,nullptr);
}
Render::TemporalStats Render::temporal_stats()const {
    const auto& p=*impl_;TemporalStats result{};result.frame_serial=p.frame_serial;
    const auto& buffer=p.buffers[6];
    if(!buffer.mapped || !p.visibility_extent.width || !p.visibility_extent.height || p.history_reset_pending)return result;
    if(!(buffer.memory_flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        throw FormatError("Path Tracing temporal diagnostics require coherent mapped history");
    const auto cells=p.lighting_cells(p.visibility_extent,p.lighting_scale);
    const auto samples=std::uint32_t(std::min<VkDeviceSize>(64,cells));
    const auto materials=p.buffers[2].populated/sizeof(GpuMaterial);
    for(unsigned bank=0;bank<result.banks.size();++bank) {
        auto& stats=result.banks[bank];stats.sampled_cells=samples;std::uint32_t sum{};
        const auto base=p.lighting_offset()+p.lighting_bytes()+bank*cells*p.history_words*sizeof(std::uint32_t);
        for(std::uint32_t sample=0;sample<samples;++sample) {
            const auto at=cells*sample/samples;
            std::array<std::uint32_t,Impl::history_words> record;
            std::memcpy(record.data(),static_cast<const std::byte*>(buffer.mapped)+base+at*sizeof(record),sizeof(record));
            if(record[7]==0xffffffffu || record[13]!=p.frame_serial)continue;
            record[12]&=31u; // Upper27 bits identify the current reflection response.
            if(record[7]>=materials || record[12]<1 || record[12]>31)
                throw FormatError("Path Tracing temporal diagnostics found an invalid completed history record");
            ++stats.sampled_valid_count;stats.maximum_count=std::max(stats.maximum_count,record[12]);sum+=record[12];
        }
        if(stats.sampled_valid_count)stats.mean_count=double(sum)/stats.sampled_valid_count;
    }
    return result;
}
void Render::prepare_environment(VkFormat color,VkFormat depth,unsigned side,bool source_available) {
    auto& p=*impl_;const bool previous=p.environment_nee_available();
    p.prepare_environment(color,depth,side);p.environment_source_available=source_available;
    if(previous!=p.environment_nee_available())p.history_reset_pending=true;
    // Production prepares the cube after upload. Keep the frame header final
    // before recording any world/foreground/environment command buffers.
    if(p.uploaded)p.update_visibility_binding();
}
void Render::begin_environment(VkCommandBuffer command,unsigned face,std::array<float,4> color) {
    auto& p=*impl_;
    if(!p.environment_side || p.environment_recording || face>=6)
        throw FormatError("Path Tracing environment needs a prepared face target");
    std::array<VkClearValue,2> clear{};std::copy(color.begin(),color.end(),clear[0].color.float32);
    clear[1].depthStencil={1,0};
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=p.environment_pass;
    begin.framebuffer=p.environment_framebuffers[face];begin.renderArea={{0,0},{p.environment_side,p.environment_side}};
    begin.clearValueCount=clear.size();begin.pClearValues=clear.data();
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);p.environment_recording=true;
    p.environment_face=face;
}
void Render::end_environment(VkCommandBuffer command) {
    if(!impl_->environment_recording)throw FormatError("Path Tracing environment has no recording phase");
    vkCmdEndRenderPass(command);impl_->environment_recording=false;
    if(impl_->environment_face==5)impl_->generate_environment_mips(command);
}
void Render::upload(const Scene& scene) {
    upload(scene,scene.uniform());
}
void Render::upload(const Scene& scene,const GpuSceneUniform& uniform) {
    auto& p=*impl_;
    if(p.visibility_recording || p.lighting_recording)throw FormatError("Path Tracing scene cannot upload during screen-pass recording");
    p.previous_history_camera=p.current_history_camera;
    p.previous_history_eye=p.current_history_eye;
    p.previous_history_camera_mask=p.current_history_camera_mask;
    p.current_history_camera_mask=0;p.previous_history_prepared=false;
    p.visibility_ready=p.lighting_ready=false;
    p.source_depth_prepared=false;
    if(p.uploaded_source!=&scene){p.calibration={1.f,1.f,1.f};p.calibration_frames=0;p.calibration_sums={};}
    p.update_calibration();
    auto calibrated=uniform;
    if(const auto* fixed=std::getenv("HALO2_PATH_TRACING_CALIBRATION");fixed && *fixed && std::strcmp(fixed,"0")==0)
        calibrated.calibration={1,1,0,1};
    else calibrated.calibration={p.calibration[0],p.calibration[1],0,p.calibration[2]};
    // z: static world triangles are separated from dynamic objects, so a
    // shadow ray can ask what only objects block (scene.glsl traceBakedBounce).
    calibrated.calibration[2]=scene.static_triangles>0 && scene.static_triangles<=scene.triangles.size()?1.f:0.f;
    p.uploaded_uniform_emitter_scale=calibrated.calibration[3];
    const std::array<std::span<const std::byte>,7> input{
        bytes(scene.nodes),bytes(scene.triangles),bytes(scene.materials),bytes(scene.lights),bytes(scene.pixels),
        std::as_bytes(std::span(&calibrated,1)),bytes(scene.color_guide_pixels)};
    // Validate the complete upload before replacing any resource. Vulkan never
    // receives an empty descriptor, including a scene with no lights or rays.
    for(unsigned i=0;i<input.size();++i) {
        const auto limit=i==5?p.properties.limits.maxUniformBufferRange:p.properties.limits.maxStorageBufferRange;
        if(std::max<std::size_t>(16,input[i].size())>limit)
            throw FormatError("Path Tracing scene bank "+std::to_string(i)+" exceeds maxStorageBufferRange or maxUniformBufferRange");
    }
    p.harvest_timings();
    if(p.context.hardware_rays && (!scene.nodes.empty() || scene.cpu_hierarchy))
        throw FormatError("Path Tracing hardware rays require a scene without a CPU hierarchy");
    p.uploaded=false;
    // Compares every triangle with the previous frame, including whether the
    // static BSP prefix changed; the copies below and the acceleration build
    // use that result.
    const bool distant_changed=p.update_shadow_changes(scene);
    std::array<VkDescriptorBufferInfo,7> infos{};
    std::array<VkWriteDescriptorSet,7> writes{};
    for(unsigned i=0;i<input.size();++i) {
        auto usage=VkBufferUsageFlags(i==5?VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        const auto limit=i==5?p.properties.limits.maxUniformBufferRange:p.properties.limits.maxStorageBufferRange;
        auto& buffer=p.buffers[i==6?7:i];
        const bool triangle_input=i==1 && p.context.hardware_rays;
        if(triangle_input)usage|=VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        const auto previous_buffer=buffer.buffer;
        p.allocate(buffer,std::max<std::size_t>(16,input[i].size()),usage,limit,triangle_input);
        // Hardware rays: unchanged static BSP triangles are already resident.
        // Rewrite only the dynamic suffix (posed objects) every frame.
        if(triangle_input && !p.static_payload_changed && previous_buffer==buffer.buffer &&
           buffer.populated>=std::size_t(scene.static_triangles)*sizeof(GpuTriangle) && !input[i].empty()) {
            const auto skip=std::size_t(scene.static_triangles)*sizeof(GpuTriangle);
            std::memcpy(static_cast<std::byte*>(buffer.mapped)+skip,input[i].data()+skip,input[i].size()-skip);
            buffer.populated=input[i].size();
            infos[i]={buffer.buffer,0,std::max<std::size_t>(16,input[i].size())};
            auto& write=writes[i];write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet=p.descriptor;write.dstBinding=i;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&infos[i];
            continue;
        }
        // Only cached host memory is suitable for a CPU read/compare. Reading
        // an uncached mapped VRAM bank back over PCIe can cost more than
        // uploading the CPU-owned scene again, so use write-only copies there.
        const bool immutable_guide=i==6 && !scene.color_guide_image_keys.empty() &&
            p.uploaded_color_guide_source==&scene && p.uploaded_color_guide_keys==scene.color_guide_image_keys &&
            buffer.populated==input[i].size();
        if(immutable_guide) {
            // The source fingerprint proves this atlas is unchanged. Do not
            // read uncached VRAM or resend megabytes of immutable map data.
        } else if(input[i].empty()) {
            std::memset(buffer.mapped,0,16);
            buffer.populated=16;
        } else {
            if(!(buffer.memory_flags&VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ||
               buffer.populated!=input[i].size() || std::memcmp(buffer.mapped,input[i].data(),input[i].size())!=0)
                std::memcpy(buffer.mapped,input[i].data(),input[i].size());
            buffer.populated=input[i].size();
        }
        infos[i]={buffer.buffer,0,std::max<std::size_t>(16,input[i].size())};
        auto& write=writes[i];write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet=p.descriptor;write.dstBinding=i==6?8:i;write.descriptorCount=1;
        write.descriptorType=i==5?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo=&infos[i];
    }
    vkUpdateDescriptorSets(p.context.device,writes.size(),writes.data(),0,nullptr);
    p.uploaded_color_guide_source=&scene;p.uploaded_color_guide_keys=scene.color_guide_image_keys;
    p.environment_fallback_available=uniform.environment[0]>0 || uniform.environment[1]>0 || uniform.environment[2]>0;
    const auto key=temporal_lighting_key(scene,uniform);
    const auto direct_key=direct_lighting_key(scene);
    if(p.context.hardware_rays)p.prepare_acceleration(scene);
    p.direct_history_reset=p.uploaded_source!=&scene || !p.lighting_key_valid || p.direct_key!=direct_key || distant_changed;
    if(p.uploaded_source!=&scene || !p.lighting_key_valid || p.lighting_key!=key)p.history_reset_pending=true;
    p.uploaded_source=&scene;p.lighting_key=key;p.direct_key=direct_key;p.lighting_key_valid=true;
    if(++p.frame_serial==0)p.history_reset_pending=true;
    p.update_visibility_binding();p.uploaded=true;
}
void Render::record_acceleration(VkCommandBuffer command) {impl_->record_acceleration(command);}
bool Render::hardware_rays()const noexcept {return impl_->context.hardware_rays;}
void Render::prepare_visibility(VkExtent2D extent) {
    prepare_visibility(extent,VkViewport{0,0,float(extent.width),float(extent.height),0,1});
}
void Render::prepare_visibility(VkExtent2D extent,const VkViewport& viewport) {
    const std::array values{viewport.x,viewport.y,viewport.width,viewport.height,viewport.minDepth,viewport.maxDepth};
    if(!std::all_of(values.begin(),values.end(),[](float value){return std::isfinite(value);}) ||
       viewport.width<=0 || viewport.height==0)
        throw FormatError("Path Tracing history requires its actual finite nonempty viewport");
    impl_->prepare_visibility(extent);impl_->visibility_viewport=viewport;
}
void Render::disable_visibility()noexcept {impl_->visibility_ready=impl_->lighting_ready=false;}
void Render::set_draw_cull_mode(VkCullModeFlags mode)noexcept {impl_->draw_cull=mode;}
void Render::begin_visibility(VkCommandBuffer command) {
    auto& p=*impl_;
    if(!p.uploaded || !p.visibility_framebuffer || p.visibility_recording || p.lighting_recording)
        throw FormatError("Path Tracing visibility needs an uploaded scene and prepared target");
    p.snapshot_previous_history(command);
    p.timestamp(command,"before_visibility");
    p.visibility_ready=p.lighting_ready=false;
    VkClearValue clear{};clear.depthStencil={1.f,0};
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass=p.visibility_pass;begin.framebuffer=p.visibility_framebuffer;
    begin.renderArea={{0,0},p.visibility_extent};begin.clearValueCount=1;begin.pClearValues=&clear;
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);p.visibility_recording=true;
}
void Render::bind_visibility(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                            std::span<const std::byte> camera,std::uint32_t material_index)const {
    if(material_index>=0x80000000u)throw FormatError("Path Tracing visibility lost its material identity");
    impl_->bind(command,material,dynamic_bank_offset,camera,material_index,true);
}
void Render::end_visibility(VkCommandBuffer command) {
    auto& p=*impl_;
    if(!p.visibility_recording)throw FormatError("Path Tracing visibility has no recording phase");
    vkCmdEndRenderPass(command);p.visibility_recording=false;
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer=p.buffers[6].buffer;barrier.offset=32;barrier.size=p.lighting_offset()-32;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    VkBufferImageCopy copy{};copy.bufferOffset=32;
    copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};
    copy.imageExtent={p.visibility_extent.width,p.visibility_extent.height,1};
    vkCmdCopyImageToBuffer(command,p.visibility_depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,p.buffers[6].buffer,1,&copy);
    p.timestamp(command,"visibility");
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    p.visibility_ready=true;
}
void Render::begin_lighting(VkCommandBuffer command) {
    auto& p=*impl_;
    if(!p.uploaded || !p.visibility_ready || !p.lighting_framebuffer || p.visibility_recording || p.lighting_recording)
        throw FormatError("Path Tracing HDR lighting requires a completed visibility pass");
    p.lighting_ready=false;
    // The material word is also the atomic ownership token. Clear every
    // record to invalid before a new camera phase reuses this view's storage.
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer=p.buffers[6].buffer;barrier.offset=p.lighting_offset();
    barrier.size=p.visibility_bytes()-barrier.offset;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,1,&barrier,0,nullptr);
    p.lighting_push_valid=false;
    vkCmdFillBuffer(command,barrier.buffer,barrier.offset,
                    p.history_reset_pending?barrier.size:p.lighting_bytes(),0xffffffffu);
    p.history_reset_pending=false;
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass=p.lighting_pass;begin.framebuffer=p.lighting_framebuffer;
    begin.renderArea={{0,0},p.visibility_extent};
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);p.lighting_recording=true;
}
void Render::bind_lighting(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                          std::span<const std::byte> camera,std::uint32_t material_index)const {
    if(material_index>=0x80000000u)throw FormatError("Path Tracing HDR lighting lost its material identity");
    impl_->bind(command,material,dynamic_bank_offset,camera,material_index,false,true);
}
void Render::end_lighting(VkCommandBuffer command) {
    auto& p=*impl_;
    if(!p.lighting_recording)throw FormatError("Path Tracing HDR lighting has no recording phase");
    vkCmdEndRenderPass(command);p.lighting_recording=false;
    p.timestamp(command,"lighting_raster");
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer=p.buffers[6].buffer;barrier.offset=p.lighting_offset();
    barrier.size=p.visibility_bytes()-barrier.offset;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    // Trace every published grid cell with this phase's camera. No lighting
    // draw means no published cell, so the dispatch is skipped.
    if(p.lighting_push_valid) {
        const auto scale=std::max(1u,p.lighting_scale);
        const auto grid_width=(p.visibility_extent.width+scale-1)/scale;
        const auto grid_height=(p.visibility_extent.height+scale-1)/scale;
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,p.trace_pipeline);
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,p.compute_layout,1,1,&p.descriptor,0,nullptr);
        vkCmdPushConstants(command,p.compute_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,camera_bytes,p.lighting_push.data());
        vkCmdDispatch(command,(grid_width+7)/8,(grid_height+7)/8,1);
        p.timestamp(command,"trace");
        // A-trous passes, steps 1,2,4...: the trace wrote packed bank 1; passes
        // alternate banks 1/2 and the last writes the records' lighting lanes.
        struct {std::array<float,4> eye;std::array<std::uint32_t,4> control;} filter{};
        std::memcpy(filter.eye.data(),p.lighting_push.data()+20*sizeof(float),3*sizeof(float));
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,p.denoise_pipeline);
        std::uint32_t source=1;
        for(unsigned pass=0;pass<p.denoise_passes;++pass) {
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,0,nullptr,1,&barrier,0,nullptr);
            const std::uint32_t destination=pass+1==p.denoise_passes?0u:source==1u?2u:1u;
            filter.control={1u<<pass,source,destination,pass};
            vkCmdPushConstants(command,p.compute_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(filter),&filter);
            vkCmdDispatch(command,(grid_width+7)/8,(grid_height+7)/8,1);
            source=destination;
        }
        p.timestamp(command,"denoise");
        // Calibration sums for the world view (the camera-attached weapon has
        // no lightmap). Mode 0 per workgroup, then mode 1 in one workgroup.
        float role{};std::memcpy(&role,p.lighting_push.data()+23*sizeof(float),sizeof(role));
        if(role!=2.f) {
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,0,nullptr,1,&barrier,0,nullptr);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,p.calibrate_pipeline);
            // y: the emitter scale this frame's grid was traced with.
            std::array<std::uint32_t,4> mode{0,std::bit_cast<std::uint32_t>(p.uploaded_uniform_emitter_scale),0,0};
            vkCmdPushConstants(command,p.compute_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(mode),mode.data());
            vkCmdDispatch(command,(grid_width+7)/8,(grid_height+7)/8,1);
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,0,nullptr,1,&barrier,0,nullptr);
            mode[0]=1;
            vkCmdPushConstants(command,p.compute_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(mode),mode.data());
            vkCmdDispatch(command,1,1,1);
            // The CPU reads the total after this frame's fence.
            VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
            host.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,
                                 0,1,&host,0,nullptr,0,nullptr);
        }
    }
    barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,0,nullptr,1,&barrier,0,nullptr);
    p.lighting_ready=true;
}
bool Render::bind_glow_layer(VkCommandBuffer command,VkDescriptorSet layer,std::uint32_t dynamic_bank_offset,
                             std::span<const std::byte> camera,std::uint32_t material_index,unsigned ordinal)const {
    if(camera.size()!=camera_bytes || material_index>=0x80000000u)
        throw FormatError("Path Tracing glow layer lost its camera or surface identity");
    // The layer programs live in the surface's root record (aliases share it).
    const auto* scene=impl_->uploaded_source;
    if(!scene || material_index>=scene->materials.size())return false;
    const auto root=material_index<scene->material_source_roots.size()?scene->material_source_roots[material_index]:material_index;
    if(root>=scene->material_layer_programs.size() || scene->material_layer_programs[root]==no_layer_programs)return false;
    const auto program=scene->materials[root].texture[0]+scene->material_layer_programs[root]+ordinal*layer_program_words;
    if(std::size_t(program)+layer_program_words>scene->pixels.size())return false;
    impl_->bind(command,layer,dynamic_bank_offset,camera,material_index,false,false,nullptr,program);
    return true;
}
void Render::bind(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                  std::span<const std::byte> camera)const {
    impl_->bind(command,material,dynamic_bank_offset,camera,~0u,false);
}
void Render::bind(VkCommandBuffer command,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                  std::span<const std::byte> camera,std::uint32_t material_index)const {
    if(camera.size()!=camera_bytes || material_index>=0x80000000u)
        throw FormatError("Path Tracing draw lost its camera or material identity");
    // The RT camera does not consume viewport.w for projection. Reserve its
    // bits for a per-draw material override, leaving the source camera bank
    // and the ordinary renderer's push constants unchanged.
    impl_->bind(command,material,dynamic_bank_offset,camera,material_index,false);
}
} // namespace halo2::path_tracing
