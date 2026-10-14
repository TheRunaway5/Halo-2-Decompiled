#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char**argv)try {
    if(argc!=4)throw FormatError("supply TU1.5 image, Xbox map and original model name fragment");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    Cache cache(argv[2]);
    const auto model=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){
        return t.group=="mode" && t.name.find(argv[3])!=std::string::npos;});
    if(model==cache.tags().end())throw FormatError("requested authored render model absent");
    auto temporary=(std::filesystem::temp_directory_path()/"halo2-model-resource-tu15-XXXXXX").string();
    if(!::mkdtemp(temporary.data()))throw FormatError("native model resource temporary directory failed");
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}} cleanup{temporary};
    PhysicsLiveWorldTu15 world(cache,argv[1],temporary);
    const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
    bool rejected=false;try{world.preload_render_model(model->datum);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("model resource load accepted an unconfigured session");
    world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    world.preload_render_model(model->datum);
    const auto mode=word(word(0x4f0414u)+(model->datum&65535u)*16u+8u);
    const auto sections=word(mode+0x28u),count=word(mode+0x24u);
    unsigned checked=0,resources=0;std::size_t bytes=0;
    for(unsigned section=0;section<count;++section) {
        const auto metadata=sections+section*0x5cu;
        if(!Bytes(world.memory().view(metadata+4u,2)).u16(0))continue;
        const auto raw=load_menu_world_source_model_section(cache,model->datum,section);
        const auto resident=word(metadata+0x34u);
        if(!resident || world.memory().view(metadata+0x56u,1)[0]!=std::byte{1})
            throw FormatError("original source request did not publish resident geometry");
        const Bytes header(raw.raw_model_header());
        for(const auto&resource:raw.resources()) {
            if(resource.type!=0 && resource.type!=8 && resource.type!=16 &&
               resource.type!=32 && resource.type!=100)continue;
            if(word(resident+resource.type)!=header.u32(resource.type))
                throw FormatError("published source geometry count differs from original resource bytes");
            const auto published=world.memory().view(word(resident+resource.type+4u),resource.bytes.size());
            if(!std::ranges::equal(published,resource.bytes))
                throw FormatError("published part/index/bound/node-map bytes differ from authored resource");
            ++resources;bytes+=published.size();
        }
        ++checked;
    }
    if(!checked || !resources || !bytes)throw FormatError("no original model resources exercised");
    world.preload_render_model(model->datum);
    rejected=false;try{world.preload_render_model(model->datum^0x10000u);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("model resource load accepted a stale full tag handle");
    std::cout<<"PASS "<<model->name<<": "<<checked<<" live sections / "<<resources<<" original resources / "<<bytes
        <<" exact authored bytes; model collection and pixels remain separate gates\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
