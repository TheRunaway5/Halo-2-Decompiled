#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace {
using namespace halo2;using J=nlohmann::json;namespace fs=std::filesystem;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
std::string hash(std::span<const std::byte> bytes){std::array<unsigned char,32> digest;unsigned count{};
    need(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&count,EVP_sha256(),nullptr)==1&&count==32,"Cannot hash original catalog context");
    constexpr char hex[]="0123456789abcdef";std::string out;for(auto b:digest){out+=hex[b>>4];out+=hex[b&15];}return out;}
std::string text_hash(const std::string& value){return hash(std::as_bytes(std::span(value.data(),value.size())));}
std::string read(const fs::path& path){need(fs::is_regular_file(fs::symlink_status(path))&&fs::file_size(path)<=64u*1024u*1024u,"Invalid original exporter catalog");
    std::ifstream input(path,std::ios::binary);need(bool(input),"Missing original exporter catalog");return {std::istreambuf_iterator<char>(input),{}};}
void write(const fs::path& path,const J& value){const auto state=fs::symlink_status(path);
    need(!fs::exists(state)||(fs::is_regular_file(state)&&fs::hard_link_count(path)==1),"Catalog output is not a private regular writer");
    std::ofstream output(path,std::ios::binary|std::ios::trunc);output<<value.dump(2)<<'\n';need(bool(output),"Cannot write original catalog context");}
}
int main(int argc,char** argv){
    if(argc!=7){std::cerr<<"Usage: prepare_original_sound_catalog_context_tu15 MCC_MAP ORIGINAL_CONTEXT EXPORT_CATALOG EXTERNAL_SHA256 OWNED_BUNDLE RECEIPT\n";return 2;}
    J report{{"passed",false},{"goal_complete",false},{"world_or_device_verified",false}};
    try {
        const auto parent=std::getenv("HALO2_TEST_SCRATCH");need(parent,"Catalog production requires owned scratch");
        const auto scratch=fs::canonical(parent),directory=fs::canonical(argv[5]);
        const auto relative=directory.lexically_relative(scratch);
        need(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=".."&&directory!=scratch&&
            fs::is_directory(fs::symlink_status(argv[5])),"Catalog output must be a private owned bundle");
        const auto raw=read(argv[3]);need(text_hash(raw)==argv[4],"Original exporter catalog differs from external pin");
        const auto prototype=J::parse(raw);J banks=J::array();const std::string context=argv[2];
        for(const auto& entry:prototype.at("cases"))if(entry.at("map")==context) {
            const fs::path from=entry.at("directory").get<std::string>();const auto name=from.filename().string();
            need(name.starts_with("bank-")&&from.parent_path()==fs::path(argv[3]).parent_path(),"Exporter bank escaped its declared owner");
            banks.push_back({{"directory",name},{"sha256",entry.at("sha256")}});
        }
        need(!banks.empty(),"No original banks for the exact requested context");
        const fs::path path=argv[1];const Cache cache(path,path.parent_path(),OriginalData::native_tu15());
        need(!cache.original_sound_catalog(),"Context producer requires a pre-catalog prepared map");
        const J row{{"runtime_map",path.filename().string()},{"original_map",context},
            {"source_runtime_sha256",hash(cache.source_runtime_cache_bytes())},{"banks",banks}};
        const J catalog{{"schema","H2_PREPARED_ORIGINAL_SOUND_CATALOG_TU15_V1"},{"contexts",J::array({row})}};
        const auto bytes=catalog.dump(2)+'\n';
        write(directory/"catalog.json",catalog);
        const auto bound=PreparedOriginalSoundCatalogTu15::load(directory,text_hash(bytes))->bind(cache);
        need(bound->size()==banks.size()&&bound->original_context()==context,"Produced catalog binding lost its exact context");
        report.update(J{{"passed",true},{"context",row},{"runtime_path",path.string()},{"original_bindings",bound->size()},
            {"export_catalog_sha256",argv[4]},{"actual_cache_conversion_and_catalog_binding",true}});
    }catch(const std::exception& error){report["error"]=error.what();}
    try{write(argv[6],report);}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
    std::cout<<report.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
