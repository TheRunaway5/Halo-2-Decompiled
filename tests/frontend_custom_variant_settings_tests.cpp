#include "halo2/custom_slayer.hpp"
#include "halo2/bomb_defusal.hpp"
#include "halo2/multiplayer.hpp"
#include "../src/ui/frontend_custom_variant_settings.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>

using namespace halo2;
using namespace halo2::detail;
namespace {
unsigned checks{};
void need(bool value,const char *message) {
  ++checks;if(!value)throw FormatError(message);
}
UiParameter parameter(unsigned id,std::initializer_list<int> values) {
  UiParameter result;result.parameter=id;result.default_value=17;
  result.title="Authored title";result.header="Authored header";result.description="Authored description";
  for(const auto value:values)result.choices.push_back({std::to_string(value),value});
  return result;
}
void admitted_choices(const UiVariant &variant,const UiParameter &settings) {
  need(!settings.choices.empty(),"Custom option has no usable choices");
  for(const auto &choice:settings.choices) {
    auto edited=variant;edited.values.set(settings.parameter,choice.value);
    const auto rules=slayer_rules(edited,true);
    need(rules.session_configuration.has_value(),"Custom option lost its actual admission owner");
    need(source_game_variant_valid(source_game_variant(edited)),"Custom picker admitted an invalid original buffer");
  }
}
bool has(const UiParameter &settings,int value) {
  return std::ranges::any_of(settings.choices,[&](const auto &choice){return choice.value==value;});
}
void stock_untouched() {
  const UiVariant stock{"Stock Slayer",0,{}};
  const auto authored=parameter(0,{-1,0,1,6,7});
  const auto result=frontend_custom_variant_parameter(stock,authored);
  need(result && result->parameter==authored.parameter && result->default_value==authored.default_value &&
      result->title==authored.title && result->header==authored.header && result->description==authored.description &&
      result->choices.size()==authored.choices.size(),"Custom policy changed stock parameter metadata");
  for(unsigned i=0;i<authored.choices.size();++i)
    need(result->choices[i].value==authored.choices[i].value && result->choices[i].label==authored.choices[i].label,
        "Custom policy filtered or relabeled a stock choice");
  need(!frontend_custom_variant(stock) && frontend_custom_variant_parameters(stock).empty() &&
      !frontend_custom_variant_parameter(stock,frontend_defusal_fuse_datum) &&
      frontend_custom_variant_description(stock).empty(),"Stock Slayer acquired custom settings");
  for(const unsigned id:{112u,frontend_defusal_fuse_datum,native_defusal_fuse_parameter,0xffffffffu})
    need(frontend_variant_parameter_value(stock,parameter(id,{}))==0,
        "Stock out-of-range descriptor acquired a native default value");
  auto limited=stock;limited.values={{15,8},{90,16}};
  need(frontend_variant_parameter_value(limited,parameter(90,{}))==16,
      "Stock active-player getter changed to custom participant-limit semantics");
}
void common_controls(const UiVariant &fiesta,const UiVariant &infection,const UiVariant &defusal) {
  const auto scores=frontend_custom_variant_parameter(fiesta,parameter(2,{0,1,25,50,32767,32768}));
  need(scores && scores->choices.size()==5 && !has(*scores,32768),"Fiesta score choices ignored original bounds");
  admitted_choices(fiesta,*scores);
  for(const auto *mode:{&fiesta,&infection,&defusal}) {
    need(frontend_custom_variant(*mode),"A bundled custom mode was not recognized");
    need(!frontend_custom_variant_description(*mode).empty(),"Custom mode lost its fixed-rule description");
    for(const unsigned id:{0u,15u,19u,20u,21u,23u,25u,26u,75u,76u,77u,78u,79u,80u,81u,82u,83u,84u,85u,86u,87u,89u,91u,94u})
      need(!frontend_custom_variant_parameter(*mode,parameter(id,{0,1,2,3})),
          "Custom mode exposed a fixed, ineffective or unsupported original control");
    for(const unsigned id:{104u,105u}) {
      const auto damage=frontend_custom_variant_parameter(*mode,parameter(id,{0,1}));
      need(damage && damage->choices.size()==2,"Custom combat damage control was omitted");
      admitted_choices(*mode,*damage);
    }
  }
  for(const auto *mode:{&infection,&defusal}) {
    need(!frontend_custom_variant_parameter(*mode,parameter(2,{0,1,13,25})),"Fixed custom ending exposed a score-limit edit");
    need(!frontend_custom_variant_parameter(*mode,parameter(16,{0,1,3,5})),"Custom mode exposed fixed life policy");
    const auto capacity=frontend_custom_variant_parameter(*mode,parameter(90,{0,1,2,8,16,17}));
    need(capacity && capacity->choices.size()==3 && !has(*capacity,1) && capacity->title=="Player Limit",
        "Two-sided custom mode exposed an impossible player limit");
    admitted_choices(*mode,*capacity);
  }
  for(const unsigned id:{17u,18u,29u,30u,88u})
    need(!frontend_custom_variant_parameter(defusal,parameter(id,{0,1,2,5})),
        "Defusal exposed ignored ordinary respawn/scoring/equipment controls");
  need(!frontend_custom_variant_parameter(infection,parameter(88,{0,1})),"Infection exposed global grenades over fixed zombie traits");
  const auto plant=frontend_custom_variant_parameter(defusal,parameter(10,{0,1,150,600,601}));
  need(plant && plant->choices.size()==3 && plant->title=="Plant Window" && !has(*plant,0) && !has(*plant,601),
      "Defusal plant window admitted infinite or oversized rounds");
  admitted_choices(defusal,*plant);
  const auto authored_minutes=frontend_custom_variant_parameter(defusal,parameter(10,{0,60,180,300,900}));
  need(authored_minutes && has(*authored_minutes,150),"Default 150-second plant window was absent from its authored minute choices");
  const auto exact=std::ranges::find(authored_minutes->choices,150,&UiChoice::value);
  need(exact!=authored_minutes->choices.end() && exact->label=="150 seconds",
      "Current custom plant window did not receive a meaningful duration label");
  admitted_choices(defusal,*authored_minutes);
  const auto survival=frontend_custom_variant_parameter(infection,parameter(10,{0,1,180,600,32767,32768}));
  need(survival && survival->choices.size()==5 && survival->title=="Survival Time",
      "Infection survival timer lost supported choices");
  admitted_choices(infection,*survival);
  const auto friendly=frontend_custom_variant_parameter(defusal,parameter(24,{0,1}));
  need(friendly && friendly->choices.size()==2,"Defusal friendly-fire control was omitted");
  admitted_choices(defusal,*friendly);
  need(!frontend_custom_variant_parameter(fiesta,parameter(2,{25})),"An unchangeable custom parameter remained as a dead picker");
  auto bounded=fiesta;bounded.values.set(15,8);
  const auto legacy_capacity=frontend_custom_variant_parameter(bounded,parameter(90,{1,2,8,9,16}));
  need(legacy_capacity && legacy_capacity->choices.size()==3 &&
      frontend_variant_parameter_value(bounded,*legacy_capacity)==8 && !has(*legacy_capacity,9) && !has(*legacy_capacity,16),
      "Custom player limit overstated its saved capacity bound");
  admitted_choices(bounded,*legacy_capacity);
  for(const auto &choice:legacy_capacity->choices) {
    auto edited=bounded;edited.values.set(90,choice.value);
    need(slayer_rules(edited,true).maximum_players==unsigned(choice.value),
        "Offered custom player limit differed from actual participant capacity");
  }
}
void timers(const UiVariant &fiesta,const UiVariant &infection,const UiVariant &defusal) {
  need(frontend_custom_variant_category<=unsigned(std::numeric_limits<int>::max()),"Custom category overflowed menu context");
  need(frontend_custom_variant_parameters(fiesta).empty() && frontend_custom_variant_parameters(infection).empty(),
      "Slayer extensions acquired unrelated bomb settings");
  const auto datums=frontend_custom_variant_parameters(defusal);
  need(datums==std::vector<unsigned>{frontend_defusal_fuse_datum,frontend_defusal_plant_datum,frontend_defusal_disarm_datum},
      "Bomb mode settings lost timer order");
  for(const auto datum:datums) {
    need(datum<=unsigned(std::numeric_limits<int>::max()),"Custom parameter overflowed menu context");
    const auto settings=frontend_custom_variant_parameter(defusal,datum);
    need(settings && settings->parameter>=native_defusal_fuse_parameter && settings->parameter<=native_defusal_disarm_parameter &&
        !settings->title.empty() && !settings->description.empty(),"Bomb timer metadata is incomplete");
    admitted_choices(defusal,*settings);
    need(has(*settings,frontend_variant_parameter_value(defusal,*settings)),"Bomb picker omitted its current value");
    need(!frontend_custom_variant_parameter(fiesta,datum) && !frontend_custom_variant_parameter(infection,datum),
        "Bomb timer leaked into another custom mode");
  }
  need(!frontend_custom_variant_parameter(defusal,frontend_custom_variant_category),"Category context was treated as a parameter");
  auto edited=defusal;
  edited.values.set(native_defusal_fuse_parameter,1370);
  edited.values.append(native_defusal_fuse_parameter,1371);
  edited.values.set(native_defusal_plant_parameter,137);
  edited.values.set(native_defusal_disarm_parameter,197);
  for(const auto datum:datums) {
    const auto settings=frontend_custom_variant_parameter(edited,datum);
    need(settings && has(*settings,frontend_variant_parameter_value(edited,*settings)),
        "Bomb picker lost an arbitrary valid saved timer");
    admitted_choices(edited,*settings);
  }
  const auto fuse=frontend_custom_variant_parameter(edited,frontend_defusal_fuse_datum);
  need(frontend_variant_parameter_value(edited,*fuse)==1371,"Native getter did not use the last ordered write");
  edited.values.set(native_defusal_fuse_parameter,5*30);
  edited.values.set(native_defusal_disarm_parameter,4*30);
  const auto shortened=frontend_custom_variant_parameter(edited,frontend_defusal_disarm_datum);
  need(shortened && has(*shortened,120) && !has(*shortened,150) && !has(*shortened,180),
      "Defuse picker admitted times that meet or exceed the fuse");
  admitted_choices(edited,*shortened);
  GameVariantSettings defaults;
  for(const auto &[id,value]:defusal.values)if(id!=native_defusal_fuse_parameter)defaults.append(id,value);
  auto omitted=defusal;omitted.values=defaults;
  const auto fallback=frontend_custom_variant_parameter(omitted,frontend_defusal_fuse_datum);
  need(fallback && frontend_variant_parameter_value(omitted,*fallback)==BombDefusalRules{}.fuse_ticks,
      "Missing native timer failed to use the actual mode default");
  need(frontend_variant_parameter_value(defusal,parameter(10,{150}))==150,
      "Ordinary parameter getter stopped reading its source field");
}
} // namespace
int main(int argc,char **argv)try {
  need(argc==2,"Usage: frontend_custom_variant_settings_tests VARIANT_DIRECTORY");
  const auto variants=load_custom_slayer_variants(argv[1]);
  const auto find=[&](auto predicate)->const UiVariant& {
    const auto found=std::ranges::find_if(variants,predicate);
    need(found!=variants.end(),"Bundled custom mode is absent");return *found;
  };
  const auto &fiesta=find([](const auto &v){return custom_slayer(v)==CustomSlayer::fiesta;});
  const auto &infection=find([](const auto &v){return custom_slayer(v)==CustomSlayer::infection;});
  const auto &defusal=find(bomb_defusal_variant);
  stock_untouched();common_controls(fiesta,infection,defusal);timers(fiesta,infection,defusal);
  std::cout<<"PASS "<<checks<<" custom settings checks: unchanged stock metadata, fixed mode rules, admitted meaningful controls, native timer values and fuse/defuse constraints\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
