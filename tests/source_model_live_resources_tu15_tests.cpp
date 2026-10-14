#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char**argv)try {
    if(argc!=4 && argc!=5 && argc!=6)throw FormatError("supply TU1.5 image, Xbox map, original model name fragment, optional --build, optional diagnostic directory");
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
    if(argc>=5) {
        if(std::string_view(argv[4])!="--build")throw FormatError("unknown model-resource diagnostic option");
        const auto biped=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){
            return t.group=="bipd" && cache.load_initial_model_definition(t.datum).render_model==model->datum;});
        if(biped==cache.tags().end())throw FormatError("resource model has no authored biped owner");
        world.initialize_map();
        const auto position=cache.spawns().at(0).position;
        const auto warmup=world.create_biped(biped->datum,position,{1,0,0});
        world.begin_model_frame();
        (void)world.build_world_model(warmup,0,1,false,10.f,1.f);
        // The first real builder loads the location's lightmap resources using
        // native cache-sector I/O. Compare a different freshly born owner with
        // these actually resident resources; no completion or sample is given.
        const auto object=world.create_biped(biped->datum,position,{1,0,0});
        world.begin_model_frame();
        for(unsigned view=0;view<4;++view) {
            const auto players=view+1;
            const auto metric=std::array{10.f,0.f,30.f,1.f}[view];
            const auto directory=argc==6?std::filesystem::path(argv[5])/std::to_string(view):std::filesystem::path{};
            const auto dump=[&](const char* name) {
                if(argc!=6)return;
                std::filesystem::create_directories(directory);
                std::ofstream out(directory/name,std::ios::binary);
                world.memory().visit_committed([&](auto address,auto bytes) {
                    const auto count=std::uint32_t(bytes.size());
                    out.write(reinterpret_cast<const char*>(&address),4);
                    out.write(reinterpret_cast<const char*>(&count),4);
                    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
                });
                if(!out)throw FormatError("model builder diagnostic export failed");
            };
            dump("before.bin");
            const auto collector=world.build_world_model(object,view,players,false,metric,1.f);
            dump("after.bin");
            if(argc==6) {
                std::ofstream out(directory/"call.json");
                out<<"{\"owner\":"<<object<<",\"collector\":"<<collector.address
                   <<",\"view\":"<<view<<",\"players\":"<<players
                   <<",\"modifier\":false,\"metric\":"<<metric<<",\"lighting_metric\":1}\n";
                if(!out)throw FormatError("model builder diagnostic call export failed");
            }
            const Bytes produced(collector.bytes);
            if(produced.u32(0)!=object || word(collector.address)!=object || !produced.u16(0x166))
                throw FormatError("live original model builder produced no owned model for the current body");
            const auto context=produced.u32(0x48);
            if(!context || word(context)!=object || word(context+4)!=0xc08e0u)
                throw FormatError("live model context lacks the TU1.5 owner/callback identity");
            std::cout<<"Live model collector: view "<<view<<" / "<<players<<" players / "
                <<produced.u16(0x166)<<" model(s), source LOD "<<produced.u16(0x168)<<'\n';
        }
        rejected=false;try{world.build_world_model(object^0x10000u,0,1,false,10.f,1.f);}catch(const FormatError&){rejected=true;}
        if(!rejected)throw FormatError("model builder accepted a stale full object handle");
    }
    std::cout<<"PASS "<<model->name<<": "<<checked<<" live sections / "<<resources<<" original resources / "<<bytes
        <<" exact authored bytes; model collection and pixels remain separate gates\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
