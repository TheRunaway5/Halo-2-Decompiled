#include "halo2/cache.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
std::string hex(std::uint32_t value,unsigned width){std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(width)<<value;return out.str();}
struct Reader {
    const Cache &cache;
    std::function<std::uint32_t(std::uint32_t)> word;
    std::function<void(const Tag&)> validate_ref;
    std::string symbol(std::uint32_t id)const{return id?cache.string_id(id):"";}
    Json ref(std::uint32_t value)const {
        if(value==0xffffffff)return nullptr;
        const auto tag=cache.find_tag(TagHandle{value});
        if(!tag)throw FormatError("Model contract references an unavailable full tag handle");
        if(validate_ref)validate_ref(*tag);
        return Json::array({tag->group,tag->name});
    }
    std::vector<std::uint32_t> block(std::uint32_t descriptor,unsigned stride)const {
        const auto count=word(descriptor),address=word(descriptor+4);std::vector<std::uint32_t> result;
        if(count>65536 || (count && (!address || std::uint64_t(address)+std::uint64_t(count)*stride>0x100000000ull)))
            throw FormatError("Model contract block escapes its owned extent");
        for(unsigned n=0;n<count;++n)result.push_back(address+n*stride);return result;
    }
    Json words(std::uint32_t at,std::initializer_list<unsigned> offsets)const {
        Json out=Json::object();for(const auto offset:offsets)out[hex(offset,2)]=hex(word(at+offset),8);return out;
    }
    Json model(std::uint32_t at,const std::string &name)const {
        Json result={{"model",name},{"collision",ref(word(at+0xc))},{"physics",ref(word(at+0x24))},
            {"damage",Json::array()},{"materials",Json::array()}};
        for(const auto material:block(at+0x58,0x14))result["materials"].push_back({
            {"name",symbol(word(material))},{"global_material",symbol(word(material+12))},{"words",words(material,{4,8,16})}});
        for(const auto info:block(at+0x60,0xf8)) {
            Json value={{"words",words(info,{0,8,0x28,0x2c,0x30,0x34,0x38,0x8c,0x94,0x98,0x9c,0xa0,0xcc,0xd0,0xd4})},
                {"indirect_material",symbol(word(info+4))},{"shield_material",symbol(word(info+0x90))},
                {"effects_and_shaders",Json::object()},{"nodes",Json::array()},{"seats",Json::array()},
                {"sections",Json::array()},{"constraints",Json::array()}};
            for(const auto offset:{0x80,0x88,0xa8,0xb0,0xb8,0xec,0xf4})value["effects_and_shaders"][hex(offset,2)]=ref(word(info+offset));
            for(const auto node:block(info+0xc4,0x10))value["nodes"].push_back(words(node,{0,4,8,12}));
            for(const auto seat:block(info+0xd8,0x14))value["seats"].push_back({{"name",symbol(word(seat))},{"words",words(seat,{4,8,12,16})}});
            for(const auto section:block(info+0xbc,0x38)) {
                Json row={{"name",symbol(word(section))},{"words",words(section,{4,8,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x34})},
                    {"restored_region",symbol(word(section+0x30))},{"responses",Json::array()}};
                for(const auto response:block(section+12,0x50))row["responses"].push_back({
                    {"words",words(response,{0,4,8,0x20,0x2c,0x30,0x44,0x4c})},
                    {"region",symbol(word(response+0x1c))},{"delay_marker",symbol(word(response+0x38))},
                    {"constraint_group",symbol(word(response+0x3c))},{"ejecting_seat",symbol(word(response+0x40))},
                    {"destroyed_child_marker",symbol(word(response+0x48))},{"effect",ref(word(response+0x10))},
                    {"damage",ref(word(response+0x18))},{"delay_effect",ref(word(response+0x34))},
                    {"effect_marker",symbol(word(response+0x24))},{"damage_marker",symbol(word(response+0x28))}});
                value["sections"].push_back(std::move(row));
            }
            for(const auto constraint:block(info+0xe0,0x14))value["constraints"].push_back({
                {"name",symbol(word(constraint))},{"damage_constraint",symbol(word(constraint+4))},
                {"group",symbol(word(constraint+8))},{"words",words(constraint,{12,16})}});
            result["damage"].push_back(std::move(value));
        }
        return result;
    }
};
}
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply original contract expectations and small report path");
    std::ifstream input(argv[1]);const auto expected=Json::parse(input);
    if(expected.at("schema")!="H2_DESTRUCTION_NATIVE_MODEL_EXPECTATIONS_V1")throw FormatError("Wrong original model expectations");
    Json report={{"passed",false},{"scope","All model fields recorded by contracts-v5, in actual Cache and delivered tag/model/reference lookup chains; no damage, entire object-tag or full-definition acceptance"},
        {"maps",Json::array()},{"differences",Json::array()},{"full_goal_complete",false}};
    unsigned cache_passed=0,image_passed=0,required=0,failures=0;std::set<std::string> contracts,definitions;
    const auto issue=[&](const std::string &map,const std::string &name,const std::string &stage,const std::string &why){
        ++failures;if(report["differences"].size()<128)report["differences"].push_back({{"map",map},{"definition",name},{"stage",stage},{"reason",why.substr(0,1200)}});
    };
    const auto save=[&]{report["required_contexts"]=required;report["cache_contracts_passed"]=cache_passed;report["delivered_contracts_passed"]=image_passed;
        report["failures"]=failures;report["unique_original_contracts"]=contracts.size();report["unique_definitions"]=definitions.size();
        std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';if(!out)throw FormatError("Cannot retain native model audit report");};
    for(const auto &row:expected.at("maps")) {
        const auto map=row.at("map").get<std::string>();Json result={{"map",map},{"required",row.at("entries").size()},{"cache_passed",0},{"image_passed",0}};
        const auto before_cache=cache_passed,before_image=image_passed;
        try {
            const auto path=row.at("mcc_path").get<std::string>();const Cache cache(path,std::filesystem::path(path).parent_path());
            if(!cache.is_mcc())throw FormatError("Native model audit requires MCC");
            const auto word=[&](std::uint32_t address){return Bytes(cache.read_tag_bytes(address,4)).u32(0);};
            Reader reader{cache,word,{}};
            std::optional<MccRuntimeCacheDelivery> delivery;std::optional<SourceRuntimeCacheTagImage> image;
            try {
                delivery.emplace(MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u));
                if(!delivery->issues().empty())throw FormatError("Native metadata plan has unresolved fields: "+delivery->issues().front().reason);
                image.emplace(delivery->tag_image());
            }catch(const std::exception &e){result["delivery_error"]=e.what();}
            const auto native_word=[&](std::uint32_t address){
                if(image)for(const auto &region:image->regions)if(address>=region.address && std::uint64_t(address)-region.address+4<=region.bytes.size())
                    return Bytes(region.bytes).u32(address-region.address);
                throw FormatError("Native model field escaped its delivered owner");
            };
            const auto native_tag=[&](const Tag &tag){
                if(!image || (tag.datum&65535u)>=image->count)throw FormatError("Native tag handle escapes the delivered table");
                const auto slot=image->table+(tag.datum&65535u)*16u;
                const auto address=tag.address?delivery->runtime_address(tag.address,tag.size):0u;
                if(native_word(slot)!=tag.type().value || native_word(slot+4)!=tag.datum ||
                    native_word(slot+8)!=address || native_word(slot+12)!=tag.size)
                    throw FormatError("Native tag lookup salt, group, pointer or extent differs from its owner");
            };
            Reader native{cache,native_word,native_tag};
            std::map<std::uint32_t,Json> cache_models,native_models;
            for(const auto &entry:row.at("entries")) {
                ++required;const auto name=entry.at("definition").get<std::string>(),hash=entry.at("original_contract").get<std::string>();
                definitions.insert(name);contracts.insert(hash);const auto &contract=expected.at("contracts").at(hash);
                const auto verify=[&](const std::string &stage,const Json &actual){
                    if(actual==contract)return true;issue(map,name,stage,Json::diff(contract,actual).dump());return false;
                };
                const Tag *object=nullptr,*model=nullptr;
                try {
                    for(const auto &tag:cache.tags())if(tag.group==name.substr(0,4) && tag.name==name.substr(5)) {
                        if(object)throw FormatError("Native object identity is ambiguous");object=&tag;
                    }
                    if(!object || !object->address)throw FormatError("Native object definition is absent");
                    model=cache.find_tag(TagHandle{word(object->address+0x38)});
                    if(!model || model->group!="hlmt" || !model->address)throw FormatError("Native object has no owned model");
                    auto [at,inserted]=cache_models.try_emplace(model->datum);
                    if(inserted)at->second=reader.model(model->address,model->name);
                    if(verify("Cache",at->second))++cache_passed;
                }catch(const std::exception &e){issue(map,name,"Cache",e.what());}
                try {
                    if(!model || !image)throw FormatError("No complete native model/tag-image owner");
                    native_tag(*object);native_tag(*model);
                    const auto object_slot=image->table+(object->datum&65535u)*16u;
                    if(native_word(native_word(object_slot+8)+0x38)!=model->datum)
                        throw FormatError("Delivered object selects a different model handle");
                    const auto model_slot=image->table+(model->datum&65535u)*16u;
                    auto [at,inserted]=native_models.try_emplace(model->datum);
                    if(inserted)at->second=native.model(native_word(model_slot+8),model->name);
                    if(verify("delivered",at->second))++image_passed;
                }catch(const std::exception &e){issue(map,name,"delivered",e.what());}
            }
        }catch(const std::exception &e){
            required+=row.at("entries").size();for(const auto &entry:row.at("entries")){definitions.insert(entry.at("definition"));contracts.insert(entry.at("original_contract"));}
            issue(map,"*","Cache",e.what());
        }
        result["cache_passed"]=cache_passed-before_cache;result["image_passed"]=image_passed-before_image;
        report["maps"].push_back(std::move(result));save();
    }
    report["passed"]=failures==0 && required>0 && cache_passed==required && image_passed==required;save();
    std::cout<<"Native model contracts: "<<cache_passed<<"/"<<required<<" Cache, "<<image_passed<<" delivered, "<<failures<<" failures\n";
    return report["passed"].get<bool>()?0:1;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
