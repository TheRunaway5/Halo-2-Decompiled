#include "halo2/source_loading_assets.hpp"
#include "halo2/texture.hpp"
namespace halo2 {
SourceLoadingBitmapAssets load_source_loading_bitmap_assets(const Cache& cache) {
    if(!cache.is_mcc())throw FormatError("Source loading requires admitted MCC bitmap resources");
    struct Role {const char*name;unsigned pages;};
    constexpr std::array<Role,5> roles{{
        {"globals\\loading_screen\\loading_screen",1},
        {"globals\\loading_screen\\loading_screen_sweep",1},
        {"globals\\loading_screen\\loading_screen_spinners",2},
        {"globals\\loading_screen\\loading_screen_logos",2},
        {"globals\\loading_screen\\loading_screen_tickers",5}}};
    SourceLoadingBitmapAssets result;
    for(unsigned bank=0;bank<roles.size();++bank) {
        const auto&role=roles[bank];const Tag*owner{};
        for(const auto&tag:cache.select_tags({.group=TagGroup{"bitm"},.name=role.name,
            .require_address=true,.minimum_size=76})) {
            if(owner)throw FormatError("Ambiguous source loading bitmap owner: "+std::string(role.name));
            owner=&tag;
        }
        if(!owner)throw FormatError("Missing source loading bitmap owner: "+std::string(role.name));
        const Bytes header(cache.read_tag_bytes(owner->address,76));
        if(header.u32(68)!=role.pages)throw FormatError("Source loading bitmap page count differs: "+std::string(role.name));
        const Bytes records(cache.read_tag_bytes(header.u32(72),std::size_t(role.pages)*168));
        auto&out=result[bank];out.name=owner->name;out.handle=owner->handle();
        for(unsigned page=0;page<role.pages;++page) {
            const auto at=page*168;const auto resource=cache.load_bitmap_resource(owner->datum,page);
            if(resource.type || resource.depth!=1 || resource.levels[0].empty() ||
                !resource.levels[1].empty() || !resource.levels[2].empty() ||
                (resource.format!=1 && resource.format!=14 && resource.format!=15))
                throw FormatError("Source loading bitmap has an unowned shape/format: "+out.name);
            const auto mips=retail_mip_count(resource.width,resource.height,1,resource.format,
                (records.u16(at+14)&16)!=0,records.u16(at+20));
            auto texture=prepare_texture(Bytes(resource.levels[0]),resource.width,resource.height,
                resource.format,(records.u16(at+14)&8)!=0,mips);
            texture.pixels=TexturePixels(std::span<const std::byte>(
                static_cast<const Texture&>(texture).pixels.data(),texture.pixels.size()));
            out.frames.push_back(std::move(texture));
        }
    }
    return result;
}
}
