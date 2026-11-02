#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include "halo2/source_runtime_resource_preparation.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "helpers/original_sound_preparation_inputs.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace {
using namespace halo2;using J=nlohmann::json;namespace fs=std::filesystem;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
std::string hash(std::span<const std::byte> bytes){std::array<unsigned char,32> digest;unsigned size{};
    need(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&size,EVP_sha256(),nullptr)==1&&size==32,"Catalog qualification hash failed");
    constexpr char hex[]="0123456789abcdef";std::string out;for(auto b:digest){out+=hex[b>>4];out+=hex[b&15];}return out;}
std::string text_hash(const std::string& text){return hash(std::as_bytes(std::span(text.data(),text.size())));}
std::string file(const fs::path& path){std::ifstream input(path,std::ios::binary);need(bool(input),"Catalog qualification input is missing");return{std::istreambuf_iterator<char>(input),{}};}
std::string write(const fs::path& directory,const J& value){const auto raw=value.dump(2)+'\n';std::ofstream output(directory/"catalog.json");output<<raw;need(bool(output),"Catalog qualification output failed");return text_hash(raw);}
template<class F>void rejects(F&& call,unsigned& controls){bool refused=false;try{call();}catch(const std::exception&){refused=true;}
    need(refused,"Invalid prepared original catalog was admitted");++controls;}
}
int main(int argc,char** argv){J report{{"passed",false},{"goal_complete",false},{"normal_product_catalog_activated",false},{"world_or_device_verified",false},{"retained_large_artifacts",J::array()}};
    if(argc!=6||!fs::is_regular_file(argv[1])||!fs::is_regular_file(argv[3]))return 77;
    try{
        report["phase"]="producer_inputs";const auto raw=file(argv[3]);need(text_hash(raw)==argv[4],"Prototype catalog differs from its external admission pin");
        const auto prototype=J::parse(raw);const fs::path map=argv[1];const std::string context=argv[2];
        auto cache=std::make_shared<const Cache>(map,map.parent_path(),OriginalData::native_tu15());
        const test_original_sound::Inputs expected(*cache,prototype,context);
        const auto source_digest=hash(cache->source_runtime_cache_bytes());
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch&&fs::is_directory(scratch),"Catalog test requires owned scratch");
        const auto directory=fs::path(scratch)/"portable";fs::create_directory(directory);J banks=J::array();
        for(const auto& entry:prototype.at("cases"))if(entry.at("map")==context){
            const fs::path from=entry.at("directory").get<std::string>();const auto name=from.filename();
            need(from.parent_path()==fs::path(argv[3]).parent_path()&&name.string().starts_with("bank-"),"Prototype fixture is not an owned prepared bank");
            fs::create_directory(directory/name);
            // Both sources are disposable exporter-owned files, never original assets.
            for(const auto leaf:{"manifest.json","resources.bin"})fs::create_hard_link(from/leaf,directory/name/leaf);
            banks.push_back({{"directory",name.string()},{"sha256",entry.at("sha256")}});
        }
        const J recipe{{"schema","H2_PREPARED_ORIGINAL_SOUND_CATALOG_TU15_V1"},{"contexts",J::array({{
            {"runtime_map",map.filename().string()},{"original_map",context},{"source_runtime_sha256",source_digest},{"banks",banks}}})}};
        const auto pin=write(directory,recipe);report["phase"]="catalog_binding";
        const auto catalog=PreparedOriginalSoundCatalogTu15::load(directory,pin);auto bound=catalog->bind(*cache);const auto actual=bound->input(*cache);
        need(bound->original_context()==context&&bound->size()==expected.bindings.size()&&actual.bindings.size()==expected.bindings.size(),"Catalog changed the complete selected-map replacement set");
        std::uint64_t encoded_routes{};
        for(unsigned n=0;n<actual.bindings.size();++n){const auto& a=actual.bindings[n];const auto& b=expected.bindings[n];
            need(a.sound==b.sound&&a.bank->metadata().name==b.bank->metadata().name&&a.bank->metadata().sound==b.bank->metadata().sound&&
                std::ranges::equal(a.bank->sectors(),b.bank->sectors()),"Catalog differs from independently admitted original input");
            for(unsigned pitch=0;pitch<a.bank->metadata().pitch_ranges.size();++pitch)for(unsigned p=0;p<a.bank->metadata().pitch_ranges[pitch].permutations.size();++p){
                const auto& x=a.bank->encoded(pitch,p);const auto& y=b.bank->encoded(pitch,p);
                need(x.data==y.data&&x.codec==y.codec&&x.sample_rate==y.sample_rate&&x.channels==y.channels&&x.samples==y.samples,"Catalog changed original encoded order or format");++encoded_routes;
            }
        }
        report["phase"]="preparation";SourceRuntimeResources::Identity identity;
        {SourceRuntimeResourcePreparation prepared(cache,*bound);auto metadata=take_prepared_mcc_world_metadata(*cache,0x90000000u,0x60000000u);
            need(bool(metadata),"Catalog preparation lost its canonical metadata");SourceRuntimeResources resources(*cache);resources.require_sound_plan(metadata->sound);
            need(resources.original_sound_owner()==metadata->delivery.original_sound_owner()&&metadata->sound.original->bindings().size()==expected.bindings.size(),"Catalog preparation split original resource ownership");identity=resources.identity();}
        unsigned controls{};rejects([&]{(void)PreparedOriginalSoundCatalogTu15::load(directory,std::string(64,'0'));},controls);
        const auto malformed=[&](auto change){auto bad=recipe;change(bad);const auto changed=write(directory,bad);
            rejects([&]{(void)PreparedOriginalSoundCatalogTu15::load(directory,changed)->bind(*cache);},controls);write(directory,recipe);};
        malformed([](auto& value){value["contexts"][0]["source_runtime_sha256"]=std::string(64,'0');});
        malformed([](auto& value){value["contexts"][0]["runtime_map"]="other.map";});
        malformed([](auto& value){value["contexts"][0]["original_map"]="other.map";});
        malformed([](auto& value){value["contexts"][0]["banks"][0]["directory"]="../escape";});
        malformed([](auto& value){value["contexts"][0]["banks"].push_back(value["contexts"][0]["banks"][0]);});
        malformed([](auto& value){value["contexts"][0]["banks"][0]["sha256"]=std::string(64,'0');});
        fs::create_directory_symlink(directory/banks[0].at("directory").get<std::string>(),directory/"alias");
        malformed([](auto& value){value["contexts"][0]["banks"][0]["directory"]="alias";});
        {const Cache foreign(map,map.parent_path(),OriginalData::native_tu15());rejects([&]{(void)bound->input(foreign);},controls);}
        report["phase"]="relocation";const auto moved=directory.parent_path()/"moved";fs::rename(directory,moved);
        const auto relocated=PreparedOriginalSoundCatalogTu15::load(moved,pin)->bind(*cache);
        {SourceRuntimeResourcePreparation prepared(cache,*relocated);auto metadata=take_prepared_mcc_world_metadata(*cache,0x90000000u,0x60000000u);
            SourceRuntimeResources resources(*cache);need(metadata&&resources.identity()==identity,"Relocated catalog changed immutable original resource identity");resources.require_sound_plan(metadata->sound);}
        bound.reset();need(actual.bindings.size()==expected.bindings.size()&&!actual.bindings.front().bank->encoded(0,0).data.empty(),"Dropped catalog wrapper invalidated its retained input storage");
        need(hash(cache->source_runtime_cache_bytes())==source_digest,"Catalog admission altered Cache bytes");
        report.update(J{{"passed",true},{"map",context},{"original_bindings",expected.bindings.size()},{"encoded_routes",encoded_routes},{"negative_controls",controls},
            {"portable_catalog_sha256",pin},{"source_runtime_sha256",source_digest},{"relocation_identity_equal",true},{"input_storage_retained",true},{"canonical_preparation_executed",true},{"raw_cache_unchanged",true}});report.erase("phase");
    }catch(const std::exception& error){report["error"]=error.what();}
    std::ofstream output(argv[5]);output<<report.dump(2)<<'\n';if(!output)return 2;std::cout<<report.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
