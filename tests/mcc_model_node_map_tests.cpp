#include "halo2/mcc_model_node_map.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/source_runtime_resources.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value)throw FormatError(why);}
void word(std::span<std::byte> bytes,unsigned at,unsigned value) {
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
std::vector<std::byte> unhex(const std::string& s) {
    std::vector<std::byte> out;
    for(unsigned i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return out;
}
void controls() {
    std::vector<std::byte> raw(256);word(raw,0x70,2);raw[192]=std::byte{3};raw[193]=std::byte{9};
    const MccModelNodeMap owner{0,0,256,192,2,10,"declared test node bank"};
    auto expected=raw;word(expected,0x6c,2);word(expected,0x70,0);
    for(unsigned first=100;first<124;++first)for(unsigned bytes=1;bytes<=16;++bytes) {
        std::vector<std::byte> slice(raw.begin()+first,raw.begin()+first+bytes);
        project_mcc_model_node_map_range(std::span(&owner,1),first,raw,slice);
        check(std::ranges::equal(slice,std::span(expected).subspan(first,bytes)),
            "Partial node-map I/O changed another field or missed its source projection");
    }
    const auto untouched=raw;
    const std::array aliases{owner,owner};auto output=raw;
    project_mcc_model_node_map_range(aliases,0,raw,output);
    check(output==expected && raw==untouched,"Identical model aliases lost immutable ownership");
    output=raw;project_mcc_model_node_map_range(std::span(&owner,1),0x80000000u,raw,output);
    check(output==raw,"Node-map projection crossed its encoded cache selector");
    for(unsigned kind=0;kind<7;++kind) {
        auto source=raw;auto bad=owner;
        if(kind==0)word(source,0x6c,1);
        if(kind==1)word(source,0x70,3);
        if(kind==2)word(source,0x74,1);
        if(kind==3)source[192]=std::byte{10};
        if(kind==4)bad.node_offset=255;
        if(kind==5)bad.resource_bytes=257;
        output=source;if(kind==6)output[0]=std::byte{1};
        const auto before=output;bool rejected=false;
        try{project_mcc_model_node_map_range(std::span(&bad,1),0,source,output);}
        catch(const FormatError&){rejected=true;}
        catch(const std::out_of_range&){rejected=true;}
        check(rejected && output==before,"Invalid node-map owner changed delivered bytes");
    }
    auto conflict=owner;conflict.node_count=3;const std::array conflicting{owner,conflict};
    output=raw;bool rejected=false;
    try{project_mcc_model_node_map_range(conflicting,0,raw,output);}catch(const FormatError&){rejected=true;}
    check(rejected && output==raw,"Conflicting aliases published a partial node-map projection");
}
}
int main(int argc,char**argv)try {
    controls();
    if(argc==1){std::cout<<"PASS node-map range/ownership controls: "<<checks<<'\n';return 0;}
    if(argc!=4 || !std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1]);check(cache.is_mcc(),"Node-map runtime inputs must be MCC");
    nlohmann::json reference;std::ifstream(argv[2])>>reference;
    const auto maps=plan_mcc_model_node_maps(cache);
    const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u);
    const auto image=delivery.tag_image();SourceRuntimeResources io(cache);
    unsigned headers{},paired{};
    for(const auto& map:maps) {
        const auto before=cache.load_source_runtime_resource(map.encoded_resource,map.resource_bytes);
        const auto after=io.load(map.encoded_resource,map.resource_bytes);
        check(before.size()==after.size(),"Node-map projection changed its raw allocation extent");
        check(Bytes(after).u32(0x6c)==map.node_count && Bytes(after).u32(0x70)==0,
            "Actual MCC raw node count/unloaded pointer differs from the original ABI");
        for(unsigned i=0;i<before.size();++i)if(i<0x6c || i>=0x74)
            check(before[i]==after[i],"Node-map projection modified another actual resource byte");
        const auto target=delivery.runtime_address(map.descriptor_field,4);
        bool found=false;
        for(const auto& region:image.regions)if(target>=region.address &&
            std::uint64_t(target)-region.address+4<=region.bytes.size()) {
            check(Bytes(region.bytes).u32(target-region.address)==0x00010064u,
                "Native canonical node-map fixup destination is not Xbox100/channel1");found=true;
        }
        check(found,"Canonical descriptor has no declared native metadata owner");
        check(cache.load_source_runtime_resource(map.encoded_resource,map.resource_bytes)==before,
            "Native delivery mutated the underlying MCC resource owner");
        if(map.model==reference.at("name").get<std::string>()) {
            const auto r=std::find_if(reference["sections"].begin(),reference["sections"].end(),
                [&](const auto& row){return row.at("encoded_resource").template get<unsigned>()==map.encoded_resource;});
            check(r!=reference["sections"].end(),"Paired hinge section has no original reference");
            const auto expected=unhex(r->at("original_header_hex").template get<std::string>());
            check(expected.size()==112 && std::ranges::equal(expected,std::span(after).subspan(8,112)),
                "Complete original112-byte hinge header differs before native pointer fixup");
            const auto expected_nodes=r->at("node_map").template get<std::vector<unsigned>>();
            check(expected_nodes.size()==map.node_count,"Actual paired node-map extent differs");
            for(unsigned i=0;i<map.node_count;++i)check(std::to_integer<unsigned>(after[map.node_offset+i])==expected_nodes[i],
                "Actual authored node-map identity changed");
            ++paired;
        }
        ++headers;
    }
    check(headers>5 && paired==5,"Actual MCC cohort or all five paired hinge sections are missing");
    const nlohmann::json report{{"passed",true},{"runtime_cache_source","MCC"},
        {"projected_model_node_maps",headers},{"paired_original_hinge_headers",paired},
        {"checks",checks},{"original_render_instructions_changed",false},
        {"native_resource_publication_verified",false},{"application_rendering_verified",false}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
