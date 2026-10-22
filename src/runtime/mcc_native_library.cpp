#include "halo2/mcc_native_library.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
#include <string_view>
#include <utility>

namespace halo2 {
namespace {
using Json=nlohmann::json;
using Key=std::pair<std::string,std::string>;
struct ClosureLibrary {std::string_view name,sha256;};
#include "runtime/mcc_native_library_closure.inc"
#include "ui/hud/mcc_native_hud_bootstrap.inc"
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> hash{};unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&size,EVP_sha256(),nullptr)!=1 || size!=hash.size())
        throw FormatError("Cannot authenticate retained native material library");
    constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(64);
    for(const auto b:hash){result+=digits[b>>4];result+=digits[b&15];}return result;
}
std::string digest(const Json& value) {
    const auto bytes=value.dump();return digest(std::as_bytes(std::span(bytes)));
}
std::vector<std::byte> read_payload(const std::filesystem::path& path,std::size_t size) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in || in.tellg()<0 || std::uint64_t(in.tellg())!=size)
        throw FormatError("Authenticated HUD closure file is unavailable or has changed extent");
    std::vector<std::byte> result(size);in.seekg(0);
    if(size && !in.read(reinterpret_cast<char*>(result.data()),std::streamsize(size)))
        throw FormatError("Authenticated HUD closure file is truncated");
    return result;
}
Key key(const Json& record) {return {record.at("group").get<std::string>(),record.at("name").get<std::string>()};}
}
std::map<std::string,std::filesystem::path> supplement_mcc_native_library(
    Json& library,const std::filesystem::path& root,std::span<const Tag> runtime_directory) {
    return supplement_mcc_native_library(library,root,runtime_directory,{});
}
std::map<std::string,std::filesystem::path> supplement_mcc_native_library(
    Json& library,const std::filesystem::path& root,std::span<const Tag> runtime_directory,
    const MccNativeLibraryRead& read_library) {
    std::map<std::string,std::filesystem::path> payloads;
    const auto map=library.at("map").get<std::string>();
    const auto admitted=std::ranges::find_if(closure_libraries,[&](const auto& p){return p.name==map;});
    // Customized libraries keep their own authored definitions and namespace.
    if(admitted==closure_libraries.end() || digest(library)!=admitted->sha256)return payloads;
    std::map<Key,const Json*> existing;
    for(const auto& record:library.at("shader_tags"))
        if(!existing.emplace(key(record),&record).second)throw FormatError("Duplicate retained native material identity");
    std::set<Key> wanted;
    for(const auto& [group,name]:closure_identities)if(!existing.contains({std::string(group),std::string(name)}))
        wanted.emplace(group,name);
    // Armory retains shared Covenant weapon HUDs even before the player has a
    // weapon. Their exact existing material instances use its original HUD
    // templates; keep this dependency scoped to the authenticated map bank.
    for(const auto& [owner,group,name]:scoped_closure_identities)
        if(owner==map && !existing.contains({std::string(group),std::string(name)}))
            wanted.emplace(group,name);
    if(wanted.empty())return payloads;
    const auto source_directory=root/std::string(closure_source_map);
    const auto source_path=source_directory/"library.json";
    Json source;
    if(read_library) {
        const auto snapshot=read_library(source_path);
        if(snapshot.size()>16u*1024u*1024u)throw FormatError("Excessive admitted native template library");
        source=Json::parse(snapshot);
    }
    else {
        std::ifstream stream(source_path,std::ios::binary|std::ios::ate);
        if(!stream || stream.tellg()<0 || std::uint64_t(stream.tellg())>16u*1024u*1024u)
            throw FormatError("Required retained native template library is unavailable");
        stream.seekg(0);source=Json::parse(stream);
    }
    if(digest(source)!=closure_source_sha256)
        throw FormatError("Required retained native template library differs from its audited closure");
    // Stage the complete missing closure before changing the caller's bank.
    Json additions=Json::array();std::set<Key> found;
    for(const auto& record:source.at("shader_tags"))if(wanted.contains(key(record))) {
        const auto identity=key(record);
        if(!found.insert(identity).second)throw FormatError("Ambiguous retained native template identity");
        for(const auto& reference:record.at("references")) {
            const Key target{reference.at(1).at(0).get<std::string>(),reference.at(1).at(1).get<std::string>()};
            bool runtime_bitmap=false;
            if(target.first=="bitm") {
                unsigned matches=0;
                for(const auto& tag:runtime_directory)if(tag.group==target.first && tag.name==target.second) {
                    if(tag.datum==0xffffffffu || !tag.address || !tag.size)
                        throw FormatError("Retained native material bitmap has no resident MCC owner");
                    ++matches;
                }
                if(matches!=1)throw FormatError("Retained native material requires one exact MCC bitmap owner");
                runtime_bitmap=true;
            }
            if(!existing.contains(target) && !wanted.contains(target) && !runtime_bitmap)
                throw FormatError("Retained native template dependency escapes its audited closure");
        }
        const auto filename=record.at("payload").get<std::string>();
        if(filename.size()!=68 || filename.substr(64)!=".bin" || filename.find_first_not_of("0123456789abcdef",0)<64)
            throw FormatError("Invalid retained native template payload identity");
        additions.push_back(record);payloads.emplace(filename,source_directory/"payloads"/filename);
    }
    if(found!=wanted)throw FormatError("Retained native template closure is incomplete");
    auto completed=library;
    for(const auto& record:additions)completed.at("shader_tags").push_back(record);
    library.swap(completed);return payloads;
}
std::map<std::string,std::filesystem::path> bootstrap_mcc_native_hud_library(
    Json& library,const std::filesystem::path& source_map,std::span<const Tag> directory,
    const MccShaderRead& read,const std::function<std::string(std::uint32_t)>& string_id,
    const std::filesystem::path& root) {
    const auto policy=Json::parse(native_hud_bootstrap_policy);
    const auto name=source_map.stem().string();
    const auto map=std::ranges::find_if(policy.at("maps"),[&](const auto& row){return row.at("map")==name;});
    if(map==policy.at("maps").end() || !library.is_null() || std::filesystem::exists(root/name/"library.json"))return {};
    std::error_code error;
    const auto size=std::filesystem::file_size(source_map,error);
    if(error || size!=map->at("bytes").get<std::uint64_t>())return {};
    std::vector<std::byte> source;
    try {source=read_payload(source_map,map->at("bytes").get<std::size_t>());}
    catch(const FormatError&) {return {};}
    return bootstrap_mcc_native_hud_library(library,source_map,directory,read,string_id,root,
        size,digest(std::span<const std::byte>(source)),{});
}
std::map<std::string,std::filesystem::path> bootstrap_mcc_native_hud_library(
    Json& library,const std::filesystem::path& source_map,std::span<const Tag> directory,
    const MccShaderRead& read,const std::function<std::string(std::uint32_t)>& string_id,
    const std::filesystem::path& root,std::uint64_t admitted_bytes,
    std::string_view admitted_sha256,const MccNativeLibraryRead& read_library) {
    const auto policy=Json::parse(native_hud_bootstrap_policy);
    const auto name=source_map.stem().string();
    const auto map=std::ranges::find_if(policy.at("maps"),[&](const auto& row){return row.at("map")==name;});
    if(map==policy.at("maps").end() || !library.is_null())return {};
    // Never supplement an existing or custom map-local library, even when a
    // caller accidentally reaches this missing-manifest-only entry point.
    if(std::filesystem::exists(root/name/"library.json"))return {};
    try {
        if(admitted_bytes!=map->at("bytes").get<std::uint64_t>() ||
           admitted_sha256!=map->at("sha256").get<std::string>())return {};
        for(const auto& owner:policy.at("owners")) {
            const auto datum=owner.at("datum").get<std::uint32_t>();const Tag* found=nullptr;
            for(const auto& tag:directory) {
                const bool identity=tag.group==owner.at("group").get<std::string>() && tag.name==owner.at("name").get<std::string>();
                if(tag.datum==datum || identity) {
                    if(!identity || tag.datum!=datum || found)return {};
                    found=&tag;
                }
            }
            if(!found || (datum&0xffffu)>=directory.size() || &directory[datum&0xffffu]!=found ||
               found->address!=owner.at("address") || found->size!=owner.at("size") ||
               digest(read(found->address,found->size))!=owner.at("sha256").get<std::string>())return {};
        }
        for(const auto& region:policy.at("grey_borrowed_regions"))
            if(digest(read(region.at("address"),region.at("bytes")))!=region.at("sha256").get<std::string>())return {};
        for(const auto& [id,name]:policy.at("grey_strings").items())
            if(string_id(std::stoul(id))!=name.get<std::string>())return {};
        // An already-owned Xbox identity is custom/partial in these two
        // complete source namespaces. Never replace it or alias the DX9 row.
        for(const auto& tag:directory)if(tag.group=="vrtx" &&
            tag.name=="rasterizer\\vertex_shaders_xbox\\screen")return {};
    } catch(const FormatError&) {return {};}
    const auto source_directory=root/policy.at("source_map").get<std::string>();
    Json retained;
    if(read_library) {
        const auto snapshot=read_library(source_directory/"library.json");
        if(snapshot.size()>16u*1024u*1024u)throw FormatError("Excessive admitted HUD closure library");
        retained=Json::parse(snapshot);
    }
    else {
        std::ifstream stream(source_directory/"library.json",std::ios::binary|std::ios::ate);
        if(!stream || stream.tellg()<0 || std::uint64_t(stream.tellg())>16u*1024u*1024u)
            throw FormatError("Required retained HUD closure library is unavailable");
        stream.seekg(0);retained=Json::parse(stream);
    }
    if(digest(retained)!=policy.at("source_canonical_sha256").get<std::string>())
        throw FormatError("Required retained HUD closure library differs from its audited source");
    Json records=Json::array();std::map<std::string,std::filesystem::path> payloads;
    for(const auto& expected:policy.at("records")) {
        const Json* found=nullptr;
        for(const auto& record:retained.at("shader_tags"))if(key(record)==key(expected)) {
            if(found || record!=expected)throw FormatError("Retained HUD closure identity/body differs");
            found=&record;
        }
        if(!found)throw FormatError("Retained HUD closure owner is missing");
        const auto filename=found->at("payload").get<std::string>();
        const auto path=source_directory/"payloads"/filename;
        if(digest(std::span<const std::byte>(read_payload(path,found->at("bytes").get<std::size_t>())))!=filename.substr(0,64))
            throw FormatError("Retained HUD closure payload differs from its authenticated program");
        records.push_back(*found);payloads.emplace(filename,path);
    }
    Json staged{{"schema","H2_PRESERVED_NATIVE_SHADERS_V1"},{"map",name},{"shader_tags",std::move(records)}};
    library.swap(staged);return payloads;
}
}
