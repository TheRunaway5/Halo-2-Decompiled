#pragma once
// Host mapping operations for the source reserve/commit owner. Source tokens,
// extents and commitment bits remain in SourceRuntimeMemory at source granularity.
#if defined(__APPLE__)
#include "macos/source_runtime_host_pages.hpp"
#else
#include <cerrno>
#include <cstddef>
#include <stdexcept>
#include <system_error>
#include <vector>
#if defined(_WIN32)
#include "platform/windows_api.hpp"
#endif
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace halo2::source_runtime_detail::host_pages {
inline std::size_t token_page_bytes() {
#if defined(_WIN32)
    SYSTEM_INFO info;::GetSystemInfo(&info);return info.dwPageSize;
#elif defined(__linux__)
    const auto bytes=::sysconf(_SC_PAGE_SIZE);
    if(bytes<=0)throw std::runtime_error("Host virtual-memory page size unavailable");
    return std::size_t(bytes);
#else
    throw std::runtime_error("Required original virtual-memory platform adapter unavailable");
#endif
}
inline void validate_virtual_pages() {
    if(token_page_bytes()!=0x1000)
        throw std::runtime_error("Reached source reservations require a4096-byte host page");
}
inline void *reserve(std::size_t bytes) {
#if defined(_WIN32)
    auto* host=::VirtualAlloc(nullptr,bytes,MEM_RESERVE,PAGE_NOACCESS);
    if(!host)windows_detail::failure("Host source reservation");return host;
#elif defined(__linux__)
    auto *host=::mmap(nullptr,bytes,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(host==MAP_FAILED)throw std::system_error(errno,std::generic_category(),"Host source reservation");
    return host;
#else
    throw std::runtime_error("Required original reserve/commit platform adapter unavailable");
#endif
}
inline int release(void *host,std::size_t bytes) noexcept {
#if defined(_WIN32)
    (void)bytes;return ::VirtualFree(host,0,MEM_RELEASE)?0:-1;
#elif defined(__linux__)
    return ::munmap(host,bytes);
#else
    return -1;
#endif
}
inline void make_writable(void *host,std::size_t offset,std::size_t bytes) {
#if defined(_WIN32)
    if(!::VirtualAlloc(static_cast<std::byte*>(host)+offset,bytes,MEM_COMMIT,PAGE_READWRITE))
        windows_detail::failure("Host source page commitment");
#elif defined(__linux__)
    if(::mprotect(static_cast<std::byte*>(host)+offset,bytes,PROT_READ|PROT_WRITE))
        throw std::system_error(errno,std::generic_category(),"Host source page commitment");
#else
    throw std::runtime_error("Required original reserve/commit platform adapter unavailable");
#endif
}
inline void rollback_commitment(void *host,std::size_t offset,std::size_t bytes,
                                const std::vector<bool> &committed) noexcept {
#if defined(_WIN32)
    for(std::size_t done=0;done<bytes;done+=0x1000)
        if(!committed[(offset+done)/0x1000])
            (void)::VirtualFree(static_cast<std::byte*>(host)+offset+done,0x1000,MEM_DECOMMIT);
#elif defined(__linux__)
    for(std::size_t done=0;done<bytes;done+=0x1000)
        if(!committed[(offset+done)/0x1000])
            (void)::mprotect(static_cast<std::byte*>(host)+offset+done,0x1000,PROT_NONE);
#endif
}
}
#endif
