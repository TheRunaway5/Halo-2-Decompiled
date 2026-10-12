#include "model_skin_vertices.hpp"
#include "model_world_pose_cache.hpp"
#include <bit>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
void equal(std::span<const Vertex> a,std::span<const Vertex> b) {
    require(a.size()==b.size(),"SIMD skin changed vertex count");
    if(!a.empty() && std::memcmp(a.data(),b.data(),a.size_bytes())) {
        for(std::size_t i=0;i<a.size();++i)if(std::memcmp(&a[i],&b[i],sizeof(Vertex)))
            throw FormatError("SIMD skin changed complete vertex bytes at "+std::to_string(i));
    }
}
void synthetic() {
    std::mt19937 rng(0x1376c0);std::uniform_real_distribution<float> f(-8,8);
    std::vector<std::array<float,12>> palette(19);
    std::vector<Vertex> source(512);std::vector<SkinBinding> bindings(source.size());
    for(unsigned frame=0;frame<256;++frame) {
        for(auto &matrix:palette)for(auto &word:matrix)word=f(rng);
        for(unsigned i=0;i<source.size();++i) {
            auto &v=source[i];v.position={f(rng),f(rng),f(rng)};v.normal={f(rng),f(rng),f(rng)};
            v.tangent={f(rng),f(rng),f(rng)};v.binormal={f(rng),f(rng),f(rng)};
            v.material=rng();v.lightmap=rng();v.source_section=rng();v.source_part=rng();
            for(unsigned j=0;j<4;++j){bindings[i].bones[j]=rng()%palette.size();bindings[i].weights[j]=j<i%5?f(rng):0;}
            // A zero in the active range is still multiplied and added.
            if(i%7==0)bindings[i].weights[1]=-0.f;
            if(i%11==0){v.normal={-0.f,0.f,-0.f};v.binormal={0.f,-0.f,0.f};}
            if(i%13==0)v.position.x=std::bit_cast<float>(std::uint32_t(1));
        }
        auto expected=source,actual=source;
        skin_model_vertices_scalar(expected,bindings,palette);
        skin_model_vertices(actual,bindings,palette,true);equal(expected,actual);
    }
    for(const std::uint32_t word:{0u,0x80000000u,1u,0x80000001u,0x7f800000u,0xff800000u,0x7fc12345u,0xffc45678u}) {
        auto special=source;auto special_palette=palette;auto special_bindings=bindings;
        special[0].position.x=std::bit_cast<float>(word);
        special[1].normal.y=std::bit_cast<float>(word);
        special_palette[0][0]=std::bit_cast<float>(word);
        special_bindings[2].bones={0,0,0,0};special_bindings[2].weights={1,0,0,0};
        auto expected=special,actual=special;
        skin_model_vertices_scalar(expected,special_bindings,special_palette);
        skin_model_vertices(actual,special_bindings,special_palette,true);equal(expected,actual);
    }
    auto invalid=bindings;invalid[0].weights={1,0,0,0};invalid[0].bones[0]=65535;
    for(bool simd:{false,true}) {
        auto output=source;bool threw=false;
        try{skin_model_vertices(output,invalid,palette,simd);}catch(const FormatError &){threw=true;}
        require(threw,"SIMD skin accepted a bone outside the palette");
    }
    std::cout<<"131072 complete synthetic vertex records exact, descending influences, zero weights, signed zero and subnormals\n";
}
void scalar_control(bool enabled) {
#ifdef _WIN32
    _putenv_s("HALO2_REFERENCE_SCALAR_SKINNING",enabled?"1":"");
#else
    if(enabled)setenv("HALO2_REFERENCE_SCALAR_SKINNING","1",1);
    else unsetenv("HALO2_REFERENCE_SCALAR_SKINNING");
#endif
}
void assets(const std::filesystem::path &root) {
    std::ifstream fixture(root/"tests/fixtures/retail_source_model_pose_assets.tsv");require(bool(fixture),"retail skin model fixture missing");
    std::map<std::string,std::unique_ptr<Cache>> caches;std::string line;std::size_t records=0,models=0;
    while(std::getline(fixture,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);std::string name;std::uint32_t datum;row>>name>>std::hex>>datum;
        if(!caches.contains(name)) {
            auto path=root/"assets/disc/maps"/(name+".map");
            if(!std::filesystem::exists(path))path=root/"assets/map-pack/maps"/(name+".map");
            caches.emplace(name,std::make_unique<Cache>(path));
        }
        const auto model=caches.at(name)->load_model(datum,false);
        ModelWorldPoseCache retained;std::vector<Transform> previous_nodes;
        for(unsigned frame=0;frame<3;++frame) {
            Transform root_transform;root_transform.position={float(frame)*3.125f,-float(frame)*.07f,float(frame)*.625f};
            root_transform.scale=frame==2?.625f:1.f;
            if(frame==1)root_transform.basis={Vec3{0,1,0},Vec3{-1,0,0},Vec3{0,0,1}};
            const auto nodes=pose_model_nodes_world(model,root_transform);
            scalar_control(true);const auto reference=skin_model_world_nodes(model,nodes);
            scalar_control(false);const auto actual=skin_model_world_nodes(model,nodes);
            equal(reference,actual);records+=actual.size();
            const bool identical=frame && previous_nodes.size()==nodes.size() &&
                (nodes.empty() || !std::memcmp(previous_nodes.data(),nodes.data(),nodes.size()*sizeof(Transform)));
            const auto built=retained.update(model,nodes);
            require(built.reused==identical,"WORLD node reuse differs from complete transform bytes");
            previous_nodes=nodes;
            equal(reference,built.vertices);
            const auto reused=retained.update(model,nodes);require(reused.reused,"equal WORLD nodes were not retained");
            equal(reference,reused.vertices);
            if(frame==2) {
                const auto mode=std::fegetround();
                for(const int changed:{FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO,mode}) {
                    require(std::fesetround(changed)==0,"set pose-cache rounding mode failed");
                    const auto expected=skin_model_world_nodes(model,nodes);
                    const auto output=retained.update(model,nodes);
                    if(changed!=mode)require(!output.reused,"changed arithmetic mode retained stale vertices");
                    equal(expected,output.vertices);
                }
                require(std::fesetround(mode)==0,"restore pose-cache rounding mode failed");
#if defined(__SSE2__) || defined(_M_X64)
                const auto saved_mxcsr=_mm_getcsr();_mm_setcsr(saved_mxcsr^0x8040u);
                const auto expected=skin_model_world_nodes(model,nodes);
                const auto output=retained.update(model,nodes);
                require(!output.reused,"changed FTZ/DAZ retained a stale skin");equal(expected,output.vertices);
                _mm_setcsr(saved_mxcsr);
#endif
            }
        }
        ++models;
    }
    require(caches.size()==21 && models==403 && records>100000,"retail skin coverage changed");
    std::cout<<models<<" original models across "<<caches.size()<<" maps, "<<records<<" complete posed vertex records exact, including retained WORLD nodes and arithmetic-mode invalidation\n";
}
}
int main(int argc,char **argv) {
    try{synthetic();if(argc==2)assets(argv[1]);return 0;}
    catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
