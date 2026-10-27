#include "halo2/mcc_native_library.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>

namespace {
using json=nlohmann::json;
namespace fs=std::filesystem;
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string read(const fs::path& path) {
    std::ifstream file(path,std::ios::binary);need(bool(file),"program closure input unavailable");
    return {std::istreambuf_iterator<char>(file),{}};
}
std::string sha(const std::string& bytes) {
    std::array<unsigned char,32> hash{};unsigned count{};
    need(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&count,EVP_sha256(),nullptr)==1&&count==32,"program closure SHA failed");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(auto v:hash){result+=hex[v>>4];result+=hex[v&15];}return result;
}
void write(const fs::path& path,const std::string& value) {
    std::ofstream file(path,std::ios::binary);need(bool(file),"owned program fixture unavailable");
    file.write(value.data(),std::streamsize(value.size()));need(bool(file),"owned program fixture write failed");
}
}
int main(int argc,char** argv){try {
    need(argc==3,"expected repository and actual native-library root");
    if(!*argv[2]){std::cout<<"Additional PC world programs require optional owned native libraries\n";return 77;}
    const fs::path repo=argv[1],native=argv[2];
    const auto env=std::getenv("HALO2_TEST_SCRATCH");need(env&&*env,"program closure requires owned scratch");
    const fs::path scratch=env;
    const auto policy=json::parse(read(repo/"tools/data/mcc-native-pc-world-programs.json"));
    const auto hud=json::parse(read(repo/"tools/data/mcc-native-hud-bootstrap.json"));
    const auto sky=json::parse(read(repo/"tools/data/mcc-native-pc-sky-closure.json"));
    const auto world=json::parse(read(repo/"tools/data/mcc-native-pc-world-closure.json"));
    unsigned cases{},added{};
    for(const auto& map:policy.at("maps")) {
        const auto name=map.at("map").get<std::string>();
        const auto count=map.at("records").size();
        need((name=="needle"&&count==34)||(name=="street_sweeper"&&count==34),"finite program census changed");
        json base={{"schema","H2_PRESERVED_NATIVE_SHADERS_V1"},{"map",name},{"shader_tags",hud.at("records")}};
        for(const auto& row:sky.at("maps"))if(row.at("map")==name)
            for(const auto& record:row.at("records"))base["shader_tags"].push_back(record);
        for(const auto& record:world.at("records"))base["shader_tags"].push_back(record);
        const auto input=native.parent_path()/"halo2/h2_maps_win64_dx11"/(name+".map");
        const auto bytes=read(input);
        need(bytes.size()==map.at("bytes").get<std::size_t>()&&sha(bytes)==map.at("sha256").get<std::string>(),
             "actual PC program input changed");
        const auto reader=[&](const fs::path& root) {
            return [&,root](const fs::path& path) {
                for(const auto& [source,_]:map.at("sources").items())
                    if(path==root/source/"library.json")return read(native/source/"library.json");
                throw halo2::FormatError("program snapshot escaped admitted source set");
            };
        };
        const auto run=[&](json& bank,const fs::path& path,const fs::path& root,std::uint64_t size,
                           const std::string& digest,const halo2::MccNativeLibraryRead& snapshot) {
            return halo2::supplement_mcc_native_pc_world_programs(bank,path,root,size,digest,snapshot);
        };
        auto good=base;const auto payloads=run(good,input,native,bytes.size(),sha(bytes),reader(native));
        need(!payloads.empty()&&good.at("shader_tags").size()==base.at("shader_tags").size()+count,
             "complete additional programs were not admitted");
        for(unsigned i=0;i<base.at("shader_tags").size();++i)
            need(good.at("shader_tags").at(i)==base.at("shader_tags").at(i),"existing bank record changed");
        for(unsigned i=unsigned(base.at("shader_tags").size());i<good.at("shader_tags").size();++i) {
            const auto& record=good.at("shader_tags").at(i);
            need(record.at("group")=="stem"||record.at("group")=="spas"||record.at("group")=="vrtx",
                 "program closure imported an image/material/global record");
            need(record.at("strings").empty(),"program closure altered StringIDs");
        }
        ++cases;added+=unsigned(count);
        const auto unchanged=[&](json bank,const fs::path& path,std::uint64_t size,const std::string& digest) {
            const auto before=bank;
            need(run(bank,path,native,size,digest,reader(native)).empty()&&bank==before,
                 "unknown/custom source changed preceding caller bank");++cases;
        };
        unchanged(base,input,bytes.size()+1,sha(bytes));
        unchanged(base,input,bytes.size(),std::string(64,'0'));
        unchanged(base,input.parent_path()/"unknown.map",bytes.size(),sha(bytes));
        unchanged(good,input,bytes.size(),sha(bytes));
        auto partial=base;partial["shader_tags"].erase(partial["shader_tags"].begin());
        unchanged(partial,input,bytes.size(),sha(bytes));
        auto altered=base;altered["shader_tags"][0]["bytes"]=1;
        unchanged(altered,input,bytes.size(),sha(bytes));
        const auto custom=scratch/(name+"-map-local-library");
        fs::create_directories(custom/name);write(custom/name/"library.json","{}");
        auto untouched=base;
        need(run(untouched,input,custom,bytes.size(),sha(bytes),reader(custom)).empty()&&untouched==base,
             "existing map-local native library was supplemented");++cases;
        const auto rejection=[&](const fs::path& root,const halo2::MccNativeLibraryRead& snapshots) {
            auto bank=base;bool failed=false;
            try{run(bank,input,root,bytes.size(),sha(bytes),snapshots);}catch(const halo2::FormatError&){failed=true;}
            need(failed&&bank==base,"corrupt source was admitted or caller bank was partially changed");++cases;
        };
        rejection(native,{});
        for(const auto& [changed,_]:map.at("sources").items()) {
            rejection(native,[&](const auto& path) {
                auto original=reader(native)(path);
                if(path==native/changed/"library.json") {
                    auto value=json::parse(original);value["shader_tags"][0]["bytes"]=1;return value.dump();
                }
                return original;
            });
        }
        const auto copy=[&](const fs::path& root) {
            std::set<std::pair<std::string,std::string>> files;
            for(const auto& item:map.at("records"))
                files.emplace(item.at("source_map").get<std::string>(),item.at("record").at("payload").get<std::string>());
            for(const auto& item:map.at("equivalent_existing_identities"))
                files.emplace(item.at("source_map").get<std::string>(),item.at("candidate_payload").get<std::string>());
            for(const auto& [source,filename]:files) {
                fs::create_directories(root/source/"payloads");
                fs::copy_file(native/source/"payloads"/filename,root/source/"payloads"/filename);
            }
        };
        for(const auto ordinal:{0u,unsigned(count-1)}) {
            const auto duplicate=scratch/(name+"-corrupt-added-"+std::to_string(ordinal));copy(duplicate);
            const auto& item=map.at("records").at(ordinal);
            const auto path=duplicate/item.at("source_map").get<std::string>()/"payloads"/
                item.at("record").at("payload").get<std::string>();
            auto raw=read(path);need(!raw.empty(),"empty program fixture");raw[0]^=1;write(path,raw);
            rejection(duplicate,reader(duplicate));
        }
        need(!map.at("equivalent_existing_identities").empty(),"missing complete overlap proof");
        const auto& overlap=map.at("equivalent_existing_identities")[0];
        const auto duplicate=scratch/(name+"-corrupt-overlap");copy(duplicate);
        const auto path=duplicate/overlap.at("source_map").get<std::string>()/"payloads"/
            overlap.at("candidate_payload").get<std::string>();
        auto raw=read(path);raw.back()^=1;write(path,raw);rejection(duplicate,reader(duplicate));
    }
    std::cout<<cases<<" exact input, namespace, full-source/payload and atomic rejection cases; "
             <<added<<" additional program records admitted\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
