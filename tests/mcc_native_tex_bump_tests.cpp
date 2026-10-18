#include "halo2/cache.hpp"
#include "halo2/mcc_native_shaders.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/object_lighting.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void require(bool test,const char* message){if(!test)throw FormatError(message);}
std::vector<std::byte> hex(std::string_view text){
    require(text.size()%2==0,"bad fixture hex");std::vector<std::byte> out;
    for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(std::string(text.substr(i,2)),nullptr,16)));
    return out;
}
void word(std::vector<std::byte>&b,std::size_t at,std::uint32_t value){
    Bytes(b).require(at,4);for(unsigned i=0;i<4;++i)b[at+i]=std::byte(value>>(i*8));
}
Tag tag(const Json& j){return {j.at("group"),j.at("name"),j.at("datum"),j.at("address"),j.at("size")};}
struct Fixture {
    Tag shader,stem;
    std::string payload_name;
    std::vector<std::byte> payload;
    std::map<std::uint32_t,Tag> names;
    std::map<std::uint32_t,unsigned> frames;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    std::optional<Tag> default_tag;Texture default_texture;
    Fixture(const Json& root,const Json& j):shader(tag(j.at("shader"))),stem(tag(j.at("native_stem"))),
        payload_name(root.at("native_payload_name")),payload(hex(root.at("native_payload_hex").get<std::string>())) {
        for(const auto& t:j.at("namespace")){auto value=tag(t);names.emplace(value.datum,std::move(value));}
        for(const auto& [key,value]:j.at("bitmap_frame_counts").items())frames.emplace(std::stoul(key),value.get<unsigned>());
        for(const auto& r:j.at("regions"))regions.emplace_back(r.at(0),hex(r.at(1).get<std::string>()));
        if(j.contains("default_detail_tag")) {
            default_tag=tag(j.at("default_detail_tag"));const auto& t=j.at("default_detail_texture");
            default_texture.width=t.at("width");default_texture.height=t.at("height");
            default_texture.format=t.at("format");default_texture.layers=t.at("layers");default_texture.depth=t.at("depth");
            for(const auto& b:t.at("pixels"))default_texture.pixels.push_back(std::byte(b.get<unsigned>()));
            for(const auto& m:t.at("mips"))default_texture.mips.push_back({m[0],m[1],m[2],m[3],m[4]});
        }
    }
    std::span<const std::byte> read(std::uint32_t at,std::size_t n)const {
        if(!n)return {};
        for(const auto& [base,data]:regions)if(at>=base && std::uint64_t(at)-base<=data.size() && n<=data.size()-(at-base))
            return std::span(data).subspan(at-base,n);
        throw FormatError("fixture source outside retained region");
    }
    void write(std::uint32_t at,std::uint32_t value) {
        bool changed=false;
        for(auto& [base,data]:regions)if(at>=base && std::uint64_t(at)-base+4<=data.size()) {
            word(data,at-base,value);changed=true;
        }
        require(changed,"negative mutation outside source");
    }
    std::optional<MccOwnedShaderProjection> run()const {
        const MccShaderRead source=[&](std::uint32_t at,std::size_t n){return read(at,n);};
        const MccShaderFind find=[&](std::uint32_t handle)->const Tag*{const auto i=names.find(handle);return i==names.end()?nullptr:&i->second;};
        const MccShaderBitmapFrame frame=[&](std::uint32_t handle,std::uint32_t index){const auto i=frames.find(handle);return i!=frames.end() && index<i->second;};
        if(default_tag)return project_mcc_unanimated_tex_bump(shader,stem,payload_name,payload,source,find,frame,{&*default_tag,&default_texture});
        return project_mcc_unanimated_tex_bump(shader,stem,payload_name,payload,source,find,frame);
    }
};
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
std::span<const std::byte> bank(const MccOwnedShaderProjection& out,unsigned pp,unsigned field,unsigned stride) {
    const Bytes bytes(out.bytes);return bytes.slice(bytes.u32(pp+field+4),std::size_t(bytes.u32(pp+field))*stride);
}
void check(const Fixture& f,const MccOwnedShaderProjection& out,const Json& j,unsigned& original_banks,unsigned& main_banks) {
    const bool restores=bool(f.default_tag);
    const Bytes b(out.bytes);const Bytes source_root(f.read(f.shader.address,84));
    require(b.u32(32)==1,"projected definition count is not Xbox-shaped");const auto pp=b.u32(36);
    const auto source_pp=source_root.u32(36);const Bytes source_definition(f.read(source_pp,124));
    const auto before=f.regions;
    std::set<unsigned> pointer_fields;
    for(const auto& [field,target]:out.pointers){
        require(pointer_fields.insert(field).second,"duplicate canonical pointer field");
        require(target<out.bytes.size() && b.u32(field)==target,"canonical relative pointer ownership mismatch");
    }
    for(unsigned i=0;i<84;++i) {
        const bool changed=(i>=16 && i<20)||(i>=28 && i<40)||(i>=48 && i<52)||(i>=80 && i<84)||(restores && i>=44 && i<48);
        if(!changed)require(out.bytes[i]==f.read(f.shader.address+i,1)[0],"unrelated shader root byte changed");
    }
    require(std::ranges::equal(bank(out,0,12,80),f.read(source_root.u32(16),80)),"runtime property bytes changed");
    if(!restores)require(std::ranges::equal(bank(out,0,44,8),f.read(source_root.u32(48),std::size_t(source_root.u32(44))*8)),"predicted resource bytes changed");
    else if(!j.at("expected_original_predictions").is_null()) {
        const Bytes predicted(bank(out,0,44,8));const auto& expected=j.at("expected_original_predictions");
        require(b.u32(44)==expected.size(),"canonical predicted resource count differs from original");
        for(unsigned i=0;i<expected.size();++i) {
            const auto h=predicted.u32(i*8+4);const auto it=f.names.find(h);
            require(it!=f.names.end() && predicted.u16(i*8)==expected[i][0] && predicted.u16(i*8+2)==expected[i][1] &&
                it->second.group==expected[i][2].get<std::string>() && it->second.name==expected[i][3].get<std::string>(),"canonical predicted resource order/identity differs from original");
        }
    }
    for(const auto [field,stride]:fields) {
        const auto output=bank(out,pp,field,stride);
        if(field!=44 && field!=52 && !(restores && (field==4 || field==20 || field==92))) {
            const auto count=source_definition.u32(field);
            require(std::ranges::equal(output,count?f.read(source_definition.u32(field+4),std::size_t(count)*stride):std::span<const std::byte>{}),
                "donor material data bytes changed");
        }
        if(!j.at("expected_original_banks").is_null() && field>=28) {
            std::vector<std::byte> expected;
            for(const auto& value:j.at("expected_original_banks").at(std::to_string(field))) {
                auto bytes=hex(value.get<std::string>());expected.insert(expected.end(),bytes.begin(),bytes.end());
            }
            require(std::ranges::equal(output,expected),"projected auxiliary bank differs from actual original Xbox bank");++original_banks;
        }
    }
    if(restores) {
        const auto output_images=bank(out,pp,4,12),output_vectors=bank(out,pp,20,16);
        const auto source_images=f.read(source_definition.u32(8),5*12),source_vectors=f.read(source_definition.u32(24),6*16);
        require(std::ranges::equal(output_images.first(4*12),source_images.first(4*12)) &&
            std::ranges::equal(output_images.subspan(5*12),source_images.subspan(4*12)),"bitmap insertion changed another donor slot");
        require(std::ranges::equal(output_vectors.first(6*16),source_vectors),"detail transform insertion changed another donor vector");
        const Bytes images(output_images),vectors(output_vectors);
        require(images.u32(4*12)==f.default_tag->datum && images.u32(4*12+4)==0 && images.u32(4*12+8)==1064697328u,
            "missing detail bitmap did not retain original cooked words");
        require(vectors.u32(6*16)==0x3f800000u && vectors.u32(6*16+4)==0x3f800000u &&
            vectors.u32(6*16+8)==0 && vectors.u32(6*16+12)==0,"missing detail transform differs from original");
    }
    if(j.contains("expected_original_main") && !j.at("expected_original_main").is_null()) {
        const auto& main=j.at("expected_original_main");
        const Bytes images(bank(out,pp,4,12)),vectors(bank(out,pp,20,16));
        require(b.u32(pp+4)==main.at("bitmap").size(),"original main bitmap count differs");
        for(unsigned i=0;i<main.at("bitmap").size();++i) {
            const auto h=images.u32(i*12);const auto& wanted=main.at("bitmap")[i];
            require(images.u32(i*12+4)==wanted[1],"original main bitmap frame differs");
            if(wanted[0][0]=="NONE")require(h==0xffffffffu,"original NONE bitmap changed");
            else {
                const auto it=f.names.find(h);require(it!=f.names.end() && it->second.group==wanted[0][0].get<std::string>() && it->second.name==wanted[0][1].get<std::string>(),
                    "original main bitmap slot identity differs");
            }
        }
        ++main_banks;
        const Bytes pixels(bank(out,pp,12,4));
        require(b.u32(pp+12)==main.at("pixel").size(),"original main pixel count differs");
        for(unsigned i=0;i<main.at("pixel").size();++i)require(pixels.u32(i*4)==main.at("pixel")[i],"original main pixel word differs");
        ++main_banks;
        require(b.u32(pp+20)==main.at("vertex").size(),"original main vector count differs");
        for(unsigned i=0;i<main.at("vertex").size();++i)for(unsigned k=0;k<4;++k)
            require(vectors.u32(i*16+k*4)==main.at("vertex")[i][k],"original main vector word differs");
        ++main_banks;
    }
    require(b.u32(pp+52)==Bytes(f.payload).u32(120),"canonical implementation count differs from original");
    require(f.regions==before,"projection mutated source banks");
}
int check_variants(const Json& root,const std::filesystem::path& maps) {
    require(root.at("profiles").size()==6 && root.at("instances").size()==211,"variant fixture census changed");
    std::set<std::pair<unsigned,std::string>> unique;
    std::map<std::string,std::vector<const Json*>> by_map;
    unsigned projected=0,negatives=0,original_banks=0,main_banks=0,delivered=0,actual_materials=0;
    for(const auto& instance:root.at("instances")) {
        const auto id=instance.at("profile").get<unsigned>();
        auto source=root.at("profiles").at(id).at("source");
        const auto& record=instance.at("record");
        const auto filename=record.at("payload").get<std::string>();
        source["native_payload_name"]=filename;source["native_payload_hex"]=instance.at("payload_hex");
        auto authored=source.at("cases").front();
        authored["native_stem"]["size"]=record.at("bytes");
        for(auto& t:authored.at("namespace"))if(t.at("datum")==authored.at("native_stem").at("datum"))t=authored.at("native_stem");
        Fixture f(source,authored);const auto out=f.run();
        if(!out)std::cerr<<"Rejected exact native variant "<<instance.at("map")<<' '<<filename<<'\n';
        require(bool(out),"verified exact native payload variant was rejected");
        check(f,*out,authored,original_banks,main_banks);++projected;unique.emplace(id,filename);
        const auto reject=[&](const auto& mutate){auto bad=f;mutate(bad);require(!bad.run(),"unknown native variant admitted");++negatives;};
        reject([](auto& b){b.payload[0]^=std::byte{1};});
        reject([](auto& b){++b.stem.size;b.names.at(b.stem.datum).size=b.stem.size;});
        reject([](auto& b){b.stem.datum^=0x10000;});
        reject([](auto& b){b.payload_name[0]=b.payload_name[0]=='f'?'e':'f';});
        reject([](auto& b){b.payload.push_back(std::byte{});++b.stem.size;b.names.at(b.stem.datum).size=b.stem.size;});
        reject([](auto& b){b.stem.name+="_unknown";b.names.at(b.stem.datum).name=b.stem.name;});
        by_map[instance.at("map").get<std::string>()].push_back(&instance);
    }
    require(projected==211 && unique.size()==199 && by_map.size()==39,"exact native variant/map census changed");
    if(!maps.empty())for(const auto& [map,instances]:by_map) {
        Cache cache(maps/(map+".map"));require(cache.is_mcc(),"variant delivery fixture is not MCC");
        const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x80061000u,256u*1024*1024);
        require(delivery.issues().empty(),"native variant opened metadata closure");
        const auto image=delivery.tag_image();
        for(const auto* entry:instances) {
            const auto& record=entry->at("record");
            const auto name=record.at("name").get<std::string>();
            const auto stem=cache.query_tag({.group=TagGroup::from_string("stem"),.name=name});
            require(stem && stem->size==record.at("bytes") && cache.is_preserved_native_shader(*stem),
                "actual variant has no exact local native owner");
            const auto raw=hex(entry->at("payload_hex").get<std::string>());
            auto owned=cache.read_tag_bytes(stem->address,stem->size);
            std::vector<std::byte> normalized(owned.begin(),owned.end());const Bytes original(raw),published(owned);
            std::vector<std::pair<std::uint32_t,std::uint32_t>> pointers;
            for(const auto& pointer:record.at("pointers")) {
                const auto field=pointer.at(0).get<std::uint32_t>(),target=pointer.at(1).get<std::uint32_t>();
                require(published.u32(field)==stem->address+target,"local native variant pointer lost ownership");
                pointers.emplace_back(field,target);word(normalized,field,original.u32(field));
                const auto logical_field=delivery.runtime_address(stem->address+field,4);
                const auto logical_target=delivery.runtime_address(stem->address+target,stem->size-target);
                bool found=false;
                for(const auto& region:image.regions)if(logical_field>=region.address &&
                    std::uint64_t(logical_field)-region.address+4<=region.bytes.size()) {
                    require(Bytes(region.bytes).u32(logical_field-region.address)==logical_target,
                        "native variant delivery lost its typed pointer");found=true;break;
                }
                require(found,"native variant pointer field not delivered");
            }
            for(const auto& reference:record.at("references")) {
                const auto field=reference.at(0).get<unsigned>();
                const auto target=cache.query_tag({.handle=TagHandle{published.u32(field)}});
                require(target && target->group==reference.at(1).at(0).get<std::string>() &&
                    target->name==reference.at(1).at(1).get<std::string>(),"native variant lost its full local namespace binding");
                word(normalized,field,original.u32(field));
            }
            for(const auto& text:record.at("strings")) {
                const auto field=text.at(0).get<unsigned>();
                require(cache.string_id(published.u32(field))==text.at(1).get<std::string>(),"native variant lost its local StringID");
                word(normalized,field,original.u32(field));
            }
            require(normalized==raw,"native variant changed bytes outside declared typed fixups");
            const auto maps=cache.preserved_native_pointer_maps();
            const auto own=std::find_if(maps.begin(),maps.end(),[&](const auto& m){return m.owner==stem->handle();});
            require(own!=maps.end() && own->address==stem->address && own->bytes==stem->size && own->pointers==pointers,
                "native variant has no exact pointer-map receipt");++delivered;
        }
        constexpr std::array<std::string_view,6> extras{
            "objects\\weapons\\rifle\\gpmg\\shaders\\gpmg_gun",
            "objects\\weapons\\rifle\\smg_silenced\\shaders\\smg_rubber_grip",
            "objects\\weapons\\support_high\\flak_cannon\\shaders\\flak_cannon_shiny",
            "objects\\weapons\\rifle\\smg_silenced\\shaders\\smg_bare_metal",
            "objects\\weapons\\support_high\\flak_cannon\\shaders\\flak_cannon_lod",
            "objects\\weapons\\support_high\\flak_cannon\\shaders\\flak_cannon_glowing"};
        for(const auto* entry:instances) {
            const auto id=entry->at("profile").get<unsigned>();
            const auto shader=cache.query_tag({.group=TagGroup::from_string("shad"),.name=extras[id]});
            if(!shader || !shader->address)continue;
            require(cache.is_preserved_native_shader(*shader),"resident donor material did not use its exact local variant");
            auto material=cache.load_material(shader->datum);
            const auto source=root.at("profiles").at(id).at("source");
            const auto native=hex(source.at("native_payload_hex").get<std::string>());
            require(material.passes.size()==Bytes(native).u32(120),"resident variant has wrong native implementation count");
            require(prepare_dynamic_material(cache,material,{},true,0.f),"resident variant dynamic preparation failed");++actual_materials;
        }
    }
    require(maps.empty() || delivered==211,"lost actual native variant delivery case");
    std::cout<<"PASS "<<projected<<" exact native projections / "<<unique.size()<<" raw variants, "<<negatives
        <<" rejection guards; "<<delivered<<" actual native stem deliveries / "<<(maps.empty()?0:by_map.size())
        <<" MCC maps / "<<actual_materials<<" resident donor materials prepared\n";
    return 0;
}
}
int main(int argc,char**argv)try {
    if(argc<2 || !std::filesystem::is_regular_file(argv[1]))return 77;
    Json root;std::ifstream(argv[1])>>root;
    if(root.at("schema")=="H2_MCC_NATIVE_TEMPLATE_VARIANT_FIXTURE_V1")
        return check_variants(root,argc>=3?std::filesystem::path(argv[2]):std::filesystem::path{});
    require(root.at("schema")=="H2_MCC_TEX_BUMP_PRIVATE_FIXTURE_V1","wrong fixture schema");
    unsigned cases=0,paired=0,banks=0,main_banks=0,negatives=0;
    for(const auto& j:root.at("cases")) {
        Fixture f(root,j);const auto before=f.regions;auto out=f.run();
        if(!out){std::cerr<<"Rejected actual profile: "<<f.shader.name<<'\n';throw FormatError("actual unanimated tex_bump fixture rejected");}
        require(f.regions==before,"source banks were mutated");check(f,*out,j,banks,main_banks);++cases;
        if(!j.at("expected_original_banks").is_null())++paired;
    }
    if(root.contains("rejected_cases"))for(const auto& j:root.at("rejected_cases")) {
        Fixture animated(root,j);const auto before=animated.regions;
        const Bytes rr(animated.read(animated.shader.address,84));
        const Bytes d(animated.read(rr.u32(36),124));
        require(d.u32(60)==2 && d.u32(68)==2 && d.u32(76)==2 && d.u32(84)==4,
            "fixture lost its actual animated authoring contract");
        require(!animated.run() && animated.regions==before,"actual animated material was admitted or mutated");++negatives;
    }
    Fixture base(root,root.at("cases").front());const Bytes r(base.read(base.shader.address,84));const auto pp=r.u32(36);
    const auto negative=[&](const auto& mutate){auto bad=base;mutate(bad);require(!bad.run(),"changed/unsupported authoring was admitted");++negatives;};
    negative([](auto& f){f.shader.datum^=0x10000;});
    negative([](auto& f){f.shader.size=83;f.names.at(f.shader.datum).size=83;});
    negative([](auto& f){f.shader.name.clear();f.names.at(f.shader.datum).name.clear();});
    negative([](auto& f){f.shader.address=0x7ffff000;f.names.at(f.shader.datum).address=f.shader.address;});
    negative([](auto& f){f.stem.datum^=0x10000;});
    negative([](auto& f){f.stem.name+="_changed";});
    negative([](auto& f){f.payload[0]^=std::byte{1};});
    negative([](auto& f){f.payload_name[0]=f.payload_name[0]=='f'?'e':'f';});
    for(const auto [field,stride]:fields)negative([&](auto& f){const Bytes d(f.read(pp,124));f.write(pp+field,d.u32(field)+1);});
    for(unsigned field:{12u,20u,24u,32u,56u,60u,68u,76u})negative([&](auto& f){f.write(f.shader.address+field,field==68?0x7fc00000:0x100);});
    negative([&](auto&f){f.write(pp+124,1);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));f.write(d.u32(44+4),0);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));f.write(d.u32(52+4),1);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));f.write(d.u32(28+4),1);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));f.write(d.u32(36+4),0xffffffff);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));f.write(d.u32(92+4),0xfffffffe);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));f.write(d.u32(20+4),0x7fc00000);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));f.write(d.u32(4+4)+8,0x7fc00000);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));const auto images=d.u32(8);f.write(images,Bytes(f.read(images,4)).u32(0)^0x10000);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));f.write(d.u32(8)+4,0xffffffff);});
    negative([&](auto&f){const Bytes d(f.read(pp,124));const auto handle=Bytes(f.read(d.u32(8),4)).u32(0);f.names.at(handle).group="mode";});
    if(base.default_tag) {
        negative([](auto& f){f.default_tag.reset();});
        negative([](auto& f){f.default_tag->datum^=0x10000;});
        negative([](auto& f){f.default_tag->name+="_unknown";f.names.at(f.default_tag->datum).name=f.default_tag->name;});
        negative([](auto& f){f.default_tag->group="mode";f.names.at(f.default_tag->datum).group="mode";});
        negative([](auto& f){f.frames.erase(f.default_tag->datum);});
        negative([](auto& f){f.default_texture.width=8;});
        negative([](auto& f){f.default_texture.format=14;});
        negative([](auto& f){f.default_texture.layers=6;});
        negative([](auto& f){f.default_texture.depth=1;});
        negative([](auto& f){f.default_texture.pixels[0]^=std::byte{1};});
        negative([](auto& f){f.default_texture.pixels.pop_back();});
        negative([](auto& f){f.default_texture.mips.pop_back();});
        negative([](auto& f){++f.default_texture.mips[1].offset;});
        negative([](auto& f){--f.default_texture.mips[1].size;});
        negative([](auto& f){++f.default_texture.mips[1].depth;});
        negative([&](auto& f){const Bytes rr(f.read(f.shader.address,84));f.write(rr.u32(48)+4,f.default_tag->datum);});
        auto changed=base;const Bytes definition(changed.read(pp,124));
        changed.write(definition.u32(96),0xffff0004u);changed.write(definition.u32(16),4);
        const auto remapped=changed.run();require(bool(remapped),"typed source bitmap property could not be reindexed");
        const auto cooked_pp=Bytes(remapped->bytes).u32(36);
        require(Bytes(bank(*remapped,cooked_pp,92,4)).u16(0)==5 && Bytes(bank(*remapped,cooked_pp,12,4)).u32(0)==4,
            "typed property reindex changed an unrelated integer");
    }
    bool indexed_resource_checked=false;
    for(const auto& j:root.at("cases")) {
        Fixture resource(root,j);const Bytes rr(resource.read(resource.shader.address,84));
        const auto n=rr.u32(44),at=rr.u32(48);const Bytes rows(resource.read(at,n*8));
        for(unsigned i=0;i<n;++i)if(rows.u16(i*8+2)==3) {
            for(unsigned bad_index:{4u,0xffffu}) {
                auto bad=resource;bad.write(at+i*8,bad_index<<16);
                require(!bad.run(),"unowned predicted bitmap image was admitted");++negatives;
            }
            indexed_resource_checked=true;break;
        }
        if(indexed_resource_checked)break;
    }
    const std::string profile=root.at("native_payload_name");
    const bool active=profile=="c94dfefd08d9a0db13faceafea0f63a6639234d60e0d0161aca3ce10d6408d48.bin";
    const bool dprs=profile=="f484a4cc29b05bd636663693ec5f135577093babb7b815fedfaf7fe1f407188d.bin";
    const bool environment=profile=="b8beca003bc52dbd1ff2164698851b3f17a684e4d12ed606ce16bbf042e5f092.bin";
    const bool no_detail=profile=="93ce301299fc283f7ebbcdc735350dfe531aedf8bbff9cfa658446372ee54f3c.bin";
    const bool illum=profile=="d7e7a09658aadca654045509ac3a88041bbc32ec66688283890d92c72fabe0b9.bin";
    if(illum) {
        for(unsigned i=0;i<6;++i)negative([&](auto& f){const Bytes d(f.read(pp,124));f.write(d.u32(8)+i*12,0xffffffffu);});
        negative([&](auto& f){const Bytes d(f.read(pp,124));f.write(r.u32(16)+4,Bytes(f.read(d.u32(8)+24,4)).u32(0));});
    }
    if(!active && !dprs && !environment && !no_detail && !illum)require(indexed_resource_checked,"fixture lost real multi-image prediction");
    require(active ? cases==8 && paired==6 : dprs ? cases==6 && paired==5 :
        environment ? cases==29 && paired==27 : illum ? cases==2 && paired==1 && main_banks==3 && root.at("rejected_cases").size()==1 : no_detail ? cases==13 && paired==11 && main_banks==33 : cases==41 && paired==32,"fixture profile census changed");
    if(argc>=3 && std::filesystem::is_regular_file(argv[2])) {
        Cache cache(argv[2]);require(cache.is_mcc(),"actual integration donor is not MCC");
        const auto shader=cache.query_tag({.group=TagGroup::from_string("shad"),.name=illum?"objects\\weapons\\support_high\\flak_cannon\\shaders\\flak_cannon_glowing":active?"objects\\weapons\\rifle\\smg_silenced\\shaders\\smg_rubber_grip":dprs?"objects\\weapons\\support_high\\flak_cannon\\shaders\\flak_cannon_shiny":environment?"objects\\weapons\\rifle\\smg_silenced\\shaders\\smg_bare_metal":no_detail?"objects\\weapons\\support_high\\flak_cannon\\shaders\\flak_cannon_lod":"objects\\weapons\\rifle\\gpmg\\shaders\\gpmg_gun"});
        require(bool(shader),"actual gpmg shader is missing");
        require(cache.is_preserved_native_shader(*shader),"canonical material was not admitted after projection");
        const auto maps=cache.preserved_native_pointer_maps();
        const auto own=std::find_if(maps.begin(),maps.end(),[&](const auto& m){return m.owner==shader->handle();});
        require(own!=maps.end() && own->address==shader->address && own->bytes==shader->size,"canonical pointer map has no exact owner");
        const Bytes owned(cache.read_tag_bytes(shader->address,shader->size));
        std::set<std::uint32_t> seen;
        for(const auto& [field,target]:own->pointers) {
            require(seen.insert(field).second && field<=shader->size && 4<=shader->size-field && target<shader->size,
                "published pointer map escapes its canonical shader owner");
            require(owned.u32(field)==shader->address+target,"published pointer differs from typed relocation map");
        }
        const auto raw_case=std::find_if(root.at("cases").begin(),root.at("cases").end(),[&](const auto& j){return j.at("name")==shader->name;});
        require(raw_case!=root.at("cases").end(),"actual source fixture identity missing");
        Fixture raw(root,*raw_case);const auto expected=raw.run();require(bool(expected),"actual source fixture stopped projecting");
        auto expected_bytes=expected->bytes;
        for(const auto& [field,target]:expected->pointers)word(expected_bytes,field,shader->address+target);
        require(std::ranges::equal(cache.read_tag_bytes(shader->address,shader->size),expected_bytes),
            "actual Cache canonical shader differs from pure owned projection");
        require(own->pointers==expected->pointers,"actual Cache pointer map differs from pure projection");
        const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x80061000u,256u*1024*1024);
        require(delivery.issues().empty(),"canonical shader opened native metadata closure");
        const auto image=delivery.tag_image();
        for(const auto& [field,target]:own->pointers) {
            const auto logical_field=delivery.runtime_address(shader->address+field,4);
            const auto logical_target=delivery.runtime_address(shader->address+target,shader->size-target);
            bool found=false;
            for(const auto& region:image.regions)if(logical_field>=region.address &&
                std::uint64_t(logical_field)-region.address+4<=region.bytes.size()) {
                require(Bytes(region.bytes).u32(logical_field-region.address)==logical_target,
                    "native publication did not relocate canonical shader pointer");found=true;break;
            }
            require(found,"native publication lost canonical shader field");
        }

        auto material=cache.load_material(shader->datum);
        require(material.passes.size()==Bytes(base.payload).u32(120),"canonical native passes were not published");
        require(prepare_dynamic_material(cache,material,{},true,0.f),"actual dynamic native material was not prepared");
    }
    std::cout<<"PASS "<<cases<<" actual unanimated tex_bump profiles, "<<paired<<" paired original profiles / "<<banks
             <<" exact auxiliary banks / "<<main_banks<<" main banks, "<<negatives<<" rejection guards; owned data and original native passes\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
