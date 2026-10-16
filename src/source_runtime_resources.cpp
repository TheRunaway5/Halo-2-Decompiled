#include "halo2/source_runtime_resources.hpp"
#include "halo2/cache.hpp"
#include "halo2/cache_file_snapshot.hpp"
#include <openssl/evp.h>
#include <fstream>
#include <algorithm>
#include <cerrno>
#include <cstdint>
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif
namespace halo2 {
namespace {
std::size_t native_page_size() noexcept {
#if defined(__linux__) && defined(MADV_PAGEOUT)
    const auto saved_errno=errno;
    const auto size=::sysconf(_SC_PAGESIZE);
    errno=saved_errno;
    return size>0?std::size_t(size):0;
#else
    return 0;
#endif
}
void release_hashed_pages(std::span<const std::byte> bytes,std::size_t page_size) noexcept {
#if defined(__linux__) && defined(MADV_PAGEOUT)
    if(!page_size || bytes.empty())return;
    const auto address=reinterpret_cast<std::uintptr_t>(bytes.data());
    const auto skip=(page_size-address%page_size)%page_size;
    if(skip>=bytes.size())return;
    const auto size=(bytes.size()-skip)/page_size*page_size;
    if(!size)return;
    // Reclaim only complete pages already consumed by the digest. PAGEOUT
    // preserves private COW edits and is optional when backing cannot reclaim.
    const auto saved_errno=errno;
    (void)::madvise(const_cast<std::byte*>(bytes.data()+skip),size,MADV_PAGEOUT);
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
    // Authenticate every admitted byte in order without retaining file-sized
    // resident working sets. The cache and shared snapshots retain ownership.
    while(!bytes.empty()) {
        const auto window=bytes.first(std::min<std::size_t>(bytes.size(),65536));
        if(EVP_DigestUpdate(context.get(),window.data(),window.size())!=1)
            throw FormatError("Cannot authenticate source resource binding");
        release_hashed_pages(window,page_size);
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
