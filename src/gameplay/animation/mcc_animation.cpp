#include "halo2/mcc_animation.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <tuple>

namespace halo2 {
namespace {
constexpr std::size_t canonical_header=0xc4,pc_header=0xac,maximum=128ull*1024*1024;
struct PointerNode {unsigned first,count;};
struct PointerField {unsigned offset,stride,child;bool data;};
#include "gameplay/animation/mcc_animation_layout.inc"
constexpr unsigned clip_node=[] {
    const auto root=pointer_nodes[0];
    for(unsigned n=0;n<root.count;++n) {
        const auto field=pointer_fields[root.first+n];
        if(field.offset==0x2c)return field.child;
    }
    return std::numeric_limits<unsigned>::max();
}();
static_assert(clip_node<pointer_nodes.size());
void word(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value,unsigned size=4) {
    Bytes(bytes).require(at,size);
    for(unsigned i=0;i<size;++i)bytes[at+i]=std::byte(value>>(i*8));
}
std::size_t aligned(std::size_t value) {return (value+15)&~std::size_t(15);}
}
std::vector<std::byte> normalize_mcc_animation_graph(
    std::span<const std::byte> payload,std::uint32_t source_address,
    std::uint32_t target_address,std::uint32_t file_offset,
    const std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>& resident_read) {
    const Bytes source(payload);source.require(0,pc_header);
    if(payload.size()>maximum || std::uint64_t(source_address)+payload.size()>0x100000000ull)
        throw FormatError("Excessive MCC animation allocation");
    auto read=[&](std::uint32_t address,std::size_t bytes) {
        if(address>=source_address && std::uint64_t(address)-source_address<=payload.size() &&
           bytes<=payload.size()-(std::uint64_t(address)-source_address))
            return source.slice(std::size_t(address-source_address),bytes);
        if(resident_read)return resident_read(address,bytes);
        throw FormatError("MCC animation pointer escapes resident metadata");
    };
    const auto count=source.u32(0x2c);
    if(count>32767)throw FormatError("MCC animation count exceeds signed resource-group indices");
    std::vector<std::byte> result(canonical_header);
    std::copy_n(payload.begin(),pc_header,result.begin());
    std::map<std::tuple<std::uint32_t,unsigned,unsigned,unsigned,bool>,std::size_t> copied;
    auto allocate=[&](std::span<const std::byte> bytes,bool resource) {
        // Original resource I/O rounds animation reads to 512 bytes. Own the
        // entire read, including padding, so it cannot expose a following
        // clip or another metadata allocation. These are file offsets; the
        // graph's virtual address need not have the same sector alignment.
        const auto at=resource?((std::size_t(file_offset)+result.size()+511)&~std::size_t(511))-file_offset:
            aligned(result.size());
        const auto size=resource?(bytes.size()+511)&~std::size_t(511):bytes.size();
        const auto extent=aligned(at+size);
        if(extent>maximum || std::uint64_t(target_address)+extent>0x100000000ull ||
           std::uint64_t(file_offset)+extent>0x40000000ull)
            throw FormatError("Canonical animation allocation exceeds address/resource limits");
        result.resize(extent);std::copy(bytes.begin(),bytes.end(),result.begin()+std::ptrdiff_t(at));return at;
    };
    auto relocate=[&](auto&& self,unsigned node,std::uint32_t address,std::size_t output,unsigned depth)->void {
        if(depth>32 || copied.size()>1000000)throw FormatError("Excessive MCC animation pointer graph");
        const auto layout=pointer_nodes.at(node);
        for(unsigned i=0;i<layout.count;++i) {
            const auto field=pointer_fields.at(layout.first+i);
            if(std::uint64_t(address)+field.offset>0xffffffffu)throw FormatError("MCC animation pointer overflow");
            const Bytes descriptor(read(address+field.offset,8));
            const auto length=descriptor.u32(0),pointer=descriptor.u32(4);
            if(!length) {word(result,output+field.offset+4,0);continue;}
            if(!field.data && length>65536)throw FormatError("Excessive MCC animation block count");
            const auto bytes=std::size_t(length)*field.stride;
            if(bytes>maximum || std::uint64_t(pointer)+bytes>0x100000000ull)throw FormatError("Excessive MCC animation pointer extent");
            const auto key=std::tuple(pointer,length,field.stride,field.child,field.data);
            auto found=copied.find(key);
            if(found==copied.end()) {
                const auto at=allocate(read(pointer,bytes),field.data);found=copied.emplace(key,at).first;
                if(!field.data)for(unsigned n=0;n<length;++n)
                    self(self,field.child,pointer+n*field.stride,at+std::size_t(n)*field.stride,depth+1);
            }
            word(result,output+field.offset+4,target_address+std::uint32_t(found->second));
        }
    };
    // MCC deduplicates banks across resident graphs. Deep-copy each referenced
    // bank through the exact resident owner, so the normalized vanilla graph
    // never retains an alias into a separate donor allocation.
    relocate(relocate,0,source_address,0,0);
    const auto clips=count?std::size_t(Bytes(result).u32(0x30)-target_address):0;
    const auto canonical_clips=aligned(result.size());
    const auto groups=aligned(canonical_clips+std::size_t(count)*0x6c);
    const auto extent=aligned(groups+std::size_t(count)*20);
    if(extent>maximum || std::uint64_t(target_address)+extent>0x100000000ull ||
       std::uint64_t(file_offset)+extent>0x40000000ull)
        throw FormatError("Canonical animation allocation exceeds address/resource limits");
    result.resize(extent);
    word(result,0x30,count?target_address+std::uint32_t(canonical_clips):0);
    word(result,0xac,count);word(result,0xb0,count?target_address+std::uint32_t(groups):0);
    // The original graph consumers take resource groups separately from the
    // clip table. Each normalized clip has one immutable, cache-owned group;
    // the ordinary resource loader then sees a local file-offset descriptor.
    for(unsigned index=0;index<count;++index) {
        const auto copy=clips+std::size_t(index)*0x60;
        const auto clip=canonical_clips+std::size_t(index)*0x6c;
        std::copy_n(result.begin()+copy,0x20,result.begin()+clip);
        // Actual TU1.5 1E5AC3/1E5AD7 selects the resource-group path with
        // internal bit4. The converter moved MCC's inline data into the
        // declared group below, so retaining the donor storage flag would
        // make 1E5AE7 return the deliberately empty inline pointer at +38.
        // This selects the owned representation; it does not mark it loaded.
        result[clip+0x16]|=std::byte{0x10};
        word(result,clip+0x20,0xffffffffu);
        word(result,clip+0x24,index);word(result,clip+0x28,0);
        word(result,clip+0x2c,0xffffu,2);
        std::copy_n(result.begin()+copy+0x20,6,result.begin()+clip+0x2e);
        word(result,clip+0x34,0xffffffffu);
        std::copy_n(result.begin()+copy+0x30,0x30,result.begin()+clip+0x3c);
        const auto resource_size=Bytes(result).u32(copy+0x28);
        const auto group=groups+std::size_t(index)*20;
        word(result,group,0xffffffffu);word(result,group+4,resource_size);
        // 126190/12630C tests +C against NONE before allocating the genuine
        // runtime tracking record. Zero names a real slot and would reuse an
        // allocation that this fresh resource owner has never requested.
        word(result,group+0xc,0xffffffffu);
        if(resource_size) {
            const auto pointer=Bytes(result).u32(copy+0x2c);
            if(pointer<target_address)throw FormatError("Canonical animation resource precedes its graph");
            const auto resource=std::size_t(pointer-target_address);Bytes(result).require(resource,resource_size);
            word(result,group+8,file_offset+std::uint32_t(resource));
        }
    }
    return result;
}
std::vector<MccNormalizedPointerField> normalized_mcc_animation_pointer_fields(
    std::span<const std::byte> payload,std::uint32_t source_address) {
    const Bytes data(payload);data.require(0,canonical_header);
    if(!source_address || payload.size()>maximum ||
       std::uint64_t(source_address)+payload.size()>0x100000000ull)
        throw FormatError("Invalid normalized MCC animation allocation");
    const auto offset=[&](std::uint32_t pointer,std::size_t bytes) {
        if(!pointer || pointer<source_address ||
           std::uint64_t(pointer)-source_address>payload.size() ||
           bytes>payload.size()-(std::uint64_t(pointer)-source_address))
            throw FormatError("Normalized MCC animation pointer escapes its owned graph");
        return std::size_t(pointer-source_address);
    };
    std::vector<MccNormalizedPointerField> result;
    std::set<std::pair<std::size_t,unsigned>> visited;
    const auto descriptor=[&](std::size_t at,unsigned stride,bool raw) {
        data.require(at,8);const auto length=data.u32(at),pointer=data.u32(at+4);
        if(!length) {
            if(pointer)throw FormatError("Normalized MCC animation empty descriptor has an address");
            return std::pair<std::size_t,std::uint32_t>{0,0};
        }
        const auto bytes=std::uint64_t(length)*stride;
        if((!raw && length>65536) || bytes>maximum)
            throw FormatError("Excessive normalized MCC animation descriptor extent");
        const auto bank=offset(pointer,std::size_t(bytes));
        result.push_back({std::uint32_t(at+4),pointer,std::uint32_t(bytes)});
        return std::pair{bank,length};
    };
    const auto visit=[&](auto&& self,unsigned node,std::size_t at,unsigned depth)->void {
        if(depth>32 || visited.size()>1000000)
            throw FormatError("Excessive normalized MCC animation pointer graph");
        if(!visited.emplace(at,node).second)return;
        const auto layout=pointer_nodes.at(node);
        for(unsigned i=0;i<layout.count;++i) {
            const auto field=pointer_fields.at(layout.first+i);
            // normalize_mcc_animation_graph moves resource data references
            // out of the canonical clip and shifts its event descriptors.
            if(node==clip_node && field.offset==0x28 && field.data)continue;
            const auto shifted=field.offset+(node==clip_node?0xcu:0u);
            const auto stride=node==0&&field.offset==0x2c?0x6cu:field.stride;
            const auto [bank,length]=descriptor(at+shifted,stride,field.data);
            if(!field.data && pointer_nodes.at(field.child).count)for(std::size_t n=0;n<length;++n)
                self(self,field.child,bank+n*stride,depth+1);
        }
    };
    visit(visit,0,0,0);
    // The normalizer adds one cache-owned group per canonical clip. group+8
    // is a local resource file offset, not a virtual metadata address.
    if(data.u32(0xac)!=data.u32(0x2c))
        throw FormatError("Normalized MCC animation resource groups differ from its clips");
    (void)descriptor(0xac,20,false);
    return result;
}
}
