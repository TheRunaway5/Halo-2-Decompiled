#pragma once
#if defined(__APPLE__)
#include "macos/source_runtime_host_physical_pages.hpp"
#else
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <set>
#include <span>
#include <stdexcept>
#include <system_error>
#include <vector>
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace halo2::source_runtime_detail::physical_pages {
inline std::size_t native_alignment() {
#if defined(__linux__)
    const auto bytes=::sysconf(_SC_PAGE_SIZE);
    if(bytes!=0x1000)throw std::runtime_error("Reached physical storage requires4096-byte host pages");
    return std::size_t(bytes);
#else
    throw std::runtime_error("Required original physical-memory platform adapter unavailable");
#endif
}
inline void *allocate(std::size_t bytes) {
#if defined(__linux__)
    auto *host=::mmap(nullptr,bytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(host==MAP_FAILED)throw std::system_error(errno,std::generic_category(),"Host source physical allocation");
    return host;
#else
    throw std::runtime_error("Required original physical-memory platform adapter unavailable");
#endif
}
inline void release(void *host,std::size_t bytes) noexcept {
#if defined(__linux__)
    (void)::munmap(host,bytes);
#endif
}
inline void protect_range(std::span<std::byte> host,std::uint32_t,std::size_t begin,
                          std::size_t end,bool read_only,
                          const std::set<std::uint32_t> &,const std::set<std::uint32_t> &) {
#if defined(__linux__)
    if(::mprotect(host.data()+begin,end-begin,read_only?PROT_READ:PROT_READ|PROT_WRITE))
        throw std::system_error(errno,std::generic_category(),"Host source physical protection");
#else
    throw std::runtime_error("Required original physical-memory platform adapter unavailable");
#endif
}
inline void reset(std::span<std::byte> host) {
#if defined(__linux__)
    if(::mprotect(host.data(),host.size(),PROT_READ|PROT_WRITE))
        throw std::system_error(errno,std::generic_category(),"Reset staged source physical protection");
#else
    throw std::runtime_error("Required original physical-memory platform adapter unavailable");
#endif
}
inline void restore_protection(std::span<std::byte> host,std::uint32_t,
                                const std::vector<std::uint8_t> &read_only) {
#if defined(__linux__)
    for(std::size_t index=0;index<read_only.size();) {
        if(!read_only[index]){++index;continue;}
        const auto first=index;
        while(index<read_only.size() && read_only[index])++index;
        if(::mprotect(host.data()+first*0x1000,(index-first)*0x1000,PROT_READ))
            throw std::system_error(errno,std::generic_category(),"Stage native source physical protection");
    }
#else
    throw std::runtime_error("Required original physical-memory platform adapter unavailable");
#endif
}
}
#endif
