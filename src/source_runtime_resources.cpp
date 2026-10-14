#include "halo2/source_runtime_resources.hpp"
#include "halo2/cache.hpp"
#include "halo2/cache_file_snapshot.hpp"
#include <openssl/evp.h>
#include <fstream>
namespace halo2 {
namespace {
SourceRuntimeResources::Digest digest(std::span<const std::byte> bytes) {
    SourceRuntimeResources::Digest result;unsigned count{};
    if(EVP_Digest(bytes.data(),bytes.size(),reinterpret_cast<unsigned char*>(result.data()),&count,EVP_sha256(),nullptr)!=1 || count!=result.size())
        throw FormatError("Cannot authenticate source resource binding");
    return result;
}
}
struct SourceRuntimeResources::Impl {
    const Cache &cache;
    struct File {
        std::shared_ptr<CacheFileSnapshot> snapshot;
        std::vector<std::byte> storage;
        std::span<const std::byte> bytes()const{return snapshot?snapshot->bytes():std::span<const std::byte>(storage);}
    };
    std::array<std::optional<File>,3> shared;
    Identity identity;
    explicit Impl(const Cache &cache):cache(cache) {
        if(cache.is_vista() || cache.is_mcc())throw FormatError("Source resource bindings require the admitted Xbox cache");
        identity.resident=digest(cache.source_runtime_cache_bytes());
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
            identity.shared[i]=digest(file.bytes());shared[i]=std::move(file);
        }
    }
};
SourceRuntimeResources::SourceRuntimeResources(const Cache &cache):impl_(std::make_unique<Impl>(cache)){}
SourceRuntimeResources::~SourceRuntimeResources()=default;
const SourceRuntimeResources::Identity &SourceRuntimeResources::identity() const noexcept{return impl_->identity;}
std::vector<std::byte> SourceRuntimeResources::load(std::uint32_t encoded,std::uint32_t bytes) const {
    if(!bytes || bytes>128u*1024u*1024u)throw FormatError("Invalid original source resource size");
    const auto location=encoded>>30,offset=encoded&0x3fffffffu;
    auto backing=impl_->cache.source_runtime_cache_bytes();
    if(location) {
        if(!impl_->shared[location-1])throw FormatError("Required bound source resource file is unavailable");
        backing=impl_->shared[location-1]->bytes();
    }
    const auto source=Bytes(backing).slice(offset,bytes);return {source.begin(),source.end()};
}
SourceRuntimeCacheAnimationImage SourceRuntimeResources::animation(std::uint32_t descriptor) const {
    return impl_->cache.load_source_runtime_animation_image(descriptor,[this](auto p,auto bytes){return load(p,bytes);});
}
}
