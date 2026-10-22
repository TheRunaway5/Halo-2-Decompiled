#include "halo2/mcc_lens_bitmap_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
unsigned word(std::span<const std::byte> row,unsigned at,unsigned bytes=2) {
    unsigned value{};
    for(unsigned n=0;n<bytes;++n)value|=std::to_integer<unsigned>(row[at+n])<<(n*8u);
    return value;
}
bool decoded_rgba(unsigned format) {
    switch(format) {
    case 0:case 1:case 2:case 3:case 6:case 8:case 9:case 10:case 11:
    case 17:case 18:case 22:case 23:return true;
    default:return false;
    }
}
std::size_t extent(unsigned width,unsigned height,unsigned format) {
    if(format>=14 && format<=16)
        return std::size_t((width+3u)/4u)*((height+3u)/4u)*(format==14?8u:16u);
    const unsigned stride=format<=2 || format==17 || format==18?1u:
        format==3 || format==6 || format==8 || format==9 || format==22 || format==23?2u:4u;
    return std::size_t(width)*height*stride;
}
}
MccLensBitmapLayout project_mcc_lens_bitmap_layout(TagHandle bitmap,unsigned frame,
    std::span<const std::byte> row,const BitmapResource& resource,const Texture& texture) {
    if(bitmap.is_null() || frame>0x7fffu || row.size()!=0xa8)
        throw FormatError("MCC lens bitmap needs a complete typed image row and signed frame index");
    if(word(row,0,4)!=tag_groups::bitmap.value)
        throw FormatError("MCC lens bitmap row signature differs");
    const auto width=word(row,4),height=word(row,6),depth=word(row,8,1);
    const auto type=word(row,0xa),format=word(row,0xc),flags=word(row,0xe),mips=word(row,0x14);
    if(!width || !height || width>4096 || height>4096 || depth!=1 || type!=0)
        throw FormatError("MCC lens bitmap requires its actual bounded 2D image");
    if(!decoded_rgba(format) && !(format>=14 && format<=16))
        throw FormatError("MCC lens bitmap format has no existing native image decoder");
    if(mips>=static_cast<unsigned>(std::bit_width(std::max(width,height))))
        throw FormatError("MCC lens bitmap mip count exceeds its image dimensions");
    if((flags&8u) && ((format>=14 && format<=16) ||
        !std::has_single_bit(width) || !std::has_single_bit(height)))
        throw FormatError("MCC lens bitmap has an unowned swizzled image layout");
    if(resource.width!=width || resource.height!=height || resource.depth!=1 ||
       resource.type!=type || resource.format!=format)
        throw FormatError("MCC lens bitmap resource differs from its authored row");
    constexpr unsigned maximum_partition=256u*1024u*1024u;
    for(unsigned n=0;n<resource.levels.size();++n) {
        const auto stored=word(row,0x50+n*4u,4);
        if((stored && (stored<8 || stored>maximum_partition || resource.levels[n].empty())) ||
           (!stored && !resource.levels[n].empty()) || resource.levels[n].size()>maximum_partition)
            throw FormatError("MCC lens bitmap lacks its declared texture partition");
    }
    if(resource.levels[0].empty() || texture.width!=width || texture.height!=height ||
       texture.layers!=1 || texture.depth || texture.mips.size()!=std::size_t(mips)+1u ||
       texture.format!=(decoded_rgba(format)?11u:format))
        throw FormatError("MCC lens bitmap decoded image shape differs");
    std::size_t encoded{},decoded{};
    auto mip_width=width,mip_height=height;
    for(const auto& mip:texture.mips) {
        const auto bytes=extent(mip_width,mip_height,texture.format);
        if(mip.width!=mip_width || mip.height!=mip_height || mip.depth!=1 ||
           mip.offset!=decoded || mip.size!=bytes || decoded>texture.pixels.size() ||
           bytes>texture.pixels.size()-decoded)
            throw FormatError("MCC lens bitmap decoded mip storage differs");
        encoded+=extent(mip_width,mip_height,format);decoded+=bytes;
        mip_width=std::max(1u,mip_width/2u);mip_height=std::max(1u,mip_height/2u);
    }
    if(encoded>resource.levels[0].size() || decoded!=texture.pixels.size())
        throw FormatError("MCC lens bitmap encoded/decoded storage is incomplete");
    MccLensBitmapLayout out{bitmap,frame,{}};
    std::copy_n(row.begin(),out.common.size(),out.common.begin());
    return out;
}
}
