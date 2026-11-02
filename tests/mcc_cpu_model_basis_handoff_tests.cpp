#include "halo2/bsp_lighting.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_sky_geometry.hpp"
#include "halo2/model.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool condition,const char* message) {if(!condition)throw FormatError(message);}
struct Section {
    unsigned vertices{};
    std::map<std::pair<unsigned,unsigned>,std::vector<std::byte>> resources;
};
Section source_section(const Cache& cache,const Tag& tag,unsigned section) {
    const auto metadata=cache.load_source_tag_metadata(tag.datum);
    const Bytes root(metadata.bytes_at(metadata.source_address(),0x68));
    need(section<root.u32(0x24),"Paired basis section outside actual model");
    const Bytes row(metadata.bytes_at(root.u32(0x28)+section*92,92));
    const auto banks=cache.load_model_resources(tag.datum);
    need(section<banks.size(),"Paired basis has no actual raw resource");
    const auto& raw=banks[section];const Bytes bytes(raw);
    need(std::uint64_t(row.u32(0x44))+4<=raw.size(),"Invalid paired resource base");
    const auto base=raw.size()-row.u32(0x44)-4;
    Section out;out.vertices=row.u16(4);
    for(unsigned i=0;i<row.u32(0x48);++i) {
        const Bytes entry(metadata.bytes_at(row.u32(0x4c)+i*16,16));
        const auto view=bytes.slice(base+entry.u32(12),entry.u32(8));
        need(out.resources.emplace(std::pair{unsigned(entry.u16(4)),unsigned(entry.u16(6))},
            std::vector<std::byte>{view.begin(),view.end()}).second,"Duplicate paired resource type/channel");
    }
    return out;
}
const Tag& named_model(const Cache& cache,const std::string& name) {
    const auto* tag=cache.query_tag({.group=tag_groups::render_model,.name=name,.require_address=true});
    need(tag,"Actual named paired model is absent");return *tag;
}
std::array<unsigned,3> words(Vec3 value) {
    return {std::bit_cast<unsigned>(value.x),std::bit_cast<unsigned>(value.y),std::bit_cast<unsigned>(value.z)};
}
}
int main(int argc,char**argv)try {
    need(argc==4,"Supply genuine MCC opening cache, Original opening cache, and captured-part fixture");
    Cache mcc(argv[1]),original(argv[2]);need(mcc.is_mcc() && !original.is_mcc() && !original.is_vista(),
        "CPU basis comparison requires MCC and Original Xbox owners");
    std::ifstream input(argv[3]);Json fixture;input>>fixture;
    need(fixture.at("schema")=="HALO2_CAPTURED_MCC_BODY_BASIS_V1","Wrong captured body basis fixture");
    std::array<std::uint64_t,3> packed_mismatches{},float_mismatches{};
    std::uint64_t occurrences{},compared_words{};unsigned sections{},parts{};
    Json examples=Json::array();std::set<std::tuple<unsigned,unsigned,unsigned>> unique;
    for(const auto& section:fixture.at("sections")) {
        const auto name=section.at("model_name").get<std::string>();
        const auto& model=named_model(mcc,name);const auto& reference=named_model(original,name);
        need(model.datum==section.at("mcc_model").get<unsigned>(),"Captured MCC model salt changed");
        const auto ordinal=section.at("section").get<unsigned>();
        const auto source=source_section(mcc,model,ordinal),old=source_section(original,reference,ordinal);
        const auto& floats=source.resources.at({56,2});const auto& packed=old.resources.at({56,2});
        need(source.vertices==old.vertices && source.vertices==section.at("vertices").get<unsigned>(),
            "Actual paired model section/vertex identity changed");
        need(floats.size()==source.vertices*36ull && packed.size()==source.vertices*12ull,
            "Actual paired basis stream extent changed");
        need(mcc_sky_geometry_sha256(floats)==section.at("mcc_float_basis_sha256").get<std::string>() &&
            mcc_sky_geometry_sha256(packed)==section.at("original_packed_basis_sha256").get<std::string>(),
            "Captured MCC or independently authenticated Original basis bytes changed");
        const std::array selected{ordinal};
        // Execute the real Cache decoder, including its MCC physical-field branch.
        const auto decoded=mcc.load_model_sections(model.datum,false,0,5,selected);
        need(decoded.mesh.vertices.size()==decoded.vertex_origins.size(),"CPU decoder lost source vertex identities");
        std::map<unsigned,unsigned> observed_parts;
        const Bytes raw(floats),old_raw(packed);
        for(std::size_t i=0;i<decoded.mesh.vertices.size();++i) {
            const auto& origin=decoded.vertex_origins[i];
            need(origin.section==ordinal && origin.vertex<source.vertices,"CPU decoded vertex escaped its section");
            const auto wanted=std::find_if(section.at("parts").begin(),section.at("parts").end(),
                [&](const auto& p){return p.at("part").template get<unsigned>()==origin.part;});
            if(wanted==section.at("parts").end())continue;
            ++observed_parts[origin.part];++occurrences;unique.emplace(model.datum,ordinal,origin.vertex);
            const auto& vertex=decoded.mesh.vertices[i];
            const Vec3 actual[]{vertex.normal,vertex.tangent,vertex.binormal};
            for(unsigned field=0;field<3;++field) {
                const auto expected=old_raw.u32(origin.vertex*12+field*4);
                const auto current=pack_bsp_incident_tu15(actual[field]);
                const auto at=origin.vertex*36+field*12;
                const std::array<unsigned,3> physical{raw.u32(at),raw.u32(at+4),raw.u32(at+8)};
                packed_mismatches[field]+=current!=expected;
                float_mismatches[field]+=words(actual[field])!=physical;++compared_words;
                if(current!=expected && examples.size()<6)examples.push_back({{"model",model.datum},
                    {"section",ordinal},{"part",origin.part},{"vertex",origin.vertex},
                    {"field_N_T_B",field},{"actual_packed",current},{"original_packed",expected}});
            }
        }
        for(const auto& part:section.at("parts")) {
            need(observed_parts[part.at("part").get<unsigned>()]==part.at("occurrences").get<unsigned>(),
                "Actual whole Cache decoder omitted captured part occurrences");++parts;
        }
        ++sections;
    }
    need(sections==fixture.at("sections_count").get<unsigned>() &&
        parts==fixture.at("parts_count").get<unsigned>() &&
        occurrences==fixture.at("captured_occurrences").get<unsigned>() &&
        unique.size()==fixture.at("unique_owner_section_vertices").get<unsigned>(),
        "Whole Cache captured body coverage changed");
    const bool passed=packed_mismatches==std::array<std::uint64_t,3>{} &&
                      float_mismatches==std::array<std::uint64_t,3>{};
    const Json report{{"passed",passed},{"whole_Cache_load_model_sections_executed",true},
        {"sections",sections},{"parts",parts},{"occurrences",occurrences},{"unique_vertices",unique.size()},
        {"packed_Original_words_compared",compared_words},{"packed_mismatches_N_T_B",packed_mismatches},
        {"MCC_physical_FLOAT_mismatches_N_T_B",float_mismatches},{"examples",examples},
        {"scope","Actual captured MCC body parts decoded by whole Cache; physical float precision retained; Original packed basis independently pinned. No posed world/kernel/pixel acceptance."}};
    if(const auto* destination=std::getenv("HALO2_MCC_CPU_BASIS_REPORT")) {
        std::ofstream output(destination);need(bool(output),"CPU basis report path unavailable");output<<report.dump(2)<<'\n';
    }
    std::cout<<report.dump()<<'\n';return passed?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
