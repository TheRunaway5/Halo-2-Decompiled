#include "halo2/cache.hpp"
#include "halo2/mcc_native_shaders.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace halo2;
using Json = nlohmann::json;
constexpr std::array<std::pair<unsigned,unsigned>,5> template_fields{{
    {0,10},{8,2},{16,10},{24,6},{32,4}}};
constexpr std::array<std::pair<unsigned,unsigned>,14> material_fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
void need(bool ok,const std::string& why) { if(!ok)throw FormatError(why); }
std::vector<std::byte> unhex(const std::string& text) {
    need(text.size()%2==0,"Odd fixture hex extent");
    std::vector<std::byte> out;
    for(std::size_t at=0;at<text.size();at+=2)
        out.push_back(std::byte(std::stoul(text.substr(at,2),nullptr,16)));
    return out;
}
void word(std::vector<std::byte>& out,std::size_t at,std::uint32_t value) {
    Bytes(out).require(at,4);
    for(unsigned n=0;n<4;++n)out[at+n]=std::byte(value>>(8*n));
}
Tag tag(const Json& value) {
    return {value.at("group"),value.at("name"),value.at("datum"),value.at("address"),value.at("size")};
}
std::vector<std::byte> typed_bank(std::span<const std::byte> raw,unsigned pp,unsigned field,unsigned stride) {
    const Bytes bytes(raw);
    const auto bank=bytes.slice(bytes.u32(pp+field+4),std::size_t(bytes.u32(pp+field))*stride);
    return {bank.begin(),bank.end()};
}
void owned_pointers(const MccOwnedShaderProjection& out) {
    std::set<std::uint32_t> seen;
    for(const auto& [field,target]:out.pointers)
        need(seen.insert(field).second&&std::uint64_t(field)+4<=out.bytes.size()&&
            target<out.bytes.size()&&Bytes(out.bytes).u32(field)==target,"Projection lost relative pointer ownership");
}
struct Fixture {
    Tag shader,source_stem,native_stem,resident_pass,resident_vertex;
    std::map<std::uint32_t,Tag> names;
    std::map<std::uint32_t,unsigned> frames;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    Fixture(const Json& input,const Json& references,std::uint32_t base=0x50000000u)
        :shader(tag(input.at("shader"))),source_stem(tag(input.at("source_mcc_stem"))),
         native_stem(tag(input.at("native_stem"))) {
        for(const auto& row:input.at("namespace")) { auto owner=tag(row);names.emplace(owner.datum,owner); }
        for(const auto& [handle,count]:input.at("bitmap_frame_counts").items())
            frames.emplace(std::stoul(handle),count.get<unsigned>());
        for(const auto& row:input.at("regions"))regions.emplace_back(row.at(0),unhex(row.at(1)));
        // Rebind the existing pass to the literal PC graph's complete handle.
        // Its code/state remain the complete preexisting resident payload.
        const auto& pass_record=references.at("pass").at("record");
        const auto& vertex_record=references.at("vertex").at("record");
        for(const auto& [handle,owner]:names)if(owner.group=="spas"&&owner.name==pass_record.at("name").get<std::string>()) {
            need(!resident_pass.address,"Ambiguous literal PC pass owner");
            resident_pass={owner.group,owner.name,handle,base,pass_record.at("bytes")};
        }
        need(resident_pass.address!=0,"Literal PC graph has no resident typed pass");
        resident_vertex={vertex_record.at("group"),vertex_record.at("name"),0x7ab51234u,base+0x10000u,vertex_record.at("bytes")};
        need(!names.contains(resident_vertex.datum),"Fixture vertex handle collides with literal namespace");
        const auto install_program=[&](const Json& reference,const Tag& owner) {
            auto bytes=unhex(reference.at("raw_hex"));
            need(bytes.size()==owner.size,"Resident fixture program extent differs");
            for(const auto& pointer:reference.at("record").at("pointers")) {
                const auto field=pointer.at(0).get<unsigned>(),target=pointer.at(1).get<unsigned>();
                need(Bytes(bytes).u32(field)==target&&target<bytes.size(),"Reference pointer is not payload relative");
                word(bytes,field,owner.address+target);
            }
            if(owner.group=="spas")word(bytes,380,resident_vertex.datum);
            names[owner.datum]=owner;regions.emplace_back(owner.address,std::move(bytes));
        };
        install_program(references.at("vertex"),resident_vertex);
        install_program(references.at("pass"),resident_pass);
        names[source_stem.datum]=source_stem;
        native_stem.address=base+0x20000u;
    }
    std::span<const std::byte> read(std::uint32_t address,std::size_t size) const {
        if(!size)return {};
        for(const auto& [first,raw]:regions)
            if(address>=first&&std::uint64_t(address)-first<=raw.size()&&size<=raw.size()-(address-first))
                return std::span(raw).subspan(address-first,size);
        throw FormatError("Resident reflection read escapes its owned literal fixture");
    }
    void write_byte(std::uint32_t address,std::byte value) {
        bool found=false;
        for(auto& [first,raw]:regions)if(address>=first&&std::uint64_t(address)-first<raw.size()) {
            raw[address-first]=value;found=true;
        }
        need(found,"Mutation escapes owned fixture");
    }
    void write_word(std::uint32_t address,std::uint32_t value) {
        for(unsigned n=0;n<4;++n)write_byte(address+n,std::byte(value>>(8*n)));
    }
    void flip(std::uint32_t address) {const auto value=read(address,1)[0]^std::byte{1};write_byte(address,value);}
    const Tag* find(std::uint32_t handle) const {
        const auto found=names.find(handle);return found==names.end()?nullptr:&found->second;
    }
    std::optional<MccOwnedShaderProjection> project_template() const {
        return project_mcc_resident_one_alpha_env_illum_template(source_stem,resident_pass,resident_vertex,
            [&](auto at,auto size){return read(at,size);},[&](auto handle){return find(handle);});
    }
    std::optional<MccOwnedShaderProjection> project_material() const {
        return project_mcc_resident_one_alpha_env_illum_material(shader,source_stem,native_stem,resident_pass,resident_vertex,
            [&](auto at,auto size){return read(at,size);},[&](auto handle){return find(handle);},
            [&](auto handle,auto index){const auto it=frames.find(handle);return it!=frames.end()&&index<it->second;});
    }
    void install(const MccOwnedShaderProjection& projection) {
        auto bytes=projection.bytes;
        for(const auto& [field,target]:projection.pointers)word(bytes,field,native_stem.address+target);
        native_stem.size=unsigned(bytes.size());names[native_stem.datum]=native_stem;
        regions.emplace_back(native_stem.address,std::move(bytes));
    }
};
void check_template(const Fixture& fixture,const MccOwnedShaderProjection& out,const Json& reference,Json& counts) {
    owned_pointers(out);
    need(out.bytes.size()==292&&out.pointers.size()==6,"Canonical metadata extent or pointer census differs");
    const auto expected=unhex(reference.at("raw_hex"));
    need(expected.size()==296,"Literal Dune metadata reference changed");
    auto root=std::vector<std::byte>(out.bytes.begin(),out.bytes.begin()+96);
    auto expected_root=std::vector<std::byte>(expected.begin(),expected.begin()+96);
    word(root,92,0);word(expected_root,92,0);
    need(root==expected_root&&Bytes(out.bytes).u32(88)==1,"Canonical template root differs from literal Dune reference");
    const auto pp=Bytes(out.bytes).u32(92),expected_pp=Bytes(expected).u32(92);
    std::set<std::uint32_t> expected_pointers{92};
    for(const auto& [field,stride]:template_fields) {
        expected_pointers.insert(pp+field+4);
        auto actual=typed_bank(out.bytes,pp,field,stride),wanted=typed_bank(expected,expected_pp,field,stride);
        need(Bytes(out.bytes).u32(pp+field)==Bytes(expected).u32(expected_pp+field),"Canonical typed template bank shape differs");
        if(field==16) {
            need(actual.size()==10&&Bytes(actual).u32(0)==0x73706173u&&
                Bytes(actual).u32(4)==fixture.resident_pass.datum,"Projected pass lost its complete typed resident handle");
            const auto offset=Bytes(expected).u32(expected_pp+field+4)+4;
            const auto& refs=reference.at("record").at("references");
            need(refs.size()==1&&refs.at(0).at(0)==offset&&
                refs.at(0).at(1).at(0).get<std::string>()==fixture.resident_pass.group&&
                refs.at(0).at(1).at(1).get<std::string>()==fixture.resident_pass.name,"Literal Dune pass type or name differs");
            word(actual,4,0);word(wanted,4,0);
        }
        need(actual==wanted,"Canonical typed template bank differs from literal Dune reference: "+std::to_string(field));
        counts["template_banks_checked"]=counts["template_banks_checked"].get<unsigned>()+1;
    }
    std::set<std::uint32_t> actual_pointers;
    for(const auto& [field,target]:out.pointers){(void)target;actual_pointers.insert(field);}
    need(actual_pointers==expected_pointers,"Canonical pointer list does not own exactly the template banks");
    const auto pixels=typed_bank(out.bytes,pp,32,4);
    constexpr std::array<std::uint32_t,4> remaps{{0x00040000u,0x01030400u,0x02040400u,0x03040040u}};
    for(unsigned n=0;n<remaps.size();++n)
        need(Bytes(pixels).u32((n+4)*4)==remaps[n],"Canonical resident pixel register remap differs");
}
void check_material(const Fixture& fixture,const MccOwnedShaderProjection& out,const Json& input,Json& counts) {
    owned_pointers(out);
    const Bytes bytes(out.bytes);
    need(bytes.u32(0)==0x7374656du&&bytes.u32(4)==fixture.native_stem.datum&&bytes.u32(32)==1,
        "Material lost canonical full-salt template ownership");
    const auto pp=bytes.u32(36);
    for(const auto& [field,stride]:material_fields) {
        const auto expected=unhex(input.at("expected_banks").at(std::to_string(field)));
        need(expected.size()%stride==0&&bytes.u32(pp+field)==expected.size()/stride&&
            typed_bank(out.bytes,pp,field,stride)==expected,"Complete paired material bank differs: "+std::to_string(field));
        if(expected.empty())need(bytes.u32(pp+field+4)==0,"Empty material bank retained a pointer");
        counts["material_banks_checked"]=counts["material_banks_checked"].get<unsigned>()+1;
    }
    need(typed_bank(out.bytes,0,12,80)==unhex(input.at("expected_runtime_hex"))&&
        typed_bank(out.bytes,0,44,8)==unhex(input.at("expected_predictions_hex")),"Runtime/predicted-resource bank differs");
    const auto source_root=unhex(input.at("source_root_hex"));
    for(unsigned at=0;at<84;++at) {
        const bool changed=(at>=16&&at<20)||(at>=28&&at<40)||(at>=48&&at<52)||(at>=80&&at<84);
        if(!changed)need(out.bytes[at]==source_root[at],"Unrelated material root byte changed");
    }
    const auto source_pp=Bytes(fixture.read(fixture.shader.address,84)).u32(36);
    const Bytes source_definition(fixture.read(source_pp,124));
    const Bytes source_pixels(fixture.read(source_definition.u32(16),16));
    const auto pixels=typed_bank(out.bytes,pp,12,4);const Bytes actual_pixels(pixels);
    need(pixels.size()==16&&actual_pixels.u32(0)==source_pixels.u32(0)&&
        actual_pixels.u32(4)==source_pixels.u32(8)&&actual_pixels.u32(8)==source_pixels.u32(4)&&
        actual_pixels.u32(12)==source_pixels.u32(12),"Environment pixel roles were not refolded exactly");
    if(input.contains("original_main")) {
        need(pixels==unhex(input.at("original_main").at("pixels_hex"))&&
            typed_bank(out.bytes,pp,20,16)==unhex(input.at("original_main").at("vectors_hex")),
            "Material constants differ from paired original main banks");
    }
}
void rejects(const Fixture& changed,const std::string& label,Json& counts,bool template_too=true) {
    if(template_too)need(!changed.project_template(),"Changed template graph admitted: "+label);
    need(!changed.project_material(),"Changed material graph admitted: "+label);
    counts["rejections"]=counts["rejections"].get<unsigned>()+1;
}
void controls(const Fixture& original,const Json& input,Json& counts,bool exhaustive) {
    for(const auto& owner:{original.resident_pass,original.resident_vertex}) {
        {auto changed=original;changed.names.at(owner.datum).group="bitm";rejects(changed,"resident owner group",counts);}
        {auto changed=original;changed.names.at(owner.datum).datum^=0x10000u;rejects(changed,"resident full-salt namespace",counts);}
        {auto changed=original;changed.names.at(owner.datum).name+="-unproved";rejects(changed,"resident owner name",counts);}
        {auto changed=original;changed.names.erase(owner.datum);rejects(changed,"missing resident owner",counts);}
    }
    {auto changed=original;changed.write_word(changed.resident_pass.address+380,changed.resident_vertex.datum^0x10000u);
        rejects(changed,"pass vertex full-salt reference",counts);}
    {auto changed=original;changed.source_stem.datum^=0x10000u;rejects(changed,"source template full salt",counts);}
    {auto changed=original;changed.native_stem.datum^=0x10000u;rejects(changed,"installed template full salt",counts,false);}
    {auto changed=original;changed.native_stem.size+=1;rejects(changed,"installed template extent",counts,false);}
    {auto changed=original;changed.names.at(changed.native_stem.datum).address+=4;
        rejects(changed,"installed template current namespace",counts,false);}
    {auto changed=original;changed.write_word(changed.source_stem.address+88,1);
        rejects(changed,"unsupported source platform shape",counts);}
    const auto source_pp=Bytes(original.read(original.source_stem.address,96)).u32(92);
    const auto pass_bank=Bytes(original.read(source_pp,40)).u32(20);
    {auto changed=original;changed.write_word(pass_bank+4,changed.resident_pass.datum^0x10000u);
        rejects(changed,"PC remap pass full-salt reference",counts);}
    const auto shader_pp=Bytes(original.read(original.shader.address,84)).u32(36);
    {auto changed=original;changed.write_word(changed.shader.address+32,2);
        rejects(changed,"unsupported authored material shape",counts,false);}
    {auto changed=original;changed.write_word(shader_pp+12,5);
        rejects(changed,"unsupported authored pixel count",counts,false);}
    {auto changed=original;const auto images=Bytes(changed.read(shader_pp,124)).u32(8);
        const auto handle=Bytes(changed.read(images,12)).u32(0);changed.frames[handle]=0;
        rejects(changed,"missing actual bitmap frame",counts,false);}
    if(!exhaustive)return;
    // Every resident byte is covered, including pass program, state, samplers,
    // vertex declarations/code and every relocated pointer word.
    for(const auto& owner:{original.resident_pass,original.resident_vertex})
        for(unsigned at=0;at<owner.size;++at) {
            auto changed=original;changed.flip(owner.address+at);
            rejects(changed,owner.group+" byte "+std::to_string(at),counts);
            const auto key=owner.group=="spas"?"resident_pass_bytes_rejected":"resident_vertex_bytes_rejected";
            counts[key]=counts[key].get<unsigned>()+1;
        }
    std::set<std::uint32_t> mutated;
    for(const auto& region:input.at("stem_mutation_regions")) {
        const auto address=region.at("address").get<std::uint32_t>(),size=region.at("bytes").get<unsigned>();
        for(unsigned at=0;at<size;++at)if(mutated.insert(address+at).second) {
            auto changed=original;changed.flip(address+at);
            rejects(changed,"literal PC state/remap byte "+std::to_string(address+at),counts);
            counts["source_graph_bytes_rejected"]=counts["source_graph_bytes_rejected"].get<unsigned>()+1;
        }
    }
    for(unsigned at=0;at<original.native_stem.size;++at) {
        auto changed=original;changed.flip(changed.native_stem.address+at);
        rejects(changed,"installed canonical metadata byte "+std::to_string(at),counts,false);
        counts["installed_template_bytes_rejected"]=counts["installed_template_bytes_rejected"].get<unsigned>()+1;
    }
}
}
int main(int argc,char** argv) {try {
    need(argc==2,"Expected the owned resident-reflection fixture");
    std::ifstream stream(argv[1]);need(bool(stream),"Missing owned resident-reflection fixture");
    const auto fixture=Json::parse(stream);
    need(fixture.at("schema")=="H2_MCC_RESIDENT_REFLECTION_COMPONENT_V1","Unsupported resident-reflection fixture");
    Json counts={{"scope",fixture.at("scope")},{"source_fixture_sha256",fixture.at("source_fixture_sha256")},
        {"cases",0u},{"pg_decal_cases",0u},{"template_banks_checked",0u},{"material_banks_checked",0u},
        {"rejections",0u},{"resident_pass_bytes_rejected",0u},{"resident_vertex_bytes_rejected",0u},
        {"source_graph_bytes_rejected",0u},{"installed_template_bytes_rejected",0u},{"maps",Json::array()}};
    bool exhaustive=false;
    for(const auto& input:fixture.at("cases")) {
        Fixture source(input,fixture);const auto before=source.regions;
        const auto projected=source.project_template();
        need(bool(projected),"Literal PC template rejected: "+input.at("map").get<std::string>());
        check_template(source,*projected,fixture.at("reference_template"),counts);
        need(source.regions==before,"Template projection mutated complete source/program allocations");
        Fixture relocated(input,fixture,0x60003000u);
        const auto relocated_projection=relocated.project_template();
        need(relocated_projection&&relocated_projection->bytes==projected->bytes&&
            relocated_projection->pointers==projected->pointers,"Resident input relocation changed canonical metadata");
        source.install(*projected);const auto installed_before=source.regions;
        const auto material=source.project_material();
        need(bool(material),"Literal PC material rejected after canonical template installation");
        check_material(source,*material,input,counts);
        need(source.regions==installed_before,"Material projection mutated source, programs or installed metadata");
        controls(source,input,counts,!exhaustive);exhaustive=true;
        counts["cases"]=counts["cases"].get<unsigned>()+1;
        if(source.shader.name.ends_with("\\pg_decal"))counts["pg_decal_cases"]=counts["pg_decal_cases"].get<unsigned>()+1;
        counts["maps"].push_back(input.at("map"));
    }
    need(counts.at("cases")==3&&counts.at("pg_decal_cases")==2&&
        counts.at("resident_pass_bytes_rejected")==576&&counts.at("resident_vertex_bytes_rejected")==5608&&
        counts.at("installed_template_bytes_rejected")==292,"Incomplete resident-reflection regression census");
    counts["passed"]=true;std::cout<<counts.dump(2)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
