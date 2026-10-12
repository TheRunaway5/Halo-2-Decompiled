#include "halo2/source_sphere_position.hpp"
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char** argv) {try {
    const bool terrain_mode=argc==4 && std::string(argv[1])=="--terrain";
    if(argc!=2 && !terrain_mode)throw FormatError("Supply original light spawn-correction fixture or --terrain MAP FIXTURE");
    std::optional<TerrainCollision> terrain;
    if(terrain_mode)terrain=Cache(argv[2]).load_raw_terrain_collision(0);
    std::ifstream f(argv[terrain_mode?3:1],std::ios::binary);
    const auto word=[&](){std::uint32_t w;f.read(reinterpret_cast<char*>(&w),4);if(!f)throw FormatError("Missing original spawn-correction word");return w;};
    if(word()!=0x31504353)throw FormatError("Invalid original spawn-correction fixture");
    const auto cases=word();unsigned corrected=0,polygons=0,compared=0,failures=0,contact_words=0;
    for(unsigned n=0;n<cases;++n) {
        const Vec3 center{std::bit_cast<float>(word()),std::bit_cast<float>(word()),std::bit_cast<float>(word())};
        const auto radius=std::bit_cast<float>(word());const auto sphere_count=word(),capsule_count=word(),count=word();
        if(count>256)throw FormatError("Original terrain polygon count exceeds256");
        std::vector<SourceCollisionSphere> spheres(sphere_count);f.read(reinterpret_cast<char*>(spheres.data()),sphere_count*32);
        std::vector<SourceCollisionCapsule> capsules(capsule_count);f.read(reinterpret_cast<char*>(capsules.data()),capsule_count*44);
        std::vector<SourceCollisionPolygon> bank(count);f.read(reinterpret_cast<char*>(bank.data()),count*108);
        SourceSphereContactStorage native_contacts;
        if(terrain_mode && radius>0) {
            if(terrain->bsps.size()!=1)throw FormatError("Original light terrain fixture requires one actual collision BSP");
            native_contacts=source_light_spawn_contacts(terrain->bsps[0],center,radius);
            const auto exact=[&](std::uint32_t got,std::uint32_t expected,const char* field) {
                ++contact_words;if(got!=expected) {if(failures<24)std::cerr<<"case="<<n<<" field="<<field<<" native="<<std::hex<<got<<" original="<<expected<<std::dec<<'\n';++failures;}
            };
            const auto contacts=[&](const auto& got,const auto& expected,unsigned stride) {
                exact(got.size(),expected.size(),"contact count");
                for(unsigned i=0;i<std::min(got.size(),expected.size());++i) {
                    const Bytes a(got[i].bytes),b(expected[i].bytes);
                    const auto words=stride==108?11+b.u32(0x28)*2:stride/4;
                    for(unsigned j=0;j<words;++j) {
                        const auto mask=stride==108 && j==9?0x00ffffffu:0xffffffffu;
                        exact(a.u32(j*4)&mask,b.u32(j*4)&mask,"contact word");
                    }
                }
            };
            contacts(native_contacts.spheres,spheres,32);contacts(native_contacts.capsules,capsules,44);contacts(native_contacts.polygons,bank,108);
        }
        const auto out=terrain_mode?source_sphere_position(center,radius,native_contacts.view()):
            source_sphere_position(center,radius,{spheres,capsules,bank});
        const std::array got{std::uint32_t(out.corrected),std::bit_cast<std::uint32_t>(out.position.x),
            std::bit_cast<std::uint32_t>(out.position.y),std::bit_cast<std::uint32_t>(out.position.z),std::bit_cast<std::uint32_t>(out.radius)};
        for(unsigned i=0;i<got.size();++i) {const auto expected=word();++compared;
            if(got[i]!=expected) {if(failures<24)std::cerr<<"case="<<n<<" lane="<<i<<" native="<<std::hex<<got[i]<<" original="<<expected<<std::dec<<'\n';++failures;}}
        corrected+=out.corrected;polygons+=count;
    }
    if(cases<512 || corrected<100 || polygons<1000 || f.peek()!=std::char_traits<char>::eof())
        throw FormatError("Original light spawn-correction coverage missing");
    if(failures)throw FormatError("Original spawn-correction differs in "+std::to_string(failures)+" words");
    std::cout<<"PASS "<<cases<<" original light spawn corrections, "<<corrected<<" adjusted, "<<polygons<<" real terrain polygons, "<<compared<<" exact output words, "<<contact_words<<" exact native-produced contact words\n";return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
