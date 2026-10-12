#include "halo2/object_initialization.hpp"
#include <algorithm>

namespace halo2 {
namespace {
void valid_placement(const ScenarioObject& object){
    if(object.type<0 || object.type>=13 || object.placement<0)
        throw FormatError("invalid source scenario construction identity");
}
bool identifier_matches(const ScenarioObject& object,const SourceScenarioObjectIdentity& other){
    return object.source!=-1 && object.unique_id==other.unique_id &&
        object.type==other.type && object.source==other.source &&
        (object.source!=0 || object.origin_bsp==other.origin_bsp);
}
template<class Predicate>
std::vector<SourceScenarioCreationKey> attempts(std::span<const ScenarioObject> objects,
                                               Predicate admit){
    std::vector<const ScenarioObject*> ordered;
    for(const auto& object:objects){valid_placement(object);ordered.push_back(&object);}
    std::sort(ordered.begin(),ordered.end(),[](const auto* a,const auto* b){
        return std::pair{a->type,a->placement}<std::pair{b->type,b->placement};});
    std::vector<SourceScenarioCreationKey> result;
    for(const auto* object:ordered)if(admit(*object))result.push_back({object->type,object->placement});
    return result;
}
}
bool source_scenario_factory_game_allowed(const ScenarioObject& object,int game_type,int engine){
    bool admitted=(object.flags&64u)==0;
    const auto games=object.valid_multiplayer_games;
    if(object.type==6 && game_type==2 && games!=0 && games!=0x7f) {
        int bit=-1;
        switch(engine) {
            case 1:bit=0;break;case 2:bit=1;break;case 3:bit=2;break;
            case 4:bit=3;break;case 7:bit=4;break;case 8:bit=5;break;
            case 9:bit=6;break;
        }
        if(bit>=0)admitted=(games&(1u<<unsigned(bit)))!=0;
    }
    return admitted;
}
int source_scenario_creation_mode(const ScenarioObject& object){
    valid_placement(object);
    if(object.bsp_policy==0){
        if(object.source==0)return 1;
        if(object.source!=1)return -1;
        if(object.origin_bsp==-1 || ((1u<<unsigned(object.type))&0x181fu))return 0;
        return 1;
    }
    if(object.bsp_policy==1)return 0;
    if(object.bsp_policy==2)return 2;
    return -1;
}
bool source_scenario_initial_creation_candidate(const ScenarioObject& object){
    return !(object.flags&1u) && (object.name!=-1 || object.damageable ||
                                source_scenario_creation_mode(object)!=1);
}
std::vector<SourceScenarioCreationKey> source_scenario_initial_creation_attempts(
    std::span<const ScenarioObject> objects,bool suppress,
    std::span<const SourceScenarioObjectIdentity> existing){
    return attempts(objects,[&](const auto& object){
        return source_scenario_initial_creation_candidate(object) &&
            (!suppress || std::none_of(existing.begin(),existing.end(),
                [&](const auto& value){return identifier_matches(object,value);}));
    });
}
std::vector<SourceScenarioCreationKey> source_scenario_bsp_creation_attempts(
    std::span<const ScenarioObject> objects,int active_bsp,
    std::span<const SourceScenarioObjectIdentity> existing){
    return attempts(objects,[&](const auto& object){
        if(object.name!=-1 || object.damageable || (object.flags&1u))return false;
        if(std::any_of(existing.begin(),existing.end(),[&](const auto& value){
            return (value.source==0 || value.source==1) && value.type==object.type &&
                   value.placement==object.placement;}))return false;
        if(object.bsp_policy==2)
            return (std::uint32_t(object.manual_bsps)&(1u<<(unsigned(active_bsp)&31u)))!=0;
        return object.origin_bsp==-1 || object.origin_bsp==active_bsp ||
               source_scenario_initial_creation_candidate(object);
    });
}
bool SourceObjectConstructorMetadata::has_verified_cold_scenery_rng_path()const{
    return type==6 && animation==0xffffffffu && creation_effect==0xffffffffu &&
        attachment_count==0 && widget_count==0 && function_count==0 &&
        change_color_count==0 && std::all_of(variant_child_counts.begin(),
            variant_child_counts.end(),[](auto count){return count==0;});
}
void source_initialize_object_noise_random(std::uint32_t& primary){
    for(unsigned i=0;i<24;++i)primary=primary*0x19660du+0x3c6ef35fu;
}
void SourceObjectInitialization::initialize_game(std::uint32_t launch_seed,
    const SourceGameRandomInitializationServices& services){
    objects_.clear();sequence_=0;*primary_=launch_seed;primary_known_=true;
    if(!services.before_noise || !services.before_noise(*primary_))primary_known_=false;
    source_initialize_object_noise_random(*primary_);
    if(!services.after_noise || !services.after_noise(*primary_))primary_known_=false;
}
const SourceInitializedObjectModel& SourceObjectInitialization::create(const Cache& cache,
    const ScenarioObject& object,std::uint32_t handle,std::int32_t tick,bool cold,
    const SourceObjectConstructorRandomServices& services){
    valid_placement(object);
    if(handle==0xffffffffu || objects_.contains(handle))
        throw FormatError("duplicate or NONE source construction handle");
    const auto metadata=cache.load_object_constructor_metadata(object.definition);
    if(!metadata || metadata->type!=object.type)
        throw FormatError("source construction definition/type mismatch");
    const bool simple=cold && metadata->has_verified_cold_scenery_rng_path();
    const auto stage=[&](SourceObjectConstructorStage point){
        if(services.apply)return services.apply(cache,object,point,*primary_);
        return simple;
    };
    if(!stage(SourceObjectConstructorStage::before_model))primary_known_=false;
    SourceInitializedObjectModel record;
    record.key={object.type,object.placement};record.handle=handle;
    record.definition=object.definition;record.created_tick=tick;
    record.creation_sequence=sequence_++;record.seed_before=*primary_;
    record.seed_before_known=primary_known_;
    const auto definition=cache.load_initial_model_definition(object.definition);
    const auto variant=object.variant?object.variant:definition.default_variant;
    record.selection=primary_known_?source_initial_model_selection(definition,variant,*primary_):
        source_deterministic_initial_model_selection(definition,variant);
    if(!stage(SourceObjectConstructorStage::after_model))primary_known_=false;
    record.constructor_consumption_known=primary_known_;record.seed_after=*primary_;
    return objects_.emplace(handle,std::move(record)).first->second;
}
void SourceObjectInitialization::destroy(std::uint32_t handle){objects_.erase(handle);}
const SourceInitializedObjectModel* SourceObjectInitialization::find(std::uint32_t handle)const{
    const auto found=objects_.find(handle);return found==objects_.end()?nullptr:&found->second;
}
const SourceInitializedObjectModel* SourceObjectInitialization::find(SourceScenarioCreationKey key)const{
    const SourceInitializedObjectModel* result=nullptr;
    for(const auto& [handle,record]:objects_)if(record.key==key &&
        (!result || result->creation_sequence<record.creation_sequence))result=&record;
    return result;
}

std::optional<SourceObjectConstructorMetadata> Cache::load_object_constructor_metadata(
    std::uint32_t datum)const{
    const auto found=query_tag({.handle=TagHandle{datum}, .require_address=true});
    if(!found || found->size<0xb4)return std::nullopt;
    const Bytes data(storage_);const auto root=metadata_offset(found->address,0xb4);
    SourceObjectConstructorMetadata result;result.type=std::int16_t(data.u16(root));
    if(result.type<0 || result.type>=13)return std::nullopt;
    static constexpr std::array<std::string_view,13> groups={
        "bipd","vehi","weap","eqip","garb","proj","scen","mach","ctrl","lifi","ssce","bloc","crea"};
    if(found->group!=groups[unsigned(result.type)])return std::nullopt;
    result.creation_effect=data.u32(root+0x50);
    result.attachment_count=data.u32(root+0x94);result.widget_count=data.u32(root+0x9c);
    result.function_count=data.u32(root+0xa4);result.change_color_count=data.u32(root+0xac);
    const auto model=data.u32(root+0x38);
    if(model==0xffffffffu)return result;
    const auto hlmt=query_tag({.handle=TagHandle{model}, .group=tag_groups::model, .require_address=true});
    if(!hlmt)throw FormatError("source constructor model metadata missing");
    const auto at=metadata_offset(hlmt->address,0x58);result.animation=data.u32(at+0x14);
    const auto count=data.u32(at+0x50);
    if(count>0x10000)throw FormatError("excessive source constructor variant count");
    if(count){
        const auto variants=metadata_offset(data.u32(at+0x54),std::size_t(count)*0x38);
        for(std::size_t i=0;i<count;++i)result.variant_child_counts.push_back(data.u32(variants+i*0x38+0x1c));
    }
    return result;
}
}
