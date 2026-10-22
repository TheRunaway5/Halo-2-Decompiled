#include "halo2/mcc_sound_geometry.hpp"
#include "halo2/cache.hpp"
#include "audio/mcc_sound_geometry_apply.hpp"
#include <algorithm>
#include <fstream>
#include <limits>

namespace halo2 {
void mcc_sound_geometry_detail::apply(MccSoundRuntimePlan& out,
    std::span<const MccSoundGeometryResource> resources) {
    for(const auto& resource:resources) {
        const auto owner=std::ranges::find(out.gestalts,resource.gestalt,&MccSoundRuntimeGestalt::owner);
        if(owner==out.gestalts.end() || resource.index>=owner->count[10])
            throw FormatError("MCC facial resource lost its merged native gestalt interval");
        const auto row=std::uint64_t(out.blocks[10].address)+
            (std::uint64_t(owner->first[10])+resource.index)*0x2cu;
        if(row<out.address || row-out.address+0x2cu>out.bytes.size() ||
           Bytes(out.bytes).u32(std::size_t(row-out.address)+8u)!=resource.source_file_offset ||
           Bytes(out.bytes).u32(std::size_t(row-out.address)+12u)!=resource.declared_bytes)
            throw FormatError("MCC facial resource differs from its exact native metadata preimage");
        const auto at=std::size_t(row-out.address)+8u;
        for(unsigned n=0;n<4;++n)out.bytes[at+n]=std::byte(resource.encoded_offset>>(n*8));
    }
}
std::vector<MccSoundGeometryResource> plan_mcc_sound_geometry_resources(const Cache& cache) {
    if(!cache.is_mcc())throw FormatError("Sound geometry projection requires an MCC cache owner");
    std::vector<MccSoundGeometryResource> result;
    std::ifstream english;
    std::uint64_t file_bytes{};
    // The token namespace starts beyond every normalized cache allocation,
    // including generated jmad resources. It cannot alias an original file byte.
    auto next=(std::uint64_t(cache.source_runtime_cache_bytes().size())+511u)&~std::uint64_t(511);
    for(const auto& tag:cache.tags())if(tag.group=="ugh!"&&tag.address&&tag.size) {
        const Bytes root(cache.read_tag_bytes(tag.address,0x78));
        const auto count=root.u32(0x70),address=root.u32(0x74);
        if(count>0x100000u)throw FormatError("MCC sound extra-info count exceeds its typed bank");
        if(!count)continue;
        const Bytes rows(cache.read_tag_bytes(address,std::size_t(count)*0x14c));
        for(unsigned n=0;n<count;++n) {
            const auto at=std::size_t(n)*0x14c+8u;
            const auto offset=rows.u32(at),size=rows.u32(at+4);
            if(!size)continue;
            const auto section=rows.u32(at+8),resource=rows.u32(at+12);
            if(size<12u || size>128u*1024u*1024u || section>size-12u || resource!=size-section-12u ||
               rows.u32(at+24)!=tag.datum)
                throw FormatError("MCC English facial geometry differs from its actual gestalt owner");
            const auto padded=(std::uint64_t(size)+511u)&~std::uint64_t(511);
            if(next+padded>0x40000000ull)
                throw FormatError("MCC facial geometry exceeds its owned native file-token namespace");
            if(!english.is_open()) {
                english.open(cache.shared_directory()/"sounds_en.dat",std::ios::binary|std::ios::ate);
                if(!english || english.tellg()<0)throw FormatError("MCC English facial resource container unavailable");
                file_bytes=std::uint64_t(english.tellg());
            }
            if(offset>file_bytes || size>file_bytes-offset)
                throw FormatError("MCC facial resource escapes its actual English DAT container");
            MccSoundGeometryResource out{tag.handle(),n,address+n*0x14cu+8u,offset,
                std::uint32_t(next),size,std::vector<std::byte>(std::size_t(padded))};
            english.seekg(offset);
            if(!english.read(reinterpret_cast<char*>(out.sector.data()),size))
                throw FormatError("MCC English facial resource is truncated");
            const Bytes payload(out.sector);
            if(payload.u32(0)!=0x626c6b68u || payload.u32(4)!=resource ||
               payload.u32(size-4)!=0x626c6b66u)
                throw FormatError("MCC facial geometry block header/footer differs from its declared resource");
            // Padding is newly owned storage for the native sector I/O adapter;
            // it never consumes the following MCC DAT block or changes payload.
            result.push_back(std::move(out));next+=padded;
        }
    }
    return result;
}
}
