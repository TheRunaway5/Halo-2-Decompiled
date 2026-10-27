#include "halo2/mcc_native_library.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
using json=nlohmann::json;
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string read(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);need(bool(input),"world closure input unavailable");
    return {std::istreambuf_iterator<char>(input),{}};
}
std::string sha(const std::string& bytes) {
    std::array<unsigned char,32> value{};unsigned size{};
    need(EVP_Digest(bytes.data(),bytes.size(),value.data(),&size,EVP_sha256(),nullptr)==1&&size==32,"world closure digest failed");
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto v:value){out+=digits[v>>4];out+=digits[v&15];}return out;
}
}
int main(int argc,char** argv){try {
    need(argc==3,"expected source root and actual prepared native-library root");
    if(!*argv[2]) {
        std::cout<<"PC world closure requires the optional owned native-library directory\n";
        return 77;
    }
    const std::filesystem::path source=argv[1],native=argv[2];
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch&&*scratch,"world closure test needs owned scratch");
    const auto policy=json::parse(read(source/"tools/data/mcc-native-pc-world-closure.json"));
    const auto hud=json::parse(read(source/"tools/data/mcc-native-hud-bootstrap.json"));
    const auto sky=json::parse(read(source/"tools/data/mcc-native-pc-sky-closure.json"));
    const auto original=read(native/policy.at("source_map").get<std::string>()/"library.json");
    unsigned cases{};
    for(const auto& map:policy.at("maps")) {
        const auto name=map.at("map").get<std::string>();
        const json* sky_map=nullptr;for(const auto& row:sky.at("maps"))if(row.at("map")==name)sky_map=&row;
        need(sky_map,"world closure lacks paired sky owner");
        json base={{"schema","H2_PRESERVED_NATIVE_SHADERS_V1"},{"map",name},{"shader_tags",hud.at("records")}};
        for(const auto& row:sky_map->at("records"))base["shader_tags"].push_back(row);
        const auto input=native.parent_path()/"halo2/h2_maps_win64_dx11"/(name+".map");
        const auto bytes=read(input);need(bytes.size()==map.at("bytes").get<std::size_t>()&&sha(bytes)==map.at("sha256").get<std::string>(),"actual encoded PC input differs");
        const auto reader=[&](const std::filesystem::path& p){need(p==native/policy.at("source_map").get<std::string>()/"library.json","world snapshot escaped admitted source");return original;};
        const auto invoke=[&](json& bank,const auto& path,std::uint64_t count,const std::string& digest,const auto& callback) {
            return halo2::supplement_mcc_native_pc_world_library(bank,path,native,count,digest,callback);
        };
        auto good=base;const auto payloads=invoke(good,input,bytes.size(),sha(bytes),reader);
        need(!payloads.empty()&&good.at("shader_tags").size()==base.at("shader_tags").size()+52,"complete original world closure not appended");++cases;
        for(const auto& row:good.at("shader_tags"))if(row.at("group")=="shad")
            need(std::find(base.at("shader_tags").begin(),base.at("shader_tags").end(),row)!=base.at("shader_tags").end(),"world closure imported a material instance");
        const auto unchanged=[&](json bank,const auto& path,std::uint64_t count,const std::string& digest) {
            const auto before=bank;need(invoke(bank,path,count,digest,reader).empty()&&bank==before,"unknown world input changed caller bank");++cases;
        };
        unchanged(base,input,bytes.size()+1,sha(bytes));unchanged(base,input,bytes.size(),std::string(64,'0'));
        unchanged(base,input.parent_path()/"unknown.map",bytes.size(),sha(bytes));unchanged(good,input,bytes.size(),sha(bytes));
        auto partial=base;partial["shader_tags"].erase(partial["shader_tags"].begin());unchanged(partial,input,bytes.size(),sha(bytes));
        auto changed=json::parse(original);changed["shader_tags"][0]["bytes"]=1;
        auto rejected=base;bool failed=false;
        try{invoke(rejected,input,bytes.size(),sha(bytes),[&](const auto&){return changed.dump();});}
        catch(const halo2::FormatError&){failed=true;}
        need(failed&&rejected==base,"changed complete original world library was admitted or partially appended");++cases;
    }
    // Corrupt only an owned duplicate of an actual original payload. This is
    // a rejection fixture, never a source asset or a prepared-cache change.
    const auto duplicate=std::filesystem::path(scratch)/"corrupt-native";
    const auto bank=policy.at("source_map").get<std::string>();
    std::filesystem::create_directories(duplicate/bank/"payloads");
    for(const auto& row:policy.at("records")) {
        const auto filename=row.at("payload").get<std::string>();
        std::filesystem::copy_file(native/bank/"payloads"/filename,duplicate/bank/"payloads"/filename,
            std::filesystem::copy_options::overwrite_existing);
    }
    const auto filename=policy.at("records")[0].at("payload").get<std::string>();
    {auto raw=read(duplicate/bank/"payloads"/filename);raw[0]^=1;std::ofstream(duplicate/bank/"payloads"/filename,std::ios::binary).write(raw.data(),raw.size());}
    const auto& map=policy.at("maps")[0];const auto name=map.at("map").get<std::string>();
    json base={{"schema","H2_PRESERVED_NATIVE_SHADERS_V1"},{"map",name},{"shader_tags",hud.at("records")}};
    for(const auto& row:sky.at("maps"))if(row.at("map")==name)for(const auto& record:row.at("records"))base["shader_tags"].push_back(record);
    auto rejected=base;bool failed=false;
    try{halo2::supplement_mcc_native_pc_world_library(rejected,name+".map",duplicate,map.at("bytes").get<std::uint64_t>(),map.at("sha256").get<std::string>(),[&](const auto&){return original;});}
    catch(const halo2::FormatError&){failed=true;}
    need(failed&&rejected==base,"changed original world payload was admitted or partially appended");++cases;
    std::cout<<cases<<" authenticated full-library/payload/input/namespace/atomicity world closure cases passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
