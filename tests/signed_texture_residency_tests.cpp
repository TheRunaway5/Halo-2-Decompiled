#include "halo2/texture.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

using namespace halo2;
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
const std::byte* bytes(const Texture& texture){return texture.pixels.data();}
}
int main()try {
    Texture source{2,2,11,{std::byte{0},std::byte{127},std::byte{128},std::byte{255},
        std::byte{255},std::byte{128},std::byte{127},std::byte{0},
        std::byte{64},std::byte{192},std::byte{32},std::byte{224},
        std::byte{1},std::byte{2},std::byte{3},std::byte{4}},{{2,2,0,16}}};
    const auto original=source;
    auto converted=prepare_signed_texture(source,0xa0000000u);
    auto copy=converted;
    check(bytes(converted)==bytes(copy),"Completed signed texture is duplicated by a material copy");
    check(converted.format==native_rgba32f && converted.pixels.size()==64 && converted.mips.size()==1 &&
        converted.mips[0].offset==0 && converted.mips[0].size==64,"Signed texture storage layout changed");
    for(unsigned at=0;at<16;++at) {
        const unsigned raw=std::to_integer<unsigned>(std::as_const(source).pixels[at]);
        const float expected=(at%4==0 || at%4==2)?float(raw>=128?int(raw)-256:int(raw))/128.f:float(raw)/255.f;
        unsigned actual;std::memcpy(&actual,bytes(converted)+at*4,4);
        check(actual==std::bit_cast<unsigned>(expected),"Signed conversion changed a texel's exact float bits");
    }
    check(source.pixels==original.pixels,"Signed conversion changed its input storage");
    Material authored;authored.diffuse=std::move(converted);
    std::vector<Material> passes(64,authored);
    for(const auto& pass:passes)check(bytes(pass.diffuse)==bytes(authored.diffuse),"Pass/LOD preload duplicated completed texture bytes");
    const auto before=std::as_const(authored.diffuse).pixels[0];
    passes[0].diffuse.pixels[0]=std::byte{7};
    check(bytes(passes[0].diffuse)!=bytes(authored.diffuse) && std::as_const(authored.diffuse).pixels[0]==before,
        "Mutable pass changed shared authored texture bytes");
    auto* exposed=passes[1].diffuse.pixels.data();auto isolated=passes[1].diffuse;
    check(bytes(isolated)!=bytes(passes[1].diffuse),"Copy after mutable exposure shared a writable view");
    const auto isolated_before=std::as_const(isolated).pixels[0];exposed[0]=std::byte{9};
    check(std::as_const(isolated).pixels[0]==isolated_before,"Previously exposed mutable pointer changed a snapshot");
    std::cout<<"Signed image shares64 material copies; all16 float words and mutable-isolation controls pass\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
