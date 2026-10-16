#include "halo2/mcc_sound_effect_definitions.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <tuple>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> hash{};unsigned size{};
    check(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&size,EVP_sha256(),nullptr)==1&&size==hash.size(),"Cannot hash retained metadata");
    constexpr char hex[]="0123456789abcdef";std::string out;
    for(auto value:hash){out+=hex[value>>4];out+=hex[value&15];}return out;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto v:bytes){auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;
}
void same_name(const MccSoundEffectDefinitions::Name& value,const Json& expected,const char* id,const char* name) {
    check(value.string_id==expected.at(id),"Effect StringID bits differ");
    if(expected.at(name).is_null())check(!value.text,"Null StringID has invented text");
    else check(value.text&&*value.text==expected.at(name).get<std::string>(),"Effect StringID resolution differs");
}
void same_data(const MccSoundEffectDefinitions::DataReference& value,const Json& expected) {
    check(value.size==expected.at("bytes")&&value.source_address==expected.at("source_pointer"),"Effect data reference differs");
    if(value.size)check(digest(value.payload.bytes)==expected.at("sha256").get<std::string>(),"Effect function bytes differ");
    else check(value.payload.bytes.empty(),"Empty effect reference has invented program bytes");
}
using Definitions=MccSoundEffectDefinitions;
const Definitions::Effect& radio_3d(const Definitions& bank) {
    for(const auto& collection:bank.collections())for(const auto& effect:collection.effects)
        if(effect.name.text&&*effect.name.text=="radio_3d")return effect;
    throw std::runtime_error("Actual MCC radio_3d definition is missing");
}
const Definitions::Override& radio_override(const Definitions& bank) {
    const auto& effect=radio_3d(bank);check(effect.instances.size()==1,"radio_3d instance count differs");
    const auto& instance=effect.instances[0];check(instance.override_records.size()==1,"radio_3d override root count differs");
    return instance.override_records[0];
}
void owned(const Definitions::Extent& raw,const Cache* owner) {
    check(raw.cache_owner.get()==owner&&!raw.bytes.empty(),"Nested extent lost its exact retained resolving cache");
    check(std::ranges::equal(raw.bytes,owner->read_tag_bytes(raw.source_address,raw.bytes.size())),"Nested extent differs from its actual cache backing");
}
void function_fields(const Definitions::Function& f,const Cache* owner) {
    owned(f.raw,owner);const Bytes raw(f.raw.bytes);
    check(std::bit_cast<std::uint16_t>(f.input)==raw.u16(0)&&std::bit_cast<std::uint16_t>(f.range_input)==raw.u16(2)&&
        f.time_period_bits==raw.u32(12)&&f.curve.size==raw.u32(4)&&f.curve.source_address==raw.u32(8),"Nested function fields changed raw bits");
    if(f.curve.size)owned(f.curve.payload,owner);
    else check(f.curve.payload.bytes.empty()&&!f.curve.payload.cache_owner,"Empty function invented an owned payload");
}
Json nested_fields(const Definitions& bank,const Cache* owner,const char* map) {
    std::size_t roots=0,components=0,functions=0,constants=0,sources=0,shared=0;
    for(const auto& collection:bank.collections())for(const auto& effect:collection.effects)for(const auto& instance:effect.instances) {
        check(instance.overrides.size()==instance.override_records.size(),"Typed override roots differ from retained raw roots");
        const auto child=[&](const Definitions::Extent& raw){owned(raw,owner);if(raw.source_address<collection.raw.source_address||
            std::uint64_t(raw.source_address)+raw.bytes.size()>std::uint64_t(collection.raw.source_address)+collection.raw.bytes.size())++shared;};
        for(std::size_t i=0;i<instance.override_records.size();++i) {
            const auto& root=instance.override_records[i];child(root.raw);++roots;
            check(root.raw.source_address==instance.overrides[i].source_address&&std::ranges::equal(root.raw.bytes,instance.overrides[i].bytes)&&
                root.component_mask==Bytes(root.raw.bytes).u32(0x10),"Typed override root bits differ");
            for(const auto& component:root.components) {
                child(component.raw);++components;check(component.parameter_mask==Bytes(component.raw.bytes).u32(0x18),"Nested parameter mask differs");
                for(const auto& f:component.functions){function_fields(f,owner);child(f.raw);if(f.curve.size)child(f.curve.payload);++functions;}
                for(const auto& c:component.constants){child(c.raw);check(c.bits==Bytes(c.raw.bytes).u32(0),"Nested constant bits changed");++constants;}
                for(const auto& s:component.sources){child(s.raw);check(s.bits==std::to_integer<std::uint8_t>(s.raw.bytes[0]),"Nested source-byte bits changed");++sources;}
            }
            for(const auto& f:root.low_frequency_inputs){function_fields(f,owner);child(f.raw);if(f.curve.size)child(f.curve.payload);++functions;}
        }
    }
    if(std::string_view(map)=="mainmenu.map") {
        const auto& root=radio_override(bank);check(root.component_mask==1&&root.components.size()==1,"Actual radio_3d root differs");
        const auto& c=root.components[0];check(c.parameter_mask==0x71&&c.functions.size()==1&&c.constants.size()==3&&c.sources.size()==4,"Actual radio_3d nested counts/mask differ");
        const auto& f=c.functions[0];check(f.input==3&&f.range_input==0&&f.time_period_bits==0x3f800000&&f.curve.size==116&&
            digest(f.curve.payload.bytes)=="39582176c12d34eff3918ac348d12362ea0414ca75dd7d9e76dd1f5608eec7b3","Actual retained MCC radio_3d curve differs from independently recovered bytes");
        constexpr std::array<std::uint32_t,3> expected_constants{0x42a00000,0xc1f00000,0x3f000000};
        constexpr std::array<std::uint8_t,4> expected_sources{1,0,2,4};
        for(std::size_t i=0;i<expected_constants.size();++i)check(c.constants[i].bits==expected_constants[i],"Actual radio_3d constant words differ");
        for(std::size_t i=0;i<expected_sources.size();++i)check(c.sources[i].bits==expected_sources[i],"Actual radio_3d source-byte records differ");
    }
    return {{"map",map},{"roots",roots},{"components",components},{"functions",functions},{"constants",constants},{"source_bytes",sources},{"nonlocal_child_extents",shared}};
}
}
int main(int argc,char** argv)try {
    if(argc!=4)throw std::runtime_error("Expected cache root, independent inventory and output report");
    if(!std::filesystem::is_regular_file(argv[2])||!std::filesystem::is_directory(std::filesystem::path(argv[1])/"halo2/h2_maps_win64_dx11")) {
        std::cerr<<"MCC audio definitions need an imported cache and independent inventory\n";return 77;
    }
    Json inventory;std::ifstream input(argv[2]);input>>inventory;
    const std::filesystem::path maps=std::filesystem::path(argv[1])/"halo2/h2_maps_win64_dx11";
    Json results=Json::array(),nested_results=Json::array();std::size_t total=0,shared_children=0,rejected_metadata=0,nested_rejections=0,raw_bit_controls=0;
    for(const auto* map:{"00a_introduction.map","mainmenu.map","lockout.map"}) {
        auto cache=std::make_shared<Cache>(maps/map,maps);
        std::weak_ptr<const Cache> weak=cache;
        const auto original_tags=cache->tags().size();
        // These edits affect only Cache's private metadata allocation. They
        // never write an imported map. Restore before retaining any views.
        const auto corrupt=[&](std::uint32_t address,std::uint32_t value) {
            auto raw=cache->read_tag_bytes(address,4);std::array<std::byte,4> saved;
            std::ranges::copy(raw,saved.begin());auto* bytes=const_cast<std::byte*>(raw.data());
            for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));
            bool rejected=false;
            try{MccSoundEffectDefinitions invalid(cache);}catch(const FormatError&){rejected=true;}
            catch(...){std::memcpy(bytes,saved.data(),4);throw;}
            std::memcpy(bytes,saved.data(),4);check(rejected,"Invalid MCC effect metadata accepted");++rejected_metadata;
        };
        const auto* global=cache->query_tag({.group=TagGroup{"sfx+"},.name="sound\\global_fx",.require_address=true});
        check(global,"Resolved MCC global effects are unavailable");
        corrupt(global->address,0xffffffffu); // negative tag-block count
        corrupt(global->address+4,0xfffffffcu); // wrapping child extent
        if(std::string_view(map)=="mainmenu.map") {
            std::uint32_t root_address{},component_address{},function_address{},constant_address{},source_address{},truncated{};
            {Definitions inspect(cache);const auto& root=radio_override(inspect);root_address=root.raw.source_address;
                const auto& c=root.components.at(0);component_address=c.raw.source_address;function_address=c.functions.at(0).raw.source_address;
                constant_address=c.constants.at(0).raw.source_address;source_address=c.sources.at(0).raw.source_address;}
            for(const auto& tag:cache->tags()) {
                const auto end=std::uint64_t(tag.address)+tag.size;if(!tag.address||!tag.size||end>0x100000000ull)continue;
                try{(void)cache->read_tag_bytes(std::uint32_t(end-1),1);}catch(const FormatError&){continue;}
                try{(void)cache->read_tag_bytes(std::uint32_t(end-1),116);}catch(const FormatError&){truncated=std::uint32_t(end-1);break;}
            }
            check(truncated,"Actual retained metadata has no testable truncated child boundary");
            for(const auto& [at,value]:std::vector<std::pair<std::uint32_t,std::uint32_t>>{
                {root_address,0xffffffffu},{root_address+4,0xabcdef00u},{root_address+4,0xfffffff0u},{root_address+4,truncated},
                {root_address+8,0xffffffffu},{component_address,0xffffffffu},{component_address,4097u},{component_address+4,0xabcdef00u},
                {component_address+8,0xffffffffu},{component_address+0xc,0xfffffffcu},{component_address+0x10,0xffffffffu},{component_address+0x14,truncated},
                {function_address+4,16u*1024u*1024u+1u},{function_address+8,0},{function_address+8,0xabcdef00u},
                {function_address+8,0xfffffff0u},{function_address+8,truncated}}) {corrupt(at,value);++nested_rejections;}
            const auto mutate=[&](const std::vector<std::tuple<std::uint32_t,unsigned,std::uint32_t>>& changes,const auto& check_bank) {
                std::vector<std::vector<std::byte>> saved;for(const auto& [at,size,value]:changes){auto raw=cache->read_tag_bytes(at,size);saved.emplace_back(raw.begin(),raw.end());auto* bytes=const_cast<std::byte*>(raw.data());for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(i*8));}
                const auto restore=[&]{for(std::size_t i=0;i<changes.size();++i){const auto [at,size,value]=changes[i];auto bytes=cache->read_tag_bytes(at,size);std::memcpy(const_cast<std::byte*>(bytes.data()),saved[i].data(),size);}};
                try{Definitions retained(cache);check_bank(retained);}catch(...){restore();throw;}restore();++raw_bit_controls;
            };
            mutate({{constant_address,4,0x80000000u},{function_address+12,4,0x7fc01234u},{source_address,1,0xff}},[&](const auto& bank){const auto& c=radio_override(bank).components.at(0);check(c.constants[0].bits==0x80000000u&&c.functions[0].time_period_bits==0x7fc01234u&&c.sources[0].bits==0xff,"Raw signed-zero/NaN/source-byte words were evaluated or normalized");});
            mutate({{function_address+4,4,0},{function_address+8,4,0xabcdef00u}},[&](const auto& bank){const auto& curve=radio_override(bank).components[0].functions[0].curve;check(!curve.size&&curve.source_address==0xabcdef00u&&curve.payload.bytes.empty()&&!curve.payload.cache_owner,"Zero-size nested curve dereferenced or normalized its retained pointer");});
            mutate({{root_address+8,4,1},{root_address+12,4,function_address}},[&](const auto& bank){const auto& root=radio_override(bank);check(root.low_frequency_inputs.size()==1&&root.low_frequency_inputs[0].raw.source_address==function_address&&digest(root.low_frequency_inputs[0].curve.payload.bytes)=="39582176c12d34eff3918ac348d12362ea0414ca75dd7d9e76dd1f5608eec7b3","Shared resident low-frequency child lost its typed fields or curve");});
        }
        std::optional<Definitions::Extent> retained_curve;
        {
            MccSoundEffectDefinitions bank(cache);cache.reset();
            check(!weak.expired(),"Effect bank lost its actual cache owner");
            check(weak.lock()->tags().size()==original_tags,"Effect import changed the cache directory");
            nested_results.push_back(nested_fields(bank,weak.lock().get(),map));
            const auto& curve=radio_override(bank).components.at(0).functions.at(0).curve.payload;
            retained_curve=curve;check(retained_curve->cache_owner.get()==weak.lock().get(),"Detached curve lacks its resolving owner identity");
            std::size_t count=0,expected_count=0;
            // Production Cache resolves unloaded directory identities from
            // shared.map. The independent inventory records donor maps
            // separately; retain local payload precedence for the same datum.
            std::map<std::uint32_t,const Json*> selected;
            for(const auto* donor:{"shared.map",map})for(const auto& row:inventory.at("actual_resident_audio_metadata")) {
                if(row.at("map")!=donor)continue;
                const auto handle=TagHandle{row.at("datum").get<std::uint32_t>()};
                if(std::string_view(donor)=="shared.map") {
                    const auto* resolved=weak.lock()->find_tag(handle);
                    if(!resolved||!resolved->address)continue;
                }
                selected[handle.value]=&row;
            }
            for(const auto& [datum,reference]:selected) {
                const auto& expected=*reference;
                const auto group=expected.at("group").get<std::string>();
                if(group!="sfx+"&&group!="<fx>"&&group!="snmx"&&group!="snde")continue;
                ++expected_count;
                const TagHandle handle{expected.at("datum").get<std::uint32_t>()};
                MccSoundEffectDefinitions::Extent root;
                if(group=="sfx+") {
                    const auto& collection=bank.collection(handle);root=collection.raw;
                    check(collection.name==expected.at("name").get<std::string>()&&collection.effects.size()==expected.at("sound_effects").size(),"Effect collection differs");
                    for(std::size_t i=0;i<collection.effects.size();++i) {
                        const auto& effect=collection.effects[i];const auto& source=expected.at("sound_effects")[i];
                        same_name(effect.name,source,"name_string_id","name");check(effect.flags==source.at("flags"),"Effect flags changed");
                        check(effect.mix.size()==source.at("mix").size()&&effect.instances.size()==source.at("sound_effect_rows").size(),"Effect child count differs");
                        for(std::size_t j=0;j<effect.mix.size();++j)check(effect.mix[j].mixbin==source.at("mix")[j].at("mixbin")&&effect.mix[j].gain_bits==source.at("mix")[j].at("gain_bits"),"Effect mix words differ");
                        const auto compare_rows=[&](const auto& rows,const char* key){check(rows.size()==source.at(key).size(),"Effect parameter count differs");for(std::size_t j=0;j<rows.size();++j)check(hex(rows[j].bytes)==source.at(key)[j].get<std::string>(),"Effect parameter bits differ");};
                        compare_rows(effect.filters,"filter_rows");compare_rows(effect.pitch_lfos,"pitch_lfo_rows");compare_rows(effect.filter_lfos,"filter_lfo_rows");
                        for(std::size_t j=0;j<effect.instances.size();++j) {
                            const auto& instance=effect.instances[j];const auto& from=source.at("sound_effect_rows")[j];
                            check(instance.effect_template.group.value==from.at("template_group_word")&&instance.effect_template.handle.value==from.at("template_datum"),"Effect salted template reference differs");
                            same_data(instance.data,from.at("data_ref"));
                            check(instance.components.size()==from.at("components").size()&&instance.overrides.size()==from.at("override_rows").size(),"Effect instance child count differs");
                            for(std::size_t k=0;k<instance.components.size();++k)check(hex(instance.components[k].bytes)==from.at("components")[k].get<std::string>(),"Effect instance component differs");
                            for(std::size_t k=0;k<instance.overrides.size();++k)check(hex(instance.overrides[k].bytes)==from.at("override_rows")[k].get<std::string>(),"Effect override differs");
                        }
                    }
                }else if(group=="<fx>") {
                    const auto& effect=bank.effect_template(handle);root=effect.raw;
                    check(effect.name==expected.at("name").get<std::string>()&&effect.input_effect_name_word==expected.at("input_effect_name_word"),"Template root words differ");
                    check(effect.additional_inputs.size()==expected.at("additional_sound_inputs").size()&&effect.platforms.size()==expected.at("platform_effect_templates").size(),"Template child count differs");
                    for(std::size_t i=0;i<effect.additional_inputs.size();++i) {
                        const auto& item=effect.additional_inputs[i];const auto& from=expected.at("additional_sound_inputs")[i];
                        same_name(item.effect,from,"dsp_effect_string_id","dsp_effect_name");same_data(item.function,from.at("function"));
                        check(item.time_period_bits==from.at("time_period_bits"),"Additional effect input period differs");
                    }
                    for(std::size_t i=0;i<effect.platforms.size();++i) {
                        const auto& platform=effect.platforms[i];const auto& from=expected.at("platform_effect_templates")[i];
                        same_name(platform.input,from,"input_dsp_effect_string_id","input_dsp_effect_name");check(platform.templates.size()==from.at("templates").size(),"Platform template count differs");
                        for(std::size_t j=0;j<platform.templates.size();++j) {
                            const auto& item=platform.templates[j];const auto& t=from.at("templates")[j];
                            same_name(item.input,t,"input_dsp_effect_string_id","input_dsp_effect_name");
                            check(hex(item.raw.bytes.subspan(4,12))==t.at("unknown_header_hex").get<std::string>()&&item.components.size()==t.at("components").size(),"Unknown template header/component count differs");
                            for(std::size_t k=0;k<item.components.size();++k) {
                                const auto& c=item.components[k];const auto& s=t.at("components")[k];
                                check(c.value_type==s.at("value_type")&&c.default_bits==s.at("default_bits")&&c.minimum_bits==s.at("minimum_bits")&&c.maximum_bits==s.at("maximum_bits"),"Template component words differ");
                            }
                        }
                    }
                }else {
                    const auto at=std::ranges::find(bank.parameters(),handle,&MccSoundEffectDefinitions::Parameters::handle);
                    check(at!=bank.parameters().end()&&at->name==expected.at("name").get<std::string>(),"Environment/mix definition is missing");root=at->raw;
                }
                check(root.source_address==expected.at("source_address")&&root.bytes.size()==expected.at("bytes")&&digest(root.bytes)==expected.at("actual_payload_sha256").get<std::string>(),"Actual retained audio payload differs from independent inventory");++count;
                check(reinterpret_cast<std::uintptr_t>(root.bytes.data())>0xffffffffu,"Retained audio definition lost its native64 backing");
            }
            if(count!=expected_count||count!=bank.collections().size()+bank.templates().size()+bank.parameters().size())
                throw std::runtime_error(std::string("Audio definition set differs in ")+map+": compared="+std::to_string(count)+
                    " expected="+std::to_string(expected_count)+" actual="+
                    std::to_string(bank.collections().size()+bank.templates().size()+bank.parameters().size()));
            for(const auto& child:inventory.at("linked_metadata_outside_primary_tag_extent"))if(child.at("map")==map)++shared_children;
            bool wrong=false;try{(void)bank.effect_template({0xffffffffu});}catch(const FormatError&){wrong=true;}check(wrong,"Unowned/null template accepted");
            results.push_back({{"map",map},{"definitions",count},{"collections",bank.collections().size()},{"templates",bank.templates().size()},{"parameters",bank.parameters().size()},{"cache_retained_after_caller_release",true}});total+=count;
        }
        check(!weak.expired()&&retained_curve&&!retained_curve->bytes.empty(),"Detached nested extent did not outlive its definition bank");
        check(digest(retained_curve->bytes)==digest(retained_curve->cache_owner->read_tag_bytes(retained_curve->source_address,retained_curve->bytes.size())),"Detached curve bytes escaped retained cache lifetime");
        retained_curve.reset();
        check(weak.expired(),"Effect cache backing leaked after the final bank lease");
    }
    bool rejected=false;try{MccSoundEffectDefinitions absent({});}catch(const FormatError&){rejected=true;}check(rejected,"Absent MCC cache accepted");
    Json report{{"schema","H2_MCC_SOUND_EFFECT_DEFINITIONS_V2"},{"passed",true},{"maps",results},{"definitions",total},{"nested_override_maps",nested_results},{"nested_invalid_metadata_rejections",nested_rejections},{"raw_bit_and_shared_child_controls",raw_bit_controls},{"detached_nested_extent_cache_lifetime_checked",true},{"inventory_shared_child_extents_in_selected_maps",shared_children},{"invalid_metadata_rejections",rejected_metadata},{"native64_backings_checked",true},{"cache_release_checked",true},{"function_evaluation_performed",false},{"xbox_program_ready",false},{"tu15_conversion_established",false},{"initializer45_returned",false}};
    std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';std::cout<<report.dump(2)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
