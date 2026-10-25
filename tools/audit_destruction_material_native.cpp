#include "halo2/cache.hpp"
#include "halo2/kernels.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
struct Reader {
    const Cache &cache;std::function<std::uint32_t(std::uint32_t)> word;
    std::vector<std::uint32_t> block(std::uint32_t p,unsigned stride)const {
        const auto count=word(p),address=word(p+4);check(count<=65536,"Material block count invalid");
        check(!count || address && std::uint64_t(address)+std::uint64_t(count)*stride<=0x100000000ull,"Material block escapes address space");
        std::vector<std::uint32_t> result;for(unsigned i=0;i<count;++i)result.push_back(address+i*stride);return result;
    }
    std::string symbol(std::uint32_t id)const{return id?cache.string_id(id):"";}
    Json globals(std::uint32_t at,std::size_t &products)const {
        Json result={{"damage_tables",Json::array()},{"materials",Json::array()}};
        for(const auto table:block(at+0xd0,8)) {
            std::vector<DamageGroup> groups;Json rows=Json::array();std::set<std::uint32_t> names;
            for(const auto group:block(table,12)) {
                const auto name=word(group);check(names.insert(name).second,"Authored material group IDs duplicated");
                DamageGroup native{name,{}};Json mods=Json::array();std::set<std::uint32_t> armors;
                for(const auto mod:block(group+4,8)) {
                    const auto armor=word(mod),bits=word(mod+4);check(armors.insert(armor).second,"Authored armor IDs duplicated");
                    native.modifiers.push_back({armor,std::bit_cast<float>(bits)});
                    mods.push_back({{"name",symbol(armor)},{"bits",bits}});
                }
                groups.push_back(std::move(native));rows.push_back({{"name",symbol(name)},{"modifiers",std::move(mods)}});
            }
            // Exercise the production binary searches using every delivered
            // raw group/armor key, preserving their actual authored order.
            check(!names.contains(0xffffffffu),"Missing group sentinel is authored");
            for(const auto &group:groups)for(const auto &modifier:group.modifiers) {
                check(modifier.name!=0xffffffffu,"Missing armor sentinel is authored");
                const auto got=damage_multiplier(groups,group.name,0xffffffffu,modifier.name,0xffffffffu);
                check(std::bit_cast<std::uint32_t>(got)==std::bit_cast<std::uint32_t>(modifier.multiplier),"Native raw StringID binary lookup selects another multiplier");++products;
            }
            result["damage_tables"].push_back(std::move(rows));
        }
        for(const auto material:block(at+0x150,0xb4))result["materials"].push_back({
            {"name",symbol(word(material))},{"parent",std::bit_cast<std::int16_t>(std::uint16_t(word(material+8)))},
            {"armor",Json::array({symbol(word(material+0x10)),symbol(word(material+0x14))})}});
        return result;
    }
};
Json named_tables(Json tables) {
    for(auto &table:tables) {
        std::set<std::string> names;
        for(auto &group:table) {
            check(names.insert(group.at("name").get<std::string>()).second,"Authored group names duplicated");
            std::set<std::string> armors;
            for(auto &modifier:group.at("modifiers")) {
                check(armors.insert(modifier.at("name").get<std::string>()).second,"Authored armor names duplicated");
                if(modifier.at("bits").is_string())modifier["bits"]=std::stoul(modifier.at("bits").get<std::string>(),nullptr,16);
            }
            std::stable_sort(group["modifiers"].begin(),group["modifiers"].end(),[](const auto &a,const auto &b){return a.at("name")<b.at("name");});
        }
        std::stable_sort(table.begin(),table.end(),[](const auto &a,const auto &b){return a.at("name")<b.at("name");});
    }
    return tables;
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply authored material census and report path");std::ifstream input(argv[1]);const auto authored=Json::parse(input);
    check(authored.at("schema")=="H2_DESTRUCTION_MATERIAL_INPUTS_V1","Wrong material authority");
    Json results=Json::array();std::size_t differences{},products{};
    for(const auto &row:authored.at("maps")) {
        Json errors=Json::array();const auto map=row.at("map").get<std::string>();
        try {
            const auto &expected=authored.at("contracts").at(row.at("original").get<std::string>());
            check(expected.size()==1,"Original globals identity ambiguous");
            const auto path=row.at("source").at("mcc_path").get<std::string>();Cache cache(path,std::filesystem::path(path).parent_path());
            const auto *globals=cache.query_tag({.group=TagGroup::from_string("matg"),.require_address=true});
            check(globals && globals->name==expected.at(0).at("tag").get<std::string>(),"Native globals owner differs");
            Reader local{cache,[&](auto p){return Bytes(cache.read_tag_bytes(p,4)).u32(0);}};
            const auto actual=local.globals(globals->address,products);
            check(named_tables(actual.at("damage_tables"))==named_tables(expected.at(0).at("damage_tables")),"Native Cache named multiplier inputs differ from original");
            const auto &materials=expected.at(0).at("materials");check(actual.at("materials").size()>=materials.size(),"Native material rows missing");
            for(std::size_t i=0;i<materials.size();++i)check(actual.at("materials").at(i)==materials.at(i),"Native material parent/armor binding differs");
            const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u);
            check(delivery.issues().empty(),"Native metadata plan has unresolved owner");const auto image=delivery.tag_image();
            Reader native{cache,[&](auto address){
                for(const auto &region:image.regions)if(address>=region.address && std::uint64_t(address)-region.address+4<=region.bytes.size())return Bytes(region.bytes).u32(address-region.address);
                throw FormatError("Delivered material address escaped its owner");
            }};
            const auto address=delivery.runtime_address(globals->address,globals->size);
            check(native.globals(address,products)==actual,"Native delivered material inputs differ from Cache");
        }catch(const std::exception &e){++differences;errors.push_back(e.what());}
        results.push_back({{"map",map},{"passed",errors.empty()},{"errors",errors}});
    }
    Json report{{"passed",differences==0},{"maps",results.size()},{"differences",differences},{"production_scalar_lookups",products},{"results",results},
        {"scope","Original named multiplier bits and material bindings in Cache/delivered metadata, plus every raw-ID single-product lookup; no new gameplay, effects or full-definition acceptance"}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return differences?1:0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
