#include "halo2/cache.hpp"
#include "halo2/mcc_native_world_animated.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
void need(bool value,const std::string& message){if(!value)throw FormatError(message);}
std::vector<std::byte> unhex(const std::string& text) {
    need(text.size()%2==0,"Incomplete literal sky fixture word");std::vector<std::byte> result;
    for(unsigned n=0;n<text.size();n+=2)result.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return result;
}
Tag tag(const Json& value){return {value.at("group"),value.at("name"),value.at("datum"),value.at("address"),value.at("size")};}
struct Fixture {
    Tag shader,source_stem,native_stem;std::string payload_name;
    std::vector<std::byte> payload;
    std::map<std::uint32_t,Tag> names;std::map<std::uint32_t,unsigned> frames;
    std::map<std::uint32_t,std::vector<std::byte>> regions;
    explicit Fixture(const Json& row):shader(tag(row.at("shader"))),source_stem(tag(row.at("source_mcc_stem"))),
        native_stem(tag(row.at("native_stem"))),payload_name(row.at("native_payload_name")),payload(unhex(row.at("native_payload_hex"))) {
        for(const auto& value:row.at("namespace")){auto t=tag(value);names.emplace(t.datum,std::move(t));}
        for(const auto& [id,n]:row.at("bitmap_frame_counts").items())frames.emplace(std::stoul(id),n);
        for(const auto& region:row.at("regions"))regions.emplace(region.at(0),unhex(region.at(1)));
    }
    std::span<const std::byte> read(std::uint32_t address,std::size_t size)const {
        if(!size)return {};
        for(const auto& [first,raw]:regions)if(address>=first && std::uint64_t(address)-first<=raw.size() && size<=raw.size()-(address-first))
            return std::span(raw).subspan(address-first,size);
        throw FormatError("Sky source read escapes literal fixture ownership");
    }
    void change(std::uint32_t address) {
        bool found=false;
        for(auto& [first,raw]:regions)if(address>=first && std::uint64_t(address)-first<raw.size()) {
            raw[address-first]^=std::byte{1};found=true;
        }
        need(found,"Sky mutation escapes its source region");
    }
    std::optional<MccOwnedShaderProjection> run()const {
        return project_mcc_animated_clock_world(shader,source_stem,native_stem,payload_name,payload,
            [&](auto at,auto n){return read(at,n);},
            [&](auto id)->const Tag*{const auto p=names.find(id);return p==names.end()?nullptr:&p->second;},
            [&](auto id,auto frame){const auto p=frames.find(id);return p!=frames.end() && frame<p->second;});
    }
};
}
int main(int argc,char** argv)try {
    need(argc==2,"Supply the owned literal sky fixture");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    need(fixture.at("schema")=="H2_MCC_NATIVE_SKY_COOKED_FIXTURE_V1","Unknown sky regression owner");
    unsigned cases{},banks{},curves{},negatives{},qualified_variants{},mod_owners{},originals{};
    std::set<std::pair<std::string,unsigned>> profiles;
    for(const auto& row:fixture.at("cases")) {
        const Fixture source(row);const auto output=source.run();
        need(bool(output),"Authored native sky cooked data remains unresolved: "+source.shader.name);
        need(source.regions==Fixture(row).regions,"Native sky projection mutated source storage");
        const Bytes b(output->bytes);const auto pp=b.u32(36);need(b.u32(32)==1,"Sky platform count differs");
        std::set<unsigned> pointers;
        for(const auto& [field,target]:output->pointers)
            need(pointers.insert(field).second && target<output->bytes.size() && b.u32(field)==target,"Sky relative pointer owner differs");
        for(const auto& [field,stride]:fields) {
            const auto expected=unhex(row.at("expected_banks").at(std::to_string(field)));
            auto actual=b.slice(b.u32(pp+field+4),std::size_t(b.u32(pp+field))*stride);
            std::vector<std::byte> normalized(actual.begin(),actual.end());
            if(field==60)for(unsigned n=0;n<normalized.size()/20;++n)
                std::fill_n(normalized.begin()+n*20+16,4,std::byte{});
            need(normalized==expected,"Sky authored bank differs at field "+std::to_string(field));++banks;
        }
        const auto overlays=b.u32(pp+64);
        for(unsigned n=0;n<row.at("expected_overlays").size();++n) {
            const auto& expected=row.at("expected_overlays").at(n);const auto at=overlays+n*20;
            need(b.u32(at)==expected.at("input") && b.u32(at+4)==expected.at("range") && b.u32(at+8)==expected.at("period_word") &&
                pointers.contains(at+16) && std::ranges::equal(b.slice(b.u32(at+16),b.u32(at+12)),unhex(expected.at("curve_hex"))),
                "Sky nested curve or period owner differs");++curves;
        }
        const auto reject=[&](const Fixture& changed,const std::string& label){need(!changed.run(),"Unproved sky input admitted: "+label);++negatives;};
        const Bytes root(source.read(source.shader.address,84));const auto definition_address=root.u32(36);
        const Bytes definition(source.read(definition_address,124));
        for(unsigned field:{0u,4u,12u,20u,24u,28u,32u,56u,76u}) {
            auto changed=source;changed.change(changed.shader.address+field);reject(changed,"shader root");
        }
        for(const auto& [field,stride]:fields) {
            (void)stride;auto changed=source;changed.change(definition_address+field);reject(changed,"bank count");
        }
        const auto key=std::pair(row.at("template").get<std::string>(),definition.u32(60));
        if(profiles.insert(key).second) {
            for(unsigned n=0;n<source.payload.size();++n) {
                auto changed=source;changed.payload[n]^=std::byte{1};reject(changed,"complete native template byte");
            }
            std::set<unsigned> seen;
            for(const auto& region:row.at("stem_mutation_regions")) {
                const auto first=region.at("address").get<unsigned>(),size=region.at("bytes").get<unsigned>();
                for(unsigned n=0;n<size;++n)if(seen.insert(first+n).second) {
                    auto changed=source;changed.change(first+n);reject(changed,"complete MCC template/remap byte");
                }
            }
            for(unsigned field:{52u,68u,76u,84u,92u}) {
                const auto stride=field==52?10u:field==76?2u:4u;
                for(unsigned n=0;n<definition.u32(field)*stride;++n) {
                    auto changed=source;changed.change(definition.u32(field+4)+n);reject(changed,"native animation role");
                }
            }
        }
        for(unsigned n=0;n<definition.u32(60);++n) {
            const auto at=definition.u32(64)+n*20;const Bytes curve(source.read(at,20));
            for(unsigned field:{0u,4u,8u,12u,16u}) {
                auto changed=source;changed.change(at+field);reject(changed,"overlay ABI");
            }
            for(unsigned i=0;i<curve.u32(12);++i) {
                auto changed=source;changed.change(curve.u32(16)+i);
                const auto value=changed.read(curve.u32(16),curve.u32(12));
                bool qualified=false;
                for(const auto& other:fixture.at("cases"))
                    if(other.at("template")==row.at("template") && other.at("expected_overlays").size()==definition.u32(60))
                        qualified|=std::ranges::equal(value,unhex(other.at("expected_overlays").at(n).at("curve_hex")));
                if(qualified) {
                    const auto admitted=changed.run();need(bool(admitted),"Independently qualified authored curve variant rejected");
                    const Bytes result(admitted->bytes);const auto d=result.u32(36),o=result.u32(d+64)+n*20;
                    need(std::ranges::equal(result.slice(result.u32(o+16),result.u32(o+12)),value),"Qualified authored curve variant was replaced");
                    ++qualified_variants;
                }else reject(changed,"literal curve byte");
            }
        }
        auto renamed=source;renamed.shader.name="custom\\authored\\renamed_sky";renamed.names.at(renamed.shader.datum)=renamed.shader;
        need(bool(renamed.run()),"Sky projection relies on a material-name alias");
        auto bad=source;bad.native_stem.datum^=0x10000u;reject(bad,"full salted template identity");
        const auto bitmap=Bytes(source.read(definition.u32(8),12)).u32(0);
        bad=source;bad.frames[bitmap]=0;reject(bad,"bitmap frame owner");
        bad=source;bad.names.at(bitmap).group="shad";reject(bad,"bitmap typed owner");
        if(row.at("kind")=="pc-geometry-owner")++mod_owners;else ++originals;
        ++cases;
    }
    need(cases==7 && mod_owners==3 && originals==4 && profiles.size()==4,"Incomplete original/mod sky cohort");
    std::cout<<"PASS sky cases="<<cases<<" authored_mods="<<mod_owners<<" original_pairs="<<originals
        <<" complete_banks="<<banks<<" nested_curves="<<curves<<" negative_controls="<<negatives
        <<" qualified_curve_variants="<<qualified_variants<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
