#include "halo2/mcc_sky_geometry.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <memory>
using namespace halo2;using J=nlohmann::json;
int main(int argc,char**argv){try{
    if(argc!=3)throw FormatError("Exact sky relocation controls require policy and actual prepared cache");
    if(std::string_view(argv[2]).empty()){std::cout<<"Exact derived sky cache not configured\n";return 77;}
    std::ifstream in(argv[1]);const auto policy=J::parse(in);const std::filesystem::path root=argv[2];
    std::string map;std::unique_ptr<Cache> cache;
    unsigned admitted{},rejected{};
    for(const auto& row:policy.at("materials")) {
        const auto wanted=row.at("map").get<std::string>();
        if(wanted!=map){map=wanted;cache=std::make_unique<Cache>(root/"halo2/h2_maps_win64_dx11"/(map+".map"),root/"halo2/h2_maps_win64_dx11",nullptr,root);}
        const auto datum=row.at("shader").get<std::uint32_t>();const auto data=cache->tag_payload({datum});
        const auto input=row.at("model_input_sha256").get<std::string>(),name=row.at("name").get<std::string>(),expected=row.at("before_payload_sha256").get<std::string>();
        const auto matches=[&](std::span<const std::byte> bytes){return mcc_sky_material_relocation_matches(input,datum,name,expected,bytes);};
        if(!matches(data))throw FormatError("Exact actual derived sky shader was rejected: "+
            J({{"map",map},{"shader",datum},{"actual_payload_sha256",mcc_sky_geometry_sha256(data)},
                {"expected_derived_payload_sha256",row.at("after_payload_sha256")}}).dump());++admitted;
        const auto no=[&](bool value){if(value)throw FormatError("Unsupported sky payload/input mutation was admitted");++rejected;};
        no(mcc_sky_material_relocation_matches("other-model",datum,name,expected,data));
        no(mcc_sky_material_relocation_matches(input,datum^0x10000u,name,expected,data));
        no(mcc_sky_material_relocation_matches(input,datum,name+"x",expected,data));
        no(mcc_sky_material_relocation_matches(input,datum,name,std::string(64,'0'),data));
        for(std::size_t at=0;at<data.size();++at){std::vector<std::byte> changed(data.begin(),data.end());changed[at]^=std::byte{1};no(matches(changed));}
        no(matches(data.first(data.size()-1)));std::vector<std::byte> extra(data.begin(),data.end());extra.push_back(std::byte{});no(matches(extra));
    }
    unsigned models{};
    for(const auto name:{"zanzibar","03a_oldmombasa"}) {
        Cache actual(root/"halo2/h2_maps_win64_dx11"/(std::string(name)+".map"),root/"halo2/h2_maps_win64_dx11",{},root);
        const auto* scenario=actual.find_tag(actual.scenario_tag(),tag_groups::scenario);
        const auto count=Bytes(actual.tag_payload(scenario->handle())).u32(8);
        for(unsigned ordinal=0;ordinal<count;++ordinal) {
            const auto sky=actual.load_sky(ordinal);if(!sky)continue;
            const auto model=actual.load_model(sky->model,true);
            if(model.mesh.vertices.empty())throw FormatError("Source sky projection emitted no vertices");++models;
        }
    }
    std::cout<<admitted<<" exact derived sky payloads, "<<rejected<<" strict rejects, "<<models<<" actual original sky projections passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
