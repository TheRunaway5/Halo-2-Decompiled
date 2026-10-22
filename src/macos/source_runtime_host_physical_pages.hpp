#pragma once
#include "runtime/source_runtime_host_pages.hpp"
#include <algorithm>
#include <cstdint>
#include <set>
#include <span>
#include <vector>

namespace halo2::source_runtime_detail::physical_pages {
inline std::size_t native_alignment() {return host_pages::native_page_bytes();}
inline void *allocate(std::size_t bytes) {
    auto *host=::mmap(nullptr,host_pages::mapped_bytes(bytes),PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANON,-1,0);
    if(host==MAP_FAILED)throw std::system_error(errno,std::generic_category(),"Darwin source physical allocation");
    return host;
}
inline void release(void *host,std::size_t bytes) noexcept { (void)host_pages::release(host,bytes); }

template<class ReadOnly>
bool native_granule_read_only(std::size_t offset,std::size_t source_bytes,const ReadOnly &read_only) {
    const auto end=std::min(offset+native_alignment(),source_bytes);
    for(auto source=offset;source<end;source+=0x1000)
        if(!read_only(source/0x1000))return false;
    return true;
}
inline void protect_range(std::span<std::byte> host,std::uint32_t source_base,
                          std::size_t begin,std::size_t end,bool,
                          const std::set<std::uint32_t> &before,
                          const std::set<std::uint32_t> &after) {
    const auto granule=native_alignment();
    struct Change {std::size_t offset;bool before,after;};
    std::vector<Change> changes;
    // Prepare every change before touching native permissions. The source
    // owner publishes its new4KiB protection rows only after this succeeds.
    for(auto offset=begin&~(granule-1);offset<end;offset+=granule) {
        const auto previous=native_granule_read_only(offset,host.size(),[&](auto index) {
            return before.contains(source_base+std::uint32_t(index*0x1000));
        });
        const auto next=native_granule_read_only(offset,host.size(),[&](auto index) {
            return after.contains(source_base+std::uint32_t(index*0x1000));
        });
        if(previous!=next)changes.push_back({offset,previous,next});
    }
    std::size_t changed=0;
    for(;changed<changes.size();++changed) {
        const auto &change=changes[changed];
        if(::mprotect(host.data()+change.offset,granule,change.after?PROT_READ:PROT_READ|PROT_WRITE)) {
            const auto error=errno;
            while(changed) {
                const auto &undo=changes[--changed];
                (void)::mprotect(host.data()+undo.offset,granule,undo.before?PROT_READ:PROT_READ|PROT_WRITE);
            }
            throw std::system_error(error,std::generic_category(),"Darwin source physical protection");
        }
    }
    // A mixed native granule remains writable for its writable source pages.
    // Existing SourceRuntimeMemory write guards enforce every4KiB page,
    // including native aliases. Mutable spans cannot cross a protection change.
}
inline void reset(std::span<std::byte> host) {
    if(::mprotect(host.data(),host_pages::mapped_bytes(host.size()),PROT_READ|PROT_WRITE))
        throw std::system_error(errno,std::generic_category(),"Reset staged Darwin physical protection");
}
inline void restore_protection(std::span<std::byte> host,std::uint32_t,
                                const std::vector<std::uint8_t> &read_only) {
    const auto granule=native_alignment();
    for(std::size_t offset=0;offset<host.size();offset+=granule)
        if(native_granule_read_only(offset,host.size(),[&](auto index){return read_only[index]!=0;}))
            if(::mprotect(host.data()+offset,granule,PROT_READ))
                throw std::system_error(errno,std::generic_category(),"Stage Darwin source physical protection");
}
}
