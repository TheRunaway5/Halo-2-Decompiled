#include "halo2/frontend.hpp"
#include "ui/frontend_og_matchmaking_layout.hpp"
#include "halo2/texture.hpp"
#include "halo2/frontend_emblem.hpp"
#include <openssl/evp.h>
#include <cstdlib>
#include <fstream>
#include <tuple>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::string hash(std::span<const std::byte> bytes){
  std::array<unsigned char,32> digest{};unsigned length{};
  check(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&length,EVP_sha256(),nullptr)==1&&length==32,"Cannot authenticate original UI pixels");
  std::string out;for(const auto b:digest){out+="0123456789abcdef"[b>>4];out+="0123456789abcdef"[b&15];}return out;
}
Texture rgba(const Texture&t){return t.format>=14&&t.format<=16?prepare_xemu_bc_texture(t):t;}
nlohmann::json retained_mcc_bank_cases(const detail::mcc_logical_layout::View &view,
                                     const FrontendContent &source,bool negatives) {
  constexpr std::string_view names[]{
    "ui\\list_skins\\pcr_lists\\player",
    "ui\\list_skins\\pcr_lists\\pvp_others",
    "ui\\list_skins\\pcr_lists\\selecto",
    "ui\\list_skins\\pcr_lists\\team",
    "ui\\screens\\game_shell\\difficulty\\difficulty_options",
    "ui\\screens\\game_shell\\postgame_statistics\\3_column",
    "ui\\screens\\game_shell\\postgame_statistics\\5_column",
    "ui\\screens\\game_shell\\postgame_statistics\\spinners",
    "ui\\screens\\game_shell\\postgame_statistics\\team",
    "ui\\screens\\game_shell\\xbox_live\\matchmaking_progress_dialog\\done",
    "ui\\screens\\game_shell\\xbox_live\\matchmaking_progress_dialog\\matchmaking",
    "ui\\screens\\game_shell\\xbox_live\\matchmaking_progress_dialog\\progress_dots",
    "ui\\screens\\game_shell\\xbox_live\\matchmaking_progress_dialog\\progress_numbers",
  };
  nlohmann::json cases=nlohmann::json::array();
  for(const auto name:names) {
    const auto id=detail::mcc_logical_layout::Codec::named(source.images,std::string(name));
    const auto &actual=source.images.at(id);
    const auto exact_texture=[&](const Texture &a,const Texture &b) {
      check(std::tie(a.width,a.height,a.format,a.layers,a.depth)==std::tie(b.width,b.height,b.format,b.layers,b.depth) &&
            a.mips.size()==b.mips.size() && a.pixels==b.pixels,"Selected MCC bank native frame bytes/extent changed");
      for(unsigned i=0;i<a.mips.size();++i) {
        const auto &x=a.mips[i],&y=b.mips[i];
        check(std::tie(x.width,x.height,x.offset,x.size,x.depth)==std::tie(y.width,y.height,y.offset,y.size,y.depth),
              "Selected MCC bank authored mip descriptor changed");
      }
    };
    const auto verify=[&](const detail::mcc_logical_layout::View &candidate) {
      check(candidate.content.images.contains(id),"Actual selected MCC bank datum is missing");
      const auto &kept=candidate.content.images.at(id);
      check(kept.name==name && kept.name==actual.name,"Selected MCC bank was replaced by a namespace/name alias");
      check(detail::mcc_logical_layout::Codec{candidate.content}.image(kept)==
            detail::mcc_logical_layout::Codec{source}.image(actual),"Selected MCC bank sprite/frame metadata changed");
      check(candidate.rasters.contains(actual.name) && kept.images.size()==actual.images.size() &&
            candidate.rasters.at(actual.name).size()==actual.images.size(),"Selected MCC native raster owner changed");
      for(unsigned frame=0;frame<actual.images.size();++frame) {
        exact_texture(actual.images[frame],kept.images[frame]);
        exact_texture(actual.images[frame],candidate.rasters.at(actual.name)[frame]);
      }
    };
    verify(view);
    unsigned rejected{};
    if(negatives)for(unsigned mutation=0;mutation<3;++mutation) {
      auto changed=view;
      if(mutation==0)changed.content.images.erase(id);
      else if(mutation==1)changed.content.images.at(id).name+="#wrong-owner";
      else {
        check(!changed.content.images.at(id).images.empty() &&
              !changed.content.images.at(id).images.front().pixels.empty(),"Native byte mutation control lacks actual source bytes");
        changed.content.images.at(id).images.front().pixels[0]^=std::byte{1};
      }
      bool denied=false;try{verify(changed);}catch(const FormatError&){denied=true;}
      check(denied,"Changed selected MCC bank owner/native-byte control was accepted");++rejected;
    }
    nlohmann::json frames=nlohmann::json::array();
    for(const auto &frame:actual.images)frames.push_back({{"bytes",frame.pixels.size()},{"native_sha256",hash(frame.pixels)}});
    cases.push_back({{"name",name},{"datum",id},{"native_frames",frames},{"rejected_controls",rejected},{"passed",true}});
  }
  check(cases.size()==13,"Complete13 selected MCC bank regression coverage required");
  return cases;
}


}
int main(int argc,char **argv)try {
  if(argc!=3)return 77;
  const Cache xbox_cache(argv[1]);
  const auto xbox=xbox_cache.load_frontend();
  const auto mcc=Cache(argv[2]).load_frontend();
  const auto &data=detail::og_matchmaking_layout::data();
  std::string generated;
  for(const auto part:detail::og_matchmaking_layout::og_matchmaking_layout_chunks){check(part.size()<=16384,"Original UI literal exceeds its portable chunk bound");generated.append(part);}
  check(generated.size()==detail::og_matchmaking_layout::og_matchmaking_layout_bytes &&
        hash(std::as_bytes(std::span(generated)))==detail::og_matchmaking_layout::og_matchmaking_layout_sha256,
        "Compiled original UI chunks differ from their canonical payload hash");
  const auto profile_path=std::filesystem::path(__FILE__).parent_path().parent_path()/"resources/frontend/tu15-matchmaking-layout.json";
  std::ifstream canonical(profile_path,std::ios::binary);check(bool(canonical),"Canonical original UI profile is unavailable");
  const std::string canonical_bytes{std::istreambuf_iterator<char>(canonical),std::istreambuf_iterator<char>()};
  check(canonical_bytes==generated,"Source JSON and compiled original UI payload are out of sync");
  nlohmann::json pixel_report{{"canonical_payload_sha256",detail::og_matchmaking_layout::og_matchmaking_layout_sha256},
      {"canonical_payload_bytes",generated.size()},{"chunks",detail::og_matchmaking_layout::og_matchmaking_layout_chunks.size()},
      {"original_mainmenu_sha256",data.at("original_mainmenu_sha256")},{"mcc_mainmenu_sha256",data.at("mcc_mainmenu_sha256")},{"banks",nlohmann::json::array()}};
  detail::mcc_logical_layout::Codec original{xbox},raw{mcc};
  check(data.at("original").at("global_strings").size()==xbox.global_strings.size(),"Shared original UI string owner changed");
  for(const auto &[sid,value]:data.at("original").at("global_strings").items())
    check(xbox.global_strings.at(std::uint32_t(std::stoul(sid)))==value.get<std::string>(),"Shared original UI string differs from its actual UNIC range");
  for(const auto &[id,definition]:data.at("original").at("screens").items())
    check(original.screen(xbox.screens.at(unsigned(std::stoul(id))))==definition,
          "Compiled original screen definition differs from actual Xbox cache bytes");
  check(original.screen(xbox.screens.at(5))==data.at("original").at("background"),"Original common-background definition changed");
  check(original.screen(xbox.screens.at(27))==data.at("original").at("friends_footer"),"Original collapsed Friends channel definition changed");
  for(const auto &[sid,value]:data.at("original").at("friends_footer_strings").items())
    check(xbox.screens.at(27).strings.at(std::uint32_t(std::stoul(sid)))==value.get<std::string>(),
          "Original collapsed Friends channel lost its actual local string bank");
  for(const auto &[id,definition]:data.at("raw").at("screens").items())
    check(raw.screen(mcc.screens.at(unsigned(std::stoul(id))))==definition,
          "Qualified MCC screen differs from actual loaded metadata");
  for(const auto &[id,definition]:data.at("original").at("skins").items())
    check(original.skin(xbox.skins.at(unsigned(std::stoul(id))))==definition,
          "Original row skin differs from actual Xbox cache bytes");
  for(const auto &[name,definition]:data.at("original").at("groups").items())
    check(original.skin(xbox.skin_groups.at(detail::mcc_logical_layout::Codec::named(xbox.skin_groups,name)))==definition,
          "Original player group differs from actual Xbox cache bytes");
  for(const auto &[name,definition]:data.at("original").at("images").items()) {
    auto expected=definition;
    for(auto &frame:expected.at("frames"))frame.erase("format");
    check(original.image(xbox.images.at(detail::mcc_logical_layout::Codec::named(xbox.images,name)))==expected,
          "Original bitmap frames or sprite UVs differ from actual Xbox cache bytes");
  }
  auto view=detail::mcc_logical_layout::make_view(mcc);
  check(view.has_value(),"MCC cache has no qualified original logical canvas");
  pixel_report["retained_mcc_banks_before"]=retained_mcc_bank_cases(*view,mcc,true);
  const auto previous_skin_zero=detail::mcc_logical_layout::Codec{view->content}.skin(view->content.skins.at(0));
  const auto previous_background=detail::mcc_logical_layout::Codec{view->content}.screen(view->content.screens.at(5));
  const auto previous_lobby=detail::mcc_logical_layout::Codec{view->content}.screen(view->content.screens.at(14));
  const auto previous_friends=detail::mcc_logical_layout::Codec{view->content}.screen(view->content.screens.at(27));
  const auto previous_shared_strings=view->content.global_strings;
  const auto previous_button_keys=view->content.button_keys;
  std::map<unsigned,detail::og_matchmaking_layout::Json> previous_dialogs;
  for(const auto id:{7u,8u})
    if(const auto it=view->content.screens.find(id);it!=view->content.screens.end())
      previous_dialogs.emplace(id,detail::mcc_logical_layout::Codec{view->content}.screen(it->second));
  std::map<std::uint32_t,detail::og_matchmaking_layout::Json> previous_groups;
  for(const auto &[id,group]:view->content.skin_groups)
    if(detail::og_matchmaking_layout::group_name(group.name)!=group.name)
      previous_groups.emplace(id,detail::mcc_logical_layout::Codec{view->content}.skin(group));
  std::map<std::uint32_t,detail::og_matchmaking_layout::Json> previous_banks;
  for(const auto &[id,image]:view->content.images)
    previous_banks.emplace(id,detail::mcc_logical_layout::Codec{view->content}.image(image));
  // The exporter qualifies the complete raw cache inventory. The finite
  // logical view legitimately carries only the banks used by its own screens.
  std::set<std::uint32_t> available_logical_banks;
  for(const auto &[name,definition]:data.at("available_images").items()) {
    const auto found=std::find_if(mcc.images.begin(),mcc.images.end(),
        [&](const auto &entry){return entry.second.name==name;});
    if(found==mcc.images.end())throw FormatError("Qualified MCC bitmap owner is missing: "+name);
    auto expected=definition;
    for(auto &frame:expected.at("frames"))frame.erase("format");
    if(raw.image(found->second)!=expected)throw FormatError("Qualified MCC bitmap metadata changed: "+name);
    if(const auto logical=view->content.images.find(found->first);logical!=view->content.images.end()) {
      check(logical->second.name==name,"Logical MCC bitmap changed its authenticated identity");
      available_logical_banks.insert(found->first);
    }
  }
  std::set<std::uint32_t> projected_available_banks;
  for(const auto &[id,image]:view->content.images)
    if(data.at("available_images").contains(image.name))projected_available_banks.insert(id);
  check(available_logical_banks==projected_available_banks,"Raw and projected MCC bitmap correspondence changed");
  check(detail::og_matchmaking_layout::augment(*view,mcc),"Original matchmaking layout was not admitted");
  {
    // Admitted numeric sampling and user celestial pages must reach both the
    // canonical bank and the finite source-layout alias without changing the
    // source cache or its original logical sprite geometry.
    const std::string name="ui\\global_bitmaps\\rank_icons_sm";
    const auto id=detail::mcc_logical_layout::Codec::named(mcc.images,name);
    const auto &source=mcc.images.at(id);
    check(source.images.size()==50,"Rank override control lost its actual fifty-page source bank");
    const auto before=source.images;
    FrontendUiAssetPack selected;selected[name]=source.images;
    for(const auto page:{0u,43u,44u,45u,46u,47u,48u,49u}) {
      auto &raster=selected.at(name).at(page);
      check(!raster.pixels.empty(),"Rank override control has no admitted source raster");
      raster.pixels[0]^=std::byte(0x80u+page);
    }
    auto customized=detail::mcc_logical_layout::make_view(mcc);
    check(customized.has_value(),"Custom-rank control lost the qualified MCC view");
    check(detail::og_matchmaking_layout::augment(*customized,mcc,selected),"Custom-rank logical view was not admitted");
    const auto alias=detail::og_matchmaking_layout::embedded_image_name(name);
    const auto alias_id=detail::mcc_logical_layout::Codec::named(customized->content.images,alias);
    const auto ordinary_alias_id=detail::mcc_logical_layout::Codec::named(view->content.images,alias);
    check(detail::mcc_logical_layout::Codec{customized->content}.image(customized->content.images.at(alias_id))==
          detail::mcc_logical_layout::Codec{view->content}.image(view->content.images.at(ordinary_alias_id)),
          "Admitted celestial rasters changed original logical rank dimensions or sprite sequences");
    for(unsigned page=0;page<before.size();++page) {
      check(customized->rasters.at(name).at(page).pixels==selected.at(name).at(page).pixels &&
            customized->rasters.at(alias).at(page).pixels==selected.at(name).at(page).pixels,
            "Selected numeric/celestial rank raster did not survive both logical-view bank routes");
      check(mcc.images.at(id).images.at(page).pixels==before.at(page).pixels,
            "Custom rank projection mutated the immutable admitted source cache");
    }
    selected.at(name).pop_back();bool rejected{};
    try {
      auto malformed=detail::mcc_logical_layout::make_view(mcc);
      check(malformed.has_value(),"Wrong-count control lost the qualified MCC view");
      (void)detail::og_matchmaking_layout::augment(*malformed,mcc,selected);
    } catch(const FormatError&){rejected=true;}
    check(rejected,"Logical rank view accepted an admitted bank with the wrong actual page count");
  }
  check(view->content.global_strings==previous_shared_strings && view->content.button_keys==previous_button_keys,
        "Original view polluted unrelated selected-cache shared strings or button keys");
  check(detail::mcc_logical_layout::Codec{view->content}.skin(view->content.skins.at(0))==previous_skin_zero,
        "Original Setup replaced the shared skin of unrelated selected-cache menus");
  check(view->original_goal_background && view->original_goal_background->flags==4 &&
        view->original_goal_background->panes.at(0).bitmaps.size()==13 &&
        detail::mcc_logical_layout::Codec{view->content}.screen(view->content.screens.at(5))==previous_background,
        "Original common background was not scoped separately from unrelated selected-cache menus");
  check(view->other_lobby && detail::mcc_logical_layout::Codec{view->content}.screen(*view->other_lobby)==previous_lobby,
        "Original ONLINE lobby discarded the existing other-mode lobby definition");
  check(view->original_friends_footer && view->original_friends_footer->id==27 &&
        view->original_friends_footer->flags==4 && view->original_friends_footer->panes.size()==1 &&
        view->original_friends_footer->panes[0].texts.size()==1 &&
        view->original_friends_footer->panes[0].bitmaps.size()==1 &&
        detail::mcc_logical_layout::Codec{view->content}.screen(view->content.screens.at(27))==previous_friends,
        "Original collapsed Friends widget replaced unrelated selected-cache navigation");
  for(const auto &[id,definition]:previous_dialogs)
    check(detail::mcc_logical_layout::Codec{view->content}.screen(view->other_goal_dialogs.at(id))==definition,
          "Original matchmaking dialog replaced an unrelated selected-cache dialog");
  check(view->other_goal_dialogs.size()==previous_dialogs.size(),"Original dialog preservation invented a previous logical owner");
  for(const auto id:{7u,8u}) {
    check(view->other_goal_dialogs.contains(id)==previous_dialogs.contains(id),
          "Unrelated dialog lost its prior raw-MCC/logical route");
  }
  for(const auto &[id,definition]:previous_groups)
    check(detail::mcc_logical_layout::Codec{view->content}.skin(view->content.skin_groups.at(id))==definition,
          "Original ONLINE group replaced a shared selected-cache group");
  pixel_report["retained_mcc_banks_after"]=retained_mcc_bank_cases(*view,mcc,false);
  for(const auto &[id,definition]:previous_banks)
    check(detail::mcc_logical_layout::Codec{view->content}.image(view->content.images.at(id))==definition,
          "Original Setup replaced an unrelated selected-cache bitmap bank");
  for(const auto id:{7u,8u,14u,16u,23u,25u,185u,210u,219u,237u})
    check(view->content.screens.contains(id),"Original matchmaking/setup screen owner missing");
  unsigned dynamic_widgets{};
  for(const auto &[key,definition]:data.at("original").at("screens").items()) {
    const auto &projected=view->content.screens.at(unsigned(std::stoul(key)));
    for(unsigned p=0;p<definition.at("panes").size();++p)
      for(unsigned b=0;b<definition.at("panes").at(p).at("bitmaps").size();++b) {
        const auto &name=definition.at("panes").at(p).at("bitmaps").at(b).at("datum");
        const auto &dynamic=data.at("dynamic_images");
        const bool expected=std::find(dynamic.begin(),dynamic.end(),name)!=dynamic.end();
        check(projected.panes.at(p).bitmaps.at(b).matchmaking_map_preview==expected,
            "Scoped original bitmap alias discarded its dynamic scenario owner");
        if(expected){++dynamic_widgets;check(projected.panes.at(p).bitmaps.at(b).datum==null_datum,
            "Dynamic matched map retained the authored static Zanzibar preview");}
      }
  }
  check(dynamic_widgets==1 && view->content.screens.at(210).panes.at(3).bitmaps.at(10).matchmaking_map_preview,
        "Original matched map widget changed its authored pane/child identity");
  for(const auto &[key,value]:data.at("original").at("button_keys").items())
    check(xbox.button_keys.at(unsigned(std::stoul(key))-1)==value.get<std::string>(),
          "Original setup/listing button-key bank changed");
  const auto &setup=view->content.screens.at(23).panes.at(0);
  check(setup.lists.size()==1 && setup.lists[0].skin>=21 && setup.lists[0].visible==7 &&
        setup.lists[0].position.x==-250 && setup.lists[0].position.y==90 && setup.lists[0].animation==32,
        "Original squad-settings list was replaced by the PC adapter");
  const auto &privacy=view->content.screens.at(25).panes.at(0);
  check(privacy.lists.size()==1 && privacy.lists[0].skin==setup.lists[0].skin && privacy.lists[0].visible==3 &&
        privacy.lists[0].position.x==-106 && privacy.lists[0].position.y==-20 &&
        privacy.lists[0].animation==24 && privacy.bitmaps.size()==4 && privacy.texts.size()==1,
        "Original party privacy channel lost its authored list/dialog geometry");
  for(const auto id:{219u,237u}) {
    const auto &listing=view->content.screens.at(id).panes.at(0);
    check(listing.lists.size()==1 && listing.lists[0].skin==setup.lists[0].skin && listing.lists[0].visible==16 &&
          listing.lists[0].position.x==-260 && listing.lists[0].position.y==142 &&
          listing.lists[0].animation==(id==219?22:32) && listing.texts.size()==3,
          "Original playlist-listing owner was replaced by a details/confirmation page");
  }
  for(const auto &[path,entry]:data.at("dynamic_map_images").items()) {
    const auto mcc_id=detail::mcc_logical_layout::Codec::named(mcc.images,entry.at("mcc").at("name"));
    auto expected=entry.at("original").at("image");
    for(auto &frame:expected.at("frames"))frame.erase("format");
    if(entry.at("original").contains("name")) {
      const auto original_id=detail::mcc_logical_layout::Codec::named(xbox.images,entry.at("original").at("name"));
      check(original.image(xbox.images.at(original_id))==expected,
            "Original map preview differs from actual scenario-owned Xbox bitmap");
    } else {
      // The unchanged DLC converter fills these actual preallocated slots;
      // registration chooses the slot, while the scenario owns its preview.
      const auto &downloaded=entry.at("original").at("downloaded");
      for(const auto &name:downloaded.at("source_resource_slots")) {
        const auto slot=std::find_if(xbox_cache.tags().begin(),xbox_cache.tags().end(),
            [&](const auto &tag){return tag.name==name.template get<std::string>();});
        check(slot!=xbox_cache.tags().end(),"Original DLC bitmap slot is absent");
        const Bytes tag(xbox_cache.tag_payload(slot->handle()));
        check(tag.u32(68)==1 && tag.u32(60)==0,"Original DLC bitmap slot changed its frame/sequence count");
        const Bytes frame(xbox_cache.read_tag_bytes(tag.u32(72),116));
        check(frame.u16(4)==entry.at("original").at("image").at("frames")[0].at("width") &&
              frame.u16(6)==entry.at("original").at("image").at("frames")[0].at("height") && frame.u16(12)==11,
              "Downloaded map preview differs from its actual original resource-slot descriptor");
      }
      check(!downloaded.at("catalog_records").empty(),"Downloaded map lost its authenticated catalog provenance");
    }
    const auto alias=detail::og_matchmaking_layout::map_preview_datum(view->content,mcc_id);
    check(alias && *alias!=mcc_id && raw.image(view->content.images.at(*alias))==expected,
          "Dynamic map preview did not select its exact source raster alias");
    const auto actual_map=std::find_if(mcc.multiplayer_maps.begin(),mcc.multiplayer_maps.end(),
        [&](const auto &map){return map.scenario_path==path;});
    check(actual_map!=mcc.multiplayer_maps.end() && actual_map->bitmap==mcc_id,
          "Map preview used a bitmap ordinal instead of actual scenario identity");
    const auto &pixels=view->rasters.at(view->content.images.at(*alias).name);
    const auto &admitted=mcc.images.at(mcc_id).images;
    check(pixels.size()==admitted.size(),"Scenario preview discarded its admitted MCC page inventory");
    for(unsigned page=0;page<pixels.size();++page)check(pixels[page].width==admitted[page].width &&
        pixels[page].height==admitted[page].height && pixels[page].format==admitted[page].format &&
        pixels[page].pixels==admitted[page].pixels,"Scenario preview replaced admitted MCC raster pixels");
  }
  check(data.at("embedded_images").empty() && data.at("asset_policy")=="admitted-mcc-artwork-source-tu15-logical-layout",
        "Production logical layout contains a DVD pixel override");
  const std::string quick="ui\\screens\\game_shell\\settings_screen\\variant_settings\\quick_options_display";
  const auto &join=data.at("mcc_image_bindings").at(quick);
  const auto &authored=join.at("widget_owner");
  check(authored.at("screen")==210 && authored.at("pane")==1 && authored.at("bitmap")==0 &&
      authored.at("original")==data.at("original").at("screens").at("210").at("panes")[1].at("bitmaps")[0] &&
      authored.at("mcc")==data.at("raw").at("screens").at("210").at("panes")[1].at("bitmaps")[0] &&
      join.at("mcc_name")==authored.at("mcc").at("datum") && join.at("frames")==nlohmann::json::array({authored.at("mcc").at("frame")}),
      "Missing original quick-options art lost its authenticated same-widget MCC role join");
  for(const auto &[name,entry]:data.at("mcc_image_bindings").items()) {
    const auto mcc_name=entry.at("mcc_name").get<std::string>();
    const auto &actual=mcc.images.at(detail::mcc_logical_layout::Codec::named(mcc.images,mcc_name)).images;
    const auto alias=detail::og_matchmaking_layout::embedded_image_name(name);
    const auto &projected=view->rasters.at(alias);
    auto descriptor=data.at("original").at("images").at(name);
    if(entry.at("logical_policy")=="retained_mcc")
      descriptor=previous_banks.at(detail::mcc_logical_layout::Codec::named(mcc.images,mcc_name));
    for(auto &frame:descriptor.at("frames"))frame.erase("format");
    check(raw.image(view->content.images.at(detail::mcc_logical_layout::Codec::named(view->content.images,alias)))==descriptor,
          "Source logical geometry lost its authenticated descriptor policy");
    check(view->content.images.at(detail::mcc_logical_layout::Codec::named(view->content.images,alias)).name==alias,
          "Source logical bank lost its explicit finite-view identity");
    check(entry.at("frames").size()==projected.size(),"Admitted MCC frame projection changed");
    nlohmann::json bank{{"name",name},{"alias",alias},{"mcc_name",mcc_name},{"frames",nlohmann::json::array()}};
    for(unsigned i=0;i<projected.size();++i) {
      const auto frame_index=entry.at("frames")[i].get<unsigned>();check(frame_index<actual.size(),"Source artwork role escapes its MCC owner");
      const auto source=rgba(actual.at(frame_index)),copy=rgba(projected[i]);
      check(source.width==copy.width && source.height==copy.height && source.format==copy.format && source.pixels==copy.pixels,
            "Logical source UI replaced its admitted higher-definition MCC pixels");
      nlohmann::json frame{{"index",i},{"mcc_frame",frame_index},{"width",source.width},{"height",source.height},
          {"mcc_decoded_sha256",hash(source.pixels)},{"alias_decoded_sha256",hash(copy.pixels)},{"mcc_pixel_identity",true}};
      bank["frames"].push_back(std::move(frame));
    }
    pixel_report["banks"].push_back(std::move(bank));
  }
  const auto &progress=view->content.screens.at(210);
  check(view->content.original_matchmaking_layout && progress.panes.size()==4,
        "Original four-pane matchmaking owner missing");
  for(const auto &pane:progress.panes) {
    check(pane.lists.size()==1 && pane.lists[0].skin==20 && pane.lists[0].visible==16,
          "Original sixteen-seat right sidebar missing");
    check(pane.lists[0].position.x==81 && pane.lists[0].position.y==150,
          "Right sidebar was placed from a guessed PC-panel offset");
  }
  check(progress.panes[3].texts.size()==12 && progress.panes[3].bitmaps.size()==14,
        "Original final pane lost its settings, total, map or game icon fields");
  for(const auto id:{16u,185u}) {
    const auto &pcr=view->content.screens.at(id);
    check(pcr.panes.size()==6,"Original PCR six-page owner missing");
    for(const auto &pane:pcr.panes)check(pane.lists.size()==1 && pane.lists[0].visible==16,
        "Original report row owner changed");
  }
  const auto &skin=view->content.skins.at(20);
  check(skin.grids.size()==1 && skin.grids[0].count==1 && view->content.skin_groups.contains(skin.grids[0].skin),
        "Original nested player name/emblem/rank row missing");
  const auto &player=view->content.skin_groups.at(skin.grids[0].skin);
  check(player.texts.size()==1 && player.bitmaps.size()==4 && player.emblems.size()==1,
        "Original sidebar player fields were replaced by plain names");
  std::set<std::string> emblem_names;
  const auto gather=[&](auto&&self,const nlohmann::json&node)->void {
    if(node.is_object())for(const auto &[key,value]:node.items()) {
      if(key=="shader"&&value.is_string()&&!value.get<std::string>().empty())emblem_names.insert(value.get<std::string>());
      else self(self,value);
    }else if(node.is_array())for(const auto&value:node)self(self,value);
  };
  gather(gather,data.at("original").at("screens"));gather(gather,data.at("original").at("skins"));gather(gather,data.at("original").at("groups"));
  pixel_report["emblem_shader_stages"]=nlohmann::json::array();
  pixel_report["goal_emblem_asset_source"]="admitted MCC";
  pixel_report["goal_emblem_pages"]=nlohmann::json::array();
  for(const auto&name:emblem_names) {
    const auto find=[&](const auto&materials)->const Material&{const auto it=std::find_if(materials.begin(),materials.end(),[&](const auto&p){return p.second.name==name;});check(it!=materials.end(),"Original emblem shader has no exact named owner");return it->second;};
    const auto &a=find(xbox.emblem_shaders),&b=find(mcc.emblem_shaders);
    const Texture*old[]{&a.diffuse,&a.detail};const Texture*sampled[]{&b.diffuse,&b.detail};
    for(unsigned stage=0;stage<2;++stage) {
      const auto source=rgba(*old[stage]),copy=rgba(*sampled[stage]);
      pixel_report["emblem_shader_stages"].push_back({{"name",name},{"stage",stage},{"original_width",source.width},{"original_height",source.height},
          {"original_decoded_sha256",hash(source.pixels)},{"mcc_decoded_sha256",hash(copy.pixels)},
          {"pixel_identity",source.width==copy.width&&source.height==copy.height&&source.format==copy.format&&source.pixels==copy.pixels}});
    }
    const auto &goal=find(view->content.emblem_shaders);
    check(goal.diffuse.pixels==b.diffuse.pixels && goal.detail.pixels==b.detail.pixels &&
          goal.texture_frames==b.texture_frames,
          "Original emblem alias replaced the shared selected-cache material");
    const auto &prepared=view->content.emblem_shaders.at(player.emblems.front().shader);
    check(prepared.name==b.name,"Original player row lost its admitted MCC emblem shader");
    check(prepared.forward_program.words==b.forward_program.words &&
          prepared.program_constants==b.program_constants && prepared.texture_frames==b.texture_frames,
          "Goal emblem changed admitted MCC numeric shader inputs");
    check(prepared.animated_textures[0].size()==32 && prepared.animated_textures[1].size()==64,
          "Goal emblem kept only the two default MCC pages");
    for(unsigned stage=0;stage<2;++stage) {
      const auto &source_pages=b.animated_textures[stage],&goal_pages=prepared.animated_textures[stage];
      check(source_pages.size()==goal_pages.size(),"MCC emblem page coverage changed");
      check(goal.animated_textures[stage].size()==b.animated_textures[stage].size(),
            "Original emblem alias changed the shared animated page count");
      for(unsigned page=0;page<source_pages.size();++page) {
        check(goal.animated_textures[stage][page].pixels==b.animated_textures[stage][page].pixels,
              "Original emblem alias changed an unrelated selected-cache page");
        const auto source=rgba(source_pages[page]),copy=rgba(goal_pages[page]);
        check(source.width==copy.width && source.height==copy.height && source.format==copy.format &&
              source.layers==copy.layers && source.depth==copy.depth && source.pixels==copy.pixels &&
              source.mips.size()==copy.mips.size(),"Prepared goal emblem page differs from its admitted MCC RGBA bank");
        for(unsigned mip=0;mip<source.mips.size();++mip) {
          const auto &x=source.mips[mip],&y=copy.mips[mip];
          check(x.width==y.width && x.height==y.height && x.depth==y.depth && x.offset==y.offset && x.size==y.size,
                "MCC emblem page lost its authored mip layout");
        }
        pixel_report["goal_emblem_pages"].push_back({{"shader",name},{"stage",stage},{"page",page},
            {"mcc_decoded_sha256",hash(source.pixels)},{"goal_decoded_sha256",hash(copy.pixels)},
            {"pixel_identity",true}});
      }
    }
    // The live appearance update selects frames from these resident banks.
    // Exercise every foreground/background page and both opacity/toggle paths,
    // rather than certifying only diffuse/detail at their initial frame0.
    for(unsigned page=0;page<64;++page)for(unsigned flags:{0u,1u,2u,4u,7u})for(float opacity:{1.f,.5f}) {
      FrontendAppearance appearance;appearance.colors={8,12,9,1};
      appearance.foreground=page;appearance.background=page%32;appearance.flags=flags;
      auto selected=prepared,reference=b;
      update_frontend_emblem(selected,appearance,view->content.appearance_palette,opacity);
      update_frontend_emblem(reference,appearance,view->content.appearance_palette,opacity);
      check(selected.texture_frames[0]==page%32 && selected.texture_frames[1]==page &&
            selected.texture_frames==reference.texture_frames &&
            selected.forward_program.words==reference.forward_program.words &&
            selected.forward_transforms==reference.forward_transforms,
            "Goal emblem changed the actual MCC page/UV/appearance animation");
    }
  }
  pixel_report["passed"]=true;pixel_report["source_banks"]=pixel_report["banks"].size();
  unsigned frames{},compared{},different{};
  for(const auto &bank:pixel_report["banks"])for(const auto &frame:bank["frames"]){++frames;if(frame.contains("mcc_pixel_identity")){++compared;different+=!frame.at("mcc_pixel_identity").get<bool>();}}
  pixel_report["source_frames"]=frames;pixel_report["mcc_compared_frames"]=compared;pixel_report["mcc_different_frames"]=different;
  if(const auto *report=std::getenv("HALO2_UI_PIXEL_REPORT"))std::ofstream(report)<<pixel_report.dump(2)<<'\n';
  std::cout<<"PASS actual original setup/listing/screen/skin/group bytes, qualified MCC correspondence, four panes, sixteen-seat right sidebar and six-page PCR owners\n";
  return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
