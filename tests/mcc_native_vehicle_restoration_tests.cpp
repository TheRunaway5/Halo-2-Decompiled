#include "halo2/cache.hpp"
#include "halo2/mcc_native_library.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace halo2;using Json=nlohmann::json;
void need(bool value,const char* message){if(!value)throw FormatError(message);}
std::vector<std::byte> unhex(const Json& value){const auto text=value.get<std::string>();
    need(text.size()%2==0,"Invalid vehicle fixture hex");std::vector<std::byte> bytes;bytes.reserve(text.size()/2);
    for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return bytes;}
struct Fixture {
    Json before,libraries;std::string name,sha;std::uint64_t extent{};unsigned string_count{};
    std::vector<Tag> directory;std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    std::map<std::uint32_t,std::set<unsigned>> frames;std::map<std::string,std::string> payloads;
    std::vector<std::optional<std::string>> strings;
    Fixture(const Json& row,const Json& all,const Json& policy):before(row.at("preceding_library")),libraries(all.at("source_libraries")),
        name(row.at("map")),sha(policy.at("sha256")),extent(policy.at("bytes")),string_count(row.at("source_string_count")) {
        for(const auto& value:row.at("namespace")){const auto datum=value.at("datum").get<std::uint32_t>();
            if(directory.size()<=(datum&0xffffu))directory.resize((datum&0xffffu)+1);
            directory[datum&0xffffu]={value.at("group"),value.at("name"),datum,value.at("address"),value.at("size")};}
        for(const auto& region:row.at("regions"))regions.emplace_back(region.at(0),unhex(region.at(1)));
        for(const auto& role:policy.at("bitmap_roles"))frames[role.at("datum").get<std::uint32_t>()].insert(role.at("frame").get<unsigned>());
        for(const auto& [key,value]:all.at("payloads").items()){const auto bytes=unhex(value);
            payloads.emplace(key,std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()));}
        for(const auto& text:row.at("literal_strings"))strings.push_back(text.is_null()?std::nullopt:std::optional(text.get<std::string>()));
        need(strings.size()==string_count,"Literal vehicle StringID extent differs");
    }
    std::span<const std::byte> read(std::uint32_t at,std::size_t bytes)const {
        for(const auto& [first,raw]:regions)if(at>=first&&std::uint64_t(at)-first<=raw.size()&&bytes<=raw.size()-(at-first))
            return std::span(raw).subspan(at-first,bytes);
        throw FormatError("Vehicle fixture allocation escaped its source owner");
    }
    std::string snapshot(const std::filesystem::path& path)const {
        if(path.filename()=="library.json")return libraries.at(path.parent_path().filename().string()).dump();
        return payloads.at(path.filename().string());
    }
    MccNativeVehicleLibrary run(Json& out)const{return supplement_mcc_native_vehicle_library(out,name+".map",directory,
        [&](auto at,auto count){return read(at,count);},[&](auto datum,auto frame){const auto it=frames.find(datum);return it!=frames.end()&&it->second.contains(frame);},
        string_count,[&](auto id){const auto index=id&0xffffu;if(index>=strings.size()||!strings[index])throw FormatError("Absent literal StringID");return *strings[index];},
        std::filesystem::path("__no_vehicle_fixture_files__"),extent,sha,[&](const auto& path){return snapshot(path);});}
    void flip(std::uint32_t at){bool found=false;for(auto& [first,raw]:regions)if(at>=first&&std::uint64_t(at)-first<raw.size()){
        raw[at-first]^=std::byte{1};found=true;}need(found,"Vehicle mutation escaped all source regions");}
};
}
int main(int argc,char**argv)try {
    need(argc==3,"Expected complete vehicle source fixture and finite policy");std::ifstream input(argv[1]),policy_input(argv[2]);
    const auto all=Json::parse(input),policy=Json::parse(policy_input);
    need(all.at("schema")=="H2_MCC_NATIVE_VEHICLE_RESTORATION_FIXTURE_V1"&&all.at("cases").size()==2,"Vehicle literal fixture census differs");
    unsigned rejects=0,payload_checks=0,region_checks=0,typed_checks=0,bitmap_checks=0,string_checks=0,records=0,semantics=0;
    for(unsigned case_index=0;case_index<2;++case_index){const auto& row=all.at("cases").at(case_index);const auto& profile=policy.at("maps").at(case_index);
        Fixture value(row,all,profile);auto bank=value.before;const auto output=value.run(bank);
        need(bank.at("shader_tags").size()==value.before.at("shader_tags").size()+profile.at("records").size(),"Vehicle complete record count differs");
        need(output.semantic_names.size()==profile.at("declared_program_namespace").size(),"Vehicle complete semantic namespace differs");
        for(unsigned i=0;i<profile.at("records").size();++i){const auto& item=profile.at("records").at(i);
            need(bank.at("shader_tags").at(value.before.at("shader_tags").size()+i)==item.at("record"),"Vehicle complete original descriptor projection differs");
            ++records;if(item.at("record").at("group")!="shad"){
                const auto& record=item.at("record");const auto key=std::pair{record.at("group").get<std::string>(),record.at("name").get<std::string>()};
                need(output.semantic_names.at(key)==item.at("semantic_name").get<std::string>(),"Vehicle original semantic label differs");++semantics;}}
        const auto fail=[&](const char* message){auto changed=value.before;bool rejected=false;try{const auto result=value.run(changed);rejected=result.payloads.empty()&&result.semantic_names.empty();}
            catch(const FormatError&){rejected=true;}need(rejected&&changed==value.before,message);++rejects;};
        // Every source region, typed allocation identity and borrowed payload is
        // independently rejected before the single atomic bank replacement.
        std::set<std::uint32_t> addresses;for(const auto& region:profile.at("source_regions"))addresses.insert(region.at("address").get<std::uint32_t>());
        for(const auto at:addresses){value.flip(at);fail("Changed opaque MCC source region admitted");value.flip(at);++region_checks;}
        for(const auto& expected:profile.at("source_tags")){auto& actual=value.directory[expected.at("datum").get<std::uint32_t>()&0xffffu];
            actual.datum^=0x10000u;fail("Wrong source full datum admitted");actual.datum^=0x10000u;
            const auto old_group=actual.group;actual.group="unowned";fail("Wrong source typed group admitted");actual.group=old_group;
            const auto old_name=actual.name;actual.name+="_changed";fail("Wrong source identity admitted");actual.name=old_name;
            ++actual.address;fail("Wrong source allocation address admitted");--actual.address;
            ++actual.size;fail("Wrong source allocation extent admitted");--actual.size;typed_checks+=5;}
        std::set<std::string> payload_names;for(const auto& item:profile.at("records"))payload_names.insert(item.at("original_record").at("payload").get<std::string>());
        for(const auto& filename:payload_names){value.payloads.at(filename).at(0)^=1;fail("Changed complete TU payload admitted");value.payloads.at(filename).at(0)^=1;++payload_checks;}
        for(const auto& role:profile.at("bitmap_roles")){const auto id=role.at("datum").get<std::uint32_t>(),frame=role.at("frame").get<unsigned>();
            value.frames.at(id).erase(frame);fail("Absent original bitmap frame admitted");value.frames.at(id).insert(frame);++bitmap_checks;}
        for(const auto& [text,id]:profile.at("source_string_ids").items())if(!id.is_null()){
            const auto index=id.get<std::uint32_t>()&0xffffu;const auto original=value.strings.at(index);value.strings.at(index)="unowned-input";
            fail("Changed original dynamic-input StringID admitted");value.strings.at(index)=original;++string_checks;}
        ++value.string_count;fail("Wrong literal StringID extent admitted");--value.string_count;
        value.before["unowned"]=true;{auto out=value.before;bool rejected=false;try{(void)value.run(out);}catch(const FormatError&){rejected=true;}
            need(rejected&&out==value.before,"Changed preceding bank admitted");++rejects;}value.before.erase("unowned");
        for(const auto& [source,unused]:profile.at("sources").items()){(void)unused;value.libraries.at(source)["unowned"]=true;
            fail("Changed original TU library admitted");value.libraries.at(source).erase("unowned");}
        value.sha[0]=value.sha[0]=='0'?'1':'0';fail("Wrong full encoded PC input admitted");value.sha=profile.at("sha256");
        ++value.extent;fail("Wrong full encoded PC extent admitted");--value.extent;
        const auto name=value.name;value.name="unrelated";fail("Unrelated map received vehicle material aliases");value.name=name;
        auto cross=value.before;value.name=name=="needle"?"street_sweeper":"needle";const auto no_match=value.run(cross);value.name=name;
        need(no_match.payloads.empty()&&cross==value.before,"Cross-profile source tuple admitted");++rejects;
    }
    std::cout<<Json{{"passed",true},{"profiles",2},{"complete_original_records",records},{"semantic_program_labels",semantics},
        {"opaque_region_controls",region_checks},{"typed_owner_controls",typed_checks},{"complete_payload_controls",payload_checks},
        {"bitmap_frame_controls",bitmap_checks},{"dynamic_StringID_controls",string_checks},{"strict_rejections",rejects},
        {"scope","Finite source-admission and atomic closure; actual Cache ABI/material/AOT/GPU acceptance is separate"}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
