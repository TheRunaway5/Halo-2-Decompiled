#include "halo2/effect_light_registration.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
using namespace halo2;
int main(int argc,char** argv) {try {
    const bool terrain_mode=argc==4 && std::string(argv[1])=="--terrain";
    if(argc!=2 && !terrain_mode)throw FormatError("Supply original light registration fixture or --terrain MAP FIXTURE");
    std::optional<TerrainCollision> terrain;
    if(terrain_mode)terrain=Cache(argv[2]).load_raw_terrain_collision(0);
    std::ifstream file(argv[terrain_mode?3:1],std::ios::binary);
    const auto read=[&](void* p,std::size_t n){file.read(static_cast<char*>(p),n);if(!file)throw FormatError("Truncated original light registration fixture");};
    const auto word=[&](){std::uint32_t w;read(&w,4);return w;};
    const auto scalar=[&](){return std::bit_cast<float>(word());};
    const auto vector=[&](){return Vec3{scalar(),scalar(),scalar()};};
    const auto short_word=[&](){std::uint16_t w;read(&w,2);return w;};
    if(word()!=0x3152474c)throw FormatError("Invalid original light registration fixture");
    const auto cases=word();const auto selected=std::int16_t(word());const auto clusters=word();
    BspAtmosphere location;ObjectLightSpatialBsp membership;
    const auto nodes=word();for(unsigned i=0;i<nodes;++i) {
        std::array<std::byte,8> raw;read(raw.data(),8);const Bytes b(raw);
        const auto packed=[&](unsigned at){return std::to_integer<unsigned>(raw[at])|
            (std::to_integer<unsigned>(raw[at+1])<<8)|(std::to_integer<unsigned>(raw[at+2])<<16);};
        location.nodes.push_back({b.u16(0),packed(2),packed(5)});
    }
    const auto planes=word();for(unsigned i=0;i<planes;++i) {ShaderVector p;for(auto& x:p)x=scalar();location.planes.push_back(p);membership.planes.push_back(p);}
    const auto leaves=word();for(unsigned i=0;i<leaves;++i)location.leaves.push_back(std::int16_t(short_word()));
    location.clusters.resize(clusters);
    for(unsigned i=0;i<clusters;++i) {const auto count=word();std::vector<std::uint16_t> refs(count);for(auto& r:refs)r=short_word();membership.clusters.push_back(std::move(refs));}
    const auto portals=word();for(unsigned i=0;i<portals;++i) {
        ObjectLightSpatialBsp::Portal p;p.back=std::int16_t(short_word());p.front=std::int16_t(short_word());p.plane=word();p.center=vector();p.radius=scalar();
        const auto vertices=word();for(unsigned j=0;j<vertices;++j)p.vertices.push_back(vector());membership.portals.push_back(std::move(p));
    }
    unsigned compared=0,registered=0,failures=0;
    const auto decode=[](const std::array<std::byte,272>& raw) {
        const Bytes b(raw);const auto vec=[&](unsigned at){return Vec3{b.f32(at),b.f32(at+4),b.f32(at+8)};};
        SourceEffectLightRegistration r;r.identity=0xe9810015;r.reference_head=b.u32(0x10);r.flags=b.u16(2);
        r.spatial_center=vec(0x18);r.spatial_radius=b.f32(0x24);r.cull_center=vec(0x28);r.cull_radius=b.f32(0x34);
        r.location_center=vec(0x38);r.location={b.u32(0x44),std::int16_t(b.u16(0x48)),std::int16_t(b.u16(0x4a))};
        r.position=vec(0x84);r.extension_radius=b.f32(0x9c);r.extension_center=vec(0xa0);return r;
    };
    for(unsigned n=0;n<cases;++n) {
        std::array<std::byte,228> definition_raw;read(definition_raw.data(),228);const auto definition=source_effect_light_definition(definition_raw);
        std::array<std::byte,272> before,after,removed;read(before.data(),272);auto light=decode(before);
        const auto spheres=word(),capsules=word(),polygons=word();
        if(spheres>256 || capsules>256 || polygons>256)throw FormatError("Original registration contact bank exceeds256");
        std::vector<SourceCollisionSphere> points(spheres);read(points.data(),spheres*32);
        std::vector<SourceCollisionCapsule> edges(capsules);read(edges.data(),capsules*44);
        std::vector<SourceCollisionPolygon> faces(polygons);read(faces.data(),polygons*108);
        const SourceSphereContactBank contacts{points,edges,faces};read(after.data(),272);read(removed.data(),272);
        auto pool=std::make_unique<SourceEffectLightSpatialPool>(clusters);
        if(terrain_mode)source_effect_light_register(definition,light,location,selected,membership,*pool,terrain->bsps.at(0));
        else source_effect_light_register_from_contacts(definition,light,location,selected,membership,*pool,&contacts);
        registered+=light.reference_head!=0xffffffffu;
        const auto exact=[&](std::uint32_t got,std::uint32_t expected,const char* field) {
            ++compared;if(got!=expected) {if(failures<16)std::cerr<<"case="<<n<<" field="<<field<<" native="<<std::hex<<got<<" original="<<expected<<std::dec<<'\n';++failures;}
        };
        const auto record=[&](const std::array<std::byte,272>& raw) {
            const auto expected=decode(raw);
            exact(light.reference_head,expected.reference_head,"reference head");exact(light.flags,expected.flags,"flags");
            const auto floats=[&](std::span<const float> a,std::span<const float> b){for(unsigned i=0;i<a.size();++i)exact(std::bit_cast<std::uint32_t>(a[i]),std::bit_cast<std::uint32_t>(b[i]),"record float");};
            for(const auto& p:std::array{std::pair{light.spatial_center,expected.spatial_center},std::pair{light.cull_center,expected.cull_center},std::pair{light.location_center,expected.location_center},std::pair{light.position,expected.position},std::pair{light.extension_center,expected.extension_center}})
                floats(std::array{p.first.x,p.first.y,p.first.z},std::array{p.second.x,p.second.y,p.second.z});
            floats(std::array{light.spatial_radius,light.cull_radius,light.extension_radius},std::array{expected.spatial_radius,expected.cull_radius,expected.extension_radius});
            exact(light.location.leaf,expected.location.leaf,"location leaf");exact(std::uint16_t(light.location.cluster),std::uint16_t(expected.location.cluster),"location cluster");exact(std::uint16_t(light.location.bsp),std::uint16_t(expected.location.bsp),"location BSP");
        };
        record(after);source_effect_light_unregister(light,*pool);record(removed);
        for(auto got:pool->heads)exact(got,word(),"final cluster head");
        for(const auto* p:{&pool->entries,&pool->references}) {
            for(auto got:std::array{p->cursor,p->highwater,p->active_count,std::uint32_t(p->next_generation)})exact(got,word(),"allocator");
            for(auto got:p->occupied)exact(got,word(),"bitmap");
            for(const auto& r:p->records)for(unsigned i=0;i<3;++i)exact(r[i],word(),"complete pool record");
        }
    }
    if(cases!=128 || registered<50 || file.peek()!=std::char_traits<char>::eof())throw FormatError("Original light registration coverage missing");
    if(failures)throw FormatError("Original registration differs in "+std::to_string(failures)+" words");
    std::cout<<"PASS "<<cases<<" whole original registration/unregistration pairs, "<<registered<<" nonempty, "<<compared<<" exact record/pool/location words\n";return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
