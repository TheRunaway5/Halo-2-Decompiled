#include "halo2/cache.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
std::optional<BitmapLowDetailResource> Cache::load_bitmap_low_detail_resource(
    std::uint32_t datum,unsigned index) const {
    if(datum==0xffffffffu)return {};
    // MCC's 168-byte records do not contain Xbox's separate immutable
    // software low-detail header. Its authored mip partitions are read by
    // load_mcc_bitmap_resource, never reinterpreted as the Xbox descriptor.
    if(mcc_)return {};
    if(vista_) {
        const auto [donor,original]=vista_texture_donor(datum);
        return donor->load_bitmap_low_detail_resource(original,index);
    }
    const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::bitmap});
    if(!tag || !tag->address || tag->size<0x4c)
        throw FormatError("missing/truncated low-detail bitmap tag");
    const Bytes data(cache_bytes());
    const auto base=metadata_offset(tag->address,tag->size);
    const auto count=data.u32(base+0x44);
    if(count>65536)throw FormatError("low-detail bitmap count exceeds source bank");
    if(index>=count)throw FormatError("low-detail bitmap index outside tag");
    const auto records=metadata_offset(data.u32(base+0x48),std::size_t(count)*0x74);
    const auto record=records+std::size_t(index)*0x74;
    const auto blob_offset=data.u32(0x158),blob_size=data.u32(0x15c);
    const auto stored_size=data.u32(record+0x64);
    //12C640 disables the software blob if either header field is absent;
    //12C960 cannot initialize an immutable Common0/size0 descriptor.
    if(!blob_offset || !blob_size || !stored_size)return {};
    if(stored_size>0x7fffffffu || data.u32(record+0x5c))
        throw FormatError("low-detail bitmap is not an immutable source descriptor");
    data.require(blob_offset,blob_size);
    BitmapLowDetailResource result{data.u16(record+0x6a),data.u16(record+0x6c),
        data.u16(record+0x6e),data.u16(record+0xa),data.u16(record+0x68),
        data.u16(record+0xe),data.u32(record+0x60),{}};
    if(!result.width || !result.height || !result.depth ||
       result.width>4096 || result.height>4096 || result.depth>4096 || result.type>2)
        throw FormatError("invalid source low-detail bitmap dimensions/type");
    if(result.shared_offset>blob_size || stored_size>blob_size-result.shared_offset)
        throw FormatError("low-detail bitmap payload outside authored shared blob");
    const auto payload=data.slice(std::size_t(blob_offset)+result.shared_offset,stored_size);
    result.pixels.assign(payload.begin(),payload.end());
    return result;
}
std::optional<Texture> Cache::load_bitmap_low_detail(std::uint32_t datum,unsigned index) const {
    auto resource=load_bitmap_low_detail_resource(datum,index);
    if(!resource)return {};
    const auto&r=*resource;
    if((r.flags&0x10u) || !std::has_single_bit(r.width) || !std::has_single_bit(r.height) ||
       !std::has_single_bit(r.depth))
        throw FormatError("source low-detail Texture requires its proved swizzled power-of-two header");
    //12C820 calls359E0 with maximum0: exactly one level. Raw full-image
    //flagbit3 does not determine this separate inline header's fetch layout.
    if(r.type==0){
        if(r.depth!=1 || r.format<14 || r.format>16)
            throw FormatError("unproved source low-detail planar Texture format/depth");
        return prepare_texture(Bytes(r.pixels),r.width,r.height,r.format,false,0);
    }
    if(r.type==2){
        if(r.depth!=1 || r.width!=r.height || r.format!=11)
            throw FormatError("unproved source low-detail cubemap Texture description");
        //12C820 overrides formats10/11 to NV0x33 for source flag0x20.
        //Only the ordinary NV6 cube layout is proved by the face oracle.
        if(r.flags&0x20u)
            throw FormatError("unproved source low-detail NV0x33 cubemap Texture format");
        //Original3FDBC0 locks faceN at base+N*128 for the supplied4x4
        //NV6 headers. Existing cube preparation preserves that face padding.
        if(r.width!=4 || r.pixels.size()!=768)
            throw FormatError("source low-detail cubemap requires its proved four-texel face layout");
        return prepare_cubemap(Bytes(r.pixels),r.width,r.format,true,0);
    }
    throw FormatError("source low-detail RGBA volume Texture decoding is not yet proved");
}
} // namespace halo2
