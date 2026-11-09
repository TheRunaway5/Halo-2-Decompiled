#include "halo2/source_runtime_resources.hpp"
#include "halo2/source_runtime_resource_preparation.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include "halo2/cache.hpp"
#include "halo2/cache_file_snapshot.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/mcc_model_node_map.hpp"
#include "halo2/mcc_sound_geometry.hpp"
#include "halo2/mcc_original_sound_runtime_delivery_tu15.hpp"
#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include "halo2/mcc_sound_dat_resources_tu15.hpp"
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
    bool strict_pair{};
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
    if(!state->resources&&state->strict_pair)
        throw FormatError("Paired resource preparation was already consumed");
    auto resources=std::move(state->resources);
    if(!state->metadata&&!state->strict_pair)registry.entries.erase(at);
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
std::unique_ptr<SourceRuntimeMccPreparation> prepare_mcc_metadata(
    const Cache& cache,const MccOriginalSoundRuntimeInputTu15* original) {
    constexpr std::uint32_t base=0x90000000u,budget=0x60000000u;
    std::optional<MccSoundRuntimePlan> sound;
    const std::function<void(MccSoundRuntimePlan&&)> capture=[&](MccSoundRuntimePlan&& value){sound.emplace(std::move(value));};
    auto delivery=original?MccRuntimeCacheDelivery::plan(cache,base,budget,capture,*original):
        MccRuntimeCacheDelivery::plan(cache,base,budget,capture);
    if(!sound)throw FormatError("MCC metadata preparation lost its sound owner");
    auto timing=build_mcc_sound_timing_reference(cache,*sound,
        [&](auto address,auto bytes){return delivery.runtime_address(address,bytes);});
    if(original) {
        if(!sound->original||delivery.original_sound_owner()!=sound->original)
            throw FormatError("Original preparation lost its exact canonical sound owner");
        sound->original->require_plan(*sound);
        std::size_t expected{};
        for(const auto& binding:sound->original->bindings()) {
            std::size_t count{};for(const auto& pitch:binding.bank->metadata().pitch_ranges)count+=pitch.permutations.size();
            if(!count||std::ranges::count(timing.bindings(),binding.sound,&MccSoundTimingBinding::sound)!=std::ptrdiff_t(count)||
                std::ranges::count(timing.stored_references(),binding.sound,&MccSoundTimingStoredReference::sound)!=1||
                std::ranges::count(timing.permutation_references(),binding.sound,&MccSoundTimingPermutationReference::sound)!=std::ptrdiff_t(count))
                throw FormatError("Original sound preparation lacks its complete joined timing inputs");
            expected+=count;
        }
        if(!expected)throw FormatError("Original sound preparation has no complete native timing routes");
    }
    return std::make_unique<SourceRuntimeMccPreparation>(
        SourceRuntimeMccPreparation{base,budget,std::move(delivery),std::move(*sound),std::move(timing)});
}
}
struct SourceRuntimeResources::Impl {
    const Cache &cache;
    struct Inputs {
        struct File {
            std::shared_ptr<CacheFileSnapshot> snapshot;
            std::vector<std::byte> storage;
            std::span<const std::byte> bytes()const{return storage.empty()&&snapshot?snapshot->bytes():std::span<const std::byte>(storage);}
        };
        std::array<std::optional<File>,3> shared;
        Identity identity;
        std::vector<MccModelNodeMap> model_node_maps;
        std::vector<MccSoundGeometryResource> sound_geometry;
        std::shared_ptr<const MccOriginalSoundRuntimeDeliveryTu15> original_sound;
        std::shared_ptr<const MccSoundDatResourcesTu15> sound_dat;
    };
    std::shared_ptr<const Inputs> inputs;
    DescriptorAddress descriptor_address;
    std::unique_ptr<SourceRuntimeMccPreparation> metadata;
    bool owns_metadata{},shared_inputs{};
    enum class SharedInputs { Reader,PreparedWorld };
    Impl(const Impl &source,SharedInputs use):cache(source.cache),inputs(source.inputs),
        shared_inputs(use==SharedInputs::Reader) {}
    explicit Impl(const Cache &cache,DescriptorAddress address,const SourceRuntimeMccPreparation* preparation=nullptr)
        :cache(cache),descriptor_address(std::move(address)) {
        auto retained=std::make_shared<Inputs>();
        auto &shared=retained->shared;
        auto &identity=retained->identity;
        auto &model_node_maps=retained->model_node_maps;
        auto &sound_geometry=retained->sound_geometry;
        auto &original_sound=retained->original_sound;
        auto &sound_dat=retained->sound_dat;
        original_sound=preparation?preparation->sound.original:nullptr;
        if(cache.is_vista())throw FormatError("Native source resources require Xbox or normalized MCC cache ownership");
        if(original_sound&&!cache.is_mcc())throw FormatError("Original sound replacement resources require their canonical MCC preparation");
        if(preparation)sound_dat=MccSoundDatResourcesTu15::prepare(cache,*preparation);
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
            Inputs::File file;file.snapshot=CacheFileSnapshot::try_open(path);
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
        if(sound_dat)identity.resident=sound_dat->bind_resource_identity(identity.resident);
        if(original_sound)identity.resident=original_sound->bind_resource_identity(identity.resident);
        inputs=std::move(retained);
    }
};
SourceRuntimeResources::SourceRuntimeResources(const Cache &cache):SourceRuntimeResources(cache,{}){}
SourceRuntimeResources::SourceRuntimeResources(const Cache &cache,DescriptorAddress address) {
    if(auto prepared=take_prepared_resources(cache)) {
        impl_=std::move(prepared->impl_);
        impl_->descriptor_address=std::move(address);
    } else if(cache.is_mcc()&&!cache.scenario_tag().is_null()) {
        std::optional<MccOriginalSoundRuntimeInputTu15> input;
        if(const auto catalog=cache.original_sound_catalog())input=catalog->input(cache);
        auto metadata=prepare_mcc_metadata(cache,input?&*input:nullptr);
        impl_=std::make_unique<Impl>(cache,std::move(address),metadata.get());
        require_sound_plan(metadata->sound);
        impl_->metadata=std::move(metadata);impl_->owns_metadata=true;
    } else impl_=std::make_unique<Impl>(cache,std::move(address));
}
SourceRuntimeResources::~SourceRuntimeResources()=default;
SourceRuntimeResources::SourceRuntimeResources(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
std::shared_ptr<const SourceRuntimeResources> SourceRuntimeResources::share_inputs(const Cache& expected)const {
    if(&expected!=&impl_->cache)throw FormatError("Shared source resources belong to another Cache");
    return std::shared_ptr<const SourceRuntimeResources>(new SourceRuntimeResources(std::make_unique<Impl>(*impl_,Impl::SharedInputs::Reader)));
}
SourceRuntimeResources::SourceRuntimeResources(const Cache& cache,DescriptorAddress address,const SourceRuntimeMccPreparation& preparation)
    :impl_(std::make_unique<Impl>(cache,std::move(address),&preparation)){}
struct SourceRuntimeResourcePreparation::Impl {
    std::shared_ptr<PreparedResourceState> state;
    static void require_available(const Cache& cache) {
        auto& registry=prepared_resources();std::lock_guard lock(registry.mutex);
        const auto at=registry.entries.find(&cache);
        if(at!=registry.entries.end() && !at->second.expired())
            throw FormatError("Cache already has an active resource preparation");
    }
    void publish() const {
        auto& registry=prepared_resources();std::lock_guard lock(registry.mutex);
        auto& entry=registry.entries[state->cache.get()];
        if(!entry.expired())throw FormatError("Cache already has an active resource preparation");
        entry=state;
    }
    explicit Impl(std::shared_ptr<const Cache> cache,const MccOriginalSoundRuntimeInputTu15* original) {
        if(!cache)throw FormatError("Resource preparation requires its immutable Cache owner");
        if(original&&!cache->is_mcc())throw FormatError("Original sound preparation requires a canonical MCC Cache owner");
        require_available(*cache);
        state=std::make_shared<PreparedResourceState>();state->cache=std::move(cache);state->strict_pair=original!=nullptr;
        // Only the explicit match-scoped preparation publishes a reusable
        // owner. Ordinary resource construction retains its original lifetime.
        struct BuildScope {
            bool previous=building_preparation;
            BuildScope(){building_preparation=true;}
            ~BuildScope(){building_preparation=previous;}
        } scope;
        if(state->cache->is_mcc()) {
            state->metadata=prepare_mcc_metadata(*state->cache,original);
            // Both owners are complete before the registry entry exists.
            // This private construction cannot consume an unrelated entry.
            state->resources.reset(new SourceRuntimeResources(*state->cache,{},*state->metadata));
            state->resources->require_sound_plan(state->metadata->sound);
        } else state->resources=std::make_unique<SourceRuntimeResources>(*state->cache);
        publish();
    }
    Impl(std::shared_ptr<const Cache> cache,const SourceRuntimeResources& source) {
        const auto &retained=*source.impl_;
        if(!cache||cache.get()!=&retained.cache||!retained.metadata||!retained.owns_metadata)
            throw FormatError("Prepared native World requires unpublished metadata from its exact Cache");
        require_available(*cache);
        state=std::make_shared<PreparedResourceState>();state->cache=std::move(cache);state->strict_pair=true;
        state->metadata=std::make_unique<SourceRuntimeMccPreparation>(*retained.metadata);
        state->resources.reset(new SourceRuntimeResources(std::make_unique<SourceRuntimeResources::Impl>(
            retained,SourceRuntimeResources::Impl::SharedInputs::PreparedWorld)));
        // This copies a previously joined immutable metadata plan. Recheck its
        // sound routes before the actual World publishes its own native bank.
        if(state->metadata->delivery.original_sound_owner()!=state->metadata->sound.original)
            throw FormatError("Prepared native World lost its retained original sound owner");
        state->resources->require_sound_plan(state->metadata->sound);
        publish();
    }
    ~Impl() {
        auto& registry=prepared_resources();std::lock_guard lock(registry.mutex);
        const auto at=registry.entries.find(state->cache.get());
        if(at!=registry.entries.end() && at->second.lock()==state)registry.entries.erase(at);
    }
};
SourceRuntimeResourcePreparation::SourceRuntimeResourcePreparation(std::shared_ptr<const Cache> cache) {
    if(cache && cache->original_sound_catalog()) {
        const auto input=cache->original_sound_catalog()->input(*cache);
        impl_=std::make_unique<Impl>(std::move(cache),&input);
    } else impl_=std::make_unique<Impl>(std::move(cache),nullptr);
}
SourceRuntimeResourcePreparation::SourceRuntimeResourcePreparation(std::shared_ptr<const Cache> cache,const MccOriginalSoundRuntimeInputTu15& original)
    :impl_(std::make_unique<Impl>(std::move(cache),&original)) {}
SourceRuntimeResourcePreparation::SourceRuntimeResourcePreparation(std::shared_ptr<const Cache> cache,const PreparedOriginalSoundCatalogBindingTu15& catalog){
    if(!cache)throw FormatError("Original catalog preparation requires its immutable Cache owner");
    const auto input=catalog.input(*cache);impl_=std::make_unique<Impl>(std::move(cache),&input);
}
SourceRuntimeResourcePreparation::SourceRuntimeResourcePreparation(std::shared_ptr<const Cache> cache,const SourceRuntimeResources& source)
    :impl_(std::make_unique<Impl>(std::move(cache),source)) {}
SourceRuntimeResourcePreparation::~SourceRuntimeResourcePreparation()=default;
std::unique_ptr<SourceRuntimeMccPreparation> take_prepared_mcc_world_metadata(
    const Cache& cache,std::uint32_t base,std::uint32_t budget) {
    auto& registry=prepared_resources();std::lock_guard lock(registry.mutex);
    const auto at=registry.entries.find(&cache);
    if(at==registry.entries.end())return {};
    const auto state=at->second.lock();
    if(!state){registry.entries.erase(at);return {};}
    if(!state->metadata&&state->strict_pair)
        throw FormatError("Paired World metadata preparation was already consumed");
    if(!state->metadata || state->metadata->base!=base || state->metadata->budget!=budget)return {};
    auto metadata=std::move(state->metadata);
    if(!state->resources&&!state->strict_pair)registry.entries.erase(at);
    return metadata;
}
const SourceRuntimeResources::Identity &SourceRuntimeResources::identity() const noexcept{return impl_->inputs->identity;}
void SourceRuntimeResources::require_sound_plan(const MccSoundRuntimePlan& plan)const{
    if(plan.original!=impl_->inputs->original_sound)throw FormatError("Native sound metadata and immutable resource identities have different original owners");
    if(impl_->inputs->sound_dat)impl_->inputs->sound_dat->require_routes(plan);
    if(impl_->inputs->original_sound)impl_->inputs->original_sound->require_plan(plan);
}
void SourceRuntimeResources::require_sound_plan(const Cache& expected,const MccSoundRuntimePlan& plan)const {
    if(&expected!=&impl_->cache)throw FormatError("Sound resource publication belongs to another Cache");
    require_sound_plan(plan);
}
std::shared_ptr<const MccOriginalSoundRuntimeDeliveryTu15> SourceRuntimeResources::original_sound_owner()const{return impl_->inputs->original_sound;}
std::shared_ptr<const MccSoundDatResourcesTu15> SourceRuntimeResources::sound_dat_owner()const{return impl_->inputs->sound_dat;}
std::unique_ptr<SourceRuntimeMccPreparation> SourceRuntimeResources::take_mcc_world_metadata(std::uint32_t base,std::uint32_t budget) {
    if(impl_->shared_inputs)throw FormatError("Shared resource inputs do not own World metadata publication");
    if(!impl_->owns_metadata)return {};
    if(!impl_->metadata)throw FormatError("Direct World metadata was already consumed");
    if(impl_->metadata->base!=base||impl_->metadata->budget!=budget)
        throw FormatError("Direct World metadata has a different canonical layout");
    return std::move(impl_->metadata);
}
std::vector<std::byte> SourceRuntimeResources::sound_cache(const SourceSoundCacheReadTu15& request)const{
    if(impl_->inputs->original_sound)if(auto original=impl_->inputs->original_sound->read_cache_resource(request))return std::move(*original);
    if(impl_->inputs->sound_dat)return impl_->inputs->sound_dat->read(request);
    if(impl_->cache.is_mcc())throw FormatError("MCC sound read has no admitted DAT resource owner");
    return load(request.encoded_offset,request.bytes);
}
std::vector<std::byte> SourceRuntimeResources::load(std::uint32_t encoded,std::uint32_t bytes) const {
    if(!bytes || bytes>128u*1024u*1024u)throw FormatError("Invalid original source resource size");
    const auto location=encoded>>30,offset=encoded&0x3fffffffu;
    if(!location)for(const auto& resource:impl_->inputs->sound_geometry)if(encoded==resource.encoded_offset) {
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
        if(!impl_->inputs->shared[location-1])throw FormatError("Required bound source resource file is unavailable");
        backing=impl_->inputs->shared[location-1]->bytes();
    }
    const auto source=Bytes(backing).slice(offset,bytes);
    std::vector<std::byte> delivered(source.begin(),source.end());
    if(impl_->cache.is_mcc())project_mcc_model_node_map_range(impl_->inputs->model_node_maps,encoded,backing,delivered);
    return delivered;
}
SourceRuntimeCacheAnimationImage SourceRuntimeResources::animation(std::uint32_t descriptor) const {
    if(impl_->descriptor_address)descriptor=impl_->descriptor_address(descriptor,20);
    return impl_->cache.load_source_runtime_animation_image(descriptor,[this](auto p,auto bytes){return load(p,bytes);});
}
}
