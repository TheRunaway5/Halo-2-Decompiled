#include "halo2/cache.hpp"
#include "halo2/mcc_native_world_default_roles.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace halo2;
using Json = nlohmann::json;
constexpr std::array<std::pair<unsigned, unsigned>, 14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
void need(bool value, const std::string& why) { if(!value) throw FormatError(why); }
std::vector<std::byte> unhex(const std::string& text) {
    need(text.size()%2==0, "odd fixture hex extent");
    std::vector<std::byte> result;
    for(std::size_t at=0;at<text.size();at+=2)
        result.push_back(std::byte(std::stoul(text.substr(at,2),nullptr,16)));
    return result;
}
Tag tag(const Json& value) {
    return {value.at("group"),value.at("name"),value.at("datum"),value.at("address"),value.at("size")};
}
void word(std::vector<std::byte>& out, std::size_t at, std::uint32_t value) {
    Bytes(out).require(at,4);
    for(unsigned n=0;n<4;++n) out[at+n]=std::byte(value>>(8*n));
}
struct Fixture {
    Tag shader, source_stem, native_stem;
    std::string payload_name;
    std::vector<std::byte> payload;
    std::map<std::uint32_t,Tag> names;
    std::map<std::uint32_t,unsigned> frames;
    std::map<std::string,Texture> default_textures;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    explicit Fixture(const Json& value)
        :shader(tag(value.at("shader"))),source_stem(tag(value.at("source_mcc_stem"))),
         native_stem(tag(value.at("native_stem"))),payload_name(value.at("native_payload_name")),
         payload(unhex(value.at("native_payload_hex"))) {
        for(const auto& row:value.at("namespace")) { auto owned=tag(row);names.emplace(owned.datum,std::move(owned)); }
        for(const auto& [id,count]:value.at("bitmap_frame_counts").items()) frames.emplace(std::stoul(id),count.get<unsigned>());
        for(const auto& row:value.at("regions")) regions.emplace_back(row.at(0),unhex(row.at(1)));
        const auto& resource=value.at("default_bitmap_resource");Texture texture;
        texture.width=resource.at("width");texture.height=resource.at("height");texture.format=resource.at("format");
        texture.depth=resource.at("depth");texture.layers=resource.at("layers");texture.pixels=unhex(resource.at("pixels_hex"));
        std::size_t offset{};for(const auto& mip:resource.at("mips")) {
            texture.mips.push_back({mip.at("width"),mip.at("height"),offset,mip.at("bytes"),1});offset+=mip.at("bytes").get<std::size_t>();
        }
        default_textures.emplace(value.at("default_bitmap").at("name").get<std::string>(),std::move(texture));
    }
    std::span<const std::byte> read(std::uint32_t address,std::size_t size) const {
        if(!size)return {};
        for(const auto& [first,raw]:regions)
            if(address>=first && std::uint64_t(address)-first<=raw.size() && size<=raw.size()-(address-first))
                return std::span(raw).subspan(address-first,size);
        throw FormatError("world cooked fixture read escapes literal source banks");
    }
    void write_byte(std::uint32_t address,std::byte value) {
        bool found=false;
        for(auto& [first,raw]:regions)if(address>=first && std::uint64_t(address)-first<raw.size()) {
            raw[address-first]=value;found=true;
        }
        need(found,"mutation escaped literal fixture source");
    }
    void write_word(std::uint32_t address,std::uint32_t value) {
        for(unsigned n=0;n<4;++n)write_byte(address+n,std::byte(value>>(8*n)));
    }
    std::optional<MccOwnedShaderProjection> run() const {
        return project_mcc_world_default_roles(shader,source_stem,native_stem,payload_name,payload,
            [&](std::uint32_t at,std::size_t size){return read(at,size);},
            [&](std::uint32_t handle)->const Tag* {const auto it=names.find(handle);return it==names.end()?nullptr:&it->second;},
            [&](std::uint32_t handle,std::uint32_t index) {const auto it=frames.find(handle);return it!=frames.end()&&index<it->second;},
            [&](std::string_view name)->std::optional<MccWorldDefaultBitmap> {
                for(const auto& [handle,tag]:names)if(tag.group=="bitm"&&tag.name==name) {
                    const auto resource=default_textures.find(std::string(name));
                    if(resource==default_textures.end())return {};
                    return MccWorldDefaultBitmap{tag,resource->second};
                }
                return {};
            });
    }
};
std::span<const std::byte> bank(const MccOwnedShaderProjection& out,unsigned pp,unsigned field,unsigned stride) {
    const Bytes bytes(out.bytes);
    return bytes.slice(bytes.u32(pp+field+4),std::size_t(bytes.u32(pp+field))*stride);
}
void check(const Fixture& fixture,const MccOwnedShaderProjection& out,const Json& input,Json& counts) {
    const Bytes bytes(out.bytes);
    need(bytes.u32(32)==1,"projected platform definition count is not one");
    const auto pp=bytes.u32(36);
    std::set<std::uint32_t> relocations;
    for(const auto& [field,target]:out.pointers) {
        need(relocations.insert(field).second && std::uint64_t(field)+4<=out.bytes.size() &&
             target<out.bytes.size() && bytes.u32(field)==target,"relative pointer ownership differs");
    }
    const auto root=unhex(input.at("source_root_hex"));
    for(unsigned at=0;at<84;++at) {
        const bool changed=(at>=16&&at<20)||(at>=28&&at<40)||(at>=48&&at<52)||(at>=80&&at<84);
        if(!changed)need(out.bytes[at]==root[at],"unrelated source root field changed");
    }
    const auto runtime=unhex(input.at("expected_runtime_hex"));
    need(std::ranges::equal(bank(out,0,12,80),runtime),"literal runtime property bank changed");
    const auto predictions=unhex(input.at("expected_predictions_hex"));
    need(std::ranges::equal(bank(out,0,44,8),predictions),"literal predicted resource bank changed");
    for(const auto& [field,stride]:fields) {
        const auto expected=unhex(input.at("expected_banks").at(std::to_string(field)));
        need(expected.size()%stride==0 && bytes.u32(pp+field)==expected.size()/stride &&
             std::ranges::equal(bank(out,pp,field,stride),expected),
             "complete projected bank differs from authenticated role mapping: "+std::to_string(field));
        if(expected.empty())need(bytes.u32(pp+field+4)==0,"empty projected bank retained a pointer");
        counts["complete_banks_checked"]=counts["complete_banks_checked"].get<unsigned>()+1;
    }
    if(input.at("kind")=="paired-original-main") {
        const auto& original=input.at("original_main");
        const Bytes images(bank(out,pp,4,12));
        const auto& expected=original.at("images");
        need(bytes.u32(pp+4)==expected.size(),"original bitmap slot count differs");
        for(unsigned i=0;i<expected.size();++i) {
            const auto handle=images.u32(i*12);
            if(expected[i][0][0]=="NONE")need(handle==0xffffffffu,"original NONE image changed");
            else {
                const auto it=fixture.names.find(handle);
                need(it!=fixture.names.end() && it->second.group==expected[i][0][0].get<std::string>() &&
                     it->second.name==expected[i][0][1].get<std::string>(),"original bitmap semantic slot differs");
            }
            need(images.u32(i*12+4)==expected[i][1] && images.u32(i*12+8)==expected[i][2],
                 "original bitmap frame/log word differs");
        }
        need(std::ranges::equal(bank(out,pp,12,4),unhex(original.at("pixels_hex"))) &&
             std::ranges::equal(bank(out,pp,20,16),unhex(original.at("vectors_hex"))),
             "projected main constants differ from literal original counterpart");
        counts["original_main_banks_checked"]=counts["original_main_banks_checked"].get<unsigned>()+3;
    }
}
void rejects(const Fixture& input,const std::string& label,Json& counts) {
    need(!input.run(),"unproved source admitted: "+label);
    counts["rejections"]=counts["rejections"].get<unsigned>()+1;
}
void controls(const Fixture& original,const Json& input,Json& counts,bool exhaustive) {
    const Bytes root(original.read(original.shader.address,84));
    const auto pp=root.u32(36);
    const Bytes definition(original.read(pp,124));
    for(unsigned field:{0u,4u,12u,20u,24u,32u,56u,60u,76u}) {
        auto changed=original;
        changed.write_word(changed.shader.address+field,field==60?0xffffffffu:root.u32(field)^1u);
        rejects(changed,"root field "+std::to_string(field),counts);
    }
    for(const auto& [field,stride]:fields) {
        (void)stride;auto changed=original;
        changed.write_word(pp+field,definition.u32(field)+1);
        rejects(changed,"cooked bank count "+std::to_string(field),counts);
    }
    for(unsigned field:{20u,100u,108u})if(definition.u32(field)) {
        auto changed=original;changed.write_word(definition.u32(field+4),0x7fc00000u);
        rejects(changed,"nonfinite authored bank "+std::to_string(field),counts);
    }
    const auto image_address=definition.u32(8);
    const Bytes image(original.read(image_address,12));
    const auto handle=image.u32(0);
    need(handle!=0xffffffffu,"positive fixture has no first owned bitmap");
    {
        auto changed=original;changed.names.at(handle).group="shad";rejects(changed,"bitmap typed namespace",counts);
    }
    {
        auto changed=original;changed.write_word(image_address,handle^0x10000u);rejects(changed,"bitmap full-salt handle",counts);
    }
    {
        auto changed=original;changed.frames[handle]=0;rejects(changed,"actual bitmap frame ownership",counts);
    }
    {
        auto changed=original;changed.write_word(image_address+4,original.frames.at(handle));rejects(changed,"bitmap frame upper bound",counts);
    }
    {
        auto changed=original;changed.write_word(image_address+8,0x7fc00000u);rejects(changed,"bitmap log dimension",counts);
    }
    {
        auto changed=original;changed.source_stem.datum^=0x10000u;rejects(changed,"source stem full-salt identity",counts);
    }
    const auto default_handle=input.at("default_bitmap").at("datum").get<std::uint32_t>();
    {
        auto changed=original;changed.names.at(default_handle).name+="unproved";rejects(changed,"default bitmap exact typed source name",counts);
    }
    {
        auto changed=original;changed.names.at(default_handle).group="shad";rejects(changed,"default bitmap group",counts);
    }
    {
        auto changed=original;changed.names.at(default_handle).address=0;rejects(changed,"default bitmap residency",counts);
    }
    for(unsigned count:{0u,2u}) {
        auto changed=original;changed.frames[default_handle]=count;rejects(changed,"default bitmap exact frame ownership",counts);
    }
    const auto default_name=input.at("default_bitmap").at("name").get<std::string>();
    {
        auto changed=original;changed.default_textures.at(default_name).width*=2;rejects(changed,"default bitmap shape",counts);
    }
    {
        auto changed=original;changed.default_textures.at(default_name).mips.back().offset=0;rejects(changed,"default mip owner/order",counts);
    }
    const auto& pixels=original.default_textures.at(default_name).pixels;
    for(unsigned at=0;at<pixels.size();++at) {
        auto changed=original;std::vector<std::byte> raw(pixels.begin(),pixels.end());raw[at]^=std::byte{1};
        changed.default_textures.at(default_name).pixels=std::move(raw);rejects(changed,"default source mip byte",counts);
    }
    {
        auto changed=original;changed.native_stem.size+=1;changed.names.at(changed.native_stem.datum)=changed.native_stem;
        rejects(changed,"native source extent",counts);
    }
    {
        auto changed=original;changed.payload_name[0]=changed.payload_name[0]=='0'?'1':'0';rejects(changed,"native payload identity",counts);
    }
    {
        auto changed=original;changed.shader.name="fixture\\renamed_source_owner";changed.names.at(changed.shader.datum)=changed.shader;
        const auto result=changed.run();need(bool(result),"conversion relies on a material-name alias");
        check(changed,*result,input,counts);
        counts["name_independent_controls"]=counts["name_independent_controls"].get<unsigned>()+1;
    }
    if(!exhaustive)return;
    for(std::size_t at=0;at<original.payload.size();++at) {
        auto changed=original;changed.payload[at]^=std::byte{1};rejects(changed,"native payload byte "+std::to_string(at),counts);
    }
    std::set<std::uint32_t> mutated;
    for(const auto& region:input.at("stem_mutation_regions")) {
        const auto address=region.at("address").get<std::uint32_t>(),size=region.at("bytes").get<unsigned>();
        for(unsigned i=0;i<size;++i)if(mutated.insert(address+i).second) {
            auto changed=original;changed.write_byte(address+i,changed.read(address+i,1)[0]^std::byte{1});
            rejects(changed,"literal source stem/remap byte "+std::to_string(address+i),counts);
        }
    }
    for(const auto& [id,source]:original.names)if(source.group=="spas") {
        auto changed=original;changed.names.at(id).name+="-unproved";rejects(changed,"source remap typed pass identity",counts);break;
    }
}
}
int main(int argc,char** argv) {try {
    need(argc==2||argc==3,"expected owned literal fixture and optional JSON report");
    std::ifstream stream(argv[1]);need(bool(stream),"missing cooked fixture");const auto fixture=Json::parse(stream);
    need(fixture.at("schema")=="H2_MCC_NATIVE_WORLD_DEFAULT_ROLES_FIXTURE_V1","unsupported cooked fixture");
    Json counts={{"pc_geometry_owners",0u},{"paired_original_fixtures",0u},{"complete_banks_checked",0u},
        {"original_main_banks_checked",0u},{"rejections",0u},{"name_independent_controls",0u},
        {"scope",fixture.at("scope")}};
    std::set<std::string> tested_profiles;
    for(const auto& input:fixture.at("cases")) {
        Fixture source(input);const auto before=source.regions;const auto payload=source.payload;
        const auto projected=source.run();
        need(bool(projected),"actual cooked source rejected: "+input.at("map").get<std::string>()+":"+source.shader.name);
        check(source,*projected,input,counts);
        need(source.regions==before&&source.payload==payload,"projection mutated source input");
        const auto key=input.at("template").get<std::string>();
        controls(source,input,counts,tested_profiles.insert(key).second);
        const auto field=input.at("kind")=="pc-geometry-owner"?"pc_geometry_owners":"paired_original_fixtures";
        counts[field]=counts[field].get<unsigned>()+1;
    }
    need(counts.at("pc_geometry_owners")==fixture.at("expected_pc_geometry_owners") &&
         counts.at("pc_geometry_owners")==3 && tested_profiles.size()==2,"incomplete PC/profile source fixture census");
    counts["profiles"]=tested_profiles.size();counts["passed"]=true;
    if(argc==3) {
        const auto owner=std::getenv("HALO2_TEST_SCRATCH");need(owner&&*owner,"report requires owned scratch");
        const auto target=std::filesystem::absolute(argv[2]).lexically_normal();
        const auto relative=target.lexically_relative(std::filesystem::absolute(owner).lexically_normal());
        need(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!="..","report escapes owned scratch");
        std::ofstream(target)<<counts.dump(2)<<'\n';
    }
    std::cout<<counts.dump(2)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
