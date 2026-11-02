#include "halo2/cache.hpp"
#include <bit>
#include <iostream>
#include <set>
#include <thread>
using namespace halo2;
namespace {
void need(bool value,const char* message){if(!value)throw FormatError(message);}
void same(const Material& a,const Material& b) {
    need(a.source_shader_datum==b.source_shader_datum && a.lods.size()==b.lods.size() &&
        a.layers.size()==b.layers.size() && a.passes.size()==b.passes.size(),"Retained authored metadata topology changed");
    for(unsigned n=0;n<a.lods.size();++n)need(a.lods[n].layers.packed==b.lods[n].layers.packed &&
        a.lods[n].available_layers==b.lods[n].available_layers &&
        std::bit_cast<std::uint32_t>(a.lods[n].projected_height)==std::bit_cast<std::uint32_t>(b.lods[n].projected_height),
        "Retained authored LOD fields changed");
    for(unsigned n=0;n<a.layers.size();++n)need(a.layers[n].packed==b.layers[n].packed,"Retained authored layer range changed");
    for(unsigned n=0;n<a.passes.size();++n) {
        const auto& x=a.passes[n];const auto& y=b.passes[n];
        need(x.template_pass==y.template_pass && x.implementation==y.implementation &&
            x.vertex_shader==y.vertex_shader && x.vertex_code==y.vertex_code,"Retained original pass/program identity changed");
    }
}
}
int main(int argc,char** argv)try {
    if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);need(cache.is_mcc(),"Metadata residency fixture requires the published MCC cache");
    const auto mesh=cache.load_bsp(0);std::set<std::uint32_t> shaders;
    for(const auto& material:mesh.materials)if(material.source_shader_datum!=0xffffffffu)shaders.insert(material.source_shader_datum);
    need(!shaders.empty(),"Metadata residency fixture has no BSP shaders");
    for(const auto shader:shaders) {
        const auto retained=cache.load_material_metadata(shader);
        need(retained==cache.load_material_metadata(shader),"Unchanged Cache shader was reparsed");
        same(*retained,cache.load_material(shader,false,false));
        same(*retained,cache.load_material(shader));
        // Caller copies may prepare/modify passes without mutating the owner.
        auto copy=*retained;if(!copy.passes.empty())copy.passes[0].vertex_code.clear();
        same(*retained,cache.load_material(shader,false,false));
    }
    const auto shader=*shaders.begin();const auto retained=cache.load_material_metadata(shader);
    std::array<std::shared_ptr<const Material>,8> joined;
    std::array<std::thread,8> workers;
    for(unsigned n=0;n<workers.size();++n)workers[n]=std::thread([&,n]{joined[n]=cache.load_material_metadata(shader);});
    for(auto& worker:workers)worker.join();for(const auto& owner:joined)need(owner==retained,"Concurrent metadata read lost its immutable owner");
    auto copy=cache;need(copy.load_material_metadata(shader)==retained,"Cache value copy lost its shared immutable backing");
    Cache other(argv[1]);need(other.load_material_metadata(shader)!=retained,"Independent Cache borrowed another owner's metadata");
    for(unsigned n=0;n<2;++n) {
        bool rejected=false;try{cache.load_material_metadata(0xffffffffu);}catch(const FormatError&){rejected=true;}
        need(rejected,"Metadata residency admitted an invalid full shader datum");
    }
    std::cout<<"PASS retained authored LOD/layer/pass/program fields vs full and metadata-only loads: "<<shaders.size()<<" Zanzibar shaders; COW, threads and owner isolation\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
