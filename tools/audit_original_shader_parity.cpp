// Read-only comparison of every resident `shad` definition as the engine's
// own loader presents it from an original Xbox cache and from the selected
// MCC import. Both sides pass through Cache::load_material, so the report
// states what the Vulkan passes actually receive: template, layer/pass
// selection, combiner definition and assembled program, vertex program bytes
// and declaration, constants, remaps, externs, render and texture states and
// the authored animation. Bitmap operands are compared by tag path.
// Usage: audit_original_shader_parity OUTPUT_DIR XBOX_MAPS MCC_MAPS NAME.map...
#include "halo2/cache.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
using Dump=std::map<std::string,std::string>;
std::uint64_t fnv(const void* data,std::size_t size) {
    std::uint64_t h=14695981039346656037ull;const auto* p=static_cast<const unsigned char*>(data);
    for(std::size_t i=0;i<size;++i)h=(h^p[i])*1099511628211ull;return h;
}
std::string hex(std::uint64_t v){std::ostringstream s;s<<std::hex<<v;return s.str();}
std::string real(float v){std::uint32_t b;std::memcpy(&b,&v,4);std::ostringstream s;s<<v<<"/"<<std::hex<<b;return s.str();}
std::string vec(const ShaderVector& v){return real(v[0])+" "+real(v[1])+" "+real(v[2])+" "+real(v[3]);}
std::string remaps(const std::vector<ShaderRemap>& rows) {
    std::ostringstream s;for(const auto& r:rows)s<<r.destination<<':'<<unsigned(r.component)<<':'<<unsigned(r.source)<<' ';return s.str();
}
std::string words(std::span<const std::uint32_t> values) {
    std::ostringstream s;s<<std::hex;for(auto v:values)s<<v<<' ';return s.str();
}
std::string tag_name(const Cache& cache,std::uint32_t datum) {
    if(datum==0xffffffffu)return "NONE";
    const auto* tag=cache.find_tag({datum});return tag?tag->group+":"+tag->name:"UNRESOLVED";
}
std::string index(const char* name,std::size_t i){return std::string(name)+"["+std::to_string(i)+"]";}
Dump dump(const Cache& cache,std::uint32_t datum) {
    Dump out;Material m;
    try{m=cache.load_material(datum,false,false);}
    catch(const std::exception& e){out["error"]=e.what();return out;}
    out["template"]=m.template_name;
    out["switches"]=std::to_string(m.shader_switches[4])+" "+std::to_string(m.shader_switches[5]);
    out["brightness"]=real(m.specular_brightness)+" "+real(m.ambient_brightness);
    {std::ostringstream s;for(const auto& gap:m.rendering_gaps)s<<gap<<" | ";out["gaps"]=s.str();}
    out["lods"]=std::to_string(m.lods.size());
    for(std::size_t i=0;i<m.lods.size();++i)out[index("lod",i)]=std::to_string(m.lods[i].layers.packed)+" "+
        std::to_string(m.lods[i].available_layers)+" "+real(m.lods[i].projected_height);
    {std::ostringstream s;for(auto v:m.layers)s<<v.packed<<' ';out["layers"]=s.str();}
    {std::ostringstream s;for(auto v:m.pass_implementations)s<<v.packed<<' ';out["pass_implementations"]=s.str();}
    out["bitmaps"]=std::to_string(m.bitmaps.size());
    for(std::size_t i=0;i<m.bitmaps.size();++i) {
        out[index("bitmap",i)]=tag_name(cache,m.bitmaps[i].datum)+"#"+std::to_string(m.bitmaps[i].index);
        out[index("bitmap_log_dimension",i)]=real(m.bitmaps[i].log_dimension);
    }
    out["pixel_constants"]=words(m.pixel_constants);
    out["vertex_constants"]=std::to_string(m.vertex_constants.size());
    for(std::size_t i=0;i<m.vertex_constants.size();++i)out[index("vertex_constant",i)]=vec(m.vertex_constants[i]);
    const auto& a=m.animation;
    {std::ostringstream s;for(const auto& l:a.lods)s<<l.available_layers<<':'<<l.layers.packed<<' ';out["animation.lods"]=s.str();}
    {std::ostringstream s;for(auto v:a.layers)s<<v.packed<<' ';out["animation.layers"]=s.str();}
    {std::ostringstream s;for(auto v:a.passes)s<<v.packed<<' ';out["animation.passes"]=s.str();}
    {std::ostringstream s;for(auto v:a.parameters)s<<v.packed<<' ';out["animation.parameters"]=s.str();}
    {std::ostringstream s;for(const auto& v:a.implementations)s<<v.bitmap_transforms.packed<<':'<<v.pixel_constants.packed<<':'<<v.vertex_constants.packed<<' ';
        out["animation.implementations"]=s.str();}
    out["animation.overlays"]=std::to_string(a.overlays.size());
    // The function input is a string id; the two caches number their string
    // tables differently, so compare the resolved text.
    const auto input_name=[&](std::uint32_t id){try{return cache.string_id(id);}catch(const std::exception&){return "unresolved:"+std::to_string(id);}};
    for(std::size_t i=0;i<a.overlays.size();++i)out[index("animation.overlay",i)]=input_name(a.overlays[i].input)+" "+
        std::to_string(a.overlays[i].range)+" "+real(a.overlays[i].period)+" curve="+std::to_string(a.overlays[i].curve.size())+":"+
        hex(fnv(a.overlays[i].curve.data(),a.overlays[i].curve.size()));
    {std::ostringstream s;for(const auto& v:a.references)s<<v.function<<':'<<v.transform<<' ';out["animation.references"]=s.str();}
    out["animation.mappings"]=remaps(a.mappings);
    out["passes"]=std::to_string(m.passes.size());
    std::size_t n=0;
    for(const MaterialPass& p:std::as_const(m.passes)) {
        const auto key=[&](const char* name){return "pass["+std::to_string(n)+"]."+name;};
        out[key("name")]=p.name;out[key("vertex_shader")]=p.vertex_shader;
        out[key("slot")]=std::to_string(p.template_pass)+"/"+std::to_string(p.implementation);
        {std::ostringstream s;for(const auto& t:p.textures)s<<unsigned(t.parameter)<<':'<<unsigned(t.external)<<':'<<unsigned(t.stage)<<':'<<unsigned(t.flags)<<' ';
            out[key("textures")]=s.str();}
        {std::ostringstream s;s<<std::hex;for(const auto& t:p.states)s<<unsigned(t.index)<<'='<<t.value<<' ';out[key("states")]=s.str();}
        for(std::size_t t=0;t<p.texture_states.size();++t) {
            const auto& v=p.texture_states[t];std::ostringstream s;s<<std::hex<<v.address_u<<' '<<v.address_v<<' '<<v.address_w<<" filter "
                <<v.mag_filter<<' '<<v.min_filter<<' '<<v.mip_filter<<" bias "<<v.lod_bias<<" max "<<v.max_mip<<" aniso "<<v.anisotropy
                <<" key "<<v.color_key_op<<' '<<v.color_sign<<' '<<v.alpha_kill<<' '<<v.border_color<<' '<<v.key_color;
            out[key("texture_state")+"["+std::to_string(t)+"]"]=s.str();
        }
        out[key("texture_states")]=std::to_string(p.texture_states.size());
        out[key("bitmap_remaps")]=remaps(p.bitmap_remaps);out[key("pixel_remaps")]=remaps(p.pixel_remaps);
        out[key("vertex_remaps")]=remaps(p.vertex_remaps);out[key("pixel_externs")]=remaps(p.pixel_externs);
        out[key("vertex_externs")]=remaps(p.vertex_externs);out[key("state_externs")]=remaps(p.state_externs);
        for(unsigned bank=0;bank<2;++bank)for(unsigned c=0;c<16;++c) {
            const auto& v=p.constants[bank][c];if(v==ShaderVector{})continue;
            out[key("constant")+"["+std::to_string(bank)+"]["+std::to_string(c)+"]"]=vec(v);
        }
        out[key("program")]=words(p.program.words);out[key("definition")]=words(p.definition.words);
        {std::ostringstream s;for(const auto& f:p.fragments)s<<f.switch_index<<':'<<f.permutations.packed<<' ';out[key("fragments")]=s.str();}
        {std::ostringstream s;for(const auto& f:p.permutations)s<<f.value<<':'<<f.flags<<':'<<f.combiners.packed<<' ';out[key("permutations")]=s.str();}
        {std::ostringstream s;s<<p.combiners.size()<<':'<<hex(fnv(p.combiners.data(),p.combiners.size()*sizeof(ShaderCombiner)));out[key("combiners")]=s.str();}
        out[key("vertex_code")]=std::to_string(p.vertex_code.size())+":"+hex(fnv(p.vertex_code.data(),p.vertex_code.size()));
        {std::ostringstream s;if(p.vertex_declaration){s<<p.vertex_declaration->class_index<<':';for(auto v:p.vertex_declaration->semantics)s<<v<<' ';}
            else s<<"none";out[key("vertex_declaration")]=s.str();}
        ++n;
    }
    return out;
}
std::string category(std::string key) {
    std::string out;bool skip=false;
    for(char c:key){if(c=='[')skip=true;else if(c==']')skip=false;else if(!skip)out+=c;}
    return out;
}
std::string clean(std::string text){for(char& c:text)if(c=='\t'||c=='\n'||c=='\r')c=' ';if(text.size()>400)text=text.substr(0,400)+"...";return text;}
std::map<std::string,std::uint32_t> shaders(const Cache& cache) {
    std::map<std::string,std::uint32_t> out;
    for(const auto& tag:cache.tags())if(tag.group=="shad" && tag.address)out.emplace(tag.name,tag.datum);
    return out;
}
}
int main(int argc,char** argv){try {
    if(argc<5){std::cerr<<"usage: OUTPUT_DIR XBOX_MAPS MCC_MAPS NAME.map...\n";return 2;}
    const std::filesystem::path output=argv[1],xbox_root=argv[2],mcc_root=argv[3];
    std::filesystem::create_directories(output);
    std::ofstream rows(output/"differences.tsv"),presence(output/"presence.tsv"),summary(output/"summary.tsv");
    rows<<"map\tshader\ttemplate\tkey\txbox\tmcc\n";presence<<"map\tshader\tstate\tdetail\n";
    summary<<"map\txbox_shaders\tmcc_shaders\tcommon\tidentical\tdifferent\tmcc_only\txbox_only\tmcc_unconverted\tmcc_error\n";
    std::map<std::string,std::set<std::string>> categories; // category -> unique shader names
    std::map<std::string,std::string> examples;
    std::set<std::string> unique_common,unique_different,unique_unconverted;
    std::map<std::string,std::set<std::string>> unconverted_templates;
    for(int arg=4;arg<argc;++arg) {
        const std::string name=argv[arg];
        if(!std::filesystem::exists(xbox_root/name) || !std::filesystem::exists(mcc_root/name)) {
            std::cout<<name<<": skipped (not present on both sides)\n";continue;
        }
        const Cache xbox(xbox_root/name),mcc(mcc_root/name);
        const auto a=shaders(xbox),b=shaders(mcc);
        unsigned common=0,identical=0,different=0,mcc_only=0,xbox_only=0,unconverted=0,errors=0;
        for(const auto& [shader,datum]:a)if(!b.contains(shader)){++xbox_only;presence<<name<<'\t'<<shader<<"\txbox_only\t\n";}
        for(const auto& [shader,datum]:b) {
            const auto original=a.find(shader);
            const auto mine=dump(mcc,datum);
            const bool gap=mine.contains("gaps") && mine.at("gaps").find("MCC material metadata")!=std::string::npos;
            const bool error=mine.contains("error");
            const auto tmpl=mine.contains("template")?mine.at("template"):std::string();
            if(gap){++unconverted;unique_unconverted.insert(shader);unconverted_templates[tmpl].insert(shader);
                presence<<name<<'\t'<<shader<<"\tmcc_unconverted\t"<<tmpl<<'\n';}
            if(error){++errors;presence<<name<<'\t'<<shader<<"\tmcc_error\t"<<clean(mine.at("error"))<<'\n';}
            if(original==a.end()){++mcc_only;presence<<name<<'\t'<<shader<<"\tmcc_only\t"<<tmpl<<'\n';continue;}
            ++common;unique_common.insert(shader);
            const auto reference=dump(xbox,original->second);
            std::set<std::string> keys;for(const auto& [k,v]:reference)keys.insert(k);for(const auto& [k,v]:mine)keys.insert(k);
            bool differs=false;
            for(const auto& key:keys) {
                const auto x=reference.find(key),y=mine.find(key);
                const std::string left=x==reference.end()?"<absent>":x->second,right=y==mine.end()?"<absent>":y->second;
                if(left==right)continue;
                differs=true;const auto kind=category(key);
                rows<<name<<'\t'<<shader<<'\t'<<tmpl<<'\t'<<key<<'\t'<<clean(left)<<'\t'<<clean(right)<<'\n';
                if(categories[kind].insert(shader).second && !examples.contains(kind))
                    examples[kind]=name+" "+shader+" "+key+": xbox="+clean(left)+" mcc="+clean(right);
            }
            if(differs){++different;unique_different.insert(shader);}else ++identical;
        }
        summary<<name<<'\t'<<a.size()<<'\t'<<b.size()<<'\t'<<common<<'\t'<<identical<<'\t'<<different<<'\t'<<mcc_only<<'\t'
            <<xbox_only<<'\t'<<unconverted<<'\t'<<errors<<'\n';
        std::cout<<name<<": xbox="<<a.size()<<" mcc="<<b.size()<<" common="<<common<<" identical="<<identical<<" different="<<different
            <<" mcc_only="<<mcc_only<<" xbox_only="<<xbox_only<<" unconverted="<<unconverted<<" errors="<<errors<<std::endl;
    }
    std::ofstream totals(output/"categories.tsv");totals<<"category\tunique_shaders\texample\n";
    for(const auto& [kind,set]:categories)totals<<kind<<'\t'<<set.size()<<'\t'<<examples[kind]<<'\n';
    std::ofstream templates(output/"unconverted-templates.tsv");templates<<"template\tunique_shaders\texample\n";
    for(const auto& [tmpl,set]:unconverted_templates)templates<<tmpl<<'\t'<<set.size()<<'\t'<<*set.begin()<<'\n';
    std::cout<<"UNIQUE: common="<<unique_common.size()<<" different="<<unique_different.size()
        <<" unconverted="<<unique_unconverted.size()<<" categories="<<categories.size()<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
