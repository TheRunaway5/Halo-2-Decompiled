// Read-only listing of the authored pass and vertex program each shader's
// top shader level submits on one source layer (default 17, the alpha-only
// bloom mask consumed by 13870). Usage: audit_shader_layer_passes MAP [LAYER]
//   MAP all SUBSTRING    every level, layer and pass of matching shaders
//                        (AUDIT_PASS_DETAIL=1 adds externs, textures, sampler
//                        and render states and the program; AUDIT_BITMAPS=1
//                        adds resolved bitmaps and animation overlays)
//   MAP model SUBSTRING  every section and part of matching render models
//   MAP functions        object functions driven by the builtin one/zero
#include "halo2/cache.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <cstdlib>
using namespace halo2;
std::string detail(const MaterialPass& p) {
    if(!std::getenv("AUDIT_PASS_DETAIL"))return {};
    std::ostringstream o;o<<"\n        pixel_externs(dest:component:source):";
    for(const auto& r:p.pixel_externs)o<<' '<<r.destination<<':'<<unsigned(r.component)<<':'<<unsigned(r.source);
    o<<"\n        vertex_externs:";for(const auto& r:p.vertex_externs)o<<' '<<r.destination<<':'<<unsigned(r.component)<<':'<<unsigned(r.source);
    o<<"\n        textures(param:extern:stage:flags):";for(const auto& t:p.textures)o<<' '<<unsigned(t.parameter)<<':'<<unsigned(t.external)<<':'<<unsigned(t.stage)<<':'<<unsigned(t.flags);
    o<<"\n        texture_states(address u,v,w mag min mip border):";for(const auto& ts:p.texture_states)o<<' '<<ts.address_u<<','<<ts.address_v<<','<<ts.address_w<<' '<<ts.mag_filter<<' '<<ts.min_filter<<' '<<ts.mip_filter<<' '<<std::hex<<ts.border_color<<std::dec<<" |";
    o<<"\n        states:"<<std::hex;for(const auto& st:p.states)o<<' '<<unsigned(st.index)<<'='<<st.value;
    o<<"\n        program:";for(auto w:p.program.words)o<<' '<<w;
    return o.str();
}
int main(int argc,char** argv){try {
    if(argc<2){std::cerr<<"usage: MAP [LAYER]\n";return 2;}
    const unsigned layer=argc>2 && std::string(argv[2])!="all" && std::string(argv[2])!="model" && std::string(argv[2])!="functions"?unsigned(std::stoul(argv[2])):17u;
    const Cache cache(argv[1]);
    if(argc>2 && std::string(argv[2])=="functions") {
        // audit_shader_layer_passes MAP functions: every object function whose
        // input, enable or scale is the cache's builtin one/zero string.
        const auto name=[&](std::uint32_t id)->std::string{if(!id)return "-";try{return cache.string_id(id);}catch(const std::exception&){return "unresolved";}};
        for(const auto& tag:cache.tags()) {
            static const std::set<std::string> objects{"bipd","vehi","weap","eqip","garb","proj","scen","mach",
                "ctrl","lifi","ssce","crea","bloc"};
            if(!tag.address || !objects.contains(tag.group))continue;
            std::vector<ObjectFunction> functions;
            try{functions=cache.load_object_functions(tag.datum);}catch(const std::exception&){continue;}
            for(const auto& f:functions) {
                const auto in=name(f.input);
                if(in!="one" && in!="zero")continue;
                std::cout<<tag.group<<' '<<tag.name<<": "<<in<<"(0x"<<std::hex<<f.input<<std::dec<<") -> "<<name(f.output)
                    <<" scale="<<name(f.scale)<<" flags="<<f.flags<<'\n';
            }
        }
        return 0;
    }
    if(argc>3 && std::string(argv[2])=="model") {
        // audit_shader_layer_passes MAP model SUBSTRING: every section and part
        // of matching render models with its type, flags and shader.
        const std::string filter=argv[3];
        for(const auto& tag:cache.tags()) {
            if(tag.group!="mode" || !tag.address || tag.name.find(filter)==std::string::npos)continue;
            std::cout<<tag.name<<'\n';
            for(unsigned section=0;section<512;++section) {
                try {
                    const auto loaded=load_menu_world_source_model_section(cache,tag.datum,section);
                    std::cout<<"  section "<<section<<" class="<<(loaded.model_geometry_class()?int(*loaded.model_geometry_class()):-1)<<" vertices="<<loaded.vertex_count()<<'\n';
                    for(std::size_t part=0;part<loaded.parts().size();++part) {
                        const auto& p=loaded.parts()[part];std::string shader="NONE";
                        if(p.shader!=0xffffffffu){const auto& owner=cache.tag_at_index(p.shader&65535u);shader=owner.name.substr(owner.name.find_last_of('\\')+1);}
                        std::cout<<"    part "<<part<<" type="<<p.type<<" flags="<<std::hex<<p.flags<<std::dec<<" indices="<<p.index_count<<" material="<<p.material<<' '<<shader<<'\n';
                    }
                }catch(const std::exception& e){std::cout<<"  section "<<section<<" end: "<<e.what()<<'\n';break;}
            }
        }
        return 0;
    }
    if(argc>3) {
        // audit_shader_layer_passes MAP all SUBSTRING: every layer of every level.
        const std::string filter=argv[3];
        for(const auto& tag:cache.tags()) {
            if(tag.group!="shad" || !tag.address || tag.name.find(filter)==std::string::npos)continue;
            const auto m=cache.load_material(tag.datum,false,false);
            std::cout<<tag.name<<" template="<<m.template_name<<" specular="<<m.specular_brightness<<" ambient="<<m.ambient_brightness
                <<" switches="<<m.shader_switches[4]<<','<<m.shader_switches[5]<<'\n';
            // AUDIT_BITMAPS=1 adds each authored bitmap parameter with its
            // resolved tag, frame and loaded dimensions.
            if(std::getenv("AUDIT_BITMAPS"))for(std::size_t b=0;b<m.bitmaps.size();++b) {
                const auto& bitmap=m.bitmaps[b];std::cout<<"  bitmap "<<b<<' ';
                if(bitmap.datum==0xffffffffu){std::cout<<"NONE\n";continue;}
                const auto& owner_tag=cache.tag_at_index(bitmap.datum&65535u);const auto* owner=owner_tag.datum==bitmap.datum?&owner_tag:nullptr;
                std::cout<<(owner?owner->name:std::string("?"))<<'#'<<bitmap.index;
                try{const auto t=cache.load_bitmap(bitmap.datum,bitmap.index);std::cout<<' '<<t.width<<'x'<<t.height<<" format="<<t.format<<" layers="<<t.layers;}
                catch(const std::exception& e){std::cout<<" load error: "<<e.what();}
                std::cout<<'\n';
            }
            if(std::getenv("AUDIT_BITMAPS"))for(std::size_t o=0;o<m.animation.overlays.size();++o) {
                const auto& overlay=m.animation.overlays[o];
                const auto name=[&](std::uint32_t id)->std::string{if(!id)return "-";try{return cache.string_id(id);}catch(const std::exception&){return "unresolved";}};
                std::cout<<"  overlay "<<o<<" input="<<name(overlay.input)<<" range="<<name(overlay.range)<<" period="<<overlay.period<<" curve="<<std::hex;
                for(auto byte:overlay.curve)std::cout<<' '<<unsigned(byte);
                std::cout<<std::dec<<'\n';
            }
            for(std::size_t l=0;l<m.lods.size();++l) {
                const auto& lod=m.lods[l];
                std::cout<<"  level "<<l<<" threshold="<<lod.projected_height<<" available="<<std::hex<<lod.available_layers<<std::dec<<'\n';
                for(unsigned layer=0;layer<lod.layers.count();++layer) {
                    if(!(lod.available_layers&(1u<<layer)))continue;
                    const auto range=m.layers.at(lod.layers.first()+layer);
                    for(const MaterialPass& p:std::as_const(m.passes))if(p.template_pass>=range.first() && p.template_pass-range.first()<range.count())
                        std::cout<<"    layer "<<layer<<" impl "<<p.implementation<<" "<<p.name.substr(p.name.find("shader_passes")+14)<<" | "
                            <<p.vertex_shader.substr(p.vertex_shader.find_last_of('\\')+1)<<detail(p)<<'\n';
                }
            }
        }
        return 0;
    }
    std::map<std::string,unsigned> programs;
    for(const auto& tag:cache.tags()) {
        if(tag.group!="shad" || !tag.address)continue;
        Material m;
        try{m=cache.load_material(tag.datum,false,false);}catch(const std::exception&){continue;}
        if(m.lods.empty())continue;
        const auto& lod=m.lods.front();
        if(!(lod.available_layers&(1u<<layer)) || layer>=lod.layers.count() || lod.layers.first()+layer>=m.layers.size())continue;
        const auto range=m.layers[lod.layers.first()+layer];
        for(const MaterialPass& p:std::as_const(m.passes)) {
            if(p.template_pass<range.first() || p.template_pass-range.first()>=range.count() || p.implementation)continue;
            const auto key=p.name+" | "+p.vertex_shader;++programs[key];
            std::cout<<tag.name<<'\t'<<m.template_name<<'\t'<<p.name<<'\t'<<p.vertex_shader<<"\tstates:"<<std::hex;
            for(const auto& state:p.states)std::cout<<' '<<unsigned(state.index)<<'='<<state.value;
            std::cout<<" state_externs:";for(const auto& r:p.state_externs)std::cout<<' '<<r.destination<<':'<<unsigned(r.component)<<':'<<unsigned(r.source);
            std::cout<<" program:";for(auto w:p.program.words)std::cout<<' '<<w;
            std::cout<<std::dec<<'\n';
        }
    }
    std::cout<<"--- pass | vertex program: shaders\n";
    for(const auto& [key,count]:programs)std::cout<<count<<'\t'<<key<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
