#pragma once
#include "halo2/model.hpp"
#include "halo2/model_skin_gpu_admission.hpp"
#include <cstdint>
#include <exception>
#include <vulkan/vulkan.h>

namespace halo2 {
// Exact flat destination mapping, independent of section/filter/group order.
// palette is a row-bank index, not a byte offset. Bones retain their authored
// indices, including zero interior influences checked by the CPU owner.
struct ModelSkinGpuRecord {
    std::uint32_t destination{},palette{};
    std::array<std::uint32_t,4> bones{};
    std::array<float,4> weights{};
    std::uint32_t post_identity_placement{}; // Only0/1; runs even for all-zero weights.
    std::uint32_t palette_count{}; // This owner's complete bank, including compounds.
};
static_assert(sizeof(ModelSkinGpuRecord)==48 && offsetof(ModelSkinGpuRecord,weights)==24 &&
              offsetof(ModelSkinGpuRecord,post_identity_placement)==40 &&
              offsetof(ModelSkinGpuRecord,palette_count)==44);
enum class ModelSkinGpuPrepareRefusalReason {
    device_quota,input_allocation,input_mapping
};
// Only a validated prepare job may refuse these resources. This exception
// itself allocates no storage, so host allocation exhaustion can reach the
// caller's existing destination allocation. Other Vulkan/programming failures
// remain hard errors. result() is VK_SUCCESS for quota/std::bad_alloc.
class ModelSkinGpuPrepareRefused final : public std::exception {
    ModelSkinGpuPrepareRefusalReason reason_;
    VkResult result_;
public:
    explicit ModelSkinGpuPrepareRefused(ModelSkinGpuPrepareRefusalReason reason,
        VkResult result=VK_SUCCESS)noexcept :reason_(reason),result_(result){}
    ModelSkinGpuPrepareRefusalReason reason()const noexcept{return reason_;}
    VkResult result()const noexcept{return result_;}
    const char *what()const noexcept override {
        switch(reason_) {
        case ModelSkinGpuPrepareRefusalReason::device_quota:return "model skin GPU device quota refused";
        case ModelSkinGpuPrepareRefusalReason::input_allocation:return "model skin GPU input allocation refused";
        case ModelSkinGpuPrepareRefusalReason::input_mapping:return "model skin GPU input host mapping refused";
        }
        return "unknown model skin GPU resource refusal";
    }
};
struct ModelSkinGpuPrepareStats {
    // Actual Vulkan calls (including failed calls), completed byte writes and
    // individual descriptor writes for this prepare; synthetic faults are not
    // API calls. Reset on entry, including empty jobs; failed work stays visible.
    std::uint64_t input_allocations{},input_maps{},input_unmaps{};
    // Packed/uploaded GPU payload sizes; record_pack_bytes counts eleven-word
    // serialized records, not the separate complete host-record shadow copy.
    std::uint64_t record_pack_bytes{},record_upload_bytes{},palette_upload_bytes{};
    std::uint64_t descriptor_writes{},record_cache_hits{};
};
// Renderer-owned compute service. prepare and destruction require its previous
// GPU submission to have retired. Callers must qualify the complete local
// vertex/binding/palette domain and host effects before eliminating CPU skins.
// It records into the caller's graphics+compute command buffer;
// source writers, Vulkan calls, submission and fence ownership stay on that caller.
class ModelSkinGpu {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    static bool supported(VkPhysicalDevice,std::uint32_t enabled_api);
    ModelSkinGpu(VkDevice,VkPhysicalDevice,std::uint32_t enabled_api,const std::filesystem::path& shader);
    ~ModelSkinGpu();
    ModelSkinGpu(const ModelSkinGpu&)=delete;
    ModelSkinGpu& operator=(const ModelSkinGpu&)=delete;
    // Resets vertices() before any work, validates the entire structural job
    // before resource refusal, and publishes a dispatch only on success.
    // Preserves the caller's complete FP environment and errno on every path.
    // A nonzero destination_generation is a caller-owned lease epoch which
    // must change whenever the destination VkBuffer is replaced, even if its
    // numeric handle/range is reused. Zero leaves identity unqualified and
    // refreshes descriptors on every success. Previous submission retirement
    // remains mandatory for every prepare, including an exact resident hit.
    void prepare(std::span<const ModelSkinGpuRecord>,std::span<const std::array<float,12>> palette,
                 VkBuffer destination,std::size_t destination_vertices,
                 std::uint64_t destination_generation=0);
    void record(VkCommandBuffer)const;
    std::size_t vertices()const;
    const ModelSkinGpuPrepareStats& prepare_stats()const;
};
}
