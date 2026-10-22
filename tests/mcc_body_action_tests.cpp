// Genuine MCC body-action selection through independently named authored rows.
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/native_player_animation.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/unit_animation_selection.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <openssl/sha.h>

using namespace halo2;
namespace {
void check(bool value,const std::string &why) {if(!value)throw FormatError(why);}
auto digest(std::span<const std::byte> bytes) {
  std::array<unsigned char,SHA256_DIGEST_LENGTH> result{};
  SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),result.data());
  return result;
}
struct Action {std::uint32_t compiled;const char *name;};
// Source operands plus independent exact names, not expected MCC integers.
constexpr std::array actions{Action{0x08000002u,"reload_1"},
    Action{0x0b00053eu,"fire_1_left"},Action{0x0d00053au,"reload_1_left"},
    Action{0x0a000014u,"move_front"}};
// An actual published FP clock can request this body action. Its absence in
// an authored body bank is legitimate and must not become an unknown-symbol
// exception, or be replaced with a guessed idle/ready clip.
constexpr Action posing{0x0600005fu,"posing"};
struct Selected {
  AnimationClip clip;std::uint32_t authored{};unsigned table{};
  std::array<std::uint32_t,3> context{};
};

// Own only the actual requested body/FP graphs and their inherited graph closure.
// This is a read-only fixture bank, not a new runtime resolver or graph schema.
struct Graphs {
  const Cache &cache;
  std::map<std::uint32_t,SourceTagMetadata> rows;
  explicit Graphs(const Cache &c):cache(c) {}
  void retain(std::uint32_t graph) {
    if(rows.contains(graph))return;
    const auto [it,inserted]=rows.emplace(graph,cache.load_source_tag_metadata(graph));
    check(inserted&&it->second.group()=="jmad","Expected actual animation graph owner");
    const auto &owner=it->second;
    const Bytes header(owner.bytes_at(owner.source_address(),0x54));
    const auto count=header.u32(0x4c),base=header.u32(0x50);
    check(count<=64,"Inherited graph list exceeds Cache's existing bound");
    for(std::uint32_t i=0;i<count;++i) {
      const auto datum=Bytes(owner.bytes_at(base+i*32+4,4)).u32(0);
      if(datum!=null_datum)retain(datum);
    }
  }
  SourceAnimationGraphMemory memory() const {
    // Same actual MCC dual namespace required by the existing raw query API.
    check(cache.string_id(0x04000557u)=="dual","MCC dual namespace changed");
    return {[this](auto graph){return rows.at(graph).source_address();},
      [this](std::uint32_t address,std::size_t extent)->std::span<const std::byte> {
        const SourceTagMetadata *selected=nullptr;
        for(const auto &[_,row]:rows)if(address>=row.source_address()) {
          const auto offset=std::uint64_t(address)-row.source_address();
          if(offset<=row.size()&&extent<=row.size()-offset) {
            check(!selected||!extent,"Ambiguous retained graph owner");selected=&row;
          }
        }
        check(selected,"Pointer escapes retained authored graph closure");
        return selected->bytes_at(address,extent);
      },0x04000557u};
  }
  void unchanged() const {
    for(const auto &[datum,before]:rows) {
      const auto after=cache.load_source_tag_metadata(datum);
      check(before.source_address()==after.source_address()&&before.size()==after.size()&&
        digest(before.bytes_at(before.source_address(),before.size()))==
        digest(after.bytes_at(after.source_address(),after.size())),
        "Body/FP selection changed actual authored graph bytes");
    }
  }
};

// Reuse only the already-qualified context iterator/class remap. Selection of
// the ACTION is independent: linear exact-name scan, never compiled_symbols,
// translator output or the production numeric action search. Thus this test
// qualifies the selector boundary, not the reused fallback iterator itself.
std::optional<Selected> named_action(const Cache &cache,const Graphs &graphs,
    std::uint32_t graph,std::array<std::uint32_t,2> ids,bool crouch,bool dual,
    std::string_view name) {
  const auto memory=graphs.memory();const auto root=memory.graph_address(graph);
  const auto cls=source_animation_weapon_class(root,dual?memory.dual_weapon_class:ids[0],ids[1],memory);
  for(unsigned table:{12u,4u}) {
    std::array<std::byte,28> iterator{};
    const auto put=[&](unsigned at,std::uint32_t value) {
      for(unsigned i=0;i<4;++i)iterator[at+i]=std::byte(value>>(8*i));
    };
    put(0,root);put(4,crouch?0x06000087u:0x06000086u);put(8,cls);put(12,ids[1]);
    std::array<std::uint32_t,3> context{};
    while(const auto weapon=next_source_animation_weapon(iterator,context,memory)) {
      const Bytes bank(memory.read(weapon+table,8));const auto count=bank.u32(0),base=bank.u32(4);
      check(count<=10000,"Authored action bank exceeds bounded fixture census");
      for(std::uint32_t i=0;i<count;++i) {
        const Bytes row(memory.read(base+i*8,8));const auto id=row.u32(0);
        if(cache.string_id(id)!=name)continue;
        check(StringId{id}.length()==name.size(),"Authored action has a wrong full-word StringID length");
        const auto index=std::bit_cast<std::int16_t>(row.u16(6));
        if(index==-1)continue; // Exact authored null row; try the remaining real fallbacks.
        check(index>=0,"Invalid negative authored clip index");
        auto selected_graph=graph;const auto inherited=std::bit_cast<std::int16_t>(row.u16(4));
        if(inherited!=-1) {
          const Bytes header(memory.read(root+0x4c,8));
          check(inherited>=0&&std::uint32_t(inherited)<header.u32(0),"Invalid authored inherited clip owner");
          selected_graph=Bytes(memory.read(header.u32(4)+std::uint32_t(inherited)*32+4,4)).u32(0);
        }
        auto clip=table==4?cache.load_action(graph,cache.string_id(context[0]),
            cache.string_id(context[1]),cache.string_id(context[2]),std::string(name)):
            cache.load_animation(selected_graph,unsigned(index));
        check(clip.source_graph==selected_graph&&clip.source_index==unsigned(index),
          "Independent authored row/decoder clip identity differs");
        if(table==4)check(clip.source_identifier==row.u32(4),"Named ordinary query changed its actual packed row");
        clip.source_identifier=row.u32(4); // Expected packed word read from this real row.
        return Selected{std::move(clip),id,table,context};
      }
    }
  }
  return std::nullopt; // Exhausted BOTH actual banks and every actual context.
}
bool same(const AnimationClip &a,const AnimationClip &b) {
  return a.source_graph==b.source_graph&&a.source_index==b.source_index&&
    a.source_identifier==b.source_identifier&&a.frame_count==b.frame_count&&
    a.animation_type==b.animation_type&&a.components==b.components&&a.nodes==b.nodes;
}
template<class F>void unsupported(F &&f) {
  bool rejected=false;
  try {f();}catch(const FormatError &e) {
    rejected=std::string_view(e.what()).starts_with("Unsupported compiled weapon animation selector");
    if(!rejected)throw;
  }
  check(rejected,"Unknown/full-word compiled selector was accepted");
}
void raw_controls(const Cache &cache,const Graphs &graphs,std::uint32_t graph,std::array<std::uint32_t,2> ids,
    bool crouch,bool dual,const Action &action,const Selected &expected) {
  const auto query=[&](std::uint32_t id)->std::optional<AnimationClip> {
    try {return cache.load_unit_weapon_action(graph,crouch?0x06000087u:0x06000086u,
        dual?0x04000557u:ids[0],ids[1],id,true);}
    catch(const FormatError &e) {
      if(!std::string_view(e.what()).starts_with("authored unit weapon action missing:"))throw;
      return std::nullopt;
    }
  };
  const auto actual=query(expected.authored);
  check(actual&&same(*actual,expected.clip),"Exact raw MCC authored selector failed");
  check(!query(expected.authored^0x01000000u),"Raw lookup ignored the StringID high-byte length");
  if(expected.authored!=action.compiled) {
    check(cache.string_id(action.compiled)!=action.name,"Foreign-namespace control lost its distinct authored meaning");
    const auto foreign=query(action.compiled);
    // A foreign integer may name a genuine different row, even a clip alias.
    // Check that row independently instead of assuming foreign always means
    // missing, or incorrectly treating equal clip payloads as translation.
    auto foreign_expected=named_action(cache,graphs,graph,ids,crouch,dual,
        cache.string_id(action.compiled));
    // string_id decodes low24 only; a differently sized authored name at that
    // index does not make this exact raw full-word key a present action.
    if(foreign_expected&&foreign_expected->authored!=action.compiled)foreign_expected.reset();
    check(bool(foreign)==bool(foreign_expected),"Raw authored API dropped or invented a foreign authored row");
    if(foreign)check(foreign_expected->authored==action.compiled&&
        same(*foreign,foreign_expected->clip),"Raw authored API translated a foreign compiled selector");
  }
}
}

int main(int argc,char **argv)try {
  check(argc==2,"Supply one explicit prepared MCC map (no XBE/Vista/reference arguments)");
  const std::filesystem::path map=argv[1];
  check(map.is_absolute()&&std::filesystem::is_regular_file(map),"Explicit MCC map must be an existing absolute file");
  Cache cache(map);check(cache.is_mcc()&&!cache.is_vista(),"BODY acceptance requires MCC runtime content");
  const auto content=cache.load_content(0,true);
  const auto weapon=[&](std::string_view suffix)->const WeaponDefinition & {
    const WeaponDefinition *found=nullptr;
    for(const auto &[_,candidate]:content.weapons)if(candidate.name.ends_with(suffix)) {
      check(!found,"Requested authored weapon path is ambiguous");found=&candidate;
    }
    check(found,"Requested authored SMG/rifle is absent from this MCC map");return *found;
  };
  const std::array weapons{&weapon("\\smg"),&weapon("\\battle_rifle")};
  check(weapons[0]->dual_wieldable&&!weapons[1]->dual_wieldable,"Actual weapon dual flags differ from scoped SMG/rifle inputs");
  Graphs graphs(cache);std::array<std::uint32_t,2> body_graphs{};
  for(unsigned species=0;species<2;++species) {
    const auto rep=species+2;const auto biped=content.player_representations.at(rep).biped.datum;
    body_graphs[species]=cache.load_model_animation_graph(biped);graphs.retain(body_graphs[species]);
    for(const auto *w:weapons)for(const auto &fp:w->first_person_models)
      if(fp.animation!=null_datum)graphs.retain(fp.animation);
  }
  unsigned selected{},missing{},requests{},contexts{},namespace_controls{},fp_bindings{};
  unsigned posing_selected{},posing_missing{},posing_requests{};
  std::array<std::array<unsigned,actions.size()>,2> positive{};
  for(unsigned species=0;species<2;++species)for(const auto *w:weapons) {
    const auto ids=cache.load_weapon_animation_identifiers(w->datum);
    NativePlayerAnimation body(cache,body_graphs[species],w->datum);
    // Query real FP bindings: BODY additions must not leak into canonical FP
    // clock activities. The exact original36 source-prefix diff is a separate
    // review receipt, not a duplicated expected table in this fixture.
    const auto fp=cache.load_first_person_binding(w->datum,int(species+2));
    check(fp.record_present&&fp.graph!=null_datum,"Actual represented FP binding missing");++fp_bindings;
    check(std::any_of(fp.clock_clips.begin(),fp.clock_clips.end(),[](const auto &c){return c.activity==0x0400000cu;}),
      "FP canonical idle activity disappeared");
    for(const auto *clips:{&fp.clock_clips,&fp.dual_clock_clips})for(const auto &clip:*clips) {
      check(std::none_of(actions.begin(),actions.end(),[&](const auto &a){return a.compiled==clip.activity;}),
        "BODY-only addition leaked into the canonical FP clock prefix");
      check(clip.activity!=posing.compiled,"BODY posing addition leaked into the canonical FP clock prefix");
    }
    for(bool dual:{false,true}) {if(dual&&!w->dual_wieldable)continue;
      for(bool crouch:{false,true}) {
        unsigned context_present=0;++contexts;
        for(std::size_t ai=0;ai<actions.size();++ai) {
          const auto &action=actions[ai];++requests;
          const auto expected=named_action(cache,graphs,body_graphs[species],ids,crouch,dual,action.name);
          // Never turn Unsupported into a legitimate authored omission.
          const auto *actual=body.action(action.compiled,crouch,dual);
          const auto label=std::to_string(species+2)+" "+w->name+" crouch="+std::to_string(crouch)+
            " dual="+std::to_string(dual)+" "+action.name;
          check(bool(expected)==bool(actual),"Named authored availability differs: "+label);
          if(!expected) {++missing;std::cout<<"AUTHORED-MISSING "<<label<<'\n';continue;}
          check(same(*actual,expected->clip),"Production BODY selector chose a different actual authored clip: "+label);
          check(body.action(action.compiled,crouch,dual)==actual,"Repeated BODY lookup changed the owned clip address");
          ++selected;++context_present;++positive[species][ai];
          raw_controls(cache,graphs,body_graphs[species],ids,crouch,dual,action,*expected);++namespace_controls;
          unsupported([&]{(void)body.action(action.compiled^0x01000000u,crouch,dual);});
        }
        const auto expected_posing=named_action(cache,graphs,body_graphs[species],ids,crouch,dual,posing.name);
        const auto *actual_posing=body.action(posing.compiled,crouch,dual);++posing_requests;
        check(bool(actual_posing)==bool(expected_posing),"Published posing clock changed named BODY availability");
        check(body.action(posing.compiled,crouch,dual)==actual_posing,"Repeated posing lookup changed the owned result");
        if(expected_posing) {
          check(same(*actual_posing,expected_posing->clip),"Published posing clock selected a different named BODY clip");
          raw_controls(cache,graphs,body_graphs[species],ids,crouch,dual,posing,*expected_posing);++posing_selected;
        } else ++posing_missing;
        unsupported([&]{(void)body.action(posing.compiled^0x01000000u,crouch,dual);});
        check(context_present,"Requested actual stance/dual context had no positive authored action");
        unsupported([&]{(void)body.action(null_datum,crouch,dual);});
      }
    }
  }
  for(const auto &species:positive)for(auto count:species)
    check(count>0,"A requested species/action has only omissions, not real authored coverage");
  check(contexts==12&&requests==48&&posing_requests==12&&fp_bindings==4,"Actual species/weapon/stance/dual census is incomplete");
  graphs.unchanged();
  std::cout<<"PASS MCC BODY named authored actions: representations=2,3 contexts="<<contexts
    <<" requests="<<requests<<" selected="<<selected<<" independently_missing="<<missing
    <<" raw_namespace_controls="<<namespace_controls<<" fp_bindings="<<fp_bindings
    <<" posing_requests="<<posing_requests<<" posing_selected="<<posing_selected<<" posing_independently_missing="<<posing_missing
    <<" unchanged_graphs="<<graphs.rows.size()<<" (selector/clip identity only; no pose or live gameplay claim)\n";
  return 0;
}catch(const std::exception &e) {std::cerr<<"MCC BODY action fixture: "<<e.what()<<'\n';return 1;}
