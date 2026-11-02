#include "halo2/cache.hpp"
#include "halo2/mcc_native_env_illum_three_channel.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>

namespace {
using namespace halo2;
using Json=nlohmann::json;
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
void need(bool value,const std::string& why){if(!value)throw FormatError(why);}
std::vector<std::byte> unhex(const std::string& text) {
    need(text.size()%2==0,"Odd literal fixture hex extent");
    std::vector<std::byte> result;
    for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
Tag tag(const Json& value){return {value.at("group"),value.at("name"),value.at("datum"),value.at("address"),value.at("size")};}
void word(std::vector<std::byte>& data,unsigned at,unsigned value) {
    Bytes(data).require(at,4);for(unsigned n=0;n<4;++n)data[at+n]=std::byte(value>>(n*8));
}
struct Fixture {
    Tag shader,source_stem,native_stem;
    std::string payload_name;
    std::vector<std::byte> payload;
    std::map<unsigned,Tag> names;
    std::map<unsigned,unsigned> frames;
    std::vector<std::pair<unsigned,std::vector<std::byte>>> regions;
    explicit Fixture(const Json& value):shader(tag(value.at("shader"))),source_stem(tag(value.at("source_mcc_stem"))),
        native_stem(tag(value.at("native_stem"))),payload_name(value.at("native_payload_name")),payload(unhex(value.at("native_payload_hex"))) {
        for(const auto& row:value.at("namespace")){auto owned=tag(row);names.emplace(owned.datum,std::move(owned));}
        for(const auto& [id,count]:value.at("bitmap_frame_counts").items())frames.emplace(std::stoul(id),count.get<unsigned>());
        for(const auto& row:value.at("regions"))regions.emplace_back(row.at(0),unhex(row.at(1)));
    }
    std::span<const std::byte> read(unsigned address,std::size_t size) const {
        if(!size)return {};
        for(const auto& [first,raw]:regions)if(address>=first&&std::uint64_t(address)-first<=raw.size()&&size<=raw.size()-(address-first))
            return std::span(raw).subspan(address-first,size);
        throw FormatError("Literal environment input read escaped its owned source allocations");
    }
    void byte(unsigned address,std::byte value) {
        bool found=false;
        for(auto& [first,raw]:regions)if(address>=first&&std::uint64_t(address)-first<raw.size()){
            raw[address-first]=value;found=true;
        }
        need(found,"Mutation escaped its literal source owner");
    }
    void put(unsigned address,unsigned value){for(unsigned n=0;n<4;++n)byte(address+n,std::byte(value>>(n*8)));}
    std::optional<MccOwnedShaderProjection> run() const {
        return project_mcc_env_illum_three_channel(shader,source_stem,native_stem,payload_name,payload,
            [&](unsigned at,std::size_t size){return read(at,size);},
            [&](unsigned handle)->const Tag*{const auto i=names.find(handle);return i==names.end()?nullptr:&i->second;},
            [&](unsigned handle,unsigned frame){const auto i=frames.find(handle);return i!=frames.end()&&frame<i->second;},
            [&](const Tag& alias,std::size_t minimum) {
                unsigned extent=0;
                for(const auto& [handle,owner]:names)if(handle!=alias.datum&&owner.address==alias.address&&owner.group==alias.group&&owner.size) {
                    if(extent&&extent!=owner.size)return false;extent=owner.size;
                }
                return extent>=minimum;
            });
    }
};
std::span<const std::byte> bank(const MccOwnedShaderProjection& value,unsigned pp,unsigned field,unsigned stride) {
    const Bytes b(value.bytes);return b.slice(b.u32(pp+field+4),std::size_t(b.u32(pp+field))*stride);
}
void check(const Fixture& fixture,const MccOwnedShaderProjection& value,const Json& input,Json& counts) {
    const Bytes b(value.bytes);need(b.u32(32)==1,"Native projection did not have one complete platform definition");
    const auto pp=b.u32(36);need(b.u32(pp)==fixture.native_stem.datum,"Native postprocess lost full-salt template ownership");
    std::set<unsigned> relocations;
    for(const auto& [field,target]:value.pointers)need(relocations.insert(field).second&&std::uint64_t(field)+4<=value.bytes.size()&&
        target<value.bytes.size()&&b.u32(field)==target,"Owned pointer relocation differs");
    const auto root=unhex(input.at("source_root_hex"));
    for(unsigned at=0;at<84;++at)if(!((at>=16&&at<20)||(at>=32&&at<40)||(at>=48&&at<52)||(at>=80&&at<84)))
        need(value.bytes[at]==root[at],"An unrelated authored shader-root field changed");
    need(std::ranges::equal(bank(value,0,12,80),unhex(input.at("expected_runtime_hex"))),"Authored runtime properties changed");
    need(std::ranges::equal(bank(value,0,44,8),unhex(input.at("expected_predictions_hex"))),"Authored predicted resources changed");
    for(const auto& [field,stride]:fields) {
        auto actual=std::vector<std::byte>(bank(value,pp,field,stride).begin(),bank(value,pp,field,stride).end());
        if(field==60)for(unsigned i=0;i<actual.size()/20;++i)word(actual,i*20+16,0);
        const auto expected=unhex(input.at("expected_banks").at(std::to_string(field)));
        need(actual==expected,"Complete native bank differs for field "+std::to_string(field));
        if(expected.empty())need(b.u32(pp+field+4)==0,"Empty native bank retained a pointer");
        if(input.contains("original_banks")&&field!=4&&field!=60)
            need(actual==unhex(input.at("original_banks").at(std::to_string(field))),"Complete independent original bank differs");
        counts["complete_banks_checked"]=counts["complete_banks_checked"].get<unsigned>()+1;
    }
    if(input.contains("original_main")) {
        const Bytes images(bank(value,pp,4,12));const auto& original=input.at("original_main").at("images");
        need(original.size()==b.u32(pp+4),"Original typed bitmap bank extent differs");
        for(unsigned i=0;i<original.size();++i) {
            const auto& owned=fixture.names.at(images.u32(i*12));
            need(original[i][0][0]==owned.group&&original[i][0][1]==owned.name&&original[i][1]==images.u32(i*12+4)&&
                original[i][2]==images.u32(i*12+8),"Original typed bitmap role/frame/scalar differs");
        }
    }
    const auto overlays=b.u32(pp+64);
    for(unsigned i=0;i<input.at("expected_overlays").size();++i) {
        const auto& o=input.at("expected_overlays")[i];const auto raw=unhex(o.at("curve_hex"));const auto at=overlays+i*20;
        need(b.u32(at)==o.at("input")&&b.u32(at+4)==o.at("range")&&b.u32(at+8)==o.at("period_word")&&b.u32(at+12)==raw.size()&&
            relocations.contains(at+16)&&std::ranges::equal(b.slice(b.u32(at+16),raw.size()),raw),"Nested authored clock curve or relocation differs");
        counts["nested_curves_checked"]=counts["nested_curves_checked"].get<unsigned>()+1;
    }
}
void controls(const Fixture& f,const Json& input,Json& counts,bool exhaustive) {
    const Bytes root(f.read(f.shader.address,84));const auto pp=root.u32(36);const Bytes d(f.read(pp,124));
    auto reject=[&](const Fixture& changed,const char* reason){need(!changed.run(),std::string("Unproved material admitted: ")+reason);
        counts["guard_cases"]=counts["guard_cases"].get<unsigned>()+1;};
    for(unsigned field:{0u,4u,12u,20u,24u,28u,32u,56u,60u,76u,80u}) {
        auto c=f;c.put(f.shader.address+field,field==60?0xffffffffu:root.u32(field)^1u);reject(c,"shader root contract");
    }
    for(const auto& [field,stride]:fields) {
        (void)stride;auto c=f;c.put(pp+field,d.u32(field)+1);reject(c,"complete bank count/allocation");
    }
    for(unsigned field:{20u,100u,108u}){auto c=f;c.put(d.u32(field+4),0x7fc00000u);reject(c,"nonfinite authored constant");}
    const auto images=d.u32(8),handle=Bytes(f.read(images,12)).u32(0);
    {auto c=f;c.put(images,handle^0x10000u);reject(c,"bitmap salt");}
    {auto c=f;c.names.at(handle).group="shad";reject(c,"bitmap typed namespace");}
    {auto c=f;c.frames[handle]=0;reject(c,"resident frame ownership");}
    {auto c=f;c.put(images+4,f.frames.at(handle));reject(c,"frame upper bound");}
    {auto c=f;c.put(images+8,0x7f800000u);reject(c,"nonfinite image scalar");}
    {auto c=f;c.source_stem.datum^=0x10000u;reject(c,"source template salt");}
    {auto c=f;c.names.at(c.shader.datum).address+=4;reject(c,"source shader allocation owner");}
    {auto c=f;c.native_stem.size+=1;c.names.at(c.native_stem.datum)=c.native_stem;reject(c,"native template extent");}
    {auto c=f;c.payload[0]^=std::byte{1};reject(c,"complete native payload hash");}
    {auto c=f;c.payload_name[0]=c.payload_name[0]=='0'?'1':'0';reject(c,"native payload identity");}
    {auto c=f;c.put(d.u32(48),Bytes(f.read(d.u32(48),4)).u32(0)^1u);reject(c,"source pass scheduling");}
    {auto c=f;c.byte(d.u32(32)+4,std::byte{1});reject(c,"authored LOD range");}
    {auto c=f;c.byte(d.u32(56)+90,std::byte{1});reject(c,"discarded shadow animation");}
    {
        auto c=f;c.shader.name="fixture\\renamed_authored_material";c.names.at(c.shader.datum)=c.shader;
        const auto value=c.run();need(bool(value),"Conversion depends on a material-name alias");check(c,*value,input,counts);
    }
    {
        auto c=f;c.put(d.u32(16)+8*4,0x12345678u);const auto value=c.run();need(bool(value),"Authored pixel value was fitted to a stock material");
        need(Bytes(bank(*value,Bytes(value->bytes).u32(36),12,4)).u32(9*4)==0x12345678u,"Authored pixel value was replaced");
    }
    {
        auto c=f;c.put(d.u32(24),0x3f123456u);const auto value=c.run();need(bool(value),"Authored vector value was fitted to a stock material");
        need(Bytes(bank(*value,Bytes(value->bytes).u32(36),20,16)).u32(0)==0x3f123456u,"Authored vector was replaced");
    }
    for(unsigned i=0;i<d.u32(60);++i) {
        const auto at=d.u32(64)+i*20;const Bytes overlay(f.read(at,20));
        for(unsigned field:{0u,4u,8u,12u,16u}) {
            auto c=f;c.put(at+field,field==8||field==16?0:field==12?4097:overlay.u32(field)^1u);
            reject(c,"clock input/period/curve extent owner");
        }
        auto c=f;c.put(at+8,0x40000000u);const auto value=c.run();need(bool(value),"Valid authored clock period was rejected");
        const Bytes out(value->bytes);need(out.u32(out.u32(out.u32(36)+64)+i*20+8)==0x40000000u,"Authored period was replaced");
    }
    if(d.u32(84)) {
        auto c=f;c.byte(d.u32(88)+3,std::byte{255});reject(c,"animation parameter upper bound");
        c=f;c.byte(d.u32(88),std::byte{255});reject(c,"unpaired animation destination");
    }
    if(exhaustive)for(const auto& region:input.at("stem_mutation_regions")) {
        const auto at=region.at("address").get<unsigned>(),size=region.at("bytes").get<unsigned>();
        for(unsigned n=0;n<size;++n){auto c=f;c.byte(at+n,f.read(at+n,1)[0]^std::byte{1});reject(c,"complete compiler graph byte");}
    }
}
}
int main(int argc,char** argv)try {
    need(argc==3,"Expected owned literal fixture and bounded output report");
    need(std::filesystem::file_size(argv[1])<=32*1024*1024,"Literal metadata fixture exceeds bound");
    Json fixture;std::ifstream(argv[1])>>fixture;
    need(fixture.at("schema")=="H2_MCC_NATIVE_ENV_ILLUM_THREE_CHANNEL_FIXTURE_V1","Wrong literal fixture schema");
    Json report={{"complete_banks_checked",0},{"nested_curves_checked",0},{"guard_cases",0}};
    std::set<std::string> variants;unsigned authored=0,paired=0,controlled=0,aliases=0;
    bool animated_control=false,unanimated_control=false;
    for(const auto& input:fixture.at("cases")) {
        Fixture f(input);const auto preserved=f.regions;const auto value=f.run();
        need(bool(value),"Authored environment material has no native conversion: "+input.at("map").get<std::string>()+":"+f.shader.name);
        check(f,*value,input,report);need(f.regions==preserved,"Borrowed source metadata was modified");
        if(!f.shader.size) {
            ++aliases;const auto owner=tag(input.at("source_alias_owner"));
            auto c=f;c.names.erase(owner.datum);need(!c.run(),"Alias with no typed allocation owner was admitted");
            c=f;c.names.at(owner.datum).group="bitm";need(!c.run(),"Alias with a wrong-group allocation owner was admitted");
            c=f;c.names.at(owner.datum).address+=4;need(!c.run(),"Alias with a neighboring allocation owner was admitted");
            c=f;c.names.at(owner.datum).size=83;need(!c.run(),"Alias with a truncated owned payload was admitted");
            report["guard_cases"]=report["guard_cases"].get<unsigned>()+4;
        }
        variants.insert(f.payload_name);const auto kind=input.at("kind").get<std::string>();
        if(kind!="native-template-variant")++authored;
        if(kind=="paired-original-main")++paired;
        const bool animated=!input.at("expected_overlays").empty();
        if((animated&&!animated_control)||(!animated&&!unanimated_control)) {
            controls(f,input,report,animated);if(animated)animated_control=true;else unanimated_control=true;++controlled;
        }
    }
    need(authored==164&&paired==114&&variants.size()==39&&controlled==2,"Incomplete authored/native/control cohort");
    report.update({{"authored_rows",authored},{"original_paired_rows",paired},{"native_template_variants",variants.size()},
        {"cases",fixture.at("cases").size()},{"typed_shader_aliases",aliases},{"passed",true},{"native_projection_admitted",false},{"GPU_qualified",false}});
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
