#include "halo2/cache.hpp"
#include <iostream>
#include <thread>
#include <utility>

using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
const std::byte* pixels(const Material& material){return material.diffuse.pixels.data();}
std::vector<std::byte> exported(const Texture& texture){return texture.pixels;}
}
int main()try{
    Material decoded;
    decoded.name="authored light receiver";
    decoded.diffuse={512,512,11,std::vector<std::byte>(512*512*4,std::byte{0x5a}),{{512,512,0,512*512*4}}};
    decoded.forward_diffuse={1,.5f,.25f,1};
    // The residency bank owns an independent authored material. Every light
    // pass then copies that complete Material, as in the real preparation path.
    const Material authored=decoded;
    check(exported(authored.diffuse)==decoded.diffuse.pixels,"Texture byte export changed its owned value");
    std::vector<Material> lights(24,authored);
    for(const auto& light:lights){
        check(pixels(light)==pixels(authored),"Copying an immutable light material duplicated its pixel allocation");
        check(light.diffuse.pixels==authored.diffuse.pixels && light.diffuse.mips.size()==1 &&
            light.diffuse.mips[0].size==512*512*4,"Texture residency changed authored bytes or mip extent");
    }
    lights[0].diffuse.pixels[7]=std::byte{0x12};lights[0].forward_diffuse[0]=.25f;
    check(authored.diffuse.pixels[7]==std::byte{0x5a} && lights[1].diffuse.pixels[7]==std::byte{0x5a} &&
        authored.forward_diffuse[0]==1 && lights[1].forward_diffuse[0]==1,
        "A per-light mutation changed another material's image or uniforms");
    Material writable=authored;auto* escaped=writable.diffuse.pixels.data();
    const Material later=writable;escaped[13]=std::byte{0x34};
    check(later.diffuse.pixels[13]==std::byte{0x5a},"A mutable view obtained before copying changed the copied texture");
    auto iterator=writable.diffuse.pixels.begin();const Material after_iterator=writable;
    *iterator=std::byte{0x45};
    check(after_iterator.diffuse.pixels[0]==std::byte{0x5a},"A pre-copy mutable iterator changed a copied texture");
    auto* before_move=writable.diffuse.pixels.data();Material moved=std::move(writable);const Material after_move=moved;
    before_move[19]=std::byte{0x67};
    check(after_move.diffuse.pixels[19]==std::byte{0x5a},"Moving storage lost the outstanding mutable-view boundary");
    Material small=authored;small.diffuse.pixels.resize(16);small.diffuse.pixels.push_back(std::byte{0x78});
    small.diffuse.pixels.insert(small.diffuse.pixels.end(),2,std::byte{0x23});small.diffuse.pixels.pop_back();
    check(small.diffuse.pixels.size()==18 && authored.diffuse.pixels.size()==512*512*4,
        "Texture resize/insert/pop changed shared authored residency");
    std::array<std::thread,4> readers;
    std::array<bool,4> okay{};
    for(unsigned i=0;i<readers.size();++i)readers[i]=std::thread([&,i]{
        okay[i]=true;
        for(unsigned n=0;n<100;++n){Material copy=authored;okay[i]=okay[i] && pixels(copy)==pixels(authored);
            copy.diffuse.pixels[i]=std::byte(i);okay[i]=okay[i] && authored.diffuse.pixels[i]==std::byte{0x5a};}
    });
    for(auto& reader:readers)reader.join();
    for(bool value:okay)check(value,"Concurrent material copies lost immutable image sharing or mutation isolation");
    std::cout<<"PASS real Material light-pass fanout shares image residency; bytes/mips/uniforms and escaped-view/move/thread mutation isolation preserved\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
