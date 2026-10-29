#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace halo2 {
// Resource admissions performed by a temporary native spatial publication
// outlive that publication. Their pools, descriptors and payload form one
// owner; restoring an older subset after later render admissions corrupts it.
class SourcePresentationResourceRegionsTu15 final {
    struct Range {std::uint32_t first;std::uint64_t end;};
    std::vector<Range> ranges_;
public:
    explicit SourcePresentationResourceRegionsTu15(const SourceRuntimeMemory &memory) {
        const auto word=[&](std::uint32_t at,unsigned bytes=4) {
            const auto data=memory.view(at,bytes);std::uint32_t value{};
            for(unsigned n=0;n<bytes;++n)value|=std::to_integer<std::uint32_t>(data[n])<<(n*8u);
            return value;
        };
        const auto add=[&](std::uint32_t at,std::uint64_t bytes) {
            if(!at || !bytes || bytes>std::uint64_t(UINT32_MAX)+1u-at)
                throw FormatError("Presented resource owner has an invalid native extent");
            (void)memory.view(at,std::size_t(bytes));
            ranges_.push_back({at,std::uint64_t(at)+bytes});
        };
        const auto pool=[&](std::uint32_t at,std::uint32_t capacity,std::uint32_t stride) {
            if(!at || word(at+0x20u)!=capacity || word(at+0x24u)!=stride ||
               word(at+0x38u)>capacity || word(at+0x3cu)>word(at+0x38u))
                throw FormatError("Presented resource pool differs from its native constructor");
            add(at,0x4cu);add(word(at+0x44u),std::uint64_t(capacity)*stride);
            add(word(at+0x48u),((capacity+31u)/32u)*4u);
        };
        // Whole125C90/125D40 owners, with the original capacities/strides.
        if(word(0x51df20u,1)!=1u)
            throw FormatError("Presented resource cache precedes native construction");
        add(0x51df20u,2u);add(0x4f0418u,0x18u);
        pool(word(0x4f0418u),0x200u,0x10u);
        pool(word(0x4f041cu),0x80u,0xcu);
        const auto allocator=word(0x4f0424u),bytes=word(0x4f0428u);
        if(!allocator || allocator>UINT32_MAX-0x30fcu ||
           word(allocator+0x64u)!=allocator+0x70u || word(allocator+0x34u)!=11u ||
           (bytes!=0x300000u && bytes!=0x400000u) || word(0x4f042cu)!=(bytes>>11u))
            throw FormatError("Presented resource allocator differs from125C90/125D40");
        add(allocator,0x30fcu);pool(allocator+0x70u,0x200u,0x18u);
        add(word(0x4f0420u),bytes);
        // The actual published native tag directory bounds every descriptor
        // owner. Do not infer metadata ownership from a broad address range.
        const auto index=word(0x556d90u),table=word(0x4f0414u);
        if(!index || !table || word(index+8u)!=table)
            throw FormatError("Presented resources lost their native tag-directory owner");
        const auto count=word(index+24u);
        if(!count || count>65536u || std::uint64_t(table)+std::uint64_t(count)*16u>0x100000000ull)
            throw FormatError("Presented resource tag directory exceeds its native bounds");
        (void)memory.view(table,std::size_t(count)*16u);
        for(std::uint32_t n=0;n<count;++n) {
            const auto row=table+n*16u;
            if(word(row)!=0x6a6d6164u)continue;
            const auto datum=word(row+4u),root=word(row+8u),size=word(row+12u);
            if(!root)continue; // An unresident graph owns no runtime descriptor.
            if(datum==UINT32_MAX || (datum&65535u)!=n || size<0xb4u ||
               std::uint64_t(root)+size>0x100000000ull)
                throw FormatError("Presented animation graph lost its full native tag owner");
            (void)memory.view(root,size);
            const auto resources=word(root+0xacu),first=word(root+0xb0u);
            if(!resources)continue;
            const auto extent=std::uint64_t(resources)*20u;
            if(resources>65536u || first<root || std::uint64_t(first)+extent>std::uint64_t(root)+size)
                throw FormatError("Presented animation descriptors escape their native graph owner");
            add(first,extent);
        }
        std::sort(ranges_.begin(),ranges_.end(),[](const auto &a,const auto &b){return a.first<b.first;});
        std::size_t used{};
        for(const auto &range:ranges_) {
            if(used && range.first<=ranges_[used-1u].end)
                ranges_[used-1u].end=std::max(ranges_[used-1u].end,range.end);
            else ranges_[used++]=range;
        }
        ranges_.resize(used);
    }
    bool persistent(std::uint32_t at,std::size_t bytes)const {
        const auto end=std::uint64_t(at)+bytes;
        if(end>0x100000000ull)throw FormatError("Presented spatial write wraps its native owner");
        if(!bytes)return false;
        const auto after=std::upper_bound(ranges_.begin(),ranges_.end(),at,
            [](auto address,const auto &range){return address<range.first;});
        if(after!=ranges_.begin() && at<std::prev(after)->end) {
            if(end<=std::prev(after)->end)return true;
            throw FormatError("Presented spatial write crosses its persistent resource owner");
        }
        if(after!=ranges_.end() && end>after->first)
            throw FormatError("Presented spatial write crosses its persistent resource owner");
        return false;
    }
};
} // namespace halo2
