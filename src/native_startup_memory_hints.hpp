#pragma once
#include "halo2/source_runtime_cache_tags.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#if defined(__linux__)
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace halo2::detail {
// Optional native residency advice at completed startup boundaries. Retain
// backing contents, ignore unsupported advice, and preserve the caller's errno.
// Only complete pages of the main image's executable, nonwritable file ranges
// are selected; retained guest banks and writable native arenas are untouched.
inline void release_native_startup_code_pages() noexcept {
#if defined(__linux__) && defined(MADV_PAGEOUT)
    const auto saved_errno=errno;
    const auto size=::sysconf(_SC_PAGESIZE);
    if(size>0) {
        const auto page=std::uintptr_t(size);
        (void)::dl_iterate_phdr([](dl_phdr_info *info,std::size_t,void *context) {
            if(info->dlpi_name && *info->dlpi_name)return 0;
            const auto page=*static_cast<const std::uintptr_t*>(context);
            constexpr auto limit=std::numeric_limits<std::uintptr_t>::max();
            for(ElfW(Half) i=0;i<info->dlpi_phnum;++i) {
                const auto &segment=info->dlpi_phdr[i];
                if(segment.p_type!=PT_LOAD || !(segment.p_flags&PF_X) ||
                    (segment.p_flags&PF_W) || !segment.p_filesz)continue;
                if(segment.p_vaddr>limit-info->dlpi_addr)continue;
                const auto first=std::uintptr_t(info->dlpi_addr+segment.p_vaddr);
                if(segment.p_filesz>limit-first || first>limit-(page-1))continue;
                const auto start=(first+page-1)/page*page;
                const auto end=(first+segment.p_filesz)/page*page;
                if(end>start)(void)::madvise(reinterpret_cast<void*>(start),end-start,MADV_PAGEOUT);
            }
            return 1;
        },const_cast<std::uintptr_t*>(&page));
    }
    errno=saved_errno;
#endif
}

// The synchronous classic tag binder has already copied these immutable Cache
// source ranges into independently owned bank storage before this hint runs.
inline void release_delivered_cache_source_pages(const SourceRuntimeCacheTagView &tags) noexcept {
#if defined(__linux__) && defined(MADV_PAGEOUT)
    const auto saved_errno=errno;
    const auto size=::sysconf(_SC_PAGESIZE);
    if(size>0) {
        const auto page=std::uintptr_t(size);
        constexpr auto limit=std::numeric_limits<std::uintptr_t>::max();
        for(const auto &region:tags.regions) {
            const auto first=reinterpret_cast<std::uintptr_t>(region.bytes.data());
            if(region.bytes.size()>limit-first || first>limit-(page-1))continue;
            const auto start=(first+page-1)/page*page;
            const auto end=(first+region.bytes.size())/page*page;
            if(end>start)(void)::madvise(reinterpret_cast<void*>(start),end-start,MADV_PAGEOUT);
        }
    }
    errno=saved_errno;
#else
    (void)tags;
#endif
}
} // namespace halo2::detail
