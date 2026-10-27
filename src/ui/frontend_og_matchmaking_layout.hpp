#pragma once
#include "ui/frontend_mcc_logical_layout.hpp"

namespace halo2::detail::og_matchmaking_layout {
using mcc_logical_layout::Json;
#include "ui/frontend_og_matchmaking_layout_data.inc"
inline const Json &data() {
  static const auto value=[] {
    static_assert(og_matchmaking_layout_bytes<=8*1024*1024);
    std::string payload;payload.reserve(og_matchmaking_layout_bytes);
    for(const auto part:og_matchmaking_layout_chunks)payload.append(part);
    if(payload.size()!=og_matchmaking_layout_bytes)throw FormatError("Original UI payload extent changed");
    return Json::parse(payload);
  }();return value;
}
inline bool goal_frame(const MenuFrame&frame,const MenuState*state=nullptr) {
  if(frame.screen==14)return state && state->online_party_screen(frame);
  if(frame.screen==25) {
    const auto lobby=state?state->lobby_view():std::nullopt;
    return lobby && lobby->protocol==2 && lobby->input.session_enum==6;
  }
  if(frame.screen==7||frame.screen==8) {
    if(!state)return false;
    const auto &stack=state->stack();auto it=std::find_if(stack.begin(),stack.end(),[&](const auto&f){return f.instance==frame.instance;});
    while(it!=stack.end()&&it!=stack.begin()) {--it;if(it->screen!=7&&it->screen!=8)return goal_frame(*it,state);}
    return false;
  }
  if((frame.screen==219||frame.screen==237)&&frame.context==online_server_browser_context)return false;
  if(is_vista_menu(frame.screen))return frame.vista_pcr &&
    ((frame.screen&0xffffu)==16 || (frame.screen&0xffffu)==185);
  return data().at("original").at("screens").contains(std::to_string(frame.screen));
}
inline std::string embedded_image_name(std::string_view name) {
  if(data().at("original").at("images").contains(std::string(name)))
    return std::string(name)+"#tu15-logical-mcc";
  for(const auto &[scenario,entry]:data().at("dynamic_map_images").items())
    if(entry.at("raster_alias")==name)return std::string(name)+"#tu15-logical-mcc";
  return std::string(name);
}
inline std::string group_name(std::string_view name) {
  return data().at("original").at("groups").contains(std::string(name))?std::string(name)+"#og-matchmaking":std::string(name);
}
inline std::string shader_name(std::string_view name) {
  return std::string(name);
}
inline std::optional<std::uint32_t> map_preview_datum(const FrontendContent &content,std::uint32_t mcc_datum) {
  const auto image=content.images.find(mcc_datum);if(image==content.images.end())return {};
  for(const auto &[scenario,entry]:data().at("dynamic_map_images").items())
    if(entry.at("mcc").at("name")==image->second.name)
      return mcc_logical_layout::Codec::named(content.images,embedded_image_name(entry.at("raster_alias").get<std::string>()));
  return {};
}
inline std::string original_string(unsigned screen,std::uint32_t id) {
  const auto &strings=data().at("original").at("strings").at(std::to_string(screen));
  const auto it=strings.find(std::to_string(id));if(it!=strings.end())return it->get<std::string>();
  const auto &global=data().at("original").at("global_strings");const auto found=global.find(std::to_string(id));
  return found==global.end()?std::string{}:found->get<std::string>();
}
inline std::string original_text(unsigned screen,unsigned pane,unsigned index) {
  const auto &texts=data().at("original").at("screens").at(std::to_string(screen)).at("panes").at(pane).at("texts");
  return index<texts.size()?original_string(screen,texts[index].at("string").get<std::uint32_t>()):std::string{};
}
inline bool bitmap_required(std::string_view name) { return data().at("original").at("images").contains(std::string(name)); }
inline bool augment(mcc_logical_layout::View &view,const FrontendContent &source,
                    const FrontendUiAssetPack &admitted={}) {
  if(!mcc_frontend_content(source))return false;
  const auto &profile=data();const auto &original=profile.at("original");
  mcc_logical_layout::Codec actual{source};
  if(actual.screen(source.screens.at(5))!=profile.at("raw").at("background"))return false;
  // Qualify complete selected-cache definitions before replacing the PC layout.
  for(const auto &[key,value]:profile.at("raw").at("screens").items()) {
    const auto it=source.screens.find(unsigned(std::stoul(key)));
    if(it==source.screens.end() || actual.screen(it->second)!=value)return false;
  }
  auto &out=view.content;
  // Keep the selected cache's admitted customization/prompt rasters in this
  // finite logical view too. This includes the user's celestial rank slots.
  for(const auto &[name,pixels]:admitted)if(const auto found=view.rasters.find(name);found!=view.rasters.end()) {
    if(found->second.size()!=pixels.size())throw FormatError("Admitted MCC view bank changed its actual frame inventory: "+name);
    found->second=pixels;
  }
  view.other_lobby=out.screens.at(14);
  for(const auto id:{7u,8u})
    if(const auto it=out.screens.find(id);it!=out.screens.end())view.other_goal_dialogs.emplace(id,it->second);
  std::uint32_t next=0xf8000000u;
  const auto allocate=[&]() {
    while(out.images.contains(next)||out.skin_groups.contains(next)||out.emblem_shaders.contains(next))++next;
    return next++;
  };
  if(!profile.at("embedded_images").empty() || profile.at("asset_policy")!="admitted-mcc-artwork-source-tu15-logical-layout")
    throw FormatError("Matchmaking layout must retain admitted MCC artwork");
  const auto scoped_images=[&](Json value) {
    const auto replace=[&](auto&&self,Json &node)->void {
      if(node.is_string())node=shader_name(group_name(embedded_image_name(node.get<std::string>())));
      else if(node.is_array()||node.is_object())for(auto &child:node)self(self,child);
    };
    replace(replace,value);return value;
  };
  const auto project_image=[&](const std::string &name,const Json &descriptor,const Json &binding) {
    const auto mcc_name=binding.at("mcc_name").get<std::string>();
    const auto id=mcc_logical_layout::Codec::named(source.images,mcc_name);
    const auto &image=source.images.at(id);
    const auto &expected=profile.at("available_images").at(mcc_name);
    auto shape=actual.image(image);auto qualified=expected;
    for(auto &frame:qualified.at("frames"))frame.erase("format");
    if(shape!=qualified)throw FormatError("Selected MCC matchmaking raster descriptor differs from source: "+name);
    UiImage logical;
    if(binding.at("logical_policy")=="retained_mcc") {
      const auto previous=out.images.find(id);
      if(previous==out.images.end())throw FormatError("Reduced MCC bank lost its admitted logical descriptor: "+name);
      logical=previous->second;
    } else {
      if(binding.at("logical_policy")!="source")throw FormatError("Unknown matchmaking artwork projection policy");
      for(const auto &frame:descriptor.at("frames")) {Texture texture;texture.width=frame.at("width");texture.height=frame.at("height");texture.format=frame.at("format");logical.images.push_back(std::move(texture));}
      for(const auto &sequence:descriptor.at("sequences")) {
      std::vector<UiSprite> sprites;
      for(const auto &s:sequence)sprites.push_back({s.at("image"),s.at("left"),s.at("right"),s.at("top"),s.at("bottom"),mcc_logical_layout::point2(s.at("registration"))});
      logical.sequences.push_back(std::move(sprites));
      }
    }
    const auto selected=admitted.find(mcc_name);
    const auto &rasters=selected==admitted.end()?image.images:selected->second;
    if(rasters.size()!=image.images.size())throw FormatError("Admitted custom MCC bank differs from its actual page inventory: "+mcc_name);
    const auto alias=embedded_image_name(name);logical.name=alias;std::vector<Texture> pixels;
    for(const auto &frame:binding.at("frames")) {
      const auto index=frame.get<unsigned>();if(index>=rasters.size())throw FormatError("Matchmaking artwork role escapes its admitted MCC bank");
      pixels.push_back(rasters.at(index));
    }
    if(pixels.size()!=logical.images.size())throw FormatError("Matchmaking logical/raster page projection differs: "+name);
    view.rasters[alias]=std::move(pixels);out.images.emplace(allocate(),std::move(logical));
  };
  // Load every actual MCC raster under the original logical descriptor.
  for(const auto &[name,descriptor]:original.at("images").items()) {
    if(std::find(profile.at("dynamic_images").begin(),profile.at("dynamic_images").end(),name)!=profile.at("dynamic_images").end())continue;
    project_image(name,descriptor,profile.at("mcc_image_bindings").at(name));
  }
  // Dynamic map records use scenario identity, not their differently ordered
  // code_global_bitmaps/multiplayerN names in Xbox and MCC mainmenu caches.
  for(const auto &[path,entry]:profile.at("dynamic_map_images").items()) {
    const auto found=std::find_if(source.multiplayer_maps.begin(),source.multiplayer_maps.end(),
        [&](const auto &map){return map.scenario_path==path;});
    const auto name=entry.at("mcc").at("name").get<std::string>();
    if(found==source.multiplayer_maps.end() || found->bitmap==null_datum ||
       source.images.at(found->bitmap).name!=name)
      throw FormatError("Selected MCC map preview differs from the qualified scenario role: "+path);
    const auto alias=entry.at("raster_alias").get<std::string>();
    project_image(alias,entry.at("original").at("image"),
        Json{{"mcc_name",name},{"frames",Json::array({0})},{"logical_policy","source"}});
  }
  // A removed MCC group is still an authored Xbox logical widget owner. Its
  // native ID addresses only this finite view; actual images keep MCC IDs.
  for(const auto &[name,group]:original.at("groups").items()) {
    UiSkin skin;skin.name=group_name(name);out.skin_groups.emplace(allocate(),std::move(skin));
  }
  for(const auto &[id,material]:source.emblem_shaders)out.emblem_shaders.try_emplace(id,material);
  // Preserve identifier namespaces. Source labels are inserted under the
  // selected MCC exact-name identifier; missing IDs have finite local owners.
  std::map<std::uint32_t,std::uint32_t> strings;
  for(const auto &[screen,bank]:original.at("strings").items())for(const auto &[key,value]:bank.items()) {
    const auto original_id=std::uint32_t(std::stoul(key));
    const auto mapped=frontend_compiled_string_id(source,original_id).value_or(original_id);
    strings[original_id]=mapped;
  }
  const auto remap_text=[&](UiText &t){t.string=frontend_compiled_string_id(source,t.string).value_or(t.string);};
  const auto remap_skin=[&](UiSkin &skin){for(auto &t:skin.texts)remap_text(t);for(auto &b:skin.bitmaps)b.placeholder=frontend_compiled_string_id(source,b.placeholder).value_or(b.placeholder);};
  mcc_logical_layout::Codec codec{out};
  for(const auto &[key,definition]:original.at("screens").items()) {
    const auto id=unsigned(std::stoul(key));out.screens.try_emplace(id,source.screens.at(id));
  }
  for(const auto &[name,value]:original.at("groups").items()) {
    auto skin=codec.skin(scoped_images(value));remap_skin(skin);out.skin_groups.insert_or_assign(mcc_logical_layout::Codec::named(out.skin_groups,group_name(name)),std::move(skin));
  }
  if(out.skins.size()<21)out.skins.resize(21);
  std::optional<unsigned> setup_skin;
  for(const auto &[key,value]:original.at("skins").items()) {
    auto skin=codec.skin(scoped_images(value));remap_skin(skin);
    if(key=="0") {
      // Skin0 is shared by unrelated selected-cache screens. Give the three
      // original setup/listing owners their exact skin through a finite view
      // alias, preserving every other menu's admitted logical definition.
      setup_skin=unsigned(out.skins.size());out.skins.push_back(std::move(skin));
    } else out.skins.at(unsigned(std::stoul(key)))=std::move(skin);
  }
  for(const auto &[key,definition]:original.at("screens").items()) {
    auto value=scoped_images(definition);
    if((key=="7" || key=="23" || key=="25" || key=="219" || key=="237") && setup_skin)
      for(auto &pane:value.at("panes"))for(auto &list:pane.at("lists"))
        if(list.at("skin")==0)list["skin"]=*setup_skin;
    std::vector<std::pair<unsigned,unsigned>> maps;
    for(unsigned p=0;p<value.at("panes").size();++p)for(unsigned b=0;b<value["panes"][p]["bitmaps"].size();++b) {
      auto &bitmap=value["panes"][p]["bitmaps"][b];
      // Dynamic roles belong to the authenticated authored bitmap. Scoped
      // aliases change resource names, never the widget's runtime ownership.
      const auto &authored=definition.at("panes").at(p).at("bitmaps").at(b).at("datum");
      if(std::find(profile["dynamic_images"].begin(),profile["dynamic_images"].end(),authored)!=profile["dynamic_images"].end()) {bitmap["datum"]="";maps.emplace_back(p,b);}
    }
    // Codec preserves the selected cache's string owner while decoding the
    // authored geometry. These screens were outside the older logical view.
    const auto id=unsigned(std::stoul(key));
    out.screens.try_emplace(id,source.screens.at(id));
    auto screen=codec.screen(value);
    screen.strings=out.global_strings;
    for(const auto &[sid,text]:original.at("global_strings").items()) {
      const auto source_id=std::uint32_t(std::stoul(sid));
      screen.strings[frontend_compiled_string_id(source,source_id).value_or(source_id)]=text.get<std::string>();
    }
    for(const auto &[sid,text]:original.at("strings").at(key).items()) {
      const auto source_id=std::uint32_t(std::stoul(sid));
      screen.strings[frontend_compiled_string_id(source,source_id).value_or(source_id)]=text.get<std::string>();
    }
    if(strings.contains(screen.header))screen.header=strings.at(screen.header);
    for(auto &pane:screen.panes){for(auto &t:pane.texts)remap_text(t);for(auto &b:pane.buttons)remap_text(b.text);}
    for(const auto &[pane,bitmap]:maps)screen.panes.at(pane).bitmaps.at(bitmap).matchmaking_map_preview=true;
    out.screens.insert_or_assign(id,screen);
    if(id==16||id==185)out.screens.insert_or_assign(vista_menu(id),std::move(screen));
  }
  view.original_goal_background=codec.screen(scoped_images(original.at("background")));
  // Screen27 is the separate global channel4 collapsed Y widget; preserve
  // the selected-cache screen27 used by all ordinary navigation owners.
  view.original_friends_footer=codec.screen(scoped_images(original.at("friends_footer")));
  auto &footer=*view.original_friends_footer;footer.strings.clear();
  for(const auto &[sid,text]:original.at("friends_footer_strings").items()) {
    const auto id=std::uint32_t(std::stoul(sid));
    footer.strings[frontend_compiled_string_id(source,id).value_or(id)]=text.get<std::string>();
  }
  for(auto &pane:footer.panes)for(auto &text:pane.texts)remap_text(text);
  out.original_matchmaking_layout=true;
  return true;
}
} // namespace halo2::detail::og_matchmaking_layout
