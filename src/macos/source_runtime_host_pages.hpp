#pragma once
// Reservation/raw-storage backing only. SourceRuntimeMemory keeps source
// commitment at4KiB independently of these rounded native mappings. This is
// not a subpage physical-protection adapter: live physical backing has a
// different contract and must not use rounded mprotect ranges blindly.
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>

namespace halo2::source_runtime_detail::host_pages {
inline std::size_t native_page_bytes() {
    static const std::size_t bytes=[] {
        const auto page=::sysconf(_SC_PAGESIZE);
        if(page<0x1000 || (page&(page-1)) || page%0x1000)
            throw std::runtime_error("Darwin host pages cannot contain complete4096-byte source pages");
        return std::size_t(page);
    }();
    return bytes;
}
inline constexpr std::size_t token_page_bytes() { return 0x1000; }
inline void validate_virtual_pages() { (void)native_page_bytes(); }
inline std::size_t mapped_bytes(std::size_t bytes) {
    const auto page=native_page_bytes();
    if(!bytes || bytes>std::numeric_limits<std::size_t>::max()-(page-1))
        throw std::invalid_argument("Invalid Darwin source mapping extent");
    return (bytes+page-1)&~(page-1);
}
inline void *reserve(std::size_t bytes) {
    auto *host=::mmap(nullptr,mapped_bytes(bytes),PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
    if(host==MAP_FAILED)throw std::system_error(errno,std::generic_category(),"Darwin source reservation");
    return host;
}
inline int release(void *host,std::size_t bytes) noexcept {
    // reserve() validated/cached the granule before this mapping was published.
    const auto page=native_page_bytes();
    return ::munmap(host,(bytes+page-1)&~(page-1));
}
inline void make_writable(void *host,std::size_t offset,std::size_t bytes) {
    const auto page=native_page_bytes();
    const auto first=offset&~(page-1),last=(offset+bytes+page-1)&~(page-1);
    if(::mprotect(static_cast<std::byte*>(host)+first,last-first,PROT_READ|PROT_WRITE))
        throw std::system_error(errno,std::generic_category(),"Darwin source page commitment");
}
inline void rollback_commitment(void *host,std::size_t offset,std::size_t bytes,
                                const std::vector<bool> &committed) noexcept {
    const auto page=native_page_bytes();
    const auto last=(offset+bytes+page-1)&~(page-1);
    for(auto at=offset&~(page-1);at<last;at+=page) {
        const auto first_source=at/0x1000;
        const auto last_source=std::min((at+page)/0x1000,committed.size());
        bool retained=false;
        for(auto source=first_source;source<last_source;++source)retained|=committed[source];
        // A neighboring committed source page still needs this same native
        // granule. SourceRuntimeMemory independently rejects all logical holes.
        (void)::mprotect(static_cast<std::byte*>(host)+at,page,
                         retained?PROT_READ|PROT_WRITE:PROT_NONE);
    }
}
}
