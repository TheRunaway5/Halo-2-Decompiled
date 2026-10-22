#include "halo2/source_runtime_resources.hpp"
#include "halo2/source_runtime_resource_preparation.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include "halo2/cache.hpp"
#include "halo2/cache_file_snapshot.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/mcc_model_node_map.hpp"
#include "halo2/mcc_sound_geometry.hpp"
#include <openssl/evp.h>
#include <fstream>
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <map>
#include <mutex>
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif
namespace halo2 {
namespace {
struct PreparedResourceState {
    std::shared_ptr<const Cache> cache;
    std::unique_ptr<SourceRuntimeResources> resources;
    std::unique_ptr<SourceRuntimeMccPreparation> metadata;
};
struct PreparedResourceRegistry {
    std::mutex mutex;
    std::map<const Cache*,std::weak_ptr<PreparedResourceState>> entries;
};
PreparedResourceRegistry& prepared_resources() {
    static PreparedResourceRegistry value;return value;
}
thread_local bool building_preparation{};
std::unique_ptr<SourceRuntimeResources> take_prepared_resources(const Cache& cache) {
    if(building_preparation)return {};
    auto& registry=prepared_resources();std::lock_guard lock(registry.mutex);
    const auto at=registry.entries.find(&cache);
    if(at==registry.entries.end())return {};
    const auto state=at->second.lock();
    if(!state){registry.entries.erase(at);return {};}
    auto resources=std::move(state->resources);
    if(!state->metadata)registry.entries.erase(at);
    return resources;
}
std::size_t native_page_size() noexcept {
#if defined(__linux__) && defined(MADV_COLD)
    const auto saved_errno=errno;
    const auto size=::sysconf(_SC_PAGESIZE);
    errno=saved_errno;
    return size>0?std::size_t(size):0;
#else
    return 0;
#endif
}
void cool_hashed_pages(std::span<const std::byte> bytes,std::size_t page_size) noexcept {
#if defined(__linux__) && defined(MADV_COLD)
    if(!page_size || bytes.empty())return;
    const auto address=reinterpret_cast<std::uintptr_t>(bytes.data());
    const auto skip=(page_size-address%page_size)%page_size;
    if(skip>=bytes.size())return;
    const auto size=(bytes.size()-skip)/page_size*page_size;
    if(!size)return;
    // Authentication is on the world-loading critical path. Mark consumed
    // pages cold so the kernel can reclaim them under pressure; forcing
    // PAGEOUT here synchronously writes private decoded banks to swap just
    // before resource consumers read them back. COLD retains every byte,
    // including private COW edits, without forcing that round trip.
    const auto saved_errno=errno;
    (void)::madvise(const_cast<std::byte*>(bytes.data()+skip),size,MADV_COLD);
    errno=saved_errno;
#else
    (void)bytes;(void)page_size;
#endif
}
SourceRuntimeResources::Digest digest(std::span<const std::byte> bytes) {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    if(!context || EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)!=1)
        throw FormatError("Cannot authenticate source resource binding");
    const auto page_size=native_page_size();
    if(bytes.empty() && EVP_DigestUpdate(context.get(),bytes.data(),0)!=1)
        throw FormatError("Cannot authenticate source resource binding");
    // Authenticate every admitted byte in order. The cache and shared
    // snapshots retain ownership; completed pages may be reclaimed as needed.
    while(!bytes.empty()) {
        const auto window=bytes.first(std::min<std::size_t>(bytes.size(),4u*1024u*1024u));
        if(EVP_DigestUpdate(context.get(),window.data(),window.size())!=1)
            throw FormatError("Cannot authenticate source resource binding");
        cool_hashed_pages(window,page_size);
        bytes=bytes.subspan(window.size());
    }
    SourceRuntimeResources::Digest result;unsigned count{};
    if(EVP_DigestFinal_ex(context.get(),reinterpret_cast<unsigned char*>(result.data()),&count)!=1 || count!=result.size())
        throw FormatError("Cannot authenticate source resource binding");
    return result;
}
}
struct SourceRuntimeResources::Impl {
    const Cache &cache;
    struct File {
        std::shared_ptr<CacheFileSnapshot> snapshot;
        std::vector<std::byte> storage;
        std::span<const std::byte> bytes()const{return storage.empty()&&snapshot?snapshot->bytes():std::span<const std::byte>(storage);}
    };
    std::array<std::optional<File>,3> shared;
    Identity identity;
    DescriptorAddress descriptor_address;
    std::vector<MccModelNodeMap> model_node_maps;
    std::vector<MccSoundGeometryResource> sound_geometry;
    explicit Impl(const Cache &cache,DescriptorAddress address):cache(cache),descriptor_address(std::move(address)) {
        if(cache.is_vista())throw FormatError("Native source resources require Xbox or normalized MCC cache ownership");
        if(cache.is_mcc())model_node_maps=plan_mcc_model_node_maps(cache);
        if(cache.is_mcc())sound_geometry=plan_mcc_sound_geometry_resources(cache);
        identity.resident=digest(cache.source_runtime_cache_bytes());
        if(!sound_geometry.empty()) {
            // Bind every retained generated resource and its exact source
            // identity. Checkpoints must reject changed English DAT payloads.
            std::vector<std::byte> binding(identity.resident.begin(),identity.resident.end());
            constexpr std::string_view domain="MCC English facial geometry v1";
            for(auto c:domain)binding.push_back(std::byte(c));
            for(const auto& resource:sound_geometry) {
                for(auto value:{resource.gestalt.value,resource.index,resource.source_descriptor,
                    resource.source_file_offset,resource.encoded_offset,resource.declared_bytes})
                    for(unsigned n=0;n<4;++n)binding.push_back(std::byte(value>>(8*n)));
                const auto hash=digest(resource.sector);binding.insert(binding.end(),hash.begin(),hash.end());
            }
            identity.resident=digest(binding);
        }
        const std::array names{"mainmenu.map","shared.map","single_player_shared.map"};
        for(unsigned i=0;i<names.size();++i) {
            const auto path=cache.shared_directory()/names[i];
            if(!std::filesystem::exists(path))continue;
            File file;file.snapshot=CacheFileSnapshot::try_open(path);
            if(!file.snapshot) {
                std::ifstream input(path,std::ios::binary|std::ios::ate);
                if(!input || input.tellg()<0)throw FormatError("Source resource binding file unavailable");
                file.storage.resize(std::size_t(input.tellg()));input.seekg(0);
                if(!input.read(reinterpret_cast<char*>(file.storage.data()),file.storage.size()))
                    throw FormatError("Truncated source resource binding file");
            }
            if(cache.is_mcc())file.storage=decode_mcc_cache(file.bytes());
            identity.shared[i]=digest(file.bytes());shared[i]=std::move(file);
        }
    }
};
SourceRuntimeResources::SourceRuntimeResources(const Cache &cache):SourceRuntimeResources(cache,{}){}
SourceRuntimeResources::SourceRuntimeResources(const Cache &cache,DescriptorAddress address) {
    if(auto prepared=take_prepared_resources(cache)) {
        impl_=std::move(prepared->impl_);
        impl_->descriptor_address=std::move(address);
    } else impl_=std::make_unique<Impl>(cache,std::move(address));
}
SourceRuntimeResources::~SourceRuntimeResources()=default;
struct SourceRuntimeResourcePreparation::Impl {
    std::shared_ptr<PreparedResourceState> state;
    explicit Impl(std::shared_ptr<const Cache> cache) {
        if(!cache)throw FormatError("Resource preparation requires its immutable Cache owner");
        auto& registry=prepared_resources();
        {
            std::lock_guard lock(registry.mutex);
            const auto at=registry.entries.find(cache.get());
            if(at!=registry.entries.end() && !at->second.expired())
                throw FormatError("Cache already has an active resource preparation");
        }
        state=std::make_shared<PreparedResourceState>();state->cache=std::move(cache);
        // Only the explicit match-scoped preparation publishes a reusable
        // owner. Ordinary resource construction retains its original lifetime.
        struct BuildScope {
            bool previous=building_preparation;
            BuildScope(){building_preparation=true;}
            ~BuildScope(){building_preparation=previous;}
        } scope;
        state->resources=std::make_unique<SourceRuntimeResources>(*state->cache);
        if(state->cache->is_mcc()) {
            constexpr std::uint32_t base=0x90000000u,budget=0x60000000u;
            std::optional<MccSoundRuntimePlan> sound;
            auto delivery=MccRuntimeCacheDelivery::plan(*state->cache,base,budget,
                [&](MccSoundRuntimePlan&& value){sound.emplace(std::move(value));});
            if(!sound)throw FormatError("MCC metadata preparation lost its sound owner");
            auto timing=build_mcc_sound_timing_reference(*state->cache,*sound,
                [&](auto address,auto bytes){return delivery.runtime_address(address,bytes);});
            state->metadata=std::make_unique<SourceRuntimeMccPreparation>(
                SourceRuntimeMccPreparation{base,budget,std::move(delivery),std::move(*sound),std::move(timing)});
        }
        std::lock_guard lock(registry.mutex);
        auto& entry=registry.entries[state->cache.get()];
        if(!entry.expired())throw FormatError("Cache already has an active resource preparation");
        entry=state;
    }
    ~Impl() {
        auto& registry=prepared_resources();std::lock_guard lock(registry.mutex);
        const auto at=registry.entries.find(state->cache.get());
        if(at!=registry.entries.end() && at->second.lock()==state)registry.entries.erase(at);
    }
};
SourceRuntimeResourcePreparation::SourceRuntimeResourcePreparation(std::shared_ptr<const Cache> cache)
    :impl_(std::make_unique<Impl>(std::move(cache))) {}
SourceRuntimeResourcePreparation::~SourceRuntimeResourcePreparation()=default;
std::unique_ptr<SourceRuntimeMccPreparation> take_prepared_mcc_world_metadata(
    const Cache& cache,std::uint32_t base,std::uint32_t budget) {
    auto& registry=prepared_resources();std::lock_guard lock(registry.mutex);
    const auto at=registry.entries.find(&cache);
    if(at==registry.entries.end())return {};
    const auto state=at->second.lock();
    if(!state){registry.entries.erase(at);return {};}
    if(!state->metadata || state->metadata->base!=base || state->metadata->budget!=budget)return {};
    auto metadata=std::move(state->metadata);
    if(!state->resources)registry.entries.erase(at);
    return metadata;
}
const SourceRuntimeResources::Identity &SourceRuntimeResources::identity() const noexcept{return impl_->identity;}
std::vector<std::byte> SourceRuntimeResources::load(std::uint32_t encoded,std::uint32_t bytes) const {
    if(!bytes || bytes>128u*1024u*1024u)throw FormatError("Invalid original source resource size");
    const auto location=encoded>>30,offset=encoded&0x3fffffffu;
    if(!location)for(const auto& resource:impl_->sound_geometry)if(encoded==resource.encoded_offset) {
        if(bytes!=resource.declared_bytes && bytes!=resource.sector.size())
            throw FormatError("MCC facial read differs from its exact owned payload/sector extent");
        return {resource.sector.begin(),resource.sector.begin()+bytes};
    }
    auto backing=impl_->cache.source_runtime_cache_bytes();
    if(impl_->cache.is_mcc() && !location) {
        const auto logical_end=Bytes(backing).u32(8);
        if(offset>logical_end || bytes>logical_end-offset)
            // Generated animation resources live beyond the original logical
            // file. Cache admits only an exact normalized jmad group with
            // its verified sector padding, never arbitrary appended data.
            return impl_->cache.load_source_runtime_resource(encoded,bytes);
        backing=backing.first(logical_end);
    }
    if(location) {
        if(!impl_->shared[location-1])throw FormatError("Required bound source resource file is unavailable");
        backing=impl_->shared[location-1]->bytes();
    }
    const auto source=Bytes(backing).slice(offset,bytes);
    std::vector<std::byte> delivered(source.begin(),source.end());
    if(impl_->cache.is_mcc())project_mcc_model_node_map_range(impl_->model_node_maps,encoded,backing,delivered);
    return delivered;
}
SourceRuntimeCacheAnimationImage SourceRuntimeResources::animation(std::uint32_t descriptor) const {
    if(impl_->descriptor_address)descriptor=impl_->descriptor_address(descriptor,20);
    return impl_->cache.load_source_runtime_animation_image(descriptor,[this](auto p,auto bytes){return load(p,bytes);});
}
}
