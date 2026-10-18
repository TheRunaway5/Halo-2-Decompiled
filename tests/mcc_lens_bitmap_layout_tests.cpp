#include "halo2/mcc_lens_bitmap_layout.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <set>

namespace {
using namespace halo2;
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
template<class T> void put(T& row,unsigned at,unsigned value,unsigned bytes=2) {
    for(unsigned n=0;n<bytes;++n)row[at+n]=std::byte(value>>(n*8u));
}
struct Fixture {
    TagHandle bitmap{0xe1230042u};unsigned frame=7;
    std::array<std::byte,0xa8> row{};
    BitmapResource resource;
    Texture texture;
    Fixture(unsigned format=1,unsigned width=8,unsigned height=4,unsigned mips=2) {
        put(row,0,tag_groups::bitmap.value,4);put(row,4,width);put(row,6,height);
        put(row,8,1,1);put(row,9,0x86,1);put(row,0xc,format);put(row,0xe,0x2201);
        put(row,0x10,0x7123);put(row,0x12,0xa456);put(row,0x14,mips);put(row,0x16,0xe721);
        put(row,0x50,128,4);
        // MCC trailing fields are neither original texture aliases nor
        // header pointers/stamps. They must never enter the result.
        put(row,0x40,0xfedcba98u,4);put(row,0x70,0x76543210u,4);
        resource={std::uint16_t(width),std::uint16_t(height),1,0,std::uint16_t(format),{}};
        resource.levels[0].resize(8192);
        for(unsigned n=0;n<resource.levels[0].size();++n)resource.levels[0][n]=std::byte(n*13u);
        if(format==17 || format==18)
            texture=prepare_palettized_texture(Bytes(resource.levels[0]),width,height,false,tu15_bump_palette(),mips);
        else texture=prepare_texture(Bytes(resource.levels[0]),width,height,std::uint16_t(format),false,mips);
    }
    auto project()const{return project_mcc_lens_bitmap_layout(bitmap,frame,row,resource,texture);}
};
unsigned portable() {
    unsigned cases{};
    for(auto format:{0u,1u,2u,3u,6u,8u,9u,10u,11u,14u,15u,16u,17u,18u,22u,23u}) {
        Fixture f(format);const auto row=f.row;const auto pixels=f.texture.pixels;const auto resource=f.resource.levels;
        const auto result=f.project();
        require(result.bitmap==f.bitmap && result.frame==f.frame && result.common.size()==0x18 &&
            std::ranges::equal(result.common,std::span(f.row).first(0x18)),
            "Lens adapter changed its complete salted identity or authored prefix");
        require(f.row==row && f.texture.pixels==pixels && f.resource.levels==resource,
            "Pure lens adapter changed source resource/image storage");++cases;
    }
    Fixture linear(11,6,5,0);put(linear.row,0xe,0x0210);(void)linear.project();++cases;
    Fixture flags;put(flags.row,0xe,0xffd7);const auto authored=flags.project();
    require(authored.common[0xe]==std::byte{0xd7} && authored.common[0xf]==std::byte{0xff} &&
        authored.common[9]==std::byte{0x86},"MCC source flags/creation byte were fabricated");++cases;
    Fixture frame;frame.frame=0x7fff;(void)frame.project();++cases;
    const auto rejected=[&](auto change) {
        Fixture f;change(f);const auto before=f.row;const auto pixels=f.texture.pixels;const auto levels=f.resource.levels;
        bool failed=false;try{(void)f.project();}catch(const FormatError&){failed=true;}
        require(failed,"Malformed lens bitmap shape/extent was admitted");
        require(before==f.row && pixels==f.texture.pixels && levels==f.resource.levels,
            "Lens rejection partly changed input storage");++cases;
    };
    rejected([](auto&f){f.bitmap={};});
    rejected([](auto&f){f.frame=0x8000;});
    rejected([](auto&f){put(f.row,0,0,4);});
    for(unsigned at:{4u,6u})for(unsigned v:{0u,4097u,0xffffu})
        rejected([&](auto&f){put(f.row,at,v);});
    for(unsigned depth:{0u,2u,0x80u,0xffu})rejected([&](auto&f){put(f.row,8,depth,1);});
    for(unsigned type:{1u,2u,0xffffu})rejected([&](auto&f){put(f.row,0xa,type);});
    for(unsigned format:{4u,5u,7u,12u,13u,19u,20u,21u,24u,0xffffu})
        rejected([&](auto&f){put(f.row,0xc,format);});
    for(unsigned mips:{4u,0x7fffu,0xffffu})rejected([&](auto&f){put(f.row,0x14,mips);});
    rejected([](auto&f){++f.resource.width;});rejected([](auto&f){++f.resource.height;});
    rejected([](auto&f){++f.resource.depth;});rejected([](auto&f){++f.resource.type;});
    rejected([](auto&f){++f.resource.format;});
    for(unsigned stored:{0u,7u,256u*1024u*1024u+1u,0xffffffffu})
        rejected([&](auto&f){put(f.row,0x50,stored,4);});
    rejected([](auto&f){f.resource.levels[0].clear();});
    rejected([](auto&f){f.resource.levels[0].resize(41);});
    rejected([](auto&f){f.resource.levels[1].push_back(std::byte{});});
    rejected([](auto&f){put(f.row,0x54,8,4);});
    rejected([](auto&f){++f.texture.width;});rejected([](auto&f){++f.texture.height;});
    for(unsigned layers:{0u,2u,6u})rejected([&](auto&f){f.texture.layers=layers;});
    rejected([](auto&f){f.texture.depth=1;});
    rejected([](auto&f){f.texture.format=14;});
    rejected([](auto&f){f.texture.pixels.pop_back();});
    rejected([](auto&f){f.texture.pixels.push_back(std::byte{});});
    rejected([](auto&f){f.texture.mips.clear();});
    rejected([](auto&f){f.texture.mips.pop_back();});
    rejected([](auto&f){f.texture.mips.push_back(f.texture.mips.back());});
    for(unsigned level:{0u,1u,2u}) {
        rejected([&](auto&f){++f.texture.mips[level].width;});
        rejected([&](auto&f){++f.texture.mips[level].height;});
        rejected([&](auto&f){++f.texture.mips[level].depth;});
        rejected([&](auto&f){++f.texture.mips[level].offset;});
        rejected([&](auto&f){++f.texture.mips[level].size;});
    }
    rejected([](auto&f){f.texture.mips[1].offset=std::numeric_limits<std::size_t>::max();});
    rejected([](auto&f){f.texture.mips[1].size=std::numeric_limits<std::size_t>::max();});
    for(unsigned bytes:{0u,0x17u,0x74u,0xa7u,0xa9u}) {
        Fixture f;std::vector<std::byte> row(bytes);std::copy_n(f.row.begin(),std::min(row.size(),f.row.size()),row.begin());
        bool failed=false;try{(void)project_mcc_lens_bitmap_layout(f.bitmap,f.frame,row,f.resource,f.texture);}
        catch(const FormatError&){failed=true;}require(failed,"Partial/overlong MCC row was admitted");++cases;
    }
    for(auto f:{Fixture(14),Fixture(1,6,5,0)}) {
        put(f.row,0xe,0x0209);bool failed=false;try{(void)f.project();}catch(const FormatError&){failed=true;}
        require(failed,"Unsupported swizzled resource layout was admitted");++cases;
    }
    return cases;
}
unsigned actual(const char*path) {
    Cache cache(path);require(cache.is_mcc(),"Actual lens fixture requires MCC");
    const Bytes globals(cache.tag_payload(cache.globals_tag()));require(globals.u32(0x108)==1,"No rasterizer owner");
    const Bytes raster(cache.read_tag_bytes(globals.u32(0x10c),0x68));std::set<unsigned> bitmaps{raster.u32(0x64)};
    for(const auto&lens:cache.select_tags({.group=TagGroup{"lens"},.require_address=true})) {
        const Bytes root(cache.tag_payload(lens.handle()));bitmaps.insert(root.u32(0x24));
    }
    unsigned images{};
    for(auto bitmap:bitmaps) {
        if(bitmap==0xffffffffu)continue;
        const auto*tag=cache.find_tag({bitmap},tag_groups::bitmap);
        require(tag && tag->address && !cache.find_tag({bitmap^0x10000u},tag_groups::bitmap),"Fixture bitmap salt/type differs");
        const Bytes root(cache.tag_payload(tag->handle()));require(root.u32(0x44)==cache.bitmap_count(bitmap),"Fixture row count differs");
        for(unsigned n=0;n<root.u32(0x44);++n) {
            const auto row=cache.read_tag_bytes(root.u32(0x48)+n*0xa8,0xa8);
            const auto resource=cache.load_bitmap_resource(bitmap,n);const auto image=cache.load_bitmap(bitmap,n);
            const auto projected=project_mcc_lens_bitmap_layout({bitmap},n,row,resource,image);
            require(projected.bitmap.value==bitmap && projected.frame==n &&
                std::ranges::equal(projected.common,row.first(0x18)),"Actual source header bytes changed");++images;
        }
    }
    return images;
}
}
int main(int argc,char**argv)try {
    const auto cases=portable();std::cout<<"Portable lens layout cases: "<<cases<<'\n';
    if(argc==2 && std::string_view(argv[1]).empty())return 77;
    if(argc==2)std::cout<<"Actual MCC lens images: "<<actual(argv[1])<<'\n';
    else if(argc!=1)return 2;
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
