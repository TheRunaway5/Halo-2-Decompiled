#include "halo2/mcc_native_meter.hpp"
#include "halo2/mcc_native_character_shared.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace halo2 {
namespace {
using Json=nlohmann::json;
#include "rendering/mcc_native_meter_profiles.inc"
const Json& registry(){static const auto root=Json::parse(resident_meter_metadata);return root;}
const Json* setup_for(const Tag& stem){
    const auto matches=[&](const Json& setup){const auto& owner=setup.at("source_stem");
        return stem.group==owner.at("group").get<std::string>()&&stem.name==owner.at("name").get<std::string>()&&
            stem.datum==owner.at("datum").get<std::uint32_t>()&&stem.address==owner.at("address").get<std::uint32_t>()&&
            stem.size==owner.at("size").get<unsigned>();};
    if(matches(registry().at("setup")))return &registry().at("setup");
    for(const auto& setup:registry().at("additional_setups"))if(matches(setup))return &setup;
    return nullptr;
}
void require(bool value,const char* message){if(!value)throw FormatError(message);}
void word(std::vector<std::byte>& raw,unsigned at,std::uint32_t value){
    Bytes(raw).require(at,4);for(unsigned i=0;i<4;++i)raw[at+i]=std::byte(value>>(8*i));
}
std::vector<std::byte> unhex(const Json& value){
    const auto text=value.get<std::string>();require(text.size()%2==0,"Invalid meter metadata bytes");
    std::vector<std::byte> result;result.reserve(text.size()/2);
    for(unsigned i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string digest(std::span<const std::byte> raw){
    std::array<unsigned char,32> bytes{};unsigned size{};
    require(EVP_Digest(raw.data(),raw.size(),bytes.data(),&size,EVP_sha256(),nullptr)==1&&size==bytes.size(),
        "Cannot authenticate meter metadata owner");
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(const auto value:bytes){result+=digits[value>>4];result+=digits[value&15];}return result;
}
std::vector<std::byte> source(const MccShaderRead& read,std::uint32_t address,std::size_t size){
    require(size&&size<=16u*1024u*1024u&&address&&std::uint64_t(address)+size<=0x100000000ull,
        "Meter source allocation escaped its bound");
    const auto raw=read(address,size);require(raw.size()==size,"Incomplete meter source allocation");
    return {raw.begin(),raw.end()};
}
bool same(const Tag& tag,const MccShaderFind& find){
    const auto* actual=find(tag.datum);return actual&&tag.datum!=0xffffffffu&&actual->datum==tag.datum&&
        actual->group==tag.group&&actual->name==tag.name&&actual->address==tag.address&&actual->size==tag.size;
}
void source_owner(const Tag& tag,const Json& expected,const MccShaderRead& read,const MccShaderFind& find){
    require(same(tag,find)&&tag.datum==expected.at("datum").get<std::uint32_t>()&&
        tag.group==expected.at("group").get<std::string>()&&tag.name==expected.at("name").get<std::string>()&&
        tag.address==expected.at("address").get<std::uint32_t>()&&tag.size==expected.at("size").get<unsigned>(),
        "Meter lost its exact full-salt MCC owner");
    require(digest(source(read,tag.address,tag.size))==expected.at("sha256").get<std::string>(),
        "Complete MCC meter source changed");
}
void resident_vertex(const Tag& vertex,const MccShaderRead& read,const MccShaderFind& find,const Json& setup){
    require(same(vertex,find)&&vertex.group=="vrtx"&&vertex.address,
        "Meter requires its actual resident vertex owner");
    for(const auto& record:setup.at("resident_vertex_records")) {
        if(vertex.name!=record.at("name").get<std::string>()||vertex.size!=record.at("bytes").get<unsigned>())continue;
        auto normalized=source(read,vertex.address,vertex.size);bool matches=true;
        require(record.at("references").empty()&&record.at("strings").empty(),"Unreviewed meter vertex fixup");
        for(const auto& pointer:record.at("pointers")) {
            const auto field=pointer.at(0).get<unsigned>(),target=pointer.at(1).get<unsigned>();
            require(target<normalized.size(),"Meter vertex pointer escapes its complete owner");
            if(std::uint64_t(vertex.address)+target>0xffffffffu||Bytes(normalized).u32(field)!=vertex.address+target){matches=false;break;}
            word(normalized,field,target);
        }
        if(matches&&digest(normalized)+".bin"==record.at("payload").get<std::string>())return;
    }
    throw FormatError("Meter resident vertex differs from every authenticated existing complete owner");
}
MccOwnedShaderProjection metadata(const Json& item,const Tag& target){
    const auto& record=item.at("record");MccOwnedShaderProjection result{unhex(item.at("metadata_hex")),{}};
    require(result.bytes.size()==record.at("bytes").get<unsigned>()&&
        digest(result.bytes)+".bin"==record.at("payload").get<std::string>()&&record.at("strings").empty(),
        "Meter declarative metadata identity changed");
    for(const auto& pointer:record.at("pointers")) {
        const auto field=pointer.at(0).get<unsigned>(),offset=pointer.at(1).get<unsigned>();
        require(offset<result.bytes.size()&&Bytes(result.bytes).u32(field)==offset,
            "Meter declarative pointer provenance changed");
        result.pointers.emplace_back(field,offset);
    }
    require(record.at("references").size()==1,"Meter declarative dependency census changed");
    const auto& reference=record.at("references").front();
    require(target.datum!=0xffffffffu&&target.group==reference.at(1).at(0).get<std::string>()&&
        target.name==reference.at(1).at(1).get<std::string>(),"Meter declarative typed dependency changed");
    word(result.bytes,reference.at(0).get<unsigned>(),target.datum);
    return result;
}
}

std::span<const MccResidentWeaponPassContract> mcc_resident_weapon_pass_contracts(){
    static const auto contracts=[] {
        std::vector<MccResidentWeaponPassContract> result;
        const auto append=[&](const Json& setup){result.push_back({setup.at("source_stem").at("name").get<std::string>(),
            setup.at("source_pass").at("name").get<std::string>(),
            setup.at("resident_vertex_records").front().at("name").get<std::string>()});};
        append(registry().at("setup"));for(const auto& setup:registry().at("additional_setups"))append(setup);
        return result;
    }();return contracts;
}

std::optional<MccResidentMeterOwners> project_mcc_resident_meter_owners(
    const Tag& stem,const Tag& pass,const Tag& vertex,const MccShaderRead& read,const MccShaderFind& find){
    try {
        if(!read||!find)return {};
        const auto* contract=setup_for(stem);if(!contract)return {};
        const auto& setup=*contract;
        source_owner(stem,setup.at("source_stem"),read,find);
        source_owner(pass,setup.at("source_pass"),read,find);
        for(const auto& expected:setup.at("pc_vertices")) {
            const auto* actual=find(expected.at("datum").get<std::uint32_t>());
            require(actual!=nullptr,"Meter source PC vertex lost its full owner");
            source_owner(*actual,expected,read,find);
        }
        for(const auto& expected:setup.at("external_regions"))
            require(digest(source(read,expected.at("address"),expected.at("bytes")))==expected.at("sha256").get<std::string>(),
                "Meter source compiled parameter allocation changed");
        resident_vertex(vertex,read,find,setup);
        // These are binding/state/remap descriptors and a known compiled
        // kernel identity, not an executable vertex or pixel instruction bank.
        return MccResidentMeterOwners{metadata(setup.at("native_pass"),vertex),metadata(setup.at("native_stem"),pass)};
    }catch(const FormatError&){return {};}
}

std::optional<MccOwnedShaderProjection> project_mcc_resident_meter_material(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,const MccShaderRead& read,
    const MccShaderFind& find,const MccShaderBitmapFrame& frames,
    const std::function<std::string(std::uint32_t)>& strings){
    const auto* setup=setup_for(source_stem);if(!setup)return {};
    const auto& native=setup->at("native_stem");const auto payload=unhex(native.at("metadata_hex"));
    return project_mcc_character_shared(shader,source_stem,native_stem,
        native.at("record").at("payload").get<std::string>(),payload,read,find,frames,strings);
}
}
