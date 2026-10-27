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
#include "runtime/mcc_native_pc_sky_closure.inc"
#include "runtime/mcc_native_pc_world_closure.inc"
#include "runtime/mcc_native_pc_world_programs.inc"
#include "runtime/mcc_native_vehicle_restoration.inc"
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
    if(error || !size || size>=16u*1024u*1024u)return {};
    std::vector<std::byte> source;
    try {source=read_payload(source_map,std::size_t(size));}
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
    const Json* map=nullptr;
    const auto select=[&](const Json& rows) {for(const auto& row:rows)
        if(row.at("map")==name && admitted_bytes==row.at("bytes").get<std::uint64_t>() &&
           admitted_sha256==row.at("sha256").get<std::string>())map=&row;};
    select(policy.at("maps"));if(policy.contains("prepared_inputs"))select(policy.at("prepared_inputs"));
    if(!map || !library.is_null())return {};
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

std::map<std::string,std::filesystem::path> supplement_mcc_native_pc_sky_library(
    Json& library,const std::filesystem::path& source_map,std::span<const Tag> directory,
    const MccShaderRead& read,const std::filesystem::path& root,
    std::uint64_t admitted_bytes,std::string_view admitted_sha256,
    const MccNativeLibraryRead& read_library) {
    const auto policy=Json::parse(native_pc_sky_closure_policy);
    const auto hud=Json::parse(native_hud_bootstrap_policy);
    const auto name=source_map.stem().string();
    const auto map=std::ranges::find_if(policy.at("maps"),[&](const auto& row){return row.at("map")==name;});
    if(map==policy.at("maps").end() || !library.is_object() || library.size()!=3 ||
       library.value("schema","")!="H2_PRESERVED_NATIVE_SHADERS_V1" || library.value("map","")!=name ||
       library.at("shader_tags")!=hud.at("records") ||
       admitted_bytes!=map->at("bytes").get<std::uint64_t>() ||
       admitted_sha256!=map->at("sha256").get<std::string>() ||
       std::filesystem::exists(root/name/"library.json"))return {};
    try {
        for(const auto& expected:map->at("owners")) {
            const auto datum=expected.at("datum").get<std::uint32_t>();const Tag* found=nullptr;
            for(const auto& tag:directory) {
                const bool identity=tag.group==expected.at("group").get<std::string>() &&
                    tag.name==expected.at("name").get<std::string>();
                if(tag.datum==datum || identity) {
                    if(found || !identity || tag.datum!=datum)return {};
                    found=&tag;
                }
            }
            if(!found || (datum&0xffffu)>=directory.size() || &directory[datum&0xffffu]!=found ||
               found->address!=expected.at("address") || found->size!=expected.at("size"))return {};
        }
        for(const auto& expected:map->at("regions"))
            if(digest(read(expected.at("address"),expected.at("bytes")))!=expected.at("sha256").get<std::string>())return {};
    }catch(const FormatError&){return {};}
    const auto source_directory=root/map->at("source_map").get<std::string>();
    if(!read_library)throw FormatError("Native PC sky closure requires admitted source library snapshots");
    const auto snapshot=read_library(source_directory/"library.json");
    if(snapshot.size()>16u*1024u*1024u)throw FormatError("Excessive admitted PC sky library");
    const auto retained=Json::parse(snapshot);
    if(digest(retained)!=map->at("source_canonical_sha256").get<std::string>())
        throw FormatError("Native PC sky source differs from its complete paired library");
    Json additions=Json::array();std::map<std::string,std::filesystem::path> result;std::set<Key> wanted;
    for(const auto& expected:map->at("records")) {
        const Json* found=nullptr;
        for(const auto& record:retained.at("shader_tags"))if(key(record)==key(expected)) {
            if(found || record!=expected)throw FormatError("Native PC sky closure identity/body differs");
            found=&record;
        }
        if(!found || !wanted.insert(key(expected)).second)
            throw FormatError("Native PC sky closure owner is absent or ambiguous");
        const auto filename=found->at("payload").get<std::string>();
        const auto path=source_directory/"payloads"/filename;
        if(digest(std::span<const std::byte>(read_payload(path,found->at("bytes").get<std::size_t>())))!=filename.substr(0,64))
            throw FormatError("Native PC sky payload differs from its authenticated program");
        additions.push_back(*found);result.emplace(filename,path);
    }
    for(const auto& record:additions)for(const auto& reference:record.at("references")) {
        const auto& identity=reference.at(1);
        const Key target{identity.at(0).get<std::string>(),identity.at(1).get<std::string>()};
        if(target.first=="bitm") {
            unsigned matches=0;
            for(const auto& owner:map->at("owners"))
                matches+=owner.at("group")==target.first && owner.at("name")==target.second;
            if(matches!=1)throw FormatError("Native PC sky bitmap escapes its proved typed namespace");
        }else if(!wanted.contains(target))throw FormatError("Native PC sky program escapes its complete closure");
    }
    auto staged=library;
    for(const auto& record:additions)staged.at("shader_tags").push_back(record);
    library.swap(staged);return result;
}

std::map<std::string,std::filesystem::path> supplement_mcc_native_pc_world_library(
    Json& library,const std::filesystem::path& source_map,const std::filesystem::path& root,
    std::uint64_t admitted_bytes,std::string_view admitted_sha256,
    const MccNativeLibraryRead& read_library) {
    const auto policy=Json::parse(native_pc_world_closure_policy);
    const auto name=source_map.stem().string();
    const auto map=std::ranges::find_if(policy.at("maps"),[&](const auto& row){return row.at("map")==name;});
    if(map==policy.at("maps").end() || !library.is_object() ||
       admitted_bytes!=map->at("bytes").get<std::uint64_t>() ||
       admitted_sha256!=map->at("sha256").get<std::string>() ||
       digest(library)!=map->at("bootstrap_canonical_sha256").get<std::string>() ||
       std::filesystem::exists(root/name/"library.json"))return {};
    if(!read_library)throw FormatError("Native PC world closure requires admitted source library snapshots");
    const auto directory=root/policy.at("source_map").get<std::string>();
    const auto snapshot=read_library(directory/"library.json");
    if(snapshot.size()>16u*1024u*1024u)throw FormatError("Excessive admitted PC world source library");
    const auto source=Json::parse(snapshot);
    if(digest(source)!=policy.at("source_canonical_sha256").get<std::string>())
        throw FormatError("Native PC world source differs from the complete authenticated TU1.5 library");
    std::set<Key> existing,wanted;
    for(const auto& record:library.at("shader_tags"))
        if(!existing.insert(key(record)).second)throw FormatError("Ambiguous authenticated PC bootstrap identity");
    for(const auto& expected:policy.at("records")) {
        const auto identity=key(expected);
        if((identity.first!="stem"&&identity.first!="spas"&&identity.first!="vrtx") ||
           !expected.at("strings").empty() || existing.contains(identity) || !wanted.insert(identity).second)
            throw FormatError("Native PC world closure would replace or escape an authenticated program identity");
    }
    Json additions=Json::array();std::map<std::string,std::filesystem::path> result;
    for(const auto& expected:policy.at("records")) {
        const Json* found=nullptr;
        for(const auto& record:source.at("shader_tags"))if(key(record)==key(expected)) {
            if(found || record!=expected)throw FormatError("Native PC world record differs from its complete original owner");
            found=&record;
        }
        if(!found)throw FormatError("Native PC world original owner is absent");
        for(const auto& reference:found->at("references")) {
            const Key target{reference.at(1).at(0).get<std::string>(),reference.at(1).at(1).get<std::string>()};
            if(!wanted.contains(target))throw FormatError("Native PC world program escapes its complete finite closure");
        }
        const auto filename=found->at("payload").get<std::string>();
        if(filename.size()!=68 || filename.substr(64)!=".bin" ||
           filename.find_first_not_of("0123456789abcdef")<64)
            throw FormatError("Invalid PC world original payload identity");
        const auto path=directory/"payloads"/filename;
        if(digest(std::span<const std::byte>(read_payload(path,found->at("bytes").get<std::size_t>())))!=filename.substr(0,64))
            throw FormatError("Native PC world complete original payload changed");
        additions.push_back(*found);result.emplace(filename,path);
    }
    auto staged=library;
    for(const auto& record:additions)staged.at("shader_tags").push_back(record);
    library.swap(staged);return result;
}

std::map<std::string,std::filesystem::path> supplement_mcc_native_pc_world_programs(
    Json& library,const std::filesystem::path& source_map,const std::filesystem::path& root,
    std::uint64_t admitted_bytes,std::string_view admitted_sha256,
    const MccNativeLibraryRead& read_library) {
    const auto policy=Json::parse(native_pc_world_programs_policy);
    const auto name=source_map.stem().string();
    const auto map=std::ranges::find_if(policy.at("maps"),[&](const auto& row){return row.at("map")==name;});
    if(map==policy.at("maps").end() || !library.is_object() ||
       admitted_bytes!=map->at("bytes").get<std::uint64_t>() ||
       admitted_sha256!=map->at("sha256").get<std::string>() ||
       digest(library)!=map->at("bootstrap_canonical_sha256").get<std::string>() ||
       std::filesystem::exists(root/name/"library.json"))return {};
    if(!read_library)throw FormatError("Additional PC world programs require admitted source library snapshots");
    std::map<std::string,Json> sources;
    for(const auto& [source,expected]:map->at("sources").items()) {
        const auto snapshot=read_library(root/source/"library.json");
        if(snapshot.size()>16u*1024u*1024u)
            throw FormatError("Excessive admitted additional PC world source library");
        auto parsed=Json::parse(snapshot);
        if(digest(parsed)!=expected.get<std::string>())
            throw FormatError("Additional PC world source differs from its complete original TU1.5 library");
        sources.emplace(source,std::move(parsed));
    }
    const auto record_in=[&](const std::string& source,const Key& identity)->const Json& {
        const auto owner=sources.find(source);
        if(owner==sources.end())throw FormatError("Additional PC world record escaped its admitted source snapshots");
        const Json* found=nullptr;
        for(const auto& record:owner->second.at("shader_tags"))if(key(record)==identity) {
            if(found)throw FormatError("Ambiguous additional PC world original program identity");
            found=&record;
        }
        if(!found)throw FormatError("Additional PC world original program owner is absent");
        return *found;
    };
    const auto program=[&](const Json& record) {
        const auto identity=key(record);
        if((identity.first!="stem"&&identity.first!="spas"&&identity.first!="vrtx") ||
           !record.at("strings").empty())
            throw FormatError("Additional PC world programs escaped the finite typed program namespace");
    };
    const auto payload=[&](const std::string& source,const Json& record) {
        program(record);
        const auto filename=record.at("payload").get<std::string>();
        if(filename.size()!=68 || filename.substr(64)!=".bin" ||
           filename.find_first_not_of("0123456789abcdef")<64)
            throw FormatError("Invalid additional PC world original payload identity");
        const auto size=record.at("bytes").get<std::size_t>();
        if(size>16u*1024u*1024u)throw FormatError("Excessive additional PC world original payload");
        const auto path=root/source/"payloads"/filename;
        if(digest(std::span<const std::byte>(read_payload(path,size)))!=filename.substr(0,64))
            throw FormatError("Additional PC world complete original program payload changed");
        return std::pair(filename,path);
    };
    std::set<Key> existing,wanted;
    for(const auto& record:library.at("shader_tags"))
        if(!existing.insert(key(record)).second)
            throw FormatError("Ambiguous preceding PC HUD/sky/world program identity");
    for(const auto& item:map->at("records")) {
        const auto& expected=item.at("record");program(expected);
        if(existing.contains(key(expected)) || !wanted.insert(key(expected)).second)
            throw FormatError("Additional PC world programs would replace an existing typed identity");
    }
    Json additions=Json::array();std::map<std::string,std::filesystem::path> result;
    for(const auto& item:map->at("records")) {
        const auto source=item.at("source_map").get<std::string>();
        const auto& expected=item.at("record");const auto& record=record_in(source,key(expected));
        if(record!=expected)throw FormatError("Additional PC world program differs from its complete original record");
        for(const auto& reference:record.at("references")) {
            const Key target{reference.at(1).at(0).get<std::string>(),reference.at(1).at(1).get<std::string>()};
            if((target.first!="stem"&&target.first!="spas"&&target.first!="vrtx") ||
               (!existing.contains(target)&&!wanted.contains(target)))
                throw FormatError("Additional PC world program has an unresolved or non-program dependency");
        }
        const auto [filename,path]=payload(source,record);
        result.emplace(filename,path);additions.push_back(record);
    }
    // The generator proved complete typed graph equivalence, including every
    // vertex class and opaque program/declaration byte. Source allocation and
    // padding receipts retain both complete payload hashes independently.
    for(const auto& proof:map->at("equivalent_existing_identities")) {
        const auto source=proof.at("source_map").get<std::string>();
        const Key identity{proof.at("group").get<std::string>(),proof.at("name").get<std::string>()};
        if(!existing.contains(identity)&&!wanted.contains(identity))
            throw FormatError("Additional PC world overlap lacks its complete retained identity");
        const Json* retained=nullptr;
        for(const auto* records:{&library.at("shader_tags"),&additions})
            for(const auto& value:*records)if(key(value)==identity) {
                if(retained)throw FormatError("Ambiguous retained additional PC world overlap");
                retained=&value;
            }
        if(!retained || retained->at("payload")!=proof.at("retained_payload"))
            throw FormatError("Additional PC world overlap differs from its complete retained source owner");
        const auto& record=record_in(source,identity);
        if(record.at("payload")!=proof.at("candidate_payload"))
            throw FormatError("Additional PC world overlap differs from its proved original owner");
        (void)payload(source,record);
    }
    for(const auto& root_record:map->at("roots")) {
        const Key identity{root_record.at("group").get<std::string>(),root_record.at("name").get<std::string>()};
        if(!existing.contains(identity)&&!wanted.contains(identity))
            throw FormatError("Additional PC world root lacks its complete native closure");
    }
    auto staged=library;
    for(const auto& record:additions)staged.at("shader_tags").push_back(record);
    library.swap(staged);return result;
}

MccNativeVehicleLibrary supplement_mcc_native_vehicle_library(
    Json& library,const std::filesystem::path& source_map,
    std::span<const Tag> directory,const MccShaderRead& read_source,
    const MccShaderBitmapFrame& owns_frame,unsigned string_count,
    const std::function<std::string(std::uint32_t)>& string_id,
    const std::filesystem::path& root,std::uint64_t admitted_bytes,
    std::string_view admitted_sha256,const MccNativeLibraryRead& read_library) {
    static const auto policy=Json::parse(native_vehicle_restoration_policy);
    MccNativeVehicleLibrary result;
    const auto name=source_map.stem().string();const Json* map=nullptr;
    for(const auto& candidate:policy.at("maps"))
        if(candidate.at("map")==name && candidate.at("bytes")==admitted_bytes &&
           candidate.at("sha256")==admitted_sha256) {
            if(map)throw FormatError("Ambiguous finite vehicle source input");
            map=&candidate;
        }
    // No other map, basename alias, existing library, or custom bootstrap
    // receives any material/program/semantic identity from this policy.
    if(!map || !library.is_object() || std::filesystem::exists(root/name/"library.json"))return result;
    if(!read_source || !owns_frame || !string_id || !read_library ||
       digest(library)!=map->at("preceding_canonical_sha256").get<std::string>())
        throw FormatError("Vehicle restoration lacks its exact authenticated preceding source bank");
    const auto require=[](bool condition,const char* message){if(!condition)throw FormatError(message);};
    const auto same_tag=[&](const Json& expected)->const Tag& {
        const auto datum=expected.at("datum").get<std::uint32_t>();const auto index=datum&0xffffu;
        require(datum!=0xffffffffu && index<directory.size(),"Vehicle source tag lacks its full salted directory owner");
        const auto& actual=directory[index];
        require(actual.datum==datum && actual.group==expected.at("group").get<std::string>() &&
            actual.name==expected.at("name").get<std::string>() &&
            actual.address==expected.at("address").get<std::uint32_t>() &&
            actual.size==expected.at("size").get<std::uint32_t>() && actual.address && actual.size,
            "Vehicle source tag differs from its complete typed MCC allocation");
        return actual;
    };
    for(const auto& expected:map->at("source_tags")) {
        const auto& tag=same_tag(expected);
        const auto bytes=read_source(tag.address,tag.size);
        require(bytes.size()==tag.size && digest(bytes)==expected.at("sha256").get<std::string>(),
            "Vehicle complete MCC source allocation changed");
    }
    for(const auto& expected:map->at("source_regions")) {
        const auto address=expected.at("address").get<std::uint32_t>();
        const auto size=expected.at("bytes").get<std::size_t>();
        require(size && size<=16u*1024u*1024u && std::uint64_t(address)+size<=0x100000000ull,
            "Vehicle source region escapes its finite allocation bound");
        const auto bytes=read_source(address,size);
        require(bytes.size()==size && digest(bytes)==expected.at("sha256").get<std::string>(),
            "Vehicle complete MCC source program/parameter region changed");
    }
    const auto bitmap=[&](const Key& identity)->const Tag& {
        const Tag* found=nullptr;
        for(const auto& tag:directory)if(tag.group==identity.first && tag.name==identity.second) {
            require(!found,"Vehicle bitmap identity is ambiguous");found=&tag;
        }
        require(found && found->group=="bitm" && found->datum!=0xffffffffu && found->address && found->size,
            "Vehicle original bitmap has no actual resident typed MCC owner");
        return *found;
    };
    for(const auto& role:map->at("bitmap_roles")) {
        const auto& actual=bitmap({role.at("group"),role.at("name")});
        require(actual.datum==role.at("datum").get<std::uint32_t>() &&
            actual.address==role.at("address").get<std::uint32_t>() && actual.size==role.at("size").get<std::uint32_t>() &&
            owns_frame(actual.datum,role.at("frame").get<std::uint32_t>()),
            "Vehicle original bitmap/frame escaped its actual source namespace");
    }
    require(string_count==map->at("source_string_count").get<unsigned>() && string_count<=65536,
        "Vehicle source StringID bank changed extent");
    std::map<std::string,unsigned> named_counts;
    for(const auto& [text,unused]:map->at("source_string_ids").items()) {
        (void)unused;named_counts.emplace(text,0);
    }
    for(unsigned index=0;index<string_count;++index) {
        std::string text;
        try{text=string_id(index);}catch(const FormatError&){continue;}
        if(auto it=named_counts.find(text);it!=named_counts.end())++it->second;
    }
    for(const auto& [text,id]:map->at("source_string_ids").items()) {
        if(id.is_null())require(named_counts.at(text)==0,"Vehicle absent source StringID became ambiguous");
        else require(named_counts.at(text)==1 && string_id(id.get<std::uint32_t>())==text,
            "Vehicle source StringID lacks unique logical resolution");
    }
    std::map<std::string,Json> sources;
    for(const auto& [source,expected]:map->at("sources").items()) {
        const auto snapshot=read_library(root/source/"library.json");
        require(snapshot.size()<=16u*1024u*1024u,"Excessive original vehicle source library");
        auto parsed=Json::parse(snapshot);
        require(digest(parsed)==expected.get<std::string>(),"Vehicle source library differs from its complete original TU authority");
        sources.emplace(source,std::move(parsed));
    }
    std::map<std::pair<std::string,Key>,Key> names;
    std::set<Key> new_programs;
    for(const auto& row:map->at("declared_program_namespace")) {
        const auto source=row.at("source_map").get<std::string>();
        const Key original{row.at("group"),row.at("original_name")},published{original.first,row.at("published_name")};
        require(sources.contains(source) && (original.first=="stem" || original.first=="spas" || original.first=="vrtx"),
            "Vehicle program namespace escaped its finite original donors");
        const auto split=original.second.find_last_of('\\');
        require(split!=std::string::npos && published.second==original.second.substr(0,split)+
                "\\__tu15_vehicle_"+source+"\\"+original.second.substr(split+1) &&
            names.emplace(std::pair{source,original},published).second && new_programs.insert(published).second,
            "Vehicle program namespace is not its exact declared parent/leaf-preserving identity");
    }
    std::set<std::string> material_names;
    for(const auto& row:map->at("material_source_profiles"))
        require(material_names.insert(row.at("name").get<std::string>()).second,
            "Duplicate finite vehicle material source profile");
    require(material_names.size()==(name=="needle"?24u:20u),"Vehicle source material cohort changed");
    const auto mapped=[&](const std::string& source,const Key& original)->Key {
        if(original.first=="shad") {
            require(material_names.contains(original.second),"Vehicle material escaped its exact source-owner set");
            return original;
        }
        const auto found=names.find({source,original});
        require(found!=names.end(),"Vehicle original program reference has no declared unique storage identity");
        return found->second;
    };
    std::set<Key> existing,wanted;
    for(const auto& record:library.at("shader_tags"))
        require(existing.insert(key(record)).second,"Ambiguous preceding vehicle program namespace");
    Json additions=Json::array();std::set<Key> material_records;
    for(const auto& item:map->at("records")) {
        const auto source=item.at("source_map").get<std::string>();
        const auto owner=sources.find(source);
        require(owner!=sources.end(),"Vehicle record escaped its original library owner");
        const auto& original=item.at("original_record");const auto identity=key(original);
        const Json* found=nullptr;
        for(const auto& record:owner->second.at("shader_tags"))if(key(record)==identity) {
            require(!found,"Vehicle original source record is ambiguous");found=&record;
        }
        require(found && *found==original,"Vehicle record differs from its complete original TU descriptor");
        auto published=original;const auto destination=mapped(source,identity);
        published["name"]=destination.second;
        for(auto& reference:published.at("references")) {
            const Key target{reference.at(1).at(0),reference.at(1).at(1)};
            if(target.first=="bitm") {(void)bitmap(target);continue;}
            const auto resolved=mapped(source,target);reference[1]=Json::array({resolved.first,resolved.second});
        }
        require(published==item.at("record") && item.at("semantic_name")==identity.second &&
            !existing.contains(destination) && wanted.insert(destination).second,
            "Vehicle publication changed more than its exact declared storage names/references");
        const auto filename=original.at("payload").get<std::string>();
        require(filename.size()==68 && filename.substr(64)==".bin" &&
            filename.find_first_not_of("0123456789abcdef")>=64,
            "Invalid complete original vehicle payload identity");
        const auto size=original.at("bytes").get<std::size_t>();
        require(size && size<=16u*1024u*1024u,"Excessive complete original vehicle payload");
        const auto path=root/source/"payloads"/filename;
        const auto snapshot=read_library(path);
        require(snapshot.size()==size && digest(std::as_bytes(std::span(snapshot.data(),snapshot.size())))==filename.substr(0,64),
            "Complete original vehicle payload changed");
        result.payloads.emplace(filename,path);
        if(identity.first=="shad")material_records.insert(destination);
        else require(result.semantic_names.emplace(destination,identity.second).second,
            "Ambiguous trusted vehicle semantic identity");
        additions.push_back(std::move(published));
    }
    require(material_records.size()==material_names.size() && result.semantic_names.size()==new_programs.size(),
        "Vehicle namespace publication is not a complete bidirectional source closure");
    for(const auto& record:additions)for(const auto& reference:record.at("references")) {
        const Key target{reference.at(1).at(0),reference.at(1).at(1)};
        require(target.first=="bitm" || wanted.contains(target),"Vehicle namespace has a dangling program reference");
    }
    auto completed=library;
    for(const auto& record:additions)completed.at("shader_tags").push_back(record);
    library.swap(completed);return result;
}

}
