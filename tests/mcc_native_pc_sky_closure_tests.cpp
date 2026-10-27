#include "halo2/cache.hpp"
#include "halo2/mcc_native_library.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using namespace halo2;
using Json=nlohmann::json;
namespace fs=std::filesystem;
unsigned checks{},negative_controls{};
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
Json json_file(const fs::path& path){std::ifstream stream(path);return Json::parse(stream);}
std::vector<std::byte> unhex(const std::string& value){
    std::vector<std::byte> result;
    for(std::size_t i=0;i<value.size();i+=2)result.push_back(std::byte(std::stoul(value.substr(i,2),nullptr,16)));
    return result;
}
void cases(const Json& row,const Json& fixture,const Json& hud,const fs::path& root) {
    std::vector<Tag> directory(65536);
    for(auto& tag:directory)tag.datum=0xffffffffu;
    for(const auto& owner:row.at("owners")) {
        const auto datum=owner.at("datum").get<std::uint32_t>();
        directory.at(datum&0xffffu)={owner.at("group"),owner.at("name"),datum,owner.at("address"),owner.at("size")};
    }
    std::map<std::uint32_t,std::vector<std::byte>> regions;
    for(const auto& region:fixture.at("regions"))regions.emplace(region.at("address"),unhex(region.at("hex")));
    const auto read=[&](std::uint32_t address,std::size_t size)->std::span<const std::byte> {
        for(const auto& [base,raw]:regions)if(address>=base && address-base<=raw.size() && size<=raw.size()-(address-base))
            return std::span(raw).subspan(address-base,size);
        throw FormatError("Missing complete source region");
    };
    const auto source=root/row.at("source_map").get<std::string>()/"library.json";
    const auto retained=json_file(source);unsigned library_reads{};
    const MccNativeLibraryRead snapshot=[&](const fs::path& path){
        ++library_reads;check(path==source,"PC closure requested an unrelated snapshot");return retained.dump();
    };
    const auto name=row.at("map").get<std::string>();
    const auto path=root/(name+".map");const auto bytes=row.at("bytes").get<std::uint64_t>();
    const auto fingerprint=row.at("sha256").get<std::string>();
    const Json initial{{"schema","H2_PRESERVED_NATIVE_SHADERS_V1"},{"map",name},{"shader_tags",hud.at("records")}};
    const auto reject=[&](const std::vector<Tag>& tags,const MccShaderRead& body,const std::string& hash,
                          std::uint64_t size,const Json& start=Json{}) {
        auto bank=start.is_null()?initial:start;const auto before=bank;library_reads=0;
        const auto result=supplement_mcc_native_pc_sky_library(bank,path,tags,body,root,size,hash,snapshot);
        check(result.empty() && bank==before,"Unproved PC sky data partially changed its native bank");
        check(library_reads==0,"Failed source data accessed native closure metadata");++negative_controls;
    };
    reject(directory,read,std::string(64,'0'),bytes);
    reject(directory,read,fingerprint,bytes+1);
    auto changed=directory;const auto first=row.at("owners").at(0).at("datum").get<std::uint32_t>()&0xffffu;
    changed[first].datum^=0x10000u;reject(changed,read,fingerprint,bytes);
    changed=directory;changed[first].name+="_unproved";reject(changed,read,fingerprint,bytes);
    changed=directory;changed[first].address+=4;reject(changed,read,fingerprint,bytes);
    changed=directory;++changed[first].size;reject(changed,read,fingerprint,bytes);
    changed=directory;changed.push_back(changed[first]);reject(changed,read,fingerprint,bytes);
    // Every complete borrowed root/bank, bitmap metadata and animation function
    // gets its own actual-byte negative control, including aliased banks.
    for(auto& [address,data]:regions)if(!data.empty()) {
        data.front()^=std::byte{1};reject(directory,read,fingerprint,bytes);data.front()^=std::byte{1};
    }
    auto custom=initial;custom["shader_tags"].push_back(Json{{"custom",true}});
    reject(directory,read,fingerprint,bytes,custom);
    custom=initial;custom["custom"]=true;reject(directory,read,fingerprint,bytes,custom);
    auto bank=initial;const auto result=supplement_mcc_native_pc_sky_library(bank,path,directory,read,root,bytes,fingerprint,snapshot);
    check(result.size()==row.at("records").size(),"Complete paired PC sky closure was rejected");
    check(bank.at("shader_tags").size()==hud.at("records").size()+row.at("records").size(),"PC sky native closure is incomplete");
    for(unsigned index=0;index<hud.at("records").size();++index)
        check(bank.at("shader_tags").at(index)==hud.at("records").at(index),"Existing HUD native program changed");
    check(!bank.contains("rasterizer"),"PC sky closure changed map rasterizer globals");
    const auto again=supplement_mcc_native_pc_sky_library(bank,path,directory,read,root,bytes,fingerprint,snapshot);
    check(again.empty(),"PC sky closure was published repeatedly");
    // Native source snapshots and complete source bodies have independent gates.
    const auto native_reject=[&](const MccNativeLibraryRead& source_read) {
        auto value=initial;bool threw{};
        try{(void)supplement_mcc_native_pc_sky_library(value,path,directory,read,root,bytes,fingerprint,source_read);}
        catch(const FormatError&){threw=true;}
        check(threw && value==initial,"Changed native PC sky program partially published");++negative_controls;
    };
    native_reject([&](const fs::path&){auto edited=retained;edited["custom"]=true;return edited.dump();});
    const auto payload=result.begin()->second;
    std::fstream stream(payload,std::ios::in|std::ios::out|std::ios::binary);char first_byte{};stream.read(&first_byte,1);
    stream.seekp(0);stream.put(first_byte^1);stream.close();native_reject(snapshot);
    stream.open(payload,std::ios::in|std::ios::out|std::ios::binary);stream.put(first_byte);stream.close();
}
}
int main(int argc,char** argv)try {
    check(argc==3,"Supply exact fixture and owned native-payload directory");
    check(std::getenv("HALO2_TEST_SCRATCH"),"PC sky fixture requires owned scratch");
    const auto fixture=json_file(argv[1]);const fs::path root(argv[2]);
    for(const auto& row:fixture.at("policy").at("maps")) {
        const auto& rows=fixture.at("regions").at("maps");
        const auto found=std::ranges::find_if(rows,[&](const auto& value){return value.at("map")==row.at("map");});
        check(found!=rows.end(),"Missing paired PC source data fixture");cases(row,*found,fixture.at("hud"),root);
    }
    std::cout<<"pc_sky_maps=2 native_materials=9 checks="<<checks<<" strict_negative_controls="<<negative_controls<<'\n';
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
